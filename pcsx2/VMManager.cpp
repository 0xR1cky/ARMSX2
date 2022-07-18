/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2022  PCSX2 Dev Team
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

#include "VMManager.h"

#include <atomic>
#include <sstream>
#include <mutex>

#include "common/Console.h"
#include "common/FileSystem.h"
#include "common/ScopedGuard.h"
#include "common/StringUtil.h"
#include "common/SettingsWrapper.h"
#include "common/Timer.h"
#include "common/Threading.h"
#include "fmt/core.h"

#include "Counters.h"
#include "CDVD/CDVD.h"
#include "DEV9/DEV9.h"
#include "Elfheader.h"
#include "FW.h"
#include "GS.h"
#include "GSDumpReplayer.h"
#include "HostDisplay.h"
#include "HostSettings.h"
#include "IopBios.h"
#include "MTVU.h"
#include "MemoryCardFile.h"
#include "Patch.h"
#include "PerformanceMetrics.h"
#include "R5900.h"
#include "SPU2/spu2.h"
#include "DEV9/DEV9.h"
#include "USB/USB.h"
#include "PAD/Host/PAD.h"
#include "Sio.h"
#include "ps2/BiosTools.h"
#include "Recording/InputRecordingControls.h"

#include "DebugTools/MIPSAnalyst.h"
#include "DebugTools/SymbolMap.h"

#include "Frontend/INISettingsInterface.h"
#include "Frontend/InputManager.h"
#include "Frontend/GameList.h"

#include "common/emitter/tools.h"
#ifdef _M_X86
#include "common/emitter/x86_intrin.h"
#endif

#ifdef _WIN32
#include "common/RedtapeWindows.h"
#endif

namespace VMManager
{
	static void LoadSettings();
	static void ApplyGameFixes();
	static bool UpdateGameSettingsLayer();
	static void CheckForConfigChanges(const Pcsx2Config& old_config);
	static void CheckForCPUConfigChanges(const Pcsx2Config& old_config);
	static void CheckForGSConfigChanges(const Pcsx2Config& old_config);
	static void CheckForFramerateConfigChanges(const Pcsx2Config& old_config);
	static void CheckForPatchConfigChanges(const Pcsx2Config& old_config);
	static void CheckForSPU2ConfigChanges(const Pcsx2Config& old_config);
	static void CheckForDEV9ConfigChanges(const Pcsx2Config& old_config);
	static void CheckForMemoryCardConfigChanges(const Pcsx2Config& old_config);

	static bool AutoDetectSource(const std::string& filename);
	static bool ApplyBootParameters(const VMBootParameters& params, std::string* state_to_load);
	static bool CheckBIOSAvailability();
	static void LoadPatches(const std::string& serial, u32 crc,
		bool show_messages, bool show_messages_when_disabled);
	static void UpdateRunningGame(bool resetting, bool game_starting);

	static std::string GetCurrentSaveStateFileName(s32 slot);
	static bool DoLoadState(const char* filename);
	static bool DoSaveState(const char* filename, s32 slot_for_message, bool zip_on_thread);
	static void ZipSaveState(std::unique_ptr<ArchiveEntryList> elist,
		std::unique_ptr<SaveStateScreenshotData> screenshot, std::string osd_key,
		const char* filename, s32 slot_for_message);
	static void ZipSaveStateOnThread(std::unique_ptr<ArchiveEntryList> elist,
		std::unique_ptr<SaveStateScreenshotData> screenshot, std::string osd_key,
		std::string filename, s32 slot_for_message);

	static void SetTimerResolutionIncreased(bool enabled);
	static void EnsureCPUInfoInitialized();
	static void SetEmuThreadAffinities();
} // namespace VMManager

static std::unique_ptr<SysMainMemory> s_vm_memory;
static std::unique_ptr<SysCpuProviderPack> s_cpu_provider_pack;
static std::unique_ptr<INISettingsInterface> s_game_settings_interface;
static std::unique_ptr<INISettingsInterface> s_input_settings_interface;

static std::atomic<VMState> s_state{VMState::Shutdown};
static bool s_cpu_implementation_changed = false;
static Threading::ThreadHandle s_vm_thread_handle;

static std::deque<std::thread> s_save_state_threads;
static std::mutex s_save_state_threads_mutex;

static std::mutex s_info_mutex;
static std::string s_disc_path;
static u32 s_game_crc;
static u32 s_patches_crc;
static std::string s_game_serial;
static std::string s_game_name;
static std::string s_elf_override;
static std::string s_input_profile_name;
static u32 s_active_game_fixes = 0;
static std::vector<u8> s_widescreen_cheats_data;
static bool s_widescreen_cheats_loaded = false;
static std::vector<u8> s_no_interlacing_cheats_data;
static bool s_no_interlacing_cheats_loaded = false;
static s32 s_active_widescreen_patches = 0;
static u32 s_active_no_interlacing_patches = 0;
static s32 s_current_save_slot = 1;
static u32 s_frame_advance_count = 0;
static u32 s_mxcsr_saved;
static std::optional<LimiterModeType> s_limiter_mode_prior_to_hold_interaction;

bool VMManager::PerformEarlyHardwareChecks(const char** error)
{
#define COMMON_DOWNLOAD_MESSAGE \
	"PCSX2 builds can be downloaded from https://pcsx2.net/downloads/"

#if defined(_M_X86)
	// On Windows, this gets called as a global object constructor, before any of our objects are constructed.
	// So, we have to put it on the stack instead.
	x86capabilities temp_x86_caps;
	temp_x86_caps.Identify();

	if (!temp_x86_caps.hasStreamingSIMD4Extensions)
	{
		*error = "PCSX2 requires the Streaming SIMD 4 Extensions instruction set, which your CPU does not support.\n\n"
				 "SSE4 is now a minimum requirement for PCSX2. You should either upgrade your CPU, or use an older build such as 1.6.0.\n\n" COMMON_DOWNLOAD_MESSAGE;
		return false;
	}

#if _M_SSE >= 0x0501
	if (!temp_x86_caps.hasAVX || !temp_x86_caps.hasAVX2)
	{
		*error = "This build of PCSX2 requires the Advanced Vector Extensions 2 instruction set, which your CPU does not support.\n\n"
				 "You should download and run the SSE4 build of PCSX2 instead, or upgrade to a CPU that supports AVX2 to use this build.\n\n" COMMON_DOWNLOAD_MESSAGE;
		return false;
	}
#endif
#endif

#undef COMMON_DOWNLOAD_MESSAGE
	return true;
}

VMState VMManager::GetState()
{
	return s_state.load(std::memory_order_acquire);
}

void VMManager::SetState(VMState state)
{
	// Some state transitions aren't valid.
	const VMState old_state = s_state.load(std::memory_order_acquire);
	pxAssert(state != VMState::Initializing && state != VMState::Shutdown);
	SetTimerResolutionIncreased(state == VMState::Running);
	s_state.store(state, std::memory_order_release);

	if (state != VMState::Stopping && (state == VMState::Paused || old_state == VMState::Paused))
	{
		if (state == VMState::Paused)
		{
			if (THREAD_VU1)
				vu1Thread.WaitVU();
			GetMTGS().WaitGS(false);
			InputManager::PauseVibration();
		}
		else
		{
			PerformanceMetrics::Reset();
			frameLimitReset();
		}

		SPU2SetOutputPaused(state == VMState::Paused);
		if (state == VMState::Paused)
			Host::OnVMPaused();
		else
			Host::OnVMResumed();
	}
}

bool VMManager::HasValidVM()
{
	const VMState state = s_state.load(std::memory_order_acquire);
	return (state == VMState::Running || state == VMState::Paused);
}

std::string VMManager::GetDiscPath()
{
	std::unique_lock lock(s_info_mutex);
	return s_disc_path;
}

u32 VMManager::GetGameCRC()
{
	std::unique_lock lock(s_info_mutex);
	return s_game_crc;
}

std::string VMManager::GetGameSerial()
{
	std::unique_lock lock(s_info_mutex);
	return s_game_serial;
}

std::string VMManager::GetGameName()
{
	std::unique_lock lock(s_info_mutex);
	return s_game_name;
}

bool VMManager::Internal::InitializeGlobals()
{
	// On Win32, we have a bunch of things which use COM (e.g. SDL, XAudio2, etc).
	// We need to initialize COM first, before anything else does, because otherwise they might
	// initialize it in single-threaded/apartment mode, which can't be changed to multithreaded.
#ifdef _WIN32
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		Host::ReportErrorAsync("Error", fmt::format("CoInitializeEx() failed: {:08X}", hr));
		return false;
	}
#endif

	x86caps.Identify();
	x86caps.CountCores();
	x86caps.SIMD_EstablishMXCSRmask();
	x86caps.CalculateMHz();
	SysLogMachineCaps();

	return true;
}

void VMManager::Internal::ReleaseGlobals()
{
#ifdef _WIN32
	CoUninitialize();
#endif
}

bool VMManager::Internal::InitializeMemory()
{
	pxAssert(!s_vm_memory && !s_cpu_provider_pack);

	s_vm_memory = std::make_unique<SysMainMemory>();
	s_cpu_provider_pack = std::make_unique<SysCpuProviderPack>();

	s_vm_memory->ReserveAll();
	return true;
}

void VMManager::Internal::ReleaseMemory()
{
	std::vector<u8>().swap(s_widescreen_cheats_data);
	s_widescreen_cheats_loaded = false;
	std::vector<u8>().swap(s_no_interlacing_cheats_data);
	s_no_interlacing_cheats_loaded = false;

	s_vm_memory->DecommitAll();
	s_vm_memory->ReleaseAll();
	s_vm_memory.reset();
	s_cpu_provider_pack.reset();
}

SysMainMemory& GetVmMemory()
{
	return *s_vm_memory;
}

SysCpuProviderPack& GetCpuProviders()
{
	return *s_cpu_provider_pack;
}

void VMManager::LoadSettings()
{
	std::unique_lock<std::mutex> lock = Host::GetSettingsLock();
	SettingsInterface* si = Host::GetSettingsInterface();
	SettingsInterface* binding_si = Host::GetSettingsInterfaceForBindings();
	SettingsLoadWrapper slw(*si);
	EmuConfig.LoadSave(slw);
	PAD::LoadConfig(*binding_si);
	InputManager::ReloadSources(*si, lock);
	InputManager::ReloadBindings(*si, *binding_si);

	// Remove any user-specified hacks in the config (we don't want stale/conflicting values when it's globally disabled).
	EmuConfig.GS.MaskUserHacks();
	EmuConfig.GS.MaskUpscalingHacks();

	// Disable interlacing if we have no-interlacing patches active.
	if (s_active_no_interlacing_patches > 0 && EmuConfig.GS.InterlaceMode == GSInterlaceMode::Automatic)
		EmuConfig.GS.InterlaceMode = GSInterlaceMode::Off;

	// Switch to 16:9 if widescreen patches are enabled, and AR is auto.
	if (s_active_widescreen_patches > 0 && EmuConfig.GS.AspectRatio == AspectRatioType::RAuto4_3_3_2)
	{
		// Don't change when reloading settings in the middle of a FMV with switch.
		if (EmuConfig.CurrentAspectRatio == EmuConfig.GS.AspectRatio)
			EmuConfig.CurrentAspectRatio = AspectRatioType::R16_9;

		EmuConfig.GS.AspectRatio = AspectRatioType::R16_9;
	}

	// Force MTVU off when playing back GS dumps, it doesn't get used.
	if (GSDumpReplayer::IsReplayingDump())
		EmuConfig.Speedhacks.vuThread = false;

	if (HasValidVM())
		ApplyGameFixes();
}

void VMManager::ApplyGameFixes()
{
	s_active_game_fixes = 0;

	const GameDatabaseSchema::GameEntry* game = GameDatabase::findGame(s_game_serial);
	if (!game)
		return;

	s_active_game_fixes += game->applyGameFixes(EmuConfig, EmuConfig.EnableGameFixes);
	s_active_game_fixes += game->applyGSHardwareFixes(EmuConfig.GS);
}

std::string VMManager::GetGameSettingsPath(const std::string_view& game_serial, u32 game_crc)
{
	std::string sanitized_serial(game_serial);
	Path::SanitizeFileName(sanitized_serial);

	return game_serial.empty() ?
			   Path::Combine(EmuFolders::GameSettings, fmt::format("{:08X}.ini", game_crc)) :
               Path::Combine(EmuFolders::GameSettings, fmt::format("{}_{:08X}.ini", sanitized_serial, game_crc));
}

std::string VMManager::GetInputProfilePath(const std::string_view& name)
{
	return Path::Combine(EmuFolders::InputProfiles, fmt::format("{}.ini", name));
}

void VMManager::RequestDisplaySize(float scale /*= 0.0f*/)
{
	int iwidth, iheight;
	GSgetInternalResolution(&iwidth, &iheight);
	if (iwidth <= 0 || iheight <= 0)
		return;

	// scale x not y for aspect ratio
	float x_scale;
	switch (GSConfig.AspectRatio)
	{
		case AspectRatioType::RAuto4_3_3_2:
			if (GSgetDisplayMode() == GSVideoMode::SDTV_480P || (GSConfig.PCRTCOverscan && GSConfig.PCRTCOffsets))
				x_scale = (3.0f / 2.0f) / (static_cast<float>(iwidth) / static_cast<float>(iheight));
			else
				x_scale = (4.0f / 3.0f) / (static_cast<float>(iwidth) / static_cast<float>(iheight));
			break;
		case AspectRatioType::R4_3:
			x_scale = (4.0f / 3.0f) / (static_cast<float>(iwidth) / static_cast<float>(iheight));
			break;
		case AspectRatioType::R16_9:
			x_scale = (16.0f / 9.0f) / (static_cast<float>(iwidth) / static_cast<float>(iheight));
			break;
		case AspectRatioType::Stretch:
		default:
			x_scale = 1.0f;
			break;
	}

	float width = static_cast<float>(iwidth) * x_scale;
	float height = static_cast<float>(iheight);

	if (scale != 0.0f)
	{
		// unapply the upscaling, then apply the scale
		scale = (1.0f / static_cast<float>(GSConfig.UpscaleMultiplier)) * scale;
		width *= scale;
		height *= scale;
	}

	iwidth = std::max(static_cast<int>(std::lroundf(width)), 1);
	iheight = std::max(static_cast<int>(std::lroundf(height)), 1);

	Host::RequestResizeHostDisplay(iwidth, iheight);
}

bool VMManager::UpdateGameSettingsLayer()
{
	std::unique_ptr<INISettingsInterface> new_interface;
	if (s_game_crc != 0)
	{
		std::string filename(GetGameSettingsPath(s_game_serial.c_str(), s_game_crc));
		if (!FileSystem::FileExists(filename.c_str()))
		{
			// try the legacy format (crc.ini)
			filename = GetGameSettingsPath({}, s_game_crc);
		}

		if (FileSystem::FileExists(filename.c_str()))
		{
			Console.WriteLn("Loading game settings from '%s'...", filename.c_str());
			new_interface = std::make_unique<INISettingsInterface>(std::move(filename));
			if (!new_interface->Load())
			{
				Console.Error("Failed to parse game settings ini '%s'", new_interface->GetFileName().c_str());
				new_interface.reset();
			}
		}
		else
		{
			DevCon.WriteLn("No game settings found (tried '%s')", filename.c_str());
		}
	}

	std::string input_profile_name;
	if (new_interface)
		new_interface->GetStringValue("EmuCore", "InputProfileName", &input_profile_name);

	if (!s_game_settings_interface && !new_interface && s_input_profile_name == input_profile_name)
		return false;

	Host::Internal::SetGameSettingsLayer(new_interface.get());
	s_game_settings_interface = std::move(new_interface);

	std::unique_ptr<INISettingsInterface> input_interface;
	if (!input_profile_name.empty())
	{
		const std::string filename(GetInputProfilePath(input_profile_name));
		if (FileSystem::FileExists(filename.c_str()))
		{
			Console.WriteLn("Loading input profile from '%s'...", filename.c_str());
			input_interface = std::make_unique<INISettingsInterface>(std::move(filename));
			if (!input_interface->Load())
			{
				Console.Error("Failed to parse input profile ini '%s'", input_interface->GetFileName().c_str());
				input_interface.reset();
				input_profile_name = {};
			}
		}
		else
		{
			DevCon.WriteLn("No game settings found (tried '%s')", filename.c_str());
			input_profile_name = {};
		}
	}

	Host::Internal::SetInputSettingsLayer(input_interface.get());
	s_input_settings_interface = std::move(input_interface);
	s_input_profile_name = std::move(input_profile_name);


	return true;
}

void VMManager::LoadPatches(const std::string& serial, u32 crc, bool show_messages, bool show_messages_when_disabled)
{
	const std::string crc_string(fmt::format("{:08X}", crc));
	s_patches_crc = crc;
	s_active_widescreen_patches = 0;
	s_active_no_interlacing_patches = 0;
	ForgetLoadedPatches();

	std::string message;

	int patch_count = 0;
	if (EmuConfig.EnablePatches)
	{
		const GameDatabaseSchema::GameEntry* game = GameDatabase::findGame(serial);
		const std::string* patches = game ? game->findPatch(crc) : nullptr;
		if (patches && (patch_count = LoadPatchesFromString(*patches)) > 0)
		{
			PatchesCon->WriteLn(Color_Green, "(GameDB) Patches Loaded: %d", patch_count);
			fmt::format_to(std::back_inserter(message), "{} game patches", patch_count);
		}
	}

	// regular cheat patches
	int cheat_count = 0;
	if (EmuConfig.EnableCheats)
	{
		cheat_count = LoadPatchesFromDir(crc_string, EmuFolders::Cheats, "Cheats", true);
		if (cheat_count > 0)
		{
			PatchesCon->WriteLn(Color_Green, "Cheats Loaded: %d", cheat_count);
			fmt::format_to(std::back_inserter(message), "{}{} cheat patches", (patch_count > 0) ? " and " : "", cheat_count);
		}
	}

	// wide screen patches
	if (EmuConfig.EnableWideScreenPatches && crc != 0)
	{
		if (s_active_widescreen_patches = LoadPatchesFromDir(crc_string, EmuFolders::CheatsWS, "Widescreen hacks", false))
		{
			Console.WriteLn(Color_Gray, "Found widescreen patches in the cheats_ws folder --> skipping cheats_ws.zip");
		}
		else
		{
			// No ws cheat files found at the cheats_ws folder, try the ws cheats zip file.
			if (!s_widescreen_cheats_loaded)
			{
				s_widescreen_cheats_loaded = true;

				std::optional<std::vector<u8>> data = Host::ReadResourceFile("cheats_ws.zip");
				if (data.has_value())
					s_widescreen_cheats_data = std::move(data.value());
			}

			if (!s_widescreen_cheats_data.empty())
			{
				s_active_widescreen_patches = LoadPatchesFromZip(crc_string, s_widescreen_cheats_data.data(), s_widescreen_cheats_data.size());
				PatchesCon->WriteLn(Color_Green, "(Wide Screen Cheats DB) Patches Loaded: %d", s_active_widescreen_patches);
			}
		}

		if (s_active_widescreen_patches > 0)
		{
			fmt::format_to(std::back_inserter(message), "{}{} widescreen patches", (patch_count > 0 || cheat_count > 0) ? " and " : "", s_active_widescreen_patches);

			// Switch to 16:9 if widescreen patches are enabled, and AR is auto.
			if (EmuConfig.GS.AspectRatio == AspectRatioType::RAuto4_3_3_2)
			{
				// Don't change when reloading settings in the middle of a FMV with switch.
				if (EmuConfig.CurrentAspectRatio == EmuConfig.GS.AspectRatio)
					EmuConfig.CurrentAspectRatio = AspectRatioType::R16_9;

				EmuConfig.GS.AspectRatio = AspectRatioType::R16_9;
			}
		}
	}

	// no-interlacing patches
	if (EmuConfig.EnableNoInterlacingPatches && crc != 0)
	{
		if (s_active_no_interlacing_patches = LoadPatchesFromDir(crc_string, EmuFolders::CheatsNI, "No-interlacing patches", false))
		{
			Console.WriteLn(Color_Gray, "Found no-interlacing patches in the cheats_ni folder --> skipping cheats_ni.zip");
		}
		else
		{
			// No ws cheat files found at the cheats_ws folder, try the ws cheats zip file.
			if (!s_no_interlacing_cheats_loaded)
			{
				s_no_interlacing_cheats_loaded = true;

				std::optional<std::vector<u8>> data = Host::ReadResourceFile("cheats_ni.zip");
				if (data.has_value())
					s_no_interlacing_cheats_data = std::move(data.value());
			}

			if (!s_no_interlacing_cheats_data.empty())
			{
				s_active_no_interlacing_patches = LoadPatchesFromZip(crc_string, s_no_interlacing_cheats_data.data(), s_no_interlacing_cheats_data.size());
				PatchesCon->WriteLn(Color_Green, "(No-Interlacing Cheats DB) Patches Loaded: %u", s_active_no_interlacing_patches);
			}
		}

		if (s_active_no_interlacing_patches > 0)
		{
			fmt::format_to(std::back_inserter(message), "{}{} no-interlacing patches", (patch_count > 0 || cheat_count > 0 || s_active_widescreen_patches > 0) ? " and " : "", s_active_no_interlacing_patches);

			// Disable interlacing in GS if active.
			if (EmuConfig.GS.InterlaceMode == GSInterlaceMode::Automatic)
			{
				EmuConfig.GS.InterlaceMode = GSInterlaceMode::Off;
				GetMTGS().ApplySettings();
			}
		}
	}
	else
	{
		s_active_no_interlacing_patches = 0;
	}

	if (show_messages)
	{
		if (cheat_count > 0 || s_active_widescreen_patches > 0 || s_active_no_interlacing_patches > 0)
		{
			message += " are active.";
			Host::AddKeyedOSDMessage("LoadPatches", std::move(message), 5.0f);
		}
		else if (show_messages_when_disabled)
		{
			Host::AddKeyedOSDMessage("LoadPatches", "No cheats or patches (widescreen, compatibility or others) are found / enabled.", 8.0f);
		}
	}
}

void VMManager::UpdateRunningGame(bool resetting, bool game_starting)
{
	// The CRC can be known before the game actually starts (at the bios), so when
	// we have the CRC but we're still at the bios and the settings are changed
	// (e.g. the user presses TAB to speed up emulation), we don't want to apply the
	// settings as if the game is already running (title, loadeding patches, etc).
	u32 new_crc;
	std::string new_serial;
	if (!GSDumpReplayer::IsReplayingDump())
	{
		const bool ingame = (ElfCRC && (g_GameLoading || g_GameStarted));
		new_crc = ingame ? ElfCRC : 0;
		new_serial = ingame ? SysGetDiscID() : SysGetBiosDiscID();
	}
	else
	{
		new_crc = GSDumpReplayer::GetDumpCRC();
		new_serial = GSDumpReplayer::GetDumpSerial();
	}

	if (!resetting && s_game_crc == new_crc && s_game_serial == new_serial)
		return;

	{
		std::unique_lock lock(s_info_mutex);
		s_game_serial = std::move(new_serial);
		s_game_crc = new_crc;
		s_game_name.clear();

		std::string memcardFilters;

		if (const GameDatabaseSchema::GameEntry* game = GameDatabase::findGame(s_game_serial))
		{
			s_game_name = game->name;
			memcardFilters = game->memcardFiltersAsString();
		}
		else
		{
			if (s_game_serial.empty() && s_game_crc == 0)
				s_game_name = "Booting PS2 BIOS...";
		}

		sioSetGameSerial(memcardFilters.empty() ? s_game_serial : memcardFilters);

		// If we don't reset the timer here, when using folder memcards the reindex will cause an eject,
		// which a bunch of games don't like since they access the memory card on boot.
		if (game_starting || resetting)
			ClearMcdEjectTimeoutNow();
	}

	UpdateGameSettingsLayer();
	ApplySettings();

	// check this here, for two cases: dynarec on, and when enable cheats is set per-game.
	if (s_patches_crc != s_game_crc)
		ReloadPatches(game_starting, false);

	GetMTGS().SendGameCRC(new_crc);

	Host::OnGameChanged(s_disc_path, s_game_serial, s_game_name, s_game_crc);

#if 0
	// TODO: Enable this when the debugger is added to Qt, and it's active. Otherwise, this is just a waste of time.
	// In other words, it should be lazily initialized.
	MIPSAnalyst::ScanForFunctions(R5900SymbolMap, ElfTextRange.first, ElfTextRange.first + ElfTextRange.second, true);
	R5900SymbolMap.UpdateActiveSymbols();
	R3000SymbolMap.UpdateActiveSymbols();
#endif
}

void VMManager::ReloadPatches(bool verbose, bool show_messages_when_disabled)
{
	LoadPatches(s_game_serial, s_game_crc, verbose, show_messages_when_disabled);
}

static LimiterModeType GetInitialLimiterMode()
{
	return EmuConfig.GS.FrameLimitEnable ? LimiterModeType::Nominal : LimiterModeType::Unlimited;
}

bool VMManager::AutoDetectSource(const std::string& filename)
{
	if (!filename.empty())
	{
		if (!FileSystem::FileExists(filename.c_str()))
		{
			Host::ReportErrorAsync("Error", fmt::format("Requested filename '{}' does not exist.", filename));
			return false;
		}

		const std::string display_name(FileSystem::GetDisplayNameFromPath(filename));
		if (IsGSDumpFileName(display_name))
		{
			CDVDsys_ChangeSource(CDVD_SourceType::NoDisc);
			return GSDumpReplayer::Initialize(filename.c_str());
		}
		else if (IsElfFileName(display_name))
		{
			// alternative way of booting an elf, change the elf override, and use no disc.
			CDVDsys_ChangeSource(CDVD_SourceType::NoDisc);
			s_elf_override = filename;
			return true;
		}
		else
		{
			// TODO: Maybe we should check if it's a valid iso here...
			CDVDsys_SetFile(CDVD_SourceType::Iso, filename);
			CDVDsys_ChangeSource(CDVD_SourceType::Iso);
			s_disc_path = filename;
			return true;
		}
	}
	else
	{
		// make sure we're not fast booting when we have no filename
		CDVDsys_ChangeSource(CDVD_SourceType::NoDisc);
		EmuConfig.UseBOOT2Injection = false;
		return true;
	}
}

bool VMManager::ApplyBootParameters(const VMBootParameters& params, std::string* state_to_load)
{
	const bool default_fast_boot = Host::GetBoolSettingValue("EmuCore", "EnableFastBoot", true);
	EmuConfig.UseBOOT2Injection = params.fast_boot.value_or(default_fast_boot);

	s_elf_override = params.elf_override;
	s_disc_path.clear();
	if (!params.save_state.empty())
		*state_to_load = params.save_state;

	// if we're loading an indexed save state, we need to get the serial/crc from the disc.
	if (params.state_index.has_value())
	{
		if (params.filename.empty())
		{
			Host::ReportErrorAsync("Error", "Cannot load an indexed save state without a boot filename.");
			return false;
		}

		*state_to_load = GetSaveStateFileName(params.filename.c_str(), params.state_index.value());
		if (state_to_load->empty())
		{
			Host::ReportErrorAsync("Error", "Could not resolve path indexed save state load.");
			return false;
		}
	}

	// resolve source type
	if (params.source_type.has_value())
	{
		if (params.source_type.value() == CDVD_SourceType::Iso && !FileSystem::FileExists(params.filename.c_str()))
		{
			Host::ReportErrorAsync("Error", fmt::format("Requested filename '{}' does not exist.", params.filename));
			return false;
		}

		// Use specified source type.
		s_disc_path = params.filename;
		CDVDsys_SetFile(params.source_type.value(), params.filename);
		CDVDsys_ChangeSource(params.source_type.value());
	}
	else
	{
		// Automatic type detection of boot parameter based on filename.
		if (!AutoDetectSource(params.filename))
			return false;
	}

	if (!s_elf_override.empty())
	{
		if (!FileSystem::FileExists(s_elf_override.c_str()))
		{
			Host::ReportErrorAsync("Error", fmt::format("Requested boot ELF '{}' does not exist.", s_elf_override));
			return false;
		}

		Hle_SetElfPath(s_elf_override.c_str());
		EmuConfig.UseBOOT2Injection = true;
	}

	return true;
}

bool VMManager::CheckBIOSAvailability()
{
	if (IsBIOSAvailable(EmuConfig.FullpathToBios()))
		return true;

	// TODO: When we translate core strings, translate this.

	const char* message = "PCSX2 requires a PS2 BIOS in order to run.\n\n"
		"For legal reasons, you *must* obtain a BIOS from an actual PS2 unit that you own (borrowing doesn't count).\n\n"
		"Once dumped, this BIOS image should be placed in the bios folder within the data directory (Tools Menu -> Open Data Directory).\n\n"
		"Please consult the FAQs and Guides for further instructions.";

	Host::ReportErrorAsync("Startup Error", message);
	return false;
}

bool VMManager::Initialize(const VMBootParameters& boot_params)
{
	const Common::Timer init_timer;
	pxAssertRel(s_state.load(std::memory_order_acquire) == VMState::Shutdown, "VM is shutdown");

	// cancel any game list scanning, we need to use CDVD!
	// TODO: we can get rid of this once, we make CDVD not use globals...
	// (or make it thread-local, but that seems silly.)
	Host::CancelGameListRefresh();

	s_state.store(VMState::Initializing, std::memory_order_release);
	s_vm_thread_handle = Threading::ThreadHandle::GetForCallingThread();
	Host::OnVMStarting();

	ScopedGuard close_state = [] {
		if (GSDumpReplayer::IsReplayingDump())
			GSDumpReplayer::Shutdown();

		s_vm_thread_handle = {};
		s_state.store(VMState::Shutdown, std::memory_order_release);
		Host::OnVMDestroyed();
	};

	LoadSettings();

	std::string state_to_load;
	if (!ApplyBootParameters(boot_params, &state_to_load))
		return false;

	EmuConfig.LimiterMode = GetInitialLimiterMode();

	// early out if we don't have a bios
	if (!GSDumpReplayer::IsReplayingDump() && !CheckBIOSAvailability())
		return false;

	Console.WriteLn("Allocating memory map...");
	s_vm_memory->CommitAll();

	Console.WriteLn("Opening CDVD...");
	if (!DoCDVDopen())
	{
		Host::ReportErrorAsync("Startup Error", "Failed to initialize CDVD.");
		return false;
	}
	ScopedGuard close_cdvd = [] { DoCDVDclose(); };

	Console.WriteLn("Opening GS...");
	if (!GetMTGS().WaitForOpen())
	{
		// we assume GS is going to report its own error
		Console.WriteLn("Failed to open GS.");
		return false;
	}

	ScopedGuard close_gs = []() { GetMTGS().WaitForClose(); };

	Console.WriteLn("Opening SPU2...");
	if (SPU2init() != 0 || SPU2open() != 0)
	{
		Host::ReportErrorAsync("Startup Error", "Failed to initialize SPU2.");
		SPU2shutdown();
		return false;
	}
	ScopedGuard close_spu2 = []() {
		SPU2close();
		SPU2shutdown();
	};

	Console.WriteLn("Opening PAD...");
	if (PADinit() != 0 || PADopen(Host::GetHostDisplay()->GetWindowInfo()) != 0)
	{
		Host::ReportErrorAsync("Startup Error", "Failed to initialize PAD.");
		return false;
	}
	ScopedGuard close_pad = []() {
		PADclose();
		PADshutdown();
	};

	Console.WriteLn("Opening DEV9...");
	if (DEV9init() != 0 || DEV9open() != 0)
	{
		Host::ReportErrorAsync("Startup Error", "Failed to initialize DEV9.");
		return false;
	}
	ScopedGuard close_dev9 = []() {
		DEV9close();
		DEV9shutdown();
	};

	Console.WriteLn("Opening USB...");
	if (USBinit() != 0 || USBopen(Host::GetHostDisplay()->GetWindowInfo()) != 0)
	{
		Host::ReportErrorAsync("Startup Error", "Failed to initialize USB.");
		return false;
	}
	ScopedGuard close_usb = []() {
		USBclose();
		USBshutdown();
	};

	Console.WriteLn("Opening FW...");
	if (FWopen() != 0)
	{
		Host::ReportErrorAsync("Startup Error", "Failed to initialize FW.");
		return false;
	}
	ScopedGuard close_fw = []() { FWclose(); };

	FileMcd_EmuOpen();

	// Don't close when we return
	close_fw.Cancel();
	close_usb.Cancel();
	close_dev9.Cancel();
	close_pad.Cancel();
	close_spu2.Cancel();
	close_gs.Cancel();
	close_cdvd.Cancel();
	close_state.Cancel();

#if defined(_M_X86)
	s_mxcsr_saved = _mm_getcsr();
#elif defined(_M_ARM64)
	s_mxcsr_saved = static_cast<u32>(a64_getfpcr());
#endif

	s_cpu_implementation_changed = false;
	s_cpu_provider_pack->ApplyConfig();
	SetCPUState(EmuConfig.Cpu.sseMXCSR, EmuConfig.Cpu.sseVUMXCSR);
	SysClearExecutionCache();
	memBindConditionalHandlers();

	ForgetLoadedPatches();
	gsUpdateFrequency(EmuConfig);
	frameLimitReset();
	cpuReset();

	Console.WriteLn("VM subsystems initialized in %.2f ms", init_timer.GetTimeMilliseconds());
	s_state.store(VMState::Paused, std::memory_order_release);
	Host::OnVMStarted();

	UpdateRunningGame(true, false);

	SetEmuThreadAffinities();

	PerformanceMetrics::Clear();

	// do we want to load state?
	if (!GSDumpReplayer::IsReplayingDump() && !state_to_load.empty())
	{
		if (!DoLoadState(state_to_load.c_str()))
		{
			Shutdown(false);
			return false;
		}
	}

	return true;
}

void VMManager::Shutdown(bool save_resume_state)
{
	// we'll probably already be stopping (this is how Qt calls shutdown),
	// but just in case, so any of the stuff we call here knows we don't have a valid VM.
	s_state.store(VMState::Stopping, std::memory_order_release);

	SetTimerResolutionIncreased(false);

	// sync everything
	if (THREAD_VU1)
		vu1Thread.WaitVU();
	GetMTGS().WaitGS();

	if (!GSDumpReplayer::IsReplayingDump() && save_resume_state)
	{
		std::string resume_file_name(GetCurrentSaveStateFileName(-1));
		if (!resume_file_name.empty() && !DoSaveState(resume_file_name.c_str(), -1, true))
			Console.Error("Failed to save resume state");
	}
	else if (GSDumpReplayer::IsReplayingDump())
	{
		GSDumpReplayer::Shutdown();
	}

	{
		std::unique_lock lock(s_info_mutex);
		s_disc_path.clear();
		s_game_crc = 0;
		s_patches_crc = 0;
		s_game_serial.clear();
		s_game_name.clear();
		Host::OnGameChanged(s_disc_path, s_game_serial, s_game_name, 0);
	}
	s_active_game_fixes = 0;
	s_active_widescreen_patches = 0;
	s_active_no_interlacing_patches = 0;
	s_limiter_mode_prior_to_hold_interaction.reset();

	UpdateGameSettingsLayer();

	std::string().swap(s_elf_override);

#ifdef _M_X86
	_mm_setcsr(s_mxcsr_saved);
#elif defined(_M_ARM64)
	a64_setfpcr(s_mxcsr_saved);
#endif

	ForgetLoadedPatches();
	R3000A::ioman::reset();
	USBclose();
	SPU2close();
	PADclose();
	DEV9close();
	DoCDVDclose();
	FWclose();
	FileMcd_EmuClose();
	GetMTGS().WaitForClose();
	USBshutdown();
	SPU2shutdown();
	PADshutdown();
	DEV9shutdown();
	GSshutdown();

	s_vm_memory->DecommitAll();

	s_state.store(VMState::Shutdown, std::memory_order_release);
	Host::OnVMDestroyed();
}

void VMManager::Reset()
{
	const bool game_was_started = g_GameStarted;

	s_active_game_fixes = 0;
	s_active_widescreen_patches = 0;
	s_active_no_interlacing_patches = 0;
	s_limiter_mode_prior_to_hold_interaction.reset();

	SysClearExecutionCache();
	memBindConditionalHandlers();
	UpdateVSyncRate();
	frameLimitReset();
	cpuReset();

	// gameid change, so apply settings
	if (game_was_started)
		UpdateRunningGame(true, false);
}

std::string VMManager::GetSaveStateFileName(const char* game_serial, u32 game_crc, s32 slot)
{
	std::string filename;
	if (game_crc != 0)
	{
		if (slot < 0)
			filename = fmt::format("{} ({:08X}).resume.p2s", game_serial, game_crc);
		else
			filename = fmt::format("{} ({:08X}).{:02d}.p2s", game_serial, game_crc, slot);

		filename = Path::Combine(EmuFolders::Savestates, filename);
	}

	return filename;
}

std::string VMManager::GetSaveStateFileName(const char* filename, s32 slot)
{
	pxAssertRel(!HasValidVM(), "Should not have a VM when calling the non-gamelist GetSaveStateFileName()");

	std::string ret;

	// try the game list first, but this won't work if we're in batch mode
	auto lock = GameList::GetLock();
	if (const GameList::Entry* entry = GameList::GetEntryForPath(filename); entry)
	{
		ret = GetSaveStateFileName(entry->serial.c_str(), entry->crc, slot);
	}
	else
	{
		// just scan it.. hopefully it'll come back okay
		GameList::Entry temp_entry;
		if (GameList::PopulateEntryFromPath(filename, &temp_entry))
		{
			ret = GetSaveStateFileName(temp_entry.serial.c_str(), temp_entry.crc, slot);
		}
	}

	return ret;
}

bool VMManager::HasSaveStateInSlot(const char* game_serial, u32 game_crc, s32 slot)
{
	std::string filename(GetSaveStateFileName(game_serial, game_crc, slot));
	return (!filename.empty() && FileSystem::FileExists(filename.c_str()));
}

std::string VMManager::GetCurrentSaveStateFileName(s32 slot)
{
	std::unique_lock lock(s_info_mutex);
	return GetSaveStateFileName(s_game_serial.c_str(), s_game_crc, slot);
}

bool VMManager::DoLoadState(const char* filename)
{
	if (GSDumpReplayer::IsReplayingDump())
		return false;

	try
	{
		Host::OnSaveStateLoading(filename);
		SaveState_UnzipFromDisk(filename);

		// HACK: LastELF isn't in the save state...
		if (!s_elf_override.empty())
			cdvdReloadElfInfo(fmt::format("host:{}", s_elf_override));
		else
			cdvdReloadElfInfo();

		UpdateRunningGame(false, false);
		Host::OnSaveStateLoaded(filename, true);
		return true;
	}
	catch (Exception::BaseException& e)
	{
		Host::ReportErrorAsync("Failed to load save state", e.UserMsg());
		Host::OnSaveStateLoaded(filename, false);
		return false;
	}
}

bool VMManager::DoSaveState(const char* filename, s32 slot_for_message, bool zip_on_thread)
{
	if (GSDumpReplayer::IsReplayingDump())
		return false;

	std::string osd_key(fmt::format("SaveStateSlot{}", slot_for_message));

	try
	{
		std::unique_ptr<ArchiveEntryList> elist(SaveState_DownloadState());
		std::unique_ptr<SaveStateScreenshotData> screenshot(SaveState_SaveScreenshot());

		if (zip_on_thread)
		{
			// lock order here is important; the thread could exit before we resume here.
			std::unique_lock lock(s_save_state_threads_mutex);
			s_save_state_threads.emplace_back(&VMManager::ZipSaveStateOnThread,
				std::move(elist), std::move(screenshot), std::move(osd_key), std::string(filename),
				slot_for_message);
		}
		else
		{
			ZipSaveState(std::move(elist), std::move(screenshot), std::move(osd_key), filename, slot_for_message);
		}

		Host::OnSaveStateSaved(filename);
		return true;
	}
	catch (Exception::BaseException& e)
	{
		Host::AddKeyedOSDMessage(std::move(osd_key), fmt::format("Failed to save save state: {}.", e.DiagMsg()), 15.0f);
		return false;
	}
}

void VMManager::ZipSaveState(std::unique_ptr<ArchiveEntryList> elist,
	std::unique_ptr<SaveStateScreenshotData> screenshot, std::string osd_key,
	const char* filename, s32 slot_for_message)
{
	Common::Timer timer;

	if (SaveState_ZipToDisk(std::move(elist), std::move(screenshot), filename))
	{
		if (slot_for_message >= 0 && VMManager::HasValidVM())
			Host::AddKeyedOSDMessage(std::move(osd_key), fmt::format("State saved to slot {}.", slot_for_message), 10.0f);
	}
	else
	{
		Host::AddKeyedOSDMessage(std::move(osd_key), fmt::format("Failed to save save state to slot {}.", slot_for_message), 15.0f);
	}

	DevCon.WriteLn("Zipping save state to '%s' took %.2f ms", filename, timer.GetTimeMilliseconds());

	Host::InvalidateSaveStateCache();
}

void VMManager::ZipSaveStateOnThread(std::unique_ptr<ArchiveEntryList> elist, std::unique_ptr<SaveStateScreenshotData> screenshot,
	std::string osd_key, std::string filename, s32 slot_for_message)
{
	ZipSaveState(std::move(elist), std::move(screenshot), std::move(osd_key), filename.c_str(), slot_for_message);

	// remove ourselves from the thread list. if we're joining, we might not be in there.
	const auto this_id = std::this_thread::get_id();
	std::unique_lock lock(s_save_state_threads_mutex);
	for (auto it = s_save_state_threads.begin(); it != s_save_state_threads.end(); ++it)
	{
		if (it->get_id() == this_id)
		{
			it->detach();
			s_save_state_threads.erase(it);
			break;
		}
	}
}

void VMManager::WaitForSaveStateFlush()
{
	std::unique_lock lock(s_save_state_threads_mutex);
	while (!s_save_state_threads.empty())
	{
		// take a thread from the list and join with it. it won't self detatch then, but that's okay,
		// since we're joining with it here.
		std::thread save_thread(std::move(s_save_state_threads.front()));
		s_save_state_threads.pop_front();
		lock.unlock();
		save_thread.join();
		lock.lock();
	}
}

bool VMManager::LoadState(const char* filename)
{
	// TODO: Save the current state so we don't need to reset.
	if (DoLoadState(filename))
		return true;

	Reset();
	return false;
}

bool VMManager::LoadStateFromSlot(s32 slot)
{
	const std::string filename(GetCurrentSaveStateFileName(slot));
	if (filename.empty())
	{
		Host::AddKeyedOSDMessage("LoadStateFromSlot", fmt::format("There is no save state in slot {}.", slot), 5.0f);
		return false;
	}

	Host::AddKeyedOSDMessage("LoadStateFromSlot", fmt::format("Loading state from slot {}...", slot), 5.0f);
	return DoLoadState(filename.c_str());
}

bool VMManager::SaveState(const char* filename, bool zip_on_thread)
{
	return DoSaveState(filename, -1, zip_on_thread);
}

bool VMManager::SaveStateToSlot(s32 slot, bool zip_on_thread)
{
	const std::string filename(GetCurrentSaveStateFileName(slot));
	if (filename.empty())
		return false;

	// if it takes more than a minute.. well.. wtf.
	Host::AddKeyedOSDMessage(fmt::format("SaveStateSlot{}", slot), fmt::format("Saving state to slot {}...", slot), 60.0f);
	return DoSaveState(filename.c_str(), slot, zip_on_thread);
}

LimiterModeType VMManager::GetLimiterMode()
{
	return EmuConfig.LimiterMode;
}

void VMManager::SetLimiterMode(LimiterModeType type)
{
	if (EmuConfig.LimiterMode == type)
		return;

	EmuConfig.LimiterMode = type;
	gsUpdateFrequency(EmuConfig);
	GetMTGS().SetVSync(EmuConfig.GetEffectiveVsyncMode());
}

void VMManager::FrameAdvance(u32 num_frames /*= 1*/)
{
	if (!HasValidVM())
		return;

	s_frame_advance_count = num_frames;
	SetState(VMState::Running);
}

bool VMManager::ChangeDisc(CDVD_SourceType source, std::string path)
{
	const CDVD_SourceType old_type = CDVDsys_GetSourceType();
	const std::string old_path(CDVDsys_GetFile(old_type));

	const std::string display_name((source != CDVD_SourceType::Iso) ? path : FileSystem::GetDisplayNameFromPath(path));
	CDVDsys_ChangeSource(source);
	if (!path.empty())
		CDVDsys_SetFile(source, std::move(path));

	const bool result = DoCDVDopen();
	if (result)
	{
		if (source == CDVD_SourceType::NoDisc)
			Host::AddKeyedOSDMessage("ChangeDisc", "Disc removed.", 5.0f);
		else
			Host::AddKeyedOSDMessage("ChangeDisc", fmt::format("Disc changed to '{}'.", display_name), 5.0f);
	}
	else
	{
		Host::AddKeyedOSDMessage("ChangeDisc", fmt::format("Failed to open new disc image '{}'. Reverting to old image.", display_name), 20.0f);
		CDVDsys_ChangeSource(old_type);
		if (!old_path.empty())
			CDVDsys_SetFile(old_type, std::move(old_path));
		if (!DoCDVDopen())
		{
			Host::AddKeyedOSDMessage("ChangeDisc", "Failed to switch back to old disc image. Removing disc.", 20.0f);
			CDVDsys_ChangeSource(CDVD_SourceType::NoDisc);
			DoCDVDopen();
		}
	}

	cdvdCtrlTrayOpen();
	return result;
}

bool VMManager::IsElfFileName(const std::string_view& path)
{
	return StringUtil::EndsWithNoCase(path, ".elf");
}

bool VMManager::IsBlockDumpFileName(const std::string_view& path)
{
	return StringUtil::EndsWithNoCase(path, ".dump");
}

bool VMManager::IsGSDumpFileName(const std::string_view& path)
{
	return (StringUtil::EndsWithNoCase(path, ".gs") ||
			StringUtil::EndsWithNoCase(path, ".gs.xz") ||
			StringUtil::EndsWithNoCase(path, ".gs.zst"));
}

bool VMManager::IsSaveStateFileName(const std::string_view& path)
{
	return StringUtil::EndsWithNoCase(path, ".p2s");
}

bool VMManager::IsLoadableFileName(const std::string_view& path)
{
	return IsElfFileName(path) || IsGSDumpFileName(path) || IsBlockDumpFileName(path) || GameList::IsScannableFilename(path);
}

void VMManager::Execute()
{
	// Check for interpreter<->recompiler switches.
	if (std::exchange(s_cpu_implementation_changed, false))
	{
		// We need to switch the cpus out, and reset the new ones if so.
		s_cpu_provider_pack->ApplyConfig();
		SysClearExecutionCache();
	}

	// Execute until we're asked to stop.
	Cpu->Execute();
}

void VMManager::SetPaused(bool paused)
{
	if (!HasValidVM())
		return;

	Console.WriteLn(paused ? "(VMManager) Pausing..." : "(VMManager) Resuming...");
	SetState(paused ? VMState::Paused : VMState::Running);
}

const std::string& VMManager::Internal::GetElfOverride()
{
	return s_elf_override;
}

bool VMManager::Internal::IsExecutionInterrupted()
{
	return s_state.load(std::memory_order_relaxed) != VMState::Running || s_cpu_implementation_changed;
}

void VMManager::Internal::EntryPointCompilingOnCPUThread()
{
	// Classic chicken and egg problem here. We don't want to update the running game
	// until the game entry point actually runs, because that can update settings, which
	// can flush the JIT, etc. But we need to apply patches for games where the entry
	// point is in the patch (e.g. WRC 4). So. Gross, but the only way to handle it really.
	LoadPatches(SysGetDiscID(), ElfCRC, true, false);
	ApplyLoadedPatches(PPT_ONCE_ON_LOAD);
}

void VMManager::Internal::GameStartingOnCPUThread()
{
	UpdateRunningGame(false, true);
	ApplyLoadedPatches(PPT_ONCE_ON_LOAD);
	ApplyLoadedPatches(PPT_COMBINED_0_1);
}

void VMManager::Internal::VSyncOnCPUThread()
{
	// TODO: Move frame limiting here to reduce CPU usage after sleeping...
	ApplyLoadedPatches(PPT_CONTINUOUSLY);
	ApplyLoadedPatches(PPT_COMBINED_0_1);

	// Frame advance must be done *before* pumping messages, because otherwise
	// we'll immediately reduce the counter we just set.
	if (s_frame_advance_count > 0)
	{
		s_frame_advance_count--;
		if (s_frame_advance_count == 0)
		{
			// auto pause at the end of frame advance
			SetState(VMState::Paused);
		}
	}

	Host::PumpMessagesOnCPUThread();
	InputManager::PollSources();
}

void VMManager::CheckForCPUConfigChanges(const Pcsx2Config& old_config)
{
	if (EmuConfig.Cpu == old_config.Cpu &&
		EmuConfig.Gamefixes == old_config.Gamefixes &&
		EmuConfig.Speedhacks == old_config.Speedhacks &&
		EmuConfig.Profiler == old_config.Profiler)
	{
		return;
	}

	Console.WriteLn("Updating CPU configuration...");
	SetCPUState(EmuConfig.Cpu.sseMXCSR, EmuConfig.Cpu.sseVUMXCSR);
	SysClearExecutionCache();
	memBindConditionalHandlers();

	// did we toggle recompilers?
	if (EmuConfig.Cpu.CpusChanged(old_config.Cpu))
	{
		// This has to be done asynchronously, since we're still executing the
		// cpu when this function is called. Break the execution as soon as
		// possible and reset next time we're called.
		s_cpu_implementation_changed = true;
	}

	if (EmuConfig.Cpu.AffinityControlMode != old_config.Cpu.AffinityControlMode ||
		EmuConfig.Speedhacks.vuThread != old_config.Speedhacks.vuThread)
	{
		SetEmuThreadAffinities();
	}
}

void VMManager::CheckForGSConfigChanges(const Pcsx2Config& old_config)
{
	if (EmuConfig.GS == old_config.GS)
		return;

	Console.WriteLn("Updating GS configuration...");

	if (EmuConfig.GS.FrameLimitEnable != old_config.GS.FrameLimitEnable)
		EmuConfig.LimiterMode = GetInitialLimiterMode();

	gsUpdateFrequency(EmuConfig);
	UpdateVSyncRate();
	frameLimitReset();
	GetMTGS().ApplySettings();
	GetMTGS().SetVSync(EmuConfig.GetEffectiveVsyncMode());
}

void VMManager::CheckForFramerateConfigChanges(const Pcsx2Config& old_config)
{
	if (EmuConfig.Framerate == old_config.Framerate)
		return;

	Console.WriteLn("Updating frame rate configuration");
	gsUpdateFrequency(EmuConfig);
	UpdateVSyncRate();
	frameLimitReset();
	GetMTGS().SetVSync(EmuConfig.GetEffectiveVsyncMode());
}

void VMManager::CheckForPatchConfigChanges(const Pcsx2Config& old_config)
{
	if (EmuConfig.EnableCheats == old_config.EnableCheats &&
		EmuConfig.EnableWideScreenPatches == old_config.EnableWideScreenPatches &&
		EmuConfig.EnablePatches == old_config.EnablePatches)
	{
		return;
	}

	ReloadPatches(true, true);
}

void VMManager::CheckForSPU2ConfigChanges(const Pcsx2Config& old_config)
{
	if (EmuConfig.SPU2 == old_config.SPU2)
		return;

	// TODO: Don't reinit on volume changes.

	Console.WriteLn("Updating SPU2 configuration");

	// kinda lazy, but until we move spu2 over...
	freezeData fd = {};
	if (SPU2freeze(FreezeAction::Size, &fd) != 0)
	{
		Console.Error("(CheckForSPU2ConfigChanges) Failed to get SPU2 freeze size");
		return;
	}

	std::unique_ptr<u8[]> fd_data = std::make_unique<u8[]>(fd.size);
	fd.data = fd_data.get();
	if (SPU2freeze(FreezeAction::Save, &fd) != 0)
	{
		Console.Error("(CheckForSPU2ConfigChanges) Failed to freeze SPU2");
		return;
	}

	SPU2close();
	SPU2shutdown();
	if (SPU2init() != 0 || SPU2open() != 0)
	{
		Console.Error("(CheckForSPU2ConfigChanges) Failed to reopen SPU2, we'll probably crash :(");
		return;
	}

	if (SPU2freeze(FreezeAction::Load, &fd) != 0)
	{
		Console.Error("(CheckForSPU2ConfigChanges) Failed to unfreeze SPU2");
		return;
	}
}

void VMManager::CheckForDEV9ConfigChanges(const Pcsx2Config& old_config)
{
	if (EmuConfig.DEV9 == old_config.DEV9)
		return;

	DEV9CheckChanges(old_config);
}

void VMManager::CheckForMemoryCardConfigChanges(const Pcsx2Config& old_config)
{
	bool changed = false;

	for (size_t i = 0; i < std::size(EmuConfig.Mcd); i++)
	{
		if (EmuConfig.Mcd[i].Enabled != old_config.Mcd[i].Enabled ||
			EmuConfig.Mcd[i].Filename != old_config.Mcd[i].Filename)
		{
			changed = true;
			break;
		}
	}

	changed |= (EmuConfig.McdEnableEjection != old_config.McdEnableEjection);
	changed |= (EmuConfig.McdFolderAutoManage != old_config.McdFolderAutoManage);

	if (!changed)
		return;

	Console.WriteLn("Updating memory card configuration");

	FileMcd_EmuClose();
	FileMcd_EmuOpen();

	// force card eject when files change
	for (u32 port = 0; port < 2; port++)
	{
		for (u32 slot = 0; slot < 4; slot++)
		{
			const uint index = FileMcd_ConvertToSlot(port, slot);
			if (EmuConfig.Mcd[index].Enabled != old_config.Mcd[index].Enabled ||
				EmuConfig.Mcd[index].Filename != old_config.Mcd[index].Filename)
			{
				Console.WriteLn("Replugging memory card %u (port %u slot %u) due to source change", index, port, slot);
				SetForceMcdEjectTimeoutNow(port, slot);
			}
		}
	}

	// force reindexing, mc folder code is janky
	std::string sioSerial;
	{
		std::unique_lock lock(s_info_mutex);
		if (const GameDatabaseSchema::GameEntry* game = GameDatabase::findGame(s_game_serial))
			sioSerial = game->memcardFiltersAsString();
		if (sioSerial.empty())
			sioSerial = s_game_serial;
	}
	sioSetGameSerial(sioSerial);
}

void VMManager::CheckForConfigChanges(const Pcsx2Config& old_config)
{
	CheckForCPUConfigChanges(old_config);
	CheckForGSConfigChanges(old_config);
	CheckForFramerateConfigChanges(old_config);
	CheckForPatchConfigChanges(old_config);
	CheckForSPU2ConfigChanges(old_config);
	CheckForDEV9ConfigChanges(old_config);
	CheckForMemoryCardConfigChanges(old_config);

	if (EmuConfig.EnableCheats != old_config.EnableCheats ||
		EmuConfig.EnableWideScreenPatches != old_config.EnableWideScreenPatches ||
		EmuConfig.EnableNoInterlacingPatches != old_config.EnableNoInterlacingPatches)
	{
		VMManager::ReloadPatches(true, true);
	}
}

void VMManager::ApplySettings()
{
	Console.WriteLn("Applying settings...");

	// if we're running, ensure the threads are synced
	const bool running = (s_state.load(std::memory_order_acquire) == VMState::Running);
	if (running)
	{
		if (THREAD_VU1)
			vu1Thread.WaitVU();
		GetMTGS().WaitGS(false);
	}

	const Pcsx2Config old_config(EmuConfig);
	LoadSettings();

	if (HasValidVM())
		CheckForConfigChanges(old_config);
}

bool VMManager::ReloadGameSettings()
{
	if (!UpdateGameSettingsLayer())
		return false;

	ApplySettings();
	return true;
}

static void HotkeyAdjustTargetSpeed(double delta)
{
	EmuConfig.Framerate.NominalScalar = EmuConfig.GS.LimitScalar + delta;
	VMManager::SetLimiterMode(LimiterModeType::Nominal);
	gsUpdateFrequency(EmuConfig);
	GetMTGS().SetVSync(EmuConfig.GetEffectiveVsyncMode());
	Host::AddKeyedOSDMessage("SpeedChanged", fmt::format("Target speed set to {:.0f}%.", std::round(EmuConfig.Framerate.NominalScalar * 100.0)), 5.0f);
}

static constexpr s32 CYCLE_SAVE_STATE_SLOTS = 10;

static void HotkeyCycleSaveSlot(s32 delta)
{
	// 1..10
	s_current_save_slot = ((s_current_save_slot - 1) + delta);
	if (s_current_save_slot < 0)
		s_current_save_slot = CYCLE_SAVE_STATE_SLOTS;
	else
		s_current_save_slot = (s_current_save_slot % CYCLE_SAVE_STATE_SLOTS) + 1;

	const std::string filename(VMManager::GetSaveStateFileName(s_game_serial.c_str(), s_game_crc, s_current_save_slot));
	FILESYSTEM_STAT_DATA sd;
	if (!filename.empty() && FileSystem::StatFile(filename.c_str(), &sd))
	{
		char date_buf[128] = {};
#ifdef _WIN32
		ctime_s(date_buf, std::size(date_buf), &sd.ModificationTime);
#else
		ctime_r(&sd.ModificationTime, date_buf);
#endif

		// remove terminating \n
		size_t len = std::strlen(date_buf);
		if (len > 0 && date_buf[len - 1] == '\n')
			date_buf[len - 1] = 0;

		Host::AddKeyedOSDMessage("CycleSaveSlot", fmt::format("Save slot {} selected (last save: {}).", s_current_save_slot, date_buf), 5.0f);
	}
	else
	{
		Host::AddKeyedOSDMessage("CycleSaveSlot", fmt::format("Save slot {} selected (no save yet).", s_current_save_slot), 5.0f);
	}
}

static void HotkeyLoadStateSlot(s32 slot)
{
	if (s_game_crc == 0)
	{
		Host::AddKeyedOSDMessage("LoadStateFromSlot", "Cannot load state from a slot without a game running.", 10.0f);
		return;
	}

	if (!VMManager::HasSaveStateInSlot(s_game_serial.c_str(), s_game_crc, slot))
	{
		Host::AddKeyedOSDMessage("LoadStateFromSlot", fmt::format("No save state found in slot {}.", slot));
		return;
	}

	VMManager::LoadStateFromSlot(slot);
}

static void HotkeySaveStateSlot(s32 slot)
{
	if (s_game_crc == 0)
	{
		Host::AddKeyedOSDMessage("SaveStateToSlot", "Cannot save state to a slot without a game running.", 10.0f);
		return;
	}

	VMManager::SaveStateToSlot(slot);
}

BEGIN_HOTKEY_LIST(g_vm_manager_hotkeys)
DEFINE_HOTKEY("TogglePause", "System", "Toggle Pause", [](s32 pressed) {
	if (!pressed && VMManager::HasValidVM())
		VMManager::SetPaused(VMManager::GetState() != VMState::Paused);
})
DEFINE_HOTKEY("ToggleFullscreen", "System", "Toggle Fullscreen", [](s32 pressed) {
	if (!pressed)
		Host::SetFullscreen(!Host::IsFullscreen());
})
DEFINE_HOTKEY("ToggleFrameLimit", "System", "Toggle Frame Limit", [](s32 pressed) {
	if (!pressed)
	{
		VMManager::SetLimiterMode((EmuConfig.LimiterMode != LimiterModeType::Unlimited) ?
                                      LimiterModeType::Unlimited :
                                      LimiterModeType::Nominal);
	}
})
DEFINE_HOTKEY("ToggleTurbo", "System", "Toggle Turbo", [](s32 pressed) {
	if (!pressed)
	{
		VMManager::SetLimiterMode((EmuConfig.LimiterMode != LimiterModeType::Turbo) ?
									  LimiterModeType::Turbo :
                                      LimiterModeType::Nominal);
	}
})
DEFINE_HOTKEY("ToggleSlowMotion", "System", "Toggle Slow Motion", [](s32 pressed) {
	if (!pressed)
	{
		VMManager::SetLimiterMode((EmuConfig.LimiterMode != LimiterModeType::Slomo) ?
                                      LimiterModeType::Slomo :
                                      LimiterModeType::Nominal);
	}
})
DEFINE_HOTKEY("HoldTurbo", "System", "Turbo (Hold)", [](s32 pressed) {
	if (pressed > 0 && !s_limiter_mode_prior_to_hold_interaction.has_value())
	{
		s_limiter_mode_prior_to_hold_interaction = VMManager::GetLimiterMode();
		VMManager::SetLimiterMode((s_limiter_mode_prior_to_hold_interaction.value() != LimiterModeType::Turbo) ?
									  LimiterModeType::Turbo :
                                      LimiterModeType::Nominal);
	}
	else if (pressed >= 0 && s_limiter_mode_prior_to_hold_interaction.has_value())
	{
		VMManager::SetLimiterMode(s_limiter_mode_prior_to_hold_interaction.value());
		s_limiter_mode_prior_to_hold_interaction.reset();
	}
})
DEFINE_HOTKEY("IncreaseSpeed", "System", "Increase Target Speed", [](s32 pressed) {
	if (!pressed)
		HotkeyAdjustTargetSpeed(0.1);
})
DEFINE_HOTKEY("DecreaseSpeed", "System", "Decrease Target Speed", [](s32 pressed) {
	if (!pressed)
		HotkeyAdjustTargetSpeed(-0.1);
})
DEFINE_HOTKEY("FrameAdvance", "System", "Frame Advance", [](s32 pressed) {
	if (!pressed)
		VMManager::FrameAdvance(1);
})
DEFINE_HOTKEY("ShutdownVM", "System", "Shut Down Virtual Machine", [](s32 pressed) {
	if (!pressed && VMManager::HasValidVM())
		Host::RequestVMShutdown(true, true);
})
DEFINE_HOTKEY("ResetVM", "System", "Reset Virtual Machine", [](s32 pressed) {
	if (!pressed && VMManager::HasValidVM())
		VMManager::Reset();
})
DEFINE_HOTKEY("InputRecToggleMode", "System", "Toggle Input Recording Mode", [](s32 pressed) {
	if (!pressed)
		g_InputRecordingControls.RecordModeToggle();
})

DEFINE_HOTKEY("PreviousSaveStateSlot", "Save States", "Select Previous Save Slot", [](s32 pressed) {
	if (!pressed)
		HotkeyCycleSaveSlot(-1);
})
DEFINE_HOTKEY("NextSaveStateSlot", "Save States", "Select Next Save Slot", [](s32 pressed) {
	if (!pressed)
		HotkeyCycleSaveSlot(1);
})
DEFINE_HOTKEY("SaveStateToSlot", "Save States", "Save State To Selected Slot", [](s32 pressed) {
	if (!pressed)
		VMManager::SaveStateToSlot(s_current_save_slot);
})
DEFINE_HOTKEY("LoadStateFromSlot", "Save States", "Load State From Selected Slot", [](s32 pressed) {
	if (!pressed)
		HotkeyLoadStateSlot(s_current_save_slot);
})

#define DEFINE_HOTKEY_SAVESTATE_X(slotnum) DEFINE_HOTKEY("SaveStateToSlot" #slotnum, \
	"Save States", "Save State To Slot " #slotnum, [](s32 pressed) { if (!pressed) HotkeySaveStateSlot(slotnum); })
#define DEFINE_HOTKEY_LOADSTATE_X(slotnum) DEFINE_HOTKEY("LoadStateFromSlot" #slotnum, \
	"Save States", "Load State From Slot " #slotnum, [](s32 pressed) { \
		if (!pressed) \
			HotkeyLoadStateSlot(slotnum); \
	})
DEFINE_HOTKEY_SAVESTATE_X(1)
DEFINE_HOTKEY_LOADSTATE_X(1)
DEFINE_HOTKEY_SAVESTATE_X(2)
DEFINE_HOTKEY_LOADSTATE_X(2)
DEFINE_HOTKEY_SAVESTATE_X(3)
DEFINE_HOTKEY_LOADSTATE_X(3)
DEFINE_HOTKEY_SAVESTATE_X(4)
DEFINE_HOTKEY_LOADSTATE_X(4)
DEFINE_HOTKEY_SAVESTATE_X(5)
DEFINE_HOTKEY_LOADSTATE_X(5)
DEFINE_HOTKEY_SAVESTATE_X(6)
DEFINE_HOTKEY_LOADSTATE_X(6)
DEFINE_HOTKEY_SAVESTATE_X(7)
DEFINE_HOTKEY_LOADSTATE_X(7)
DEFINE_HOTKEY_SAVESTATE_X(8)
DEFINE_HOTKEY_LOADSTATE_X(8)
DEFINE_HOTKEY_SAVESTATE_X(9)
DEFINE_HOTKEY_LOADSTATE_X(9)
DEFINE_HOTKEY_SAVESTATE_X(10)
DEFINE_HOTKEY_LOADSTATE_X(10)
#undef DEFINE_HOTKEY_SAVESTATE_X
#undef DEFINE_HOTKEY_LOADSTATE_X

END_HOTKEY_LIST()

#ifdef _WIN32

#include "common/RedtapeWindows.h"

static bool s_timer_resolution_increased = false;

void VMManager::SetTimerResolutionIncreased(bool enabled)
{
	if (s_timer_resolution_increased == enabled)
		return;

	if (enabled)
	{
		s_timer_resolution_increased = (timeBeginPeriod(1) == TIMERR_NOERROR);
	}
	else if (s_timer_resolution_increased)
	{
		timeEndPeriod(1);
		s_timer_resolution_increased = false;
	}
}

#else

void VMManager::SetTimerResolutionIncreased(bool enabled)
{
}

#endif

static std::vector<u32> s_processor_list;
static std::once_flag s_processor_list_initialized;

#if defined(__linux__) || defined(_WIN32)

#include "cpuinfo.h"

static u32 GetProcessorIdForProcessor(const cpuinfo_processor* proc)
{
#if defined(__linux__)
	return static_cast<u32>(proc->linux_id);
#elif defined(_WIN32)
	return static_cast<u32>(proc->windows_processor_id);
#else
	return 0;
#endif
}

static void InitializeCPUInfo()
{
	if (!cpuinfo_initialize())
	{
		Console.Error("Failed to initialize cpuinfo");
		return;
	}

	const u32 cluster_count = cpuinfo_get_clusters_count();
	if (cluster_count == 0)
	{
		Console.Error("Invalid CPU count returned");
		return;
	}

	Console.WriteLn(Color_StrongYellow, "Processor count: %u cores, %u processors", cpuinfo_get_cores_count(), cpuinfo_get_processors_count());
	Console.WriteLn(Color_StrongYellow, "Cluster count: %u", cluster_count);

	static std::vector<const cpuinfo_processor*> ordered_processors;
	for (u32 i = 0; i < cluster_count; i++)
	{
		const cpuinfo_cluster* cluster = cpuinfo_get_cluster(i);
		for (u32 j = 0; j < cluster->processor_count; j++)
		{
			const cpuinfo_processor* proc = cpuinfo_get_processor(cluster->processor_start + j);
			if (!proc)
				continue;

			ordered_processors.push_back(proc);
		}
	}
	// find the large and small clusters based on frequency
	// this is assuming the large cluster is always clocked higher
	// sort based on core, so that hyperthreads get pushed down
	std::sort(ordered_processors.begin(), ordered_processors.end(), [](const cpuinfo_processor* lhs, const cpuinfo_processor* rhs) {
		return (lhs->core->frequency > rhs->core->frequency || lhs->smt_id < rhs->smt_id);
	});

	s_processor_list.reserve(ordered_processors.size());
	std::stringstream ss;
	ss << "Ordered processor list: ";
	for (const cpuinfo_processor* proc : ordered_processors)
	{
		if (proc != ordered_processors.front())
			ss << ", ";

		const u32 procid = GetProcessorIdForProcessor(proc);
		ss << procid;
		if (proc->smt_id != 0)
			ss << "[SMT " << proc->smt_id << "]";

		s_processor_list.push_back(procid);
	}
	Console.WriteLn(ss.str());
}

static void SetMTVUAndAffinityControlDefault(Pcsx2Config& config)
{
	VMManager::EnsureCPUInfoInitialized();

	const u32 cluster_count = cpuinfo_get_clusters_count();
	if (cluster_count == 0)
	{
		Console.Error("Invalid CPU count returned");
		return;
	}

	Console.WriteLn("Cluster count: %u", cluster_count);

	for (u32 i = 0; i < cluster_count; i++)
	{
		const cpuinfo_cluster* cluster = cpuinfo_get_cluster(i);
		Console.WriteLn("  Cluster %u: %u cores and %u processors at %u MHz",
			i, cluster->core_count, cluster->processor_count, static_cast<u32>(cluster->frequency /* / 1000000u*/));
	}

	const bool has_big_little = cluster_count > 1;
	Console.WriteLn("Big-Little: %s", has_big_little ? "yes" : "no");

	const u32 big_cores = cpuinfo_get_cluster(0)->core_count + ((cluster_count > 2) ? cpuinfo_get_cluster(1)->core_count : 0u);
	Console.WriteLn("Guessing we have %u big/medium cores...", big_cores);

	bool mtvu_enable;
	bool affinity_control;
	if (big_cores >= 3 || big_cores == 1)
	{
		Console.WriteLn("  So enabling MTVU and disabling affinity control");
		mtvu_enable = true;
		affinity_control = false;
	}
	else
	{
		Console.WriteLn("  So disabling MTVU and enabling affinity control");
		mtvu_enable = false;
		affinity_control = true;
	}

	config.Speedhacks.vuThread = mtvu_enable;
	config.Cpu.AffinityControlMode = affinity_control ? 1 : 0;
}

#else

static void InitializeCPUInfo()
{
	DevCon.WriteLn("(VMManager) InitializeCPUInfo() not implemented.");
}

static void SetMTVUAndAffinityControlDefault(Pcsx2Config& config)
{
}

#endif

void VMManager::EnsureCPUInfoInitialized()
{
	std::call_once(s_processor_list_initialized, InitializeCPUInfo);
}

void VMManager::SetEmuThreadAffinities()
{
	EnsureCPUInfoInitialized();

	if (s_processor_list.empty())
	{
		// not supported on this platform
		return;
	}

	if (EmuConfig.Cpu.AffinityControlMode == 0 ||
		s_processor_list.size() < (EmuConfig.Speedhacks.vuThread ? 3 : 2))
	{
		if (EmuConfig.Cpu.AffinityControlMode != 0)
			Console.Error("Insufficient processors for affinity control.");

		GetMTGS().GetThreadHandle().SetAffinity(0);
		vu1Thread.GetThreadHandle().SetAffinity(0);
		s_vm_thread_handle.SetAffinity(0);
		return;
	}

	static constexpr u8 processor_assignment[7][2][3] = {
		//EE xx GS  EE VU GS
		{{0, 2, 1}, {0, 1, 2}}, // Disabled
		{{0, 2, 1}, {0, 1, 2}}, // EE > VU > GS
		{{0, 2, 1}, {0, 2, 1}}, // EE > GS > VU
		{{0, 2, 1}, {1, 0, 2}}, // VU > EE > GS
		{{1, 2, 0}, {2, 0, 1}}, // VU > GS > EE
		{{1, 2, 0}, {1, 2, 0}}, // GS > EE > VU
		{{1, 2, 0}, {2, 1, 0}}, // GS > VU > EE
	};

	// steal vu's thread if mtvu is off
	const u8* this_proc_assigment = processor_assignment[EmuConfig.Cpu.AffinityControlMode][EmuConfig.Speedhacks.vuThread];
	const u32 ee_index = s_processor_list[this_proc_assigment[0]];
	const u32 vu_index = s_processor_list[this_proc_assigment[1]];
	const u32 gs_index = s_processor_list[this_proc_assigment[2]];
	Console.WriteLn("Processor order assignment: EE=%u, VU=%u, GS=%u",
		this_proc_assigment[0], this_proc_assigment[1], this_proc_assigment[2]);

	const u64 ee_affinity = static_cast<u64>(1) << ee_index;
	Console.WriteLn(Color_StrongGreen, "EE thread is on processor %u (0x%llx)", ee_index, ee_affinity);
	s_vm_thread_handle.SetAffinity(ee_affinity);

	if (EmuConfig.Speedhacks.vuThread)
	{
		const u64 vu_affinity = static_cast<u64>(1) << vu_index;
		Console.WriteLn(Color_StrongGreen, "VU thread is on processor %u (0x%llx)", vu_index, vu_affinity);
		vu1Thread.GetThreadHandle().SetAffinity(vu_affinity);
	}
	else
	{
		vu1Thread.GetThreadHandle().SetAffinity(0);
	}

	const u64 gs_affinity = static_cast<u64>(1) << gs_index;
	Console.WriteLn(Color_StrongGreen, "GS thread is on processor %u (0x%llx)", gs_index, gs_affinity);
	GetMTGS().GetThreadHandle().SetAffinity(gs_affinity);
}

void VMManager::SetHardwareDependentDefaultSettings(Pcsx2Config& config)
{
	SetMTVUAndAffinityControlDefault(config);
}

const std::vector<u32>& VMManager::GetSortedProcessorList()
{
	EnsureCPUInfoInitialized();
	return s_processor_list;
}
