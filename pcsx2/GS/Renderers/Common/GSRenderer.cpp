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
#include "GSRenderer.h"
#include "GS/GSGL.h"
#include "Host.h"
#include "HostDisplay.h"
#include "PerformanceMetrics.h"
#include "pcsx2/Config.h"
#include "common/FileSystem.h"
#include "common/StringUtil.h"

#ifndef PCSX2_CORE
#include "gui/AppCoreThread.h"
#if defined(__unix__)
#include <X11/keysym.h>
#elif defined(__APPLE__)
#include <Carbon/Carbon.h>
#endif

static std::string GetDumpName()
{
	return StringUtil::wxStringToUTF8String(GameInfo::gameName);
}
static std::string GetDumpSerial()
{
	return StringUtil::wxStringToUTF8String(GameInfo::gameSerial);
}
#else
#include "VMManager.h"

static std::string GetDumpName()
{
	return VMManager::GetGameName();
}
static std::string GetDumpSerial()
{
	return VMManager::GetGameSerial();
}
#endif

GSRenderer::GSRenderer()
	: m_shift_key(false)
	, m_control_key(false)
	, m_texture_shuffle(false)
	, m_real_size(0, 0)
{
}

GSRenderer::~GSRenderer()
{
}

void GSRenderer::Destroy()
{
}

bool GSRenderer::Merge(int field)
{
	bool en[2];

	GSVector4i fr[2];
	GSVector4i dr[2];
	GSVector2i display_offsets[2];

	GSVector2i display_baseline = {INT_MAX, INT_MAX};
	GSVector2i frame_baseline = {INT_MAX, INT_MAX};
	GSVector2i display_combined = { 0, 0 };
	bool feedback_merge = m_regs->EXTWRITE.WRITE == 1;

	for (int i = 0; i < 2; i++)
	{
		en[i] = IsEnabled(i) || (m_regs->EXTBUF.FBIN == i && feedback_merge);

		if (en[i])
		{
			fr[i] = GetFrameRect(i);
			dr[i] = GetDisplayRect(i);
			display_offsets[i] = GetResolutionOffset(i);

			display_combined.x = std::max(GetFrameMagnifiedRect(i).right + abs(display_offsets[i].x), display_combined.x);
			display_combined.y = std::max(GetFrameMagnifiedRect(i).bottom + abs(display_offsets[i].y), display_combined.y);
			display_baseline.x = std::min(dr[i].left, display_baseline.x);
			display_baseline.y = std::min(dr[i].top, display_baseline.y);
			frame_baseline.x = std::min(fr[i].left, frame_baseline.x);
			frame_baseline.y = std::min(fr[i].top, frame_baseline.y);

			//DevCon.Warning("Read offset was X %d(left %d) Y %d(top %d)", display_baseline.x, dr[i].left, display_baseline.y, dr[i].top);
			//printf("[%d]: %d %d %d %d, %d %d %d %d\n", i, fr[i].x,fr[i].y,fr[i].z,fr[i].w , dr[i].x,dr[i].y,dr[i].z,dr[i].w);
		}
	}

	//display_combined.y = display_combined.y * (IsAnalogue() ? (isinterlaced() + 1) : 1);

	if (!en[0] && !en[1])
	{
		return false;
	}

	GL_PUSH("Renderer Merge %d (0: enabled %d 0x%x, 1: enabled %d 0x%x)", s_n, en[0], m_regs->DISP[0].DISPFB.Block(), en[1], m_regs->DISP[1].DISPFB.Block());

	// try to avoid fullscreen blur, could be nice on tv but on a monitor it's like double vision, hurts my eyes (persona 4, guitar hero)
	//
	// NOTE: probably the technique explained in graphtip.pdf (Antialiasing by Supersampling / 4. Reading Odd/Even Scan Lines Separately with the PCRTC then Blending)

	bool samesrc =
		en[0] && en[1] &&
		m_regs->DISP[0].DISPFB.FBP == m_regs->DISP[1].DISPFB.FBP &&
		m_regs->DISP[0].DISPFB.FBW == m_regs->DISP[1].DISPFB.FBW &&
		m_regs->DISP[0].DISPFB.PSM == m_regs->DISP[1].DISPFB.PSM;

	if (samesrc /*&& m_regs->PMODE.SLBG == 0 && m_regs->PMODE.MMOD == 1 && m_regs->PMODE.ALP == 0x80*/)
	{
		// persona 4:
		//
		// fr[0] = 0 0 640 448
		// fr[1] = 0 1 640 448
		// dr[0] = 159 50 779 498
		// dr[1] = 159 50 779 497
		//
		// second image shifted up by 1 pixel and blended over itself
		//
		// god of war:
		//
		// fr[0] = 0 1 512 448
		// fr[1] = 0 0 512 448
		// dr[0] = 127 50 639 497
		// dr[1] = 127 50 639 498
		//
		// same just the first image shifted
		//
		// These kinds of cases are now fixed by the more generic frame_diff code below, as the code here was too specific and has become obsolete.
		// NOTE: Persona 4 and God Of War are not rare exceptions, many games have the same(or very similar) offsets.

		int topDiff = fr[0].top - fr[1].top;
		if (dr[0].eq(dr[1]) && (fr[0].eq(fr[1] + GSVector4i(0, topDiff, 0, topDiff)) || fr[1].eq(fr[0] + GSVector4i(0, topDiff, 0, topDiff))))
		{
			// dq5:
			//
			// fr[0] = 0 1 512 445
			// fr[1] = 0 0 512 444
			// dr[0] = 127 50 639 494
			// dr[1] = 127 50 639 494

			int top = std::min(fr[0].top, fr[1].top);
			int bottom = std::min(fr[0].bottom, fr[1].bottom);

			fr[0].top = fr[1].top = top;
			fr[0].bottom = fr[1].bottom = bottom;
		}
	}

	GSVector2i fs(0, 0);
	GSVector2i ds(0, 0);

	GSTexture* tex[3] = {NULL, NULL, NULL};
	int y_offset[3] = {0, 0, 0};

	s_n++;

	if (samesrc && fr[0].bottom == fr[1].bottom && !feedback_merge)
	{
		tex[0] = GetOutput(0, y_offset[0]);
		tex[1] = tex[0]; // saves one texture fetch
		y_offset[1] = y_offset[0];
	}
	else
	{
		if (en[0])
			tex[0] = GetOutput(0, y_offset[0]);
		if (en[1])
			tex[1] = GetOutput(1, y_offset[1]);
		if (feedback_merge)
			tex[2] = GetFeedbackOutput();
	}

	GSVector4 src_out_rect[2];
	GSVector4 src_gs_read[2];
	GSVector4 dst[3];

	const bool slbg = m_regs->PMODE.SLBG;

	const GSVector2i resolution(GetResolution());
	bool scanmask_frame = true;

	for (int i = 0; i < 2; i++)
	{
		if (!en[i] || !tex[i])
			continue;

		GSVector4i r = GetFrameMagnifiedRect(i);
		GSVector4 scale = GSVector4(tex[i]->GetScale()).xyxy();

		const bool ignore_offset = !GSConfig.PCRTCOffsets;

		GSVector2i off(ignore_offset ? 0 : display_offsets[i]);
		GSVector2i display_diff(dr[i].left - display_baseline.x, dr[i].top - display_baseline.y);
		GSVector2i frame_diff(fr[i].left - frame_baseline.x, fr[i].top - frame_baseline.y);

		// If using scanmsk we have to keep the single line offset, regardless of upscale
		// so we handle this separately after the rect calculations.
		const int interlace_offset = display_diff.y & 1;

		if (m_scanmask_used && interlace_offset)
		{
			display_diff.y &= ~1;
			scanmask_frame = false;
			if (!ignore_offset)
				off.y &= ~1;
		}

		// All the following code is literally just to try and fill the window as much as possible and reduce blur put in by gamedevs by offsetting the DISPLAY's.
		if (!ignore_offset && display_combined.y < (resolution.y-1) && display_combined.x < (resolution.x-1))
		{
			float difference[2];
			difference[0] = resolution.x / (float)display_combined.x;
			difference[1] = resolution.y / (float)display_combined.y;

			if (difference[0] > 1.0f)
			{
				float difference_to_use = (difference[0] < difference[1]) ? difference[0] : difference[1];
				int width_change = (r.right * difference_to_use) - r.right;

				r.right += width_change;
				off.x -= width_change >> 1;

				int height_change = (r.bottom * difference_to_use) - r.bottom;
				if (height_change > 4)
				{
					r.bottom += height_change;
					off.y -= height_change >> 1;
				}
			}
			// Anti blur hax
			if (display_diff.x < 4)
			{
				off.x -= display_diff.x;
			}

			if (display_diff.y < 4)
			{
				off.y -= display_diff.y;
			}
		}
		else if(ignore_offset) // Stretch to fit the window.
		{
			float difference[2];

			//If the picture is offset we want to make sure we don't make it bigger, so this is the only place we need to now about the offset!
			difference[0] = resolution.x / (float)((display_combined.x - display_offsets[i].x) + display_diff.x);
			difference[1] = resolution.y / (float)((display_combined.y - display_offsets[i].y) + display_diff.y);

			if (difference[0] > 1.0f)
			{
				const float difference_to_use = difference[0];
				const int width_change = (r.right * difference_to_use) - r.right;

				r.right += width_change;
			}

			const float difference_to_use = difference[1];
			const int height_change = (r.bottom * difference_to_use) - r.bottom;

			if (difference[1] > 1.0f && (!m_scanmask_used || height_change > 4))
			{
				r.bottom += height_change;
			}
			// Anti blur hax.
			if (!slbg || !feedback_merge)
			{
				if (display_diff.x > 4)
					off.x = display_diff.x;

				if (display_diff.y > 4)
					off.y = display_diff.y;
			}
			
			if (!slbg || !feedback_merge)
			{
				if (samesrc)
				{
					if (display_diff.x < 4 && off.x)
						off.x = 0;
					if (display_diff.y < 4)
						off.y = 0;

					if (display_diff.x > 4)
						off.x = display_diff.x;

					if (display_diff.y > 4)
						off.y = display_diff.y;

					if (frame_diff.x == 1)
						off.x += 1;
					if (frame_diff.y == 1)
						off.y += 1;
				}
				else
				{
					if (display_diff.x > 4)
						off.x = display_diff.x;

					if (display_diff.y > 4)
						off.y = display_diff.y;
				}
			}
		}
		// Anti blur hax.
		else if (samesrc)
		{
			if (display_diff.x < 4)
				off.x -= display_diff.x;
			if (display_diff.y < 4)
				off.y -= display_diff.y;

			if (frame_diff.x == 1)
				off.x += 1;
			if (frame_diff.y == 1)
				off.y += 1;
		}
		// End of Resize/Anti-Blur code.

		// Offsets are in full rect form, needs resizing for the actual draw if interlaced half frame.
		if (m_regs->SMODE2.INT && m_regs->SMODE2.FFMD)
			off.y /= 2;

		// src_gs_read is the size which we're really reading from GS memory.
		src_gs_read[i] = ((GSVector4(fr[i]) + GSVector4(0, y_offset[i], 0, y_offset[i])) * scale) / GSVector4(tex[i]->GetSize()).xyxy();

		// src_out_rect is the resized rect for output.
		src_out_rect[i] = (GSVector4(r) * scale) / GSVector4(tex[i]->GetSize()).xyxy();

		// dst is the final destination rect with offset on the screen.
		dst[i] = scale * (GSVector4(off).xyxy() + GSVector4(r.rsize()));

		// Restore the single line offset for scanmsk.
		if (m_scanmask_used && interlace_offset)
			dst[i] += GSVector4(0.0f, 1.0f, 0.0f, 1.0f);
	}

	if (feedback_merge && tex[2])
	{
		GSVector4 scale = GSVector4(tex[2]->GetScale()).xyxy();
		GSVector4i feedback_rect;

		feedback_rect.left = m_regs->EXTBUF.WDX;
		feedback_rect.right = feedback_rect.left + ((m_regs->EXTDATA.WW + 1) / ((m_regs->EXTDATA.SMPH - m_regs->DISP[m_regs->EXTBUF.FBIN].DISPLAY.MAGH) + 1));
		feedback_rect.top = m_regs->EXTBUF.WDY;
		feedback_rect.bottom = ((m_regs->EXTDATA.WH + 1) * (2 - m_regs->EXTBUF.WFFMD)) / ((m_regs->EXTDATA.SMPV - m_regs->DISP[m_regs->EXTBUF.FBIN].DISPLAY.MAGV) + 1);

		dst[2] = GSVector4(scale * GSVector4(feedback_rect.rsize()));
	}

	fs = resolution * GSVector2i(GetUpscaleMultiplier());
	ds = fs;

	// When interlace(FRAME) mode, the rect is half height, so it needs to be stretched.
	if (m_regs->SMODE2.INT && m_regs->SMODE2.FFMD)
	{
		ds.y *= 2;
	}

	m_real_size = ds;
	
	if (tex[0] || tex[1])
	{
		if ((tex[0] == tex[1]) && (src_out_rect[0] == src_out_rect[1]).alltrue() && (dst[0] == dst[1]).alltrue() && !feedback_merge && !slbg)
		{
			// the two outputs are identical, skip drawing one of them (the one that is alpha blended)

			tex[0] = NULL;
		}

		GSVector4 c = GSVector4((int)m_regs->BGCOLOR.R, (int)m_regs->BGCOLOR.G, (int)m_regs->BGCOLOR.B, (int)m_regs->PMODE.ALP) / 255;

		g_gs_device->Merge(tex, src_gs_read, dst, fs, m_regs->PMODE, m_regs->EXTBUF, c);

		if (m_regs->SMODE2.INT && GSConfig.InterlaceMode != GSInterlaceMode::Off)
		{
			const bool scanmask = m_scanmask_used && scanmask_frame && GSConfig.InterlaceMode == GSInterlaceMode::Automatic;

			if (GSConfig.InterlaceMode == GSInterlaceMode::Automatic && (m_regs->SMODE2.FFMD)) // Auto interlace enabled / Odd frame interlace setting
			{
				constexpr int field2 = 1;
				constexpr int mode = 2;
				g_gs_device->Interlace(ds, field ^ field2, mode, tex[1] ? tex[1]->GetScale().y : tex[0]->GetScale().y);
			}
			else
			{
				const int field2 = scanmask ? 0 : 1 - ((static_cast<int>(GSConfig.InterlaceMode) - 1) & 1);
				const int offset = tex[1] ? tex[1]->GetScale().y : tex[0]->GetScale().y;
				const int mode = scanmask ? 2 : ((static_cast<int>(GSConfig.InterlaceMode) - 1) >> 1);

				g_gs_device->Interlace(ds, field ^ field2, mode, offset);
			}
		}

		if (GSConfig.ShadeBoost)
		{
			g_gs_device->ShadeBoost();
		}

		if (GSConfig.ShaderFX)
		{
			g_gs_device->ExternalFX();
		}

		if (GSConfig.FXAA)
		{
			g_gs_device->FXAA();
		}
	}

	return true;
}

GSVector2i GSRenderer::GetInternalResolution()
{
	return m_real_size;
}

static float GetCurrentAspectRatioFloat(bool is_progressive)
{
	static constexpr std::array<float, static_cast<size_t>(AspectRatioType::MaxCount) + 1> ars = { {4.0f / 3.0f, 4.0f / 3.0f, 4.0f / 3.0f, 16.0f / 9.0f, 3.0f / 2.0f} };
	return ars[static_cast<u32>(GSConfig.AspectRatio) + (3u * (is_progressive && GSConfig.AspectRatio == AspectRatioType::RAuto4_3_3_2))];
}

static GSVector4 CalculateDrawRect(s32 window_width, s32 window_height, s32 texture_width, s32 texture_height, HostDisplay::Alignment alignment, bool flip_y, bool is_progressive)
{
	const float f_width = static_cast<float>(window_width);
	const float f_height = static_cast<float>(window_height);
	const float clientAr = f_width / f_height;

	float targetAr = clientAr;
	if (EmuConfig.CurrentAspectRatio == AspectRatioType::RAuto4_3_3_2)
	{
		if (is_progressive)
			targetAr = 3.0f / 2.0f;
		else
			targetAr = 4.0f / 3.0f;
	}
	else if (EmuConfig.CurrentAspectRatio == AspectRatioType::R4_3)
	{
		targetAr = 4.0f / 3.0f;
	}
	else if (EmuConfig.CurrentAspectRatio == AspectRatioType::R16_9)
		targetAr = 16.0f / 9.0f;

	const double arr = targetAr / clientAr;
	float target_width = f_width;
	float target_height = f_height;
	if (arr < 1)
		target_width = std::floor(f_width * arr + 0.5f);
	else if (arr > 1)
		target_height = std::floor(f_height / arr + 0.5f);

	float zoom = GSConfig.Zoom / 100.0;
	if (zoom == 0) //auto zoom in untill black-bars are gone (while keeping the aspect ratio).
		zoom = std::max((float)arr, (float)(1.0 / arr));

	target_width *= zoom;
	target_height *= zoom * GSConfig.StretchY / 100.0f;

	if (GSConfig.IntegerScaling)
	{
		// make target width/height an integer multiple of the texture width/height
		const float t_width = static_cast<double>(texture_width);
		const float t_height = static_cast<double>(texture_height);

		float scale;
		if ((t_width / t_height) >= 1.0)
			scale = target_width / t_width;
		else
			scale = target_height / t_height;

		if (scale > 1.0)
		{
			const float adjust = std::floor(scale)  / scale;
			target_width = target_width * adjust;
			target_height = target_height * adjust;
		}
	}

	float target_x, target_y;
	if (target_width >= f_width)
	{
		target_x = -((target_width - f_width) * 0.5f);
	}
	else
	{
		switch (alignment)
		{
		case HostDisplay::Alignment::Center:
			target_x = (f_width - target_width) * 0.5f;
			break;
		case HostDisplay::Alignment::RightOrBottom:
			target_x = (f_width - target_width);
			break;
		case HostDisplay::Alignment::LeftOrTop:
		default:
			target_x = 0.0f;
			break;
		}
	}
	if (target_height >= f_height)
	{
		target_y = -((target_height - f_height) * 0.5f);
	}
	else
	{
		switch (alignment)
		{
		case HostDisplay::Alignment::Center:
			target_y = (f_height - target_height) * 0.5f;
			break;
		case HostDisplay::Alignment::RightOrBottom:
			target_y = (f_height - target_height);
			break;
		case HostDisplay::Alignment::LeftOrTop:
		default:
			target_y = 0.0f;
			break;
		}
	}

	const float unit = .01f * std::min(target_x, target_y);
	target_x += unit * GSConfig.OffsetX;
	target_y += unit * GSConfig.OffsetY;

	GSVector4 ret(target_x, target_y, target_x + target_width, target_y + target_height);

	if (flip_y)
	{
		const float height = ret.w - ret.y;
		ret.y = static_cast<float>(window_height) - ret.w;
		ret.w = ret.y + height;
	}

	return ret;
}

void GSRenderer::VSync(u32 field, bool registers_written)
{
	Flush();

	if (s_dump && s_n >= s_saven)
	{
		m_regs->Dump(root_sw + format("%05d_f%lld_gs_reg.txt", s_n, g_perfmon.GetFrame()));
	}

	const int fb_sprite_blits = g_perfmon.GetDisplayFramebufferSpriteBlits();
	const bool fb_sprite_frame = (fb_sprite_blits > 0);
	PerformanceMetrics::Update(registers_written, fb_sprite_frame);

	g_gs_device->AgePool();

	const bool blank_frame = !Merge(field);
	const bool skip_frame = m_frameskip;

	if (blank_frame || skip_frame)
	{
		g_gs_device->ResetAPIState();
		if (Host::BeginPresentFrame(skip_frame))
			Host::EndPresentFrame();
		g_gs_device->RestoreAPIState();
		return;
	}

	g_perfmon.EndFrame();
	if ((g_perfmon.GetFrame() & 0x1f) == 0)
		g_perfmon.Update();

	g_gs_device->ResetAPIState();
	if (Host::BeginPresentFrame(false))
	{
		GSTexture* current = g_gs_device->GetCurrent();
		if (current)
		{
			HostDisplay* const display = g_gs_device->GetDisplay();
			const GSVector4 draw_rect(CalculateDrawRect(display->GetWindowWidth(), display->GetWindowHeight(),
				current->GetWidth(), current->GetHeight(), display->GetDisplayAlignment(), display->UsesLowerLeftOrigin(), GetVideoMode() == GSVideoMode::SDTV_480P));

			static constexpr ShaderConvert s_shader[5] = {ShaderConvert::COPY, ShaderConvert::SCANLINE,
				ShaderConvert::DIAGONAL_FILTER, ShaderConvert::TRIANGULAR_FILTER,
				ShaderConvert::COMPLEX_FILTER}; // FIXME

			g_gs_device->StretchRect(current, nullptr, draw_rect, s_shader[GSConfig.TVShader], GSConfig.LinearPresent);
		}

		Host::EndPresentFrame();

		if (GSConfig.OsdShowGPU)
			PerformanceMetrics::OnGPUPresent(Host::GetHostDisplay()->GetAndResetAccumulatedGPUTime());
	}
	g_gs_device->RestoreAPIState();

	// snapshot

	if (!m_snapshot.empty())
	{
		if (!m_dump && m_shift_key)
		{
			freezeData fd = {0, nullptr};
			Freeze(&fd, true);
			fd.data = new u8[fd.size];
			Freeze(&fd, false);

			// keep the screenshot relatively small so we don't bloat the dump
			static constexpr u32 DUMP_SCREENSHOT_WIDTH = 640;
			static constexpr u32 DUMP_SCREENSHOT_HEIGHT = 480;
			std::vector<u32> screenshot_pixels;
			SaveSnapshotToMemory(DUMP_SCREENSHOT_WIDTH, DUMP_SCREENSHOT_HEIGHT, &screenshot_pixels);

			if (m_control_key)
			{
				m_dump = std::unique_ptr<GSDumpBase>(new GSDumpUncompressed(m_snapshot, GetDumpSerial(), m_crc,
					DUMP_SCREENSHOT_WIDTH, DUMP_SCREENSHOT_HEIGHT,
					screenshot_pixels.empty() ? nullptr : screenshot_pixels.data(),
					fd, m_regs));
			}
			else
			{
				m_dump = std::unique_ptr<GSDumpBase>(new GSDumpXz(m_snapshot, GetDumpSerial(), m_crc,
					DUMP_SCREENSHOT_WIDTH, DUMP_SCREENSHOT_HEIGHT,
					screenshot_pixels.empty() ? nullptr : screenshot_pixels.data(),
					fd, m_regs));
			}

			delete[] fd.data;
		}

		if (GSTexture* t = g_gs_device->GetCurrent())
		{
			t->Save(m_snapshot + ".png");
		}

		m_snapshot.clear();
	}
	else if (m_dump)
	{
		if (m_dump->VSync(field, !m_control_key, m_regs))
			m_dump.reset();
	}

	// capture

	if (m_capture.IsCapturing())
	{
		if (GSTexture* current = g_gs_device->GetCurrent())
		{
			GSVector2i size = m_capture.GetSize();

			bool res;
			GSTexture::GSMap m;
			if (size == current->GetSize())
				res = g_gs_device->DownloadTexture(current, GSVector4i(0, 0, size.x, size.y), m);
			else
				res = g_gs_device->DownloadTextureConvert(current, GSVector4(0, 0, 1, 1), size, GSTexture::Format::Color, ShaderConvert::COPY, m, true);

			if (res)
			{
				m_capture.DeliverFrame(m.bits, m.pitch, !g_gs_device->IsRBSwapped());
				g_gs_device->DownloadTextureComplete();
			}
		}
	}
}

bool GSRenderer::MakeSnapshot(const std::string& path)
{
	if (m_snapshot.empty())
	{
		// Allows for providing a complete path
		if (path.substr(path.size() - 4, 4) == ".png")
			m_snapshot = path.substr(0, path.size() - 4);
		else
		{
			time_t cur_time = time(nullptr);
			static time_t prev_snap;
			// The variable 'n' is used for labelling the screenshots when multiple screenshots are taken in
			// a single second, we'll start using this variable for naming when a second screenshot request is detected
			// at the same time as the first one. Hence, we're initially setting this counter to 2 to imply that
			// the captured image is the 2nd image captured at this specific time.
			static int n = 2;
			char local_time[16];

			if (strftime(local_time, sizeof(local_time), "%Y%m%d%H%M%S", localtime(&cur_time)))
			{
				if (cur_time == prev_snap)
					m_snapshot = format("%s_%s_(%d)", path.c_str(), local_time, n++);
				else
				{
					n = 2;
					m_snapshot = format("%s_%s", path.c_str(), local_time);
				}
				prev_snap = cur_time;
			}

			// append the game serial and title
			if (std::string name(GetDumpName()); !name.empty())
			{
				FileSystem::SanitizeFileName(name);
				m_snapshot += format("_%s", name.c_str());
			}
			if (std::string serial(GetDumpSerial()); !serial.empty())
			{
				FileSystem::SanitizeFileName(serial);
				m_snapshot += format("_%s", serial.c_str());
			}
		}
	}

	return true;
}

bool GSRenderer::BeginCapture(std::string& filename)
{
	return m_capture.BeginCapture(GetTvRefreshRate(), GetInternalResolution(), GetCurrentAspectRatioFloat(GetVideoMode() == GSVideoMode::SDTV_480P), filename);
}

void GSRenderer::EndCapture()
{
	m_capture.EndCapture();
}

void GSRenderer::KeyEvent(const HostKeyEvent& e)
{
#if !defined(PCSX2_CORE)
#ifdef _WIN32
	m_shift_key = !!(::GetAsyncKeyState(VK_SHIFT) & 0x8000);
	m_control_key = !!(::GetAsyncKeyState(VK_CONTROL) & 0x8000);
#elif defined(__APPLE__)
	m_shift_key = CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, kVK_Shift)
	           || CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, kVK_RightShift);
	m_control_key = CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, kVK_Control)
	             || CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, kVK_RightControl)
	             || CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, kVK_Command)
	             || CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, kVK_RightCommand);
#else
	switch (e.key)
	{
		case XK_Shift_L:
		case XK_Shift_R:
			m_shift_key = (e.type == HostKeyEvent::Type::KeyPressed);
			return;
		case XK_Control_L:
		case XK_Control_R:
			m_control_key = (e.type == HostKeyEvent::Type::KeyReleased);
			return;
	}
#endif

	if (e.type == HostKeyEvent::Type::KeyPressed)
	{

		int step = m_shift_key ? -1 : 1;

#if defined(__unix__)
#define VK_F5 XK_F5
#define VK_DELETE XK_Delete
#define VK_NEXT XK_Next
#elif defined(__APPLE__)
#define VK_F5 kVK_F5
#define VK_DELETE kVK_ForwardDelete
#define VK_NEXT kVK_PageDown
#endif

		// NOTE: These are all BROKEN! They mess with GS thread state from the UI thread.

		switch (e.key)
		{
			case VK_F5:
				GSConfig.InterlaceMode = static_cast<GSInterlaceMode>((static_cast<int>(GSConfig.InterlaceMode) + static_cast<int>(GSInterlaceMode::Count) + step) % static_cast<int>(GSInterlaceMode::Count));
				theApp.SetConfig("deinterlace", static_cast<int>(GSConfig.InterlaceMode));
				printf("GS: Set deinterlace mode to %d (%s).\n", static_cast<int>(GSConfig.InterlaceMode), theApp.m_gs_deinterlace.at(static_cast<int>(GSConfig.InterlaceMode)).name.c_str());
				return;
			case VK_DELETE:
				GSConfig.AA1 = !GSConfig.AA1;
				theApp.SetConfig("aa1", GSConfig.AA1);
				printf("GS: (Software) Edge anti-aliasing is now %s.\n", GSConfig.AA1 ? "enabled" : "disabled");
				return;
			case VK_NEXT: // As requested by Prafull, to be removed later
				char dither_msg[3][16] = {"disabled", "auto", "auto unscaled"};
				GSConfig.Dithering = (GSConfig.Dithering + 1) % 3;
				printf("GS: Dithering is now %s.\n", dither_msg[GSConfig.Dithering]);
				return;
		}
	}
#endif // PCSX2_CORE
}

void GSRenderer::PurgePool()
{
	g_gs_device->PurgePool();
}

void GSRenderer::PurgeTextureCache()
{
}

bool GSRenderer::SaveSnapshotToMemory(u32 width, u32 height, std::vector<u32>* pixels)
{
	GSTexture* const current = g_gs_device->GetCurrent();
	if (!current)
		return false;

	GSVector4 draw_rect(CalculateDrawRect(width, height, current->GetWidth(), current->GetHeight(),
		HostDisplay::Alignment::LeftOrTop, false, GetVideoMode() == GSVideoMode::SDTV_480P));
	u32 draw_width = static_cast<u32>(draw_rect.z - draw_rect.x);
	u32 draw_height = static_cast<u32>(draw_rect.w - draw_rect.y);
	if (draw_width > width)
	{
		draw_width = width;
		draw_rect.left = 0;
		draw_rect.right = width;
	}
	if (draw_height > height)
	{
		draw_height = height;
		draw_rect.top = 0;
		draw_rect.bottom = height;
	}

	GSTexture::GSMap map;
	const bool result = g_gs_device->DownloadTextureConvert(
		current, GSVector4(0.0f, 0.0f, 1.0f, 1.0f),
		GSVector2i(draw_width, draw_height), GSTexture::Format::Color,
		ShaderConvert::COPY, map, true);
	if (result)
	{
		const u32 pad_x = (width - draw_width) / 2;
		const u32 pad_y = (height - draw_height) / 2;
		pixels->resize(width * height, 0);
		StringUtil::StrideMemCpy(pixels->data() + pad_y * width + pad_x, width * sizeof(u32),
			map.bits, map.pitch, draw_width * sizeof(u32), draw_height);

		g_gs_device->DownloadTextureComplete();
	}

	return result;
}
