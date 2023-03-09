/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2021 PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"
#include "GS/Renderers/Metal/GSTextureMTL.h"
#include "GS/Renderers/Metal/GSDeviceMTL.h"
#include "GS/GSPerfMon.h"
#include "common/Align.h"
#include "common/Console.h"

#ifdef __APPLE__

// Uploads/downloads need 32-byte alignment for AVX2.
static constexpr u32 PITCH_ALIGNMENT = 32;

GSTextureMTL::GSTextureMTL(GSDeviceMTL* dev, MRCOwned<id<MTLTexture>> texture, Type type, Format format)
	: m_dev(dev)
	, m_texture(std::move(texture))
{
	m_type = type;
	m_format = format;
	m_size.x = [m_texture width];
	m_size.y = [m_texture height];
	m_mipmap_levels = [m_texture mipmapLevelCount];
}
GSTextureMTL::~GSTextureMTL()
{
}

void GSTextureMTL::RequestColorClear(GSVector4 color)
{
	m_needs_color_clear = true;
	m_clear_color = color;
}
void GSTextureMTL::RequestDepthClear(float depth)
{
	m_needs_depth_clear = true;
	m_clear_depth = depth;
}
void GSTextureMTL::RequestStencilClear(int stencil)
{
	m_needs_stencil_clear = true;
	m_clear_stencil = stencil;
}
bool GSTextureMTL::GetResetNeedsColorClear(GSVector4& colorOut)
{
	if (m_needs_color_clear)
	{
		m_needs_color_clear = false;
		colorOut = m_clear_color;
		return true;
	}
	return false;
}
bool GSTextureMTL::GetResetNeedsDepthClear(float& depthOut)
{
	if (m_needs_depth_clear)
	{
		m_needs_depth_clear = false;
		depthOut = m_clear_depth;
		return true;
	}
	return false;
}
bool GSTextureMTL::GetResetNeedsStencilClear(int& stencilOut)
{
	if (m_needs_stencil_clear)
	{
		m_needs_stencil_clear = false;
		stencilOut = m_clear_stencil;
		return true;
	}
	return false;
}

void GSTextureMTL::FlushClears()
{
	if (!m_needs_color_clear && !m_needs_depth_clear && !m_needs_stencil_clear)
		return;

	m_dev->BeginRenderPass(@"Clear",
		m_needs_color_clear   ? this : nullptr, MTLLoadActionLoad,
		m_needs_depth_clear   ? this : nullptr, MTLLoadActionLoad,
		m_needs_stencil_clear ? this : nullptr, MTLLoadActionLoad);
}

void* GSTextureMTL::GetNativeHandle() const
{
	return (__bridge void*)m_texture;
}

void GSTextureMTL::InvalidateClears()
{
	m_needs_color_clear = false;
	m_needs_depth_clear = false;
	m_needs_stencil_clear = false;
}

bool GSTextureMTL::Update(const GSVector4i& r, const void* data, int pitch, int layer)
{
	if (void* buffer = MapWithPitch(r, pitch, layer))
	{
		memcpy(buffer, data, CalcUploadSize(r.height(), pitch));
		return true;
	}
	return false;
}

bool GSTextureMTL::Map(GSMap& m, const GSVector4i* _r, int layer)
{
	GSVector4i r = _r ? *_r : GSVector4i(0, 0, m_size.x, m_size.y);
	u32 block_size = GetCompressedBlockSize();
	u32 blocks_wide = (r.width() + block_size - 1) / block_size;
	m.pitch = Common::AlignUpPow2(blocks_wide * GetCompressedBytesPerBlock(), PITCH_ALIGNMENT);
	if (void* buffer = MapWithPitch(r, m.pitch, layer))
	{
		m.bits = static_cast<u8*>(buffer);
		return true;
	}
	return false;
}

void* GSTextureMTL::MapWithPitch(const GSVector4i& r, int pitch, int layer)
{ @autoreleasepool {
	if (layer >= m_mipmap_levels)
		return nullptr;
	m_has_mipmaps = false;

	size_t size = CalcUploadSize(r.height(), pitch);
	GSDeviceMTL::Map map;

	bool needs_clear = false;
	if (m_needs_color_clear)
	{
		m_needs_color_clear = false;
		// Not uploading to full texture
		needs_clear = r.left > 0 || r.top > 0 || r.right < m_size.x || r.bottom < m_size.y;
	}

	id<MTLBlitCommandEncoder> enc;
	if (m_last_read == m_dev->m_current_draw || needs_clear)
	{
		if (needs_clear)
		{
			m_needs_color_clear = true;
			m_dev->BeginRenderPass(@"Pre-Upload Clear", this, MTLLoadActionLoad, nullptr, MTLLoadActionDontCare);
		}
		enc = m_dev->GetLateTextureUploadEncoder();
		map = m_dev->Allocate(m_dev->m_vertex_upload_buf, size);
	}
	else
	{
		enc = m_dev->GetTextureUploadEncoder();
		map = m_dev->Allocate(m_dev->m_texture_upload_buf, size);
	}
	// Copy is scheduled now, won't happen until the encoder is committed so no problems with ordering
	[enc copyFromBuffer:map.gpu_buffer
	       sourceOffset:map.gpu_offset
	  sourceBytesPerRow:pitch
	sourceBytesPerImage:size
	         sourceSize:MTLSizeMake(r.width(), r.height(), 1)
	          toTexture:m_texture
	   destinationSlice:0
	   destinationLevel:layer
	  destinationOrigin:MTLOriginMake(r.x, r.y, 0)];

	g_perfmon.Put(GSPerfMon::TextureUploads, 1);
	return map.cpu_buffer;
}}

void GSTextureMTL::Unmap()
{
	// Nothing to do here, upload is already scheduled
}

void GSTextureMTL::GenerateMipmap()
{ @autoreleasepool {
	if (m_mipmap_levels > 1 && !m_has_mipmaps)
	{
		id<MTLBlitCommandEncoder> enc = m_dev->GetTextureUploadEncoder();
		[enc generateMipmapsForTexture:m_texture];
	}
}}

void GSTextureMTL::Swap(GSTexture* other)
{
	GSTexture::Swap(other);

	GSTextureMTL* mtex = static_cast<GSTextureMTL*>(other);
	pxAssert(m_dev == mtex->m_dev);
#define SWAP(x) std::swap(x, mtex->x)
	SWAP(m_texture);
	SWAP(m_has_mipmaps);
	SWAP(m_needs_color_clear);
	SWAP(m_needs_depth_clear);
	SWAP(m_needs_stencil_clear);
	SWAP(m_clear_color);
	SWAP(m_clear_depth);
	SWAP(m_clear_stencil);
#undef SWAP
}

GSDownloadTextureMTL::GSDownloadTextureMTL(GSDeviceMTL* dev, MRCOwned<id<MTLBuffer>> buffer,
	u32 width, u32 height, GSTexture::Format format)
	: GSDownloadTexture(width, height, format)
	, m_dev(dev)
	, m_buffer(std::move(buffer))
{
	m_map_pointer = static_cast<const u8*>([m_buffer contents]);
}

GSDownloadTextureMTL::~GSDownloadTextureMTL() = default;

std::unique_ptr<GSDownloadTextureMTL> GSDownloadTextureMTL::Create(GSDeviceMTL* dev, u32 width, u32 height, GSTexture::Format format)
{
	const u32 buffer_size = GetBufferSize(width, height, format, PITCH_ALIGNMENT);

	MRCOwned<id<MTLBuffer>> buffer = MRCTransfer([dev->m_dev.dev newBufferWithLength:buffer_size options:MTLResourceStorageModeShared]);
	if (!buffer)
	{
		Console.Error("Failed to allocate %u byte download texture buffer (out of memory?)", buffer_size);
		return {};
	}

	return std::unique_ptr<GSDownloadTextureMTL>(new GSDownloadTextureMTL(dev, buffer, width, height, format));
}

void GSDownloadTextureMTL::CopyFromTexture(
	const GSVector4i& drc, GSTexture* stex, const GSVector4i& src, u32 src_level, bool use_transfer_pitch)
{ @autoreleasepool {
	GSTextureMTL* const mtlTex = static_cast<GSTextureMTL*>(stex);

	pxAssert(mtlTex->GetFormat() == m_format);
	pxAssert(drc.width() == src.width() && drc.height() == src.height());
	pxAssert(src.z <= mtlTex->GetWidth() && src.w <= mtlTex->GetHeight());
	pxAssert(static_cast<u32>(drc.z) <= m_width && static_cast<u32>(drc.w) <= m_height);
	pxAssert(src_level < static_cast<u32>(mtlTex->GetMipmapLevels()));
	pxAssert((drc.left == 0 && drc.top == 0) || !use_transfer_pitch);

	u32 copy_offset, copy_size, copy_rows;
	m_current_pitch =
		GetTransferPitch(use_transfer_pitch ? static_cast<u32>(drc.width()) : m_width, PITCH_ALIGNMENT);
	GetTransferSize(drc, &copy_offset, &copy_size, &copy_rows);

	m_dev->EndRenderPass();
	mtlTex->FlushClears();
	g_perfmon.Put(GSPerfMon::Readbacks, 1);

	m_copy_cmdbuffer = MRCRetain(m_dev->GetRenderCmdBuf());

	[m_copy_cmdbuffer pushDebugGroup:@"GSDownloadTextureMTL::CopyFromTexture"];
	id<MTLBlitCommandEncoder> encoder = [m_copy_cmdbuffer blitCommandEncoder];
	[encoder copyFromTexture:mtlTex->GetTexture()
	             sourceSlice:0
	             sourceLevel:src_level
	            sourceOrigin:MTLOriginMake(src.x, src.y, 0)
	              sourceSize:MTLSizeMake(src.width(), src.height(), 1)
	                toBuffer:m_buffer
	       destinationOffset:copy_offset
	  destinationBytesPerRow:m_current_pitch
	destinationBytesPerImage:m_current_pitch * copy_rows];

	if (id<MTLFence> fence = m_dev->GetSpinFence())
		[encoder updateFence:fence];

	[encoder endEncoding];
	[m_copy_cmdbuffer popDebugGroup];

	m_needs_flush = true;
}}

bool GSDownloadTextureMTL::Map(const GSVector4i& read_rc)
{
	// Always mapped.
	return true;
}

void GSDownloadTextureMTL::Unmap()
{
	// Always mapped.
}

void GSDownloadTextureMTL::Flush()
{
	if (!m_needs_flush)
		return;

	m_needs_flush = false;

	// If it's the same buffer currently being encoded, we need to kick it (and spin).
	if (m_copy_cmdbuffer == m_dev->GetRenderCmdBufWithoutCreate())
		m_dev->FlushEncodersForReadback();

	if (IsCommandBufferCompleted([m_copy_cmdbuffer status]))
	{
		// Asynchronous readback which already completed.
		m_copy_cmdbuffer = nil;
		return;
	}

	// Asynchrous readback, but the GPU isn't done yet.
	if (GSConfig.HWSpinCPUForReadbacks)
	{
		do
		{
			ShortSpin();
		}
		while (!IsCommandBufferCompleted([m_copy_cmdbuffer status]));
	}
	else
	{
		[m_copy_cmdbuffer waitUntilCompleted];
	}

	m_copy_cmdbuffer = nil;
}

#endif
