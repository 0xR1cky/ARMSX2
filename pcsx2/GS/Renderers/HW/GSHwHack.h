/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2023 PCSX2 Dev Team
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

#include "GS/Renderers/HW/GSRendererHW.h"

class GSHwHack
{
public:
	static bool GSC_BigMuthaTruckers(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_DeathByDegreesTekkenNinaWilliams(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_GiTS(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_Manhunt2(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_CrashBandicootWoC(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_SacredBlaze(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_Spartan(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_Oneechanbara2Special(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_SakuraTaisen(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_SFEX3(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_Tekken5(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_TombRaiderAnniversary(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_TombRaiderLegend(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_TombRaiderUnderWorld(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_BurnoutGames(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_BlackAndBurnoutSky(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_MidnightClub3(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_TalesOfLegendia(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_Kunoichi(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_ZettaiZetsumeiToshi2(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_SakuraWarsSoLongMyLove(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_FightingBeautyWulong(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_GodHand(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_KnightsOfTheTemple2(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_UltramanFightingEvolution(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_TalesofSymphonia(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_Simple2000Vol114(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_UrbanReign(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_SteambotChronicles(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_YakuzaGames(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_GetawayGames(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_AceCombat4(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_FFXGames(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_Okami(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_RedDeadRevolver(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_ShinOnimusha(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_XenosagaE3(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_BlueTongueGames(GSRendererHW& r, const GSFrameInfo& fi, int& skip);
	static bool GSC_Battlefield2(GSRendererHW& r, const GSFrameInfo& fi, int& skip);

	static bool OI_PointListPalette(GSRendererHW& r, GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	static bool OI_BigMuthaTruckers(GSRendererHW& r, GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	static bool OI_DBZBTGames(GSRendererHW& r, GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	static bool OI_FFXII(GSRendererHW& r, GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	static bool OI_FFX(GSRendererHW& r, GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	static bool OI_MetalSlug6(GSRendererHW& r, GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	static bool OI_RozenMaidenGebetGarden(GSRendererHW& r, GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	static bool OI_SonicUnleashed(GSRendererHW& r, GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	static bool OI_ArTonelico2(GSRendererHW& r, GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	static bool OI_JakGames(GSRendererHW& r, GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	static bool OI_BurnoutGames(GSRendererHW& r, GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);

	static bool OI_Battlefield2(GSRendererHW& r, GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);

	template <typename F>
	struct Entry
	{
		const char* name;
		F ptr;
		CRCHackLevel level;
	};

	static const Entry<GSRendererHW::GSC_Ptr> s_get_skip_count_functions[];
	static const Entry<GSRendererHW::OI_Ptr> s_before_draw_functions[];
};
