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
#include "common/Assertions.h"
#include "Frontend/LayeredSettingsInterface.h"
#include "GS.h"
#include "GS/Renderers/HW/GSTextureReplacements.h"
#include "Host.h"
#include "HostSettings.h"
#include "MemoryCardFile.h"
#include "Sio.h"
#include "VMManager.h"

static std::mutex s_settings_mutex;
static LayeredSettingsInterface s_layered_settings_interface;

std::unique_lock<std::mutex> Host::GetSettingsLock()
{
	return std::unique_lock<std::mutex>(s_settings_mutex);
}

SettingsInterface* Host::GetSettingsInterface()
{
	return &s_layered_settings_interface;
}

SettingsInterface* Host::GetSettingsInterfaceForBindings()
{
	SettingsInterface* input_layer = s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_INPUT);
	return input_layer ? input_layer : &s_layered_settings_interface;
}

std::string Host::GetBaseStringSettingValue(const char* section, const char* key, const char* default_value /*= ""*/)
{
	std::unique_lock lock(s_settings_mutex);
	return s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE)->GetStringValue(section, key, default_value);
}

bool Host::GetBaseBoolSettingValue(const char* section, const char* key, bool default_value /*= false*/)
{
	std::unique_lock lock(s_settings_mutex);
	return s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE)->GetBoolValue(section, key, default_value);
}

int Host::GetBaseIntSettingValue(const char* section, const char* key, int default_value /*= 0*/)
{
	std::unique_lock lock(s_settings_mutex);
	return s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE)->GetIntValue(section, key, default_value);
}

uint Host::GetBaseUIntSettingValue(const char* section, const char* key, uint default_value /*= 0*/)
{
	std::unique_lock lock(s_settings_mutex);
	return s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE)->GetUIntValue(section, key, default_value);
}

float Host::GetBaseFloatSettingValue(const char* section, const char* key, float default_value /*= 0.0f*/)
{
	std::unique_lock lock(s_settings_mutex);
	return s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE)->GetFloatValue(section, key, default_value);
}

double Host::GetBaseDoubleSettingValue(const char* section, const char* key, double default_value /* = 0.0f */)
{
	std::unique_lock lock(s_settings_mutex);
	return s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE)->GetDoubleValue(section, key, default_value);
}

std::vector<std::string> Host::GetBaseStringListSetting(const char* section, const char* key)
{
	std::unique_lock lock(s_settings_mutex);
	return s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE)->GetStringList(section, key);
}

void Host::SetBaseBoolSettingValue(const char* section, const char* key, bool value)
{
	std::unique_lock lock(s_settings_mutex);
	s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE)->SetBoolValue(section, key, value);
}

void Host::SetBaseIntSettingValue(const char* section, const char* key, int value)
{
	std::unique_lock lock(s_settings_mutex);
	s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE)->SetIntValue(section, key, value);
}

void Host::SetBaseUIntSettingValue(const char* section, const char* key, uint value)
{
	std::unique_lock lock(s_settings_mutex);
	s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE)->SetUIntValue(section, key, value);
}

void Host::SetBaseFloatSettingValue(const char* section, const char* key, float value)
{
	std::unique_lock lock(s_settings_mutex);
	s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE)->SetFloatValue(section, key, value);
}

void Host::SetBaseStringSettingValue(const char* section, const char* key, const char* value)
{
	std::unique_lock lock(s_settings_mutex);
	s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE)->SetStringValue(section, key, value);
}

void Host::SetBaseStringListSettingValue(const char* section, const char* key, const std::vector<std::string>& values)
{
	std::unique_lock lock(s_settings_mutex);
	s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE)->SetStringList(section, key, values);
}

void Host::DeleteBaseSettingValue(const char* section, const char* key)
{
	std::unique_lock lock(s_settings_mutex);
	s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE)->DeleteValue(section, key);
}

void Host::CommitBaseSettingChanges()
{
	std::unique_lock lock(s_settings_mutex);
	s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE)->Save();
}

std::string Host::GetStringSettingValue(const char* section, const char* key, const char* default_value /*= ""*/)
{
	std::unique_lock lock(s_settings_mutex);
	return s_layered_settings_interface.GetStringValue(section, key, default_value);
}

bool Host::GetBoolSettingValue(const char* section, const char* key, bool default_value /*= false*/)
{
	std::unique_lock lock(s_settings_mutex);
	return s_layered_settings_interface.GetBoolValue(section, key, default_value);
}

int Host::GetIntSettingValue(const char* section, const char* key, int default_value /*= 0*/)
{
	std::unique_lock lock(s_settings_mutex);
	return s_layered_settings_interface.GetIntValue(section, key, default_value);
}

uint Host::GetUIntSettingValue(const char* section, const char* key, uint default_value /*= 0*/)
{
	std::unique_lock lock(s_settings_mutex);
	return s_layered_settings_interface.GetUIntValue(section, key, default_value);
}

float Host::GetFloatSettingValue(const char* section, const char* key, float default_value /*= 0.0f*/)
{
	std::unique_lock lock(s_settings_mutex);
	return s_layered_settings_interface.GetFloatValue(section, key, default_value);
}

double Host::GetDoubleSettingValue(const char* section, const char* key, double default_value /*= 0.0f*/)
{
	std::unique_lock lock(s_settings_mutex);
	return s_layered_settings_interface.GetDoubleValue(section, key, default_value);
}

std::vector<std::string> Host::GetStringListSetting(const char* section, const char* key)
{
	std::unique_lock lock(s_settings_mutex);
	return s_layered_settings_interface.GetStringList(section, key);
}

SettingsInterface* Host::Internal::GetBaseSettingsLayer()
{
	return s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE);
}

SettingsInterface* Host::Internal::GetGameSettingsLayer()
{
	return s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_GAME);
}

SettingsInterface* Host::Internal::GetInputSettingsLayer()
{
	return s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_INPUT);
}

void Host::Internal::SetBaseSettingsLayer(SettingsInterface* sif)
{
	pxAssertRel(s_layered_settings_interface.GetLayer(LayeredSettingsInterface::LAYER_BASE) == nullptr, "Base layer has not been set");
	s_layered_settings_interface.SetLayer(LayeredSettingsInterface::LAYER_BASE, sif);
}

void Host::Internal::SetGameSettingsLayer(SettingsInterface* sif)
{
	std::unique_lock lock(s_settings_mutex);
	s_layered_settings_interface.SetLayer(LayeredSettingsInterface::LAYER_GAME, sif);
}

void Host::Internal::SetInputSettingsLayer(SettingsInterface* sif)
{
	std::unique_lock lock(s_settings_mutex);
	s_layered_settings_interface.SetLayer(LayeredSettingsInterface::LAYER_INPUT, sif);
}

void Host::Internal::UpdateEmuFolders()
{
	const std::string old_cheats_directory(EmuFolders::Cheats);
	const std::string old_cheats_ws_directory(EmuFolders::CheatsWS);
	const std::string old_cheats_ni_directory(EmuFolders::CheatsNI);
	const std::string old_memcards_directory(EmuFolders::MemoryCards);
	const std::string old_textures_directory(EmuFolders::Textures);

	EmuFolders::LoadConfig(*GetBaseSettingsLayer());
	EmuFolders::EnsureFoldersExist();

	if (VMManager::HasValidVM())
	{
		if (EmuFolders::Cheats != old_cheats_directory ||
			EmuFolders::CheatsWS != old_cheats_ws_directory ||
			EmuFolders::CheatsNI != old_cheats_ni_directory)
		{
			VMManager::ReloadPatches(true, true);
		}

		if (EmuFolders::MemoryCards != old_memcards_directory)
		{
			FileMcd_EmuClose();
			FileMcd_EmuOpen();

			for (u32 port = 0; port < 2; port++)
			{
				for (u32 slot = 0; slot < 4; slot++)
					SetForceMcdEjectTimeoutNow(port, slot);
			}
		}

		if (EmuFolders::Textures != old_textures_directory)
		{
			GetMTGS().RunOnGSThread([]() {
				if (VMManager::HasValidVM())
					GSTextureReplacements::ReloadReplacementMap();
			});
		}
	}
}
