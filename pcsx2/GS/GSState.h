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

#pragma once

#include "GS.h"
#include "GSLocalMemory.h"
#include "GSDrawingContext.h"
#include "GSDrawingEnvironment.h"
#include "Renderers/Common/GSVertex.h"
#include "Renderers/Common/GSVertexTrace.h"
#include "GSUtil.h"
#include "GSPerfMon.h"
#include "GSVector.h"
#include "Renderers/Common/GSDevice.h"
#include "GSCrc.h"
#include "GSAlignedClass.h"
#include "GSDump.h"

class GSState : public GSAlignedClass<32>
{
public:
	GSState();
	virtual ~GSState();

	static constexpr int GetSaveStateSize();

private:
	// RESTRICT prevents multiple loads of the same part of the register when accessing its bitfields (the compiler is happy to know that memory writes in-between will not go there)

	typedef void (GSState::*GIFPackedRegHandler)(const GIFPackedReg* RESTRICT r);

	GIFPackedRegHandler m_fpGIFPackedRegHandlers[16] = {};
	GIFPackedRegHandler m_fpGIFPackedRegHandlerXYZ[8][4] = {};

	void CheckFlushes();

	void GIFPackedRegHandlerNull(const GIFPackedReg* RESTRICT r);
	void GIFPackedRegHandlerRGBA(const GIFPackedReg* RESTRICT r);
	void GIFPackedRegHandlerSTQ(const GIFPackedReg* RESTRICT r);
	void GIFPackedRegHandlerUV(const GIFPackedReg* RESTRICT r);
	void GIFPackedRegHandlerUV_Hack(const GIFPackedReg* RESTRICT r);
	template<u32 prim, u32 adc, bool auto_flush, bool index_swap> void GIFPackedRegHandlerXYZF2(const GIFPackedReg* RESTRICT r);
	template<u32 prim, u32 adc, bool auto_flush, bool index_swap> void GIFPackedRegHandlerXYZ2(const GIFPackedReg* RESTRICT r);
	void GIFPackedRegHandlerFOG(const GIFPackedReg* RESTRICT r);
	void GIFPackedRegHandlerA_D(const GIFPackedReg* RESTRICT r);
	void GIFPackedRegHandlerNOP(const GIFPackedReg* RESTRICT r);

	typedef void (GSState::*GIFRegHandler)(const GIFReg* RESTRICT r);

	GIFRegHandler m_fpGIFRegHandlers[256] = {};
	GIFRegHandler m_fpGIFRegHandlerXYZ[8][4] = {};

	typedef void (GSState::*GIFPackedRegHandlerC)(const GIFPackedReg* RESTRICT r, u32 size);

	GIFPackedRegHandlerC m_fpGIFPackedRegHandlersC[2] = {};
	GIFPackedRegHandlerC m_fpGIFPackedRegHandlerSTQRGBAXYZF2[8] = {};
	GIFPackedRegHandlerC m_fpGIFPackedRegHandlerSTQRGBAXYZ2[8] = {};

	template<u32 prim, bool auto_flush, bool index_swap> void GIFPackedRegHandlerSTQRGBAXYZF2(const GIFPackedReg* RESTRICT r, u32 size);
	template<u32 prim, bool auto_flush, bool index_swap> void GIFPackedRegHandlerSTQRGBAXYZ2(const GIFPackedReg* RESTRICT r, u32 size);
	void GIFPackedRegHandlerNOP(const GIFPackedReg* RESTRICT r, u32 size);

	template<int i> void ApplyTEX0(GIFRegTEX0& TEX0);
	void ApplyPRIM(u32 prim);

	void GIFRegHandlerNull(const GIFReg* RESTRICT r);
	void GIFRegHandlerPRIM(const GIFReg* RESTRICT r);
	void GIFRegHandlerRGBAQ(const GIFReg* RESTRICT r);
	void GIFRegHandlerST(const GIFReg* RESTRICT r);
	void GIFRegHandlerUV(const GIFReg* RESTRICT r);
	void GIFRegHandlerUV_Hack(const GIFReg* RESTRICT r);
	template<u32 prim, u32 adc, bool auto_flush, bool index_swap> void GIFRegHandlerXYZF2(const GIFReg* RESTRICT r);
	template<u32 prim, u32 adc, bool auto_flush, bool index_swap> void GIFRegHandlerXYZ2(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerTEX0(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerCLAMP(const GIFReg* RESTRICT r);
	void GIFRegHandlerFOG(const GIFReg* RESTRICT r);
	void GIFRegHandlerNOP(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerTEX1(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerTEX2(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerXYOFFSET(const GIFReg* RESTRICT r);
	void GIFRegHandlerPRMODECONT(const GIFReg* RESTRICT r);
	void GIFRegHandlerPRMODE(const GIFReg* RESTRICT r);
	void GIFRegHandlerTEXCLUT(const GIFReg* RESTRICT r);
	void GIFRegHandlerSCANMSK(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerMIPTBP1(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerMIPTBP2(const GIFReg* RESTRICT r);
	void GIFRegHandlerTEXA(const GIFReg* RESTRICT r);
	void GIFRegHandlerFOGCOL(const GIFReg* RESTRICT r);
	void GIFRegHandlerTEXFLUSH(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerSCISSOR(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerALPHA(const GIFReg* RESTRICT r);
	void GIFRegHandlerDIMX(const GIFReg* RESTRICT r);
	void GIFRegHandlerDTHE(const GIFReg* RESTRICT r);
	void GIFRegHandlerCOLCLAMP(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerTEST(const GIFReg* RESTRICT r);
	void GIFRegHandlerPABE(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerFBA(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerFRAME(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerZBUF(const GIFReg* RESTRICT r);
	void GIFRegHandlerBITBLTBUF(const GIFReg* RESTRICT r);
	void GIFRegHandlerTRXPOS(const GIFReg* RESTRICT r);
	void GIFRegHandlerTRXREG(const GIFReg* RESTRICT r);
	void GIFRegHandlerTRXDIR(const GIFReg* RESTRICT r);
	void GIFRegHandlerHWREG(const GIFReg* RESTRICT r);

	template<bool auto_flush, bool index_swap>
	void SetPrimHandlers();

	struct GSTransferBuffer
	{
		int x = 0, y = 0;
		int start = 0, end = 0, total = 0;
		u8* buff = nullptr;
		GIFRegBITBLTBUF m_blit = {};

		GSTransferBuffer();
		virtual ~GSTransferBuffer();

		void Init(int tx, int ty, const GIFRegBITBLTBUF& blit);
		bool Update(int tw, int th, int bpp, int& len);

	} m_tr;

private:
	void CalcAlphaMinMax();

protected:
	GSVertex m_v = {};
	float m_q = 1.0f;
	GSVector4i m_scissor = {};
	GSVector4i m_ofxy = {};

	u8 m_scanmask_used = 0;
	bool tex_flushed = true;
	bool m_isPackedUV_HackFlag = false;

	struct
	{
		GSVertex* buff;
		u32 head, tail, next, maxcount; // head: first vertex, tail: last vertex + 1, next: last indexed + 1
		u32 xy_tail;
		u64 xy[4];
	} m_vertex = {};

	struct
	{
		u32* buff;
		u32 tail;
	} m_index = {};

	void UpdateContext();
	void UpdateScissor();

	void UpdateVertexKick();

	void GrowVertexBuffer();
	bool IsAutoFlushDraw();
	template<u32 prim, bool index_swap>
	void HandleAutoFlush();
	void CLUTAutoFlush(u32 prim);

	template <u32 prim, bool auto_flush, bool index_swap>
	void VertexKick(u32 skip);

	// following functions need m_vt to be initialized

	GSVertexTrace m_vt;
	GSVertexTrace::VertexAlpha& GetAlphaMinMax()
	{
		if (!m_vt.m_alpha.valid)
			CalcAlphaMinMax();
		return m_vt.m_alpha;
	}
	struct TextureMinMaxResult
	{
		enum UsesBoundary
		{
			USES_BOUNDARY_LEFT   = 1 << 0,
			USES_BOUNDARY_TOP    = 1 << 1,
			USES_BOUNDARY_RIGHT  = 1 << 2,
			USES_BOUNDARY_BOTTOM = 1 << 3,
			USES_BOUNDARY_U = USES_BOUNDARY_LEFT | USES_BOUNDARY_RIGHT,
			USES_BOUNDARY_V = USES_BOUNDARY_TOP | USES_BOUNDARY_BOTTOM,
		};
		GSVector4i coverage; ///< Part of the texture used
		u8 uses_boundary;    ///< Whether or not the usage touches the left, top, right, or bottom edge (and therefore needs wrap modes preserved)
	};
	TextureMinMaxResult GetTextureMinMax(const GIFRegTEX0& TEX0, const GIFRegCLAMP& CLAMP, bool linear);
	bool TryAlphaTest(u32& fm, const u32 fm_mask, u32& zm);
	bool IsOpaque();
	bool IsMipMapDraw();
	bool IsMipMapActive();
	bool IsCoverageAlpha();

public:
	struct GSUploadQueue
	{
		GIFRegBITBLTBUF blit;
		int draw;
	};

	GIFPath m_path[4] = {};
	GIFRegPRIM* PRIM = nullptr;
	GSPrivRegSet* m_regs = nullptr;
	GSLocalMemory m_mem;
	GSDrawingEnvironment m_env = {};
	GSDrawingEnvironment m_backup_env = {};
	GSDrawingEnvironment m_prev_env = {};
	GSVector4i temp_draw_rect = {};
	GSDrawingContext* m_context = nullptr;
	u32 m_crc = 0;
	CRC::Game m_game = {};
	std::unique_ptr<GSDumpBase> m_dump;
	bool m_nativeres = false;
	bool m_mipmap = false;
	u8 m_force_preload = 0;
	u32 m_dirty_gs_regs = 0;
	int m_backed_up_ctx = 0;
	std::vector<GSUploadQueue> m_draw_transfers;

	static int s_n;

	static constexpr u32 STATE_VERSION = 8;

	enum REG_DIRTY
	{
		DIRTY_REG_ALPHA,
		DIRTY_REG_CLAMP,
		DIRTY_REG_COLCLAMP,
		DIRTY_REG_DIMX,
		DIRTY_REG_DTHE,
		DIRTY_REG_FBA,
		DIRTY_REG_FOGCOL,
		DIRTY_REG_FRAME,
		DIRTY_REG_MIPTBP1,
		DIRTY_REG_MIPTBP2,
		DIRTY_REG_PABE,
		DIRTY_REG_PRIM,
		DIRTY_REG_SCANMSK,
		DIRTY_REG_SCISSOR,
		DIRTY_REG_TEST,
		DIRTY_REG_TEX0,
		DIRTY_REG_TEX1,
		DIRTY_REG_TEXA,
		DIRTY_REG_XYOFFSET,
		DIRTY_REG_ZBUF
	};

	enum GSFlushReason
	{
		UNKNOWN = 1 << 0,
		RESET = 1 << 1,
		CONTEXTCHANGE = 1 << 2,
		CLUTCHANGE = 1 << 3,
		TEXFLUSH = 1 << 4,
		GSTRANSFER = 1 << 5,
		UPLOADDIRTYTEX = 1 << 6,
		LOCALTOLOCALMOVE = 1 << 7,
		DOWNLOADFIFO = 1 << 8,
		SAVESTATE = 1 << 9,
		LOADSTATE = 1 << 10,
		AUTOFLUSH = 1 << 11,
		VSYNC  = 1 << 12,
		GSREOPEN = 1 << 13,
	};

	GSFlushReason m_state_flush_reason = UNKNOWN;

	enum PRIM_OVERLAP
	{
		PRIM_OVERLAP_UNKNOW,
		PRIM_OVERLAP_YES,
		PRIM_OVERLAP_NO
	};

	PRIM_OVERLAP m_prim_overlap = PRIM_OVERLAP_UNKNOW;
	std::vector<size_t> m_drawlist;

	struct GSPCRTCRegs
	{
		// The horizontal offset values (under z) for PAL and NTSC have been tweaked
		// they should be apparently 632 and 652 respectively, but that causes a thick black line on the left
		// these values leave a small black line on the right in a bunch of games, but it's not so bad.
		// The only conclusion I can come to is there is horizontal overscan expected so there would normally
		// be black borders either side anyway, or both sides slightly covered.
		static inline constexpr GSVector4i VideoModeOffsets[6] = {
			GSVector4i::cxpr(640, 224, 642, 25),
			GSVector4i::cxpr(640, 256, 676, 36),
			GSVector4i::cxpr(640, 480, 276, 34),
			GSVector4i::cxpr(720, 480, 232, 35),
			GSVector4i::cxpr(1280, 720, 302, 24),
			GSVector4i::cxpr(1920, 540, 238, 40)
		};

		static inline constexpr GSVector4i VideoModeOffsetsOverscan[6] = {
			GSVector4i::cxpr(711, 243, 498, 12),
			GSVector4i::cxpr(702, 288, 532, 18),
			GSVector4i::cxpr(640, 480, 276, 34),
			GSVector4i::cxpr(720, 480, 232, 35),
			GSVector4i::cxpr(1280, 720, 302, 24),
			GSVector4i::cxpr(1920, 540, 238, 40)
		};

		static inline constexpr GSVector4i VideoModeDividers[6] = {
			GSVector4i::cxpr(3, 0, 2559, 239),
			GSVector4i::cxpr(3, 0, 2559, 287),
			GSVector4i::cxpr(1, 0, 1279, 479),
			GSVector4i::cxpr(1, 0, 1439, 479),
			GSVector4i::cxpr(0, 0, 1279, 719),
			GSVector4i::cxpr(0, 0, 1919, 1079)
		};

		struct PCRTCDisplay
		{
			bool enabled;
			int FBP;
			int FBW;
			int PSM;
			GSRegDISPFB prevFramebufferReg;
			GSVector2i prevDisplayOffset;
			GSVector2i displayOffset;
			GSVector4i displayRect;
			GSVector2i magnification;
			GSVector2i prevFramebufferOffsets;
			GSVector2i framebufferOffsets;
			GSVector4i framebufferRect;

			int Block()
			{
				return FBP << 5;
			}
		};

		int videomode = 0;
		int interlaced = 0;
		int FFMD = 0;
		bool PCRTCSameSrc = false;
		bool toggling_field = false;
		PCRTCDisplay PCRTCDisplays[2] = {};

		bool IsAnalogue()
		{
			GSVideoMode video = static_cast<GSVideoMode>(videomode + 1);
			return video == GSVideoMode::NTSC || video == GSVideoMode::PAL || video == GSVideoMode::HDTV_1080I;
		}

		// Calculates which display is closest to matching zero offsets in either direction.
		GSVector2i NearestToZeroOffset()
		{
			GSVector2i returnValue = { 1, 1 };

			if (!PCRTCDisplays[0].enabled && !PCRTCDisplays[1].enabled)
				return returnValue;

			for (int i = 0; i < 2; i++)
			{
				if (!PCRTCDisplays[i].enabled)
				{
					returnValue.x = 1 - i;
					returnValue.y = 1 - i;
					return returnValue;
				}
			}

			if (abs(PCRTCDisplays[0].displayOffset.x - VideoModeOffsets[videomode].z) <
				abs(PCRTCDisplays[1].displayOffset.x - VideoModeOffsets[videomode].z))
				returnValue.x = 0;

			// When interlaced, the vertical base offset is doubled
			int verticalOffset = VideoModeOffsets[videomode].w * (1 << interlaced);

			if (abs(PCRTCDisplays[0].displayOffset.y - verticalOffset) <
				abs(PCRTCDisplays[1].displayOffset.y - verticalOffset))
				returnValue.y = 0;

			return returnValue;
		}

		void SetVideoMode(GSVideoMode videoModeIn)
		{
			videomode = static_cast<int>(videoModeIn) - 1;
		}

		// Enable each of the displays.
		void EnableDisplays(GSRegPMODE pmode, GSRegSMODE2 smode2, bool smodetoggle)
		{
			PCRTCDisplays[0].enabled = pmode.EN1;
			PCRTCDisplays[1].enabled = pmode.EN2;

			interlaced = smode2.INT && IsAnalogue();
			FFMD = smode2.FFMD;
			toggling_field = smodetoggle && IsAnalogue();
		}

		void CheckSameSource()
		{
			if (PCRTCDisplays[0].enabled != PCRTCDisplays[1].enabled || (PCRTCDisplays[0].enabled | PCRTCDisplays[1].enabled) == false)
			{
				PCRTCSameSrc = false;
				return;
			}

			PCRTCSameSrc = PCRTCDisplays[0].FBP == PCRTCDisplays[1].FBP &&
			PCRTCDisplays[0].FBW == PCRTCDisplays[1].FBW &&
			GSUtil::HasCompatibleBits(PCRTCDisplays[0].PSM, PCRTCDisplays[1].PSM);
		}
		
		bool FrameWrap()
		{
			GSVector4i combined_rect = GSVector4i(PCRTCDisplays[0].framebufferRect.runion(PCRTCDisplays[1].framebufferRect));
			return combined_rect.w >= 2048 || combined_rect.z >= 2048;
		}

		// If the start point of both frames match, we can do a single read
		bool FrameRectMatch()
		{
			return PCRTCSameSrc;
		}

		GSVector2i GetResolution()
		{
			GSVector2i resolution;

			const GSVector4i offsets = !GSConfig.PCRTCOverscan ? VideoModeOffsets[videomode] : VideoModeOffsetsOverscan[videomode];
			const bool is_full_height = interlaced || (toggling_field && GSConfig.InterlaceMode != GSInterlaceMode::Off) || GSConfig.InterlaceMode == GSInterlaceMode::Off;

			if (!GSConfig.PCRTCOffsets)
			{
				if (PCRTCDisplays[0].enabled && PCRTCDisplays[1].enabled)
				{
					GSVector4i combined_size = PCRTCDisplays[0].displayRect.runion(PCRTCDisplays[1].displayRect);
					resolution = { combined_size.width(), combined_size.height() };
				}
				else if (PCRTCDisplays[0].enabled)
				{
					resolution = { PCRTCDisplays[0].displayRect.width(), PCRTCDisplays[0].displayRect.height() };
				}
				else
				{
					resolution = { PCRTCDisplays[1].displayRect.width(), PCRTCDisplays[1].displayRect.height() };
				}
			}
			else
			{
				const int shift = is_full_height ? 1 : 0;
				resolution = { offsets.x, offsets.y << shift };
			}

			resolution.x = std::min(resolution.x, offsets.x);
			resolution.y = std::min(resolution.y, is_full_height ? offsets.y << 1 : offsets.y);

			return resolution;
		}

		GSVector4i GetFramebufferRect(int display)
		{
			if (display == -1)
			{
				return GSVector4i(PCRTCDisplays[0].framebufferRect.runion(PCRTCDisplays[1].framebufferRect));
			}
			else
			{
				return PCRTCDisplays[display].framebufferRect;
			}
		}

		int GetFramebufferBitDepth()
		{
			if (PCRTCDisplays[0].enabled)
				return GSLocalMemory::m_psm[PCRTCDisplays[0].PSM].bpp;
			else if (PCRTCDisplays[1].enabled)
				return GSLocalMemory::m_psm[PCRTCDisplays[1].PSM].bpp;

			return 32;
		}

		GSVector2i GetFramebufferSize(int display)
		{
			if (display == -1)
			{
				GSVector4i combined_rect = PCRTCDisplays[0].framebufferRect.runion(PCRTCDisplays[1].framebufferRect);

				if (combined_rect.z >= 2048)
				{
					int high_x = (PCRTCDisplays[0].framebufferRect.x > PCRTCDisplays[1].framebufferRect.x) ? PCRTCDisplays[0].framebufferRect.x : PCRTCDisplays[1].framebufferRect.x;
					combined_rect.z -= GSConfig.UseHardwareRenderer() ? 2048 : high_x;
					combined_rect.x = 0;
				}

				if (combined_rect.w >= 2048)
				{
					int high_y = (PCRTCDisplays[0].framebufferRect.y > PCRTCDisplays[1].framebufferRect.y) ? PCRTCDisplays[0].framebufferRect.y : PCRTCDisplays[1].framebufferRect.y;
					combined_rect.w -= GSConfig.UseHardwareRenderer() ? 2048 : high_y;
					combined_rect.y = 0;
				}
				return GSVector2i(combined_rect.z, combined_rect.w);
			}
			else
			{
				GSVector4i out_rect = PCRTCDisplays[display].framebufferRect;

				if (out_rect.z >= 2048)
				{
					out_rect.z -= GSConfig.UseHardwareRenderer() ? 2048 : out_rect.x;
					out_rect.x = 0;
				}

				if (out_rect.w >= 2048)
				{
					out_rect.w -= GSConfig.UseHardwareRenderer() ? 2048 : out_rect.y;
					out_rect.y = 0;
				}
				return GSVector2i(out_rect.z, out_rect.w);
			}
		}

		// Sets up the rectangles for both the framebuffer read and the displays for the merge circuit.
		void SetRects(int display, GSRegDISPLAY displayReg, GSRegDISPFB framebufferReg)
		{
			// Save framebuffer information first, while we're here.
			PCRTCDisplays[display].FBP = framebufferReg.FBP;
			PCRTCDisplays[display].FBW = framebufferReg.FBW;
			PCRTCDisplays[display].PSM = framebufferReg.PSM;
			PCRTCDisplays[display].prevFramebufferReg = framebufferReg;
			// Probably not really enabled but will cause a mess.
			// Q-Ball Billiards enables both circuits but doesn't set one of them up.
			if (PCRTCDisplays[display].FBW == 0 && displayReg.DW == 0 && displayReg.DH == 0 && displayReg.MAGH == 0)
			{
				PCRTCDisplays[display].enabled = false;
				return;
			}
			PCRTCDisplays[display].magnification = GSVector2i(displayReg.MAGH + 1, displayReg.MAGV + 1);
			const u32 DW = displayReg.DW + 1;
			const u32 DH = displayReg.DH + 1;

			const int renderWidth = DW / PCRTCDisplays[display].magnification.x;
			const int renderHeight = DH / PCRTCDisplays[display].magnification.y;

			u32 finalDisplayWidth = renderWidth;
			u32 finalDisplayHeight = renderHeight;
			// When using screen offsets the screen gets squashed/resized in to the actual screen size.
			if (GSConfig.PCRTCOffsets)
			{
				finalDisplayWidth = DW / (VideoModeDividers[videomode].x + 1);
				finalDisplayHeight = DH / (VideoModeDividers[videomode].y + 1);
			}
			else
			{
				finalDisplayWidth = std::min(finalDisplayWidth ,DW / (VideoModeDividers[videomode].x + 1));
				finalDisplayHeight = std::min(finalDisplayHeight, DH / (VideoModeDividers[videomode].y + 1));
			}

			// Framebuffer size and offsets.
			PCRTCDisplays[display].prevFramebufferOffsets = PCRTCDisplays[display].framebufferOffsets;
			PCRTCDisplays[display].framebufferRect.x = 0;
			PCRTCDisplays[display].framebufferRect.y = 0;
			PCRTCDisplays[display].framebufferRect.z = renderWidth;

			if(FFMD && interlaced) // Round up the height as if it's an odd value, this will cause havok with the merge circuit.
				PCRTCDisplays[display].framebufferRect.w = (renderHeight + 1) >> (FFMD * interlaced); // Half height read if FFMD + INT enabled.
			else
				PCRTCDisplays[display].framebufferRect.w = renderHeight;
			PCRTCDisplays[display].framebufferOffsets.x = framebufferReg.DBX;
			PCRTCDisplays[display].framebufferOffsets.y = framebufferReg.DBY;

			const bool is_interlaced_resolution = interlaced || (toggling_field && GSConfig.InterlaceMode != GSInterlaceMode::Off);

			// If the interlace flag isn't set, but it's still interlacing, the height is likely reported wrong.
			// Q-Ball Billiards.
			if (is_interlaced_resolution && !interlaced)
				finalDisplayHeight *= 2;

			// Display size and offsets.
			PCRTCDisplays[display].displayRect.x = 0;
			PCRTCDisplays[display].displayRect.y = 0;
			PCRTCDisplays[display].displayRect.z = finalDisplayWidth;
			PCRTCDisplays[display].displayRect.w = finalDisplayHeight;
			PCRTCDisplays[display].prevDisplayOffset = PCRTCDisplays[display].displayOffset;
			PCRTCDisplays[display].displayOffset.x = displayReg.DX;
			PCRTCDisplays[display].displayOffset.y = displayReg.DY;
		}

		// Calculate framebuffer read offsets, should be considered if only one circuit is enabled, or difference is more than 1 line.
		// Only considered if "Anti-blur" is enabled.
		void CalculateFramebufferOffset()
		{
			if (GSConfig.PCRTCAntiBlur && PCRTCDisplays[0].enabled && PCRTCSameSrc)
			{
				if (abs(PCRTCDisplays[1].framebufferOffsets.y - PCRTCDisplays[0].framebufferOffsets.y) == 1
					&& PCRTCDisplays[0].displayOffset.y == PCRTCDisplays[1].displayOffset.y)
				{
					if (PCRTCDisplays[1].framebufferOffsets.y < PCRTCDisplays[0].framebufferOffsets.y)
						PCRTCDisplays[0].framebufferOffsets.y = PCRTCDisplays[1].framebufferOffsets.y;
					else
						PCRTCDisplays[1].framebufferOffsets.y = PCRTCDisplays[0].framebufferOffsets.y;
				}
			}
			PCRTCDisplays[0].framebufferRect.x += PCRTCDisplays[0].framebufferOffsets.x;
			PCRTCDisplays[0].framebufferRect.z += PCRTCDisplays[0].framebufferOffsets.x;
			PCRTCDisplays[0].framebufferRect.y += PCRTCDisplays[0].framebufferOffsets.y;
			PCRTCDisplays[0].framebufferRect.w += PCRTCDisplays[0].framebufferOffsets.y;

			PCRTCDisplays[1].framebufferRect.x += PCRTCDisplays[1].framebufferOffsets.x;
			PCRTCDisplays[1].framebufferRect.z += PCRTCDisplays[1].framebufferOffsets.x;
			PCRTCDisplays[1].framebufferRect.y += PCRTCDisplays[1].framebufferOffsets.y;
			PCRTCDisplays[1].framebufferRect.w += PCRTCDisplays[1].framebufferOffsets.y;
		}

		// Used in software mode to align the buffer when reading. Offset is accounted for (block aligned) by GetOutput.
		void RemoveFramebufferOffset(int display)
		{
			if (display >= 0)
			{
				// Hardware needs nothing but handling for wrapped framebuffers.
				if (GSConfig.UseHardwareRenderer())
				{
					if (PCRTCDisplays[display].framebufferRect.z >= 2048)
					{
						PCRTCDisplays[display].framebufferRect.x = 0;
						PCRTCDisplays[display].framebufferRect.z -= 2048;
					}
					if (PCRTCDisplays[display].framebufferRect.w >= 2048)
					{
						PCRTCDisplays[display].framebufferRect.y = 0;
						PCRTCDisplays[display].framebufferRect.w -= 2048;
					}
				}
				else
				{
					const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[PCRTCDisplays[display].PSM];

					GSVector4i r = PCRTCDisplays[display].framebufferRect;
					r = r.ralign<Align_Outside>(psm.bs);

					PCRTCDisplays[display].framebufferRect.z -= r.x;
					PCRTCDisplays[display].framebufferRect.w -= r.y;
					PCRTCDisplays[display].framebufferRect.x -= r.x;
					PCRTCDisplays[display].framebufferRect.y -= r.y;
				}
			}
			else
			{
				// This code is to read the framebuffer nicely block aligned in software, then leave the remaining offset in to the block.
				// In hardware mode this doesn't happen, it reads the whole framebuffer, so we need to keep the offset.
				if (!GSConfig.UseHardwareRenderer())
				{
					const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[PCRTCDisplays[1].PSM];

					GSVector4i r = PCRTCDisplays[0].framebufferRect.runion(PCRTCDisplays[1].framebufferRect);
					r = r.ralign<Align_Outside>(psm.bs);

					PCRTCDisplays[0].framebufferRect.x -= r.x;
					PCRTCDisplays[0].framebufferRect.y -= r.y;
					PCRTCDisplays[0].framebufferRect.z -= r.x;
					PCRTCDisplays[0].framebufferRect.w -= r.y;
					PCRTCDisplays[1].framebufferRect.x -= r.x;
					PCRTCDisplays[1].framebufferRect.y -= r.y;
					PCRTCDisplays[1].framebufferRect.z -= r.x;
					PCRTCDisplays[1].framebufferRect.w -= r.y;
				}
			}
		}

		// If the two displays are offset from each other, move them to the correct offsets.
		// If using screen offsets, calculate the positions here.
		void CalculateDisplayOffset(bool scanmask)
		{
			// Offsets are generally ignored, the "hacky" way of doing the displays, but direct to framebuffers.
			if (!GSConfig.PCRTCOffsets)
			{
				const GSVector4i offsets = !GSConfig.PCRTCOverscan ? VideoModeOffsets[videomode] : VideoModeOffsetsOverscan[videomode];
				int int_off[2] = { 0, 0 };
				GSVector2i zeroDisplay = NearestToZeroOffset();
				GSVector2i baseOffset = PCRTCDisplays[zeroDisplay.y].displayOffset;

				int blurOffset = abs(PCRTCDisplays[1].displayOffset.y - PCRTCDisplays[0].displayOffset.y);
				if (GSConfig.PCRTCAntiBlur && !scanmask && blurOffset < 4)
				{
					if (PCRTCDisplays[1].displayOffset.y > PCRTCDisplays[0].displayOffset.y)
						PCRTCDisplays[1].displayOffset.y -= blurOffset;
					else
						PCRTCDisplays[0].displayOffset.y -= blurOffset;
				}

				// If there's a single pixel offset, account for it else it can throw interlacing out.
				for (int i = 0; i < 2; i++)
				{
					if (!PCRTCDisplays[i].enabled)
						continue;

					// Should this be MAGV/H in the DISPLAY register rather than the "default" magnification?
					const int offset = (PCRTCDisplays[i].displayOffset.y - (offsets.w * (interlaced + 1))) / (VideoModeDividers[videomode].y + 1);

					if (offset > 4)
						continue;

					int_off[i] = offset & 1;
					if (offset < 0)
						int_off[i] = -int_off[i];

					PCRTCDisplays[i].displayRect.y += int_off[i];
					PCRTCDisplays[i].displayRect.w += int_off[i];
				}

				// Handle difference in offset between the two displays, used in games like DmC and Time Crisis 2 (for split screen).
				// Offset is not screen based, but relative to each other.
				if (PCRTCDisplays[0].enabled && PCRTCDisplays[1].enabled)
				{
					GSVector2i offset;

					offset.x = (PCRTCDisplays[1 - zeroDisplay.x].displayOffset.x - PCRTCDisplays[zeroDisplay.x].displayOffset.x) / (VideoModeDividers[videomode].x + 1);
					offset.y = (PCRTCDisplays[1 - zeroDisplay.y].displayOffset.y - PCRTCDisplays[zeroDisplay.y].displayOffset.y) / (VideoModeDividers[videomode].y + 1);

					if (offset.x >= 4 || !GSConfig.PCRTCAntiBlur)
					{
						PCRTCDisplays[1 - zeroDisplay.x].displayRect.x += offset.x;
						PCRTCDisplays[1 - zeroDisplay.x].displayRect.z += offset.x;
					}
					if (offset.y >= 4 || !GSConfig.PCRTCAntiBlur)
					{
						PCRTCDisplays[1 - zeroDisplay.y].displayRect.y += offset.y - int_off[1 - zeroDisplay.y];
						PCRTCDisplays[1 - zeroDisplay.y].displayRect.w += offset.y - int_off[1 - zeroDisplay.y];
					}

					baseOffset = PCRTCDisplays[zeroDisplay.y].displayOffset;
				}

				// Handle any large vertical offset from the zero position on the screen.
				// Example: Hokuto no Ken, does a rougly -14 offset to bring the screen up.
				// Ignore the lowest bit, we've already accounted for this
				int vOffset = ((static_cast<int>(baseOffset.y) - (offsets.w * (interlaced + 1))) / (VideoModeDividers[videomode].y + 1));

				if(vOffset <= 4 && vOffset != 0)
				{
					PCRTCDisplays[0].displayRect.y += vOffset - int_off[0];
					PCRTCDisplays[0].displayRect.w += vOffset - int_off[0];
					PCRTCDisplays[1].displayRect.y += vOffset - int_off[1];
					PCRTCDisplays[1].displayRect.w += vOffset - int_off[1];
				}
			}
			else // We're using screen offsets, so just calculate the entire offset.
			{
				const GSVector4i offsets = !GSConfig.PCRTCOverscan ? VideoModeOffsets[videomode] : VideoModeOffsetsOverscan[videomode];
				GSVector2i offset = { 0, 0 };

				int blurOffset = abs(PCRTCDisplays[1].displayOffset.y - PCRTCDisplays[0].displayOffset.y);
				if (GSConfig.PCRTCAntiBlur && !scanmask && blurOffset < 4)
				{
					if (PCRTCDisplays[1].displayOffset.y > PCRTCDisplays[0].displayOffset.y)
						PCRTCDisplays[1].displayOffset.y -= blurOffset;
					else
						PCRTCDisplays[0].displayOffset.y -= blurOffset;
				}

				for (int i = 0; i < 2; i++)
				{
					// Should this be MAGV/H in the DISPLAY register rather than the "default" magnification?
					offset.x = (static_cast<int>(PCRTCDisplays[i].displayOffset.x) - offsets.z) / (VideoModeDividers[videomode].x + 1);
					offset.y = (static_cast<int>(PCRTCDisplays[i].displayOffset.y) - (offsets.w * (interlaced + 1))) / (VideoModeDividers[videomode].y + 1);

					PCRTCDisplays[i].displayRect.x += offset.x;
					PCRTCDisplays[i].displayRect.z += offset.x;
					PCRTCDisplays[i].displayRect.y += offset.y;
					PCRTCDisplays[i].displayRect.w += offset.y;
				}
			}
		}
	} PCRTCDisplays;

public:
	/// Returns the appropriate directory for draw dumping.
	static std::string GetDrawDumpPath(const char* format, ...);

	void ResetHandlers();
	void ResetPCRTC();

	GSVideoMode GetVideoMode();

	bool isinterlaced();
	bool isReallyInterlaced();

	float GetTvRefreshRate();

	virtual void Reset(bool hardware_reset);
	virtual void UpdateSettings(const Pcsx2Config::GSOptions& old_config);

	void CopyEnv(GSDrawingEnvironment* dest, GSDrawingEnvironment* src, int ctx);
	void Flush(GSFlushReason reason);
	void FlushPrim();
	bool TestDrawChanged();
	void FlushWrite();
	virtual void Draw() = 0;
	virtual void PurgePool() = 0;
	virtual void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r, bool eewrite = false) {}
	virtual void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r, bool clut = false) {}
	virtual void ExpandTarget(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r) {}

	virtual void Move();

	void Write(const u8* mem, int len);
	void Read(u8* mem, int len);
	void InitReadFIFO(u8* mem, int len);

	void SoftReset(u32 mask);
	void WriteCSR(u32 csr) { m_regs->CSR.U32[1] = csr; }
	void ReadFIFO(u8* mem, int size);
	void ReadLocalMemoryUnsync(u8* mem, int qwc, GIFRegBITBLTBUF BITBLTBUF, GIFRegTRXPOS TRXPOS, GIFRegTRXREG TRXREG);
	template<int index> void Transfer(const u8* mem, u32 size);
	int Freeze(freezeData* fd, bool sizeonly);
	int Defrost(const freezeData* fd);

	u32 GetGameCRC() const { return m_crc; }
	virtual void SetGameCRC(u32 crc);
	virtual void UpdateCRCHacks();

	u8* GetRegsMem() const { return reinterpret_cast<u8*>(m_regs); }
	void SetRegsMem(u8* basemem) { m_regs = reinterpret_cast<GSPrivRegSet*>(basemem); }

	void DumpVertices(const std::string& filename);

	PRIM_OVERLAP PrimitiveOverlap();
	GIFRegTEX0 GetTex0Layer(u32 lod);
};
