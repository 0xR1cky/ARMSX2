﻿/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2021  PCSX2 Dev Team
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

#include "common/FileSystem.h"
#include "common/Path.h"
#include "common/StringUtil.h"
#include "common/SettingsInterface.h"

#include "Frontend/InputManager.h"
#include "HostSettings.h"

#include "PAD/Host/Global.h"
#include "PAD/Host/PAD.h"
#include "PAD/Host/KeyStatus.h"
#include "PAD/Host/StateManagement.h"

#include <array>

const u32 revision = 3;
const u32 build = 0; // increase that with each version
#define PAD_SAVE_STATE_VERSION ((revision << 8) | (build << 0))

PAD::KeyStatus g_key_status;

namespace PAD
{
	struct MacroButton
	{
		std::vector<u32> buttons; ///< Buttons to activate.
		u32 toggle_frequency; ///< Interval at which the buttons will be toggled, if not 0.
		u32 toggle_counter; ///< When this counter reaches zero, buttons will be toggled.
		bool toggle_state; ///< Current state for turbo.
		bool trigger_state; ///< Whether the macro button is active.
	};

	static std::string GetConfigSection(u32 pad_index);
	static void LoadMacroButtonConfig(const SettingsInterface& si, u32 pad, const std::string_view& type, const std::string& section);
	static void ApplyMacroButton(u32 pad, const MacroButton& mb);
	static void UpdateMacroButtons();

	static std::array<std::array<MacroButton, NUM_MACRO_BUTTONS_PER_CONTROLLER>, NUM_CONTROLLER_PORTS> s_macro_buttons;
} // namespace PAD

s32 PADinit()
{
	Pad::reset_all();

	query.reset();

	for (int port = 0; port < 2; port++)
		slots[port] = 0;

	return 0;
}

void PADshutdown()
{
}

s32 PADopen(const WindowInfo& wi)
{
	g_key_status.Init();
	return 0;
}

void PADclose()
{
}

s32 PADsetSlot(u8 port, u8 slot)
{
	port--;
	slot--;
	if (port > 1 || slot > 3)
	{
		return 0;
	}
	// Even if no pad there, record the slot, as it is the active slot regardless.
	slots[port] = slot;

	return 1;
}

s32 PADfreeze(FreezeAction mode, freezeData* data)
{
	if (!data)
		return -1;

	if (mode == FreezeAction::Size)
	{
		data->size = sizeof(PadFullFreezeData);
	}
	else if (mode == FreezeAction::Load)
	{
		PadFullFreezeData* pdata = (PadFullFreezeData*)(data->data);

		Pad::stop_vibrate_all();

		if (data->size != sizeof(PadFullFreezeData) || pdata->version != PAD_SAVE_STATE_VERSION ||
			strncmp(pdata->format, "LinPad", sizeof(pdata->format)))
			return 0;

		query = pdata->query;
		if (pdata->query.slot < 4)
		{
			query = pdata->query;
		}

		// Tales of the Abyss - pad fix
		// - restore data for both ports
		for (int port = 0; port < 2; port++)
		{
			for (int slot = 0; slot < 4; slot++)
			{
				u8 mode = pdata->padData[port][slot].mode;

				if (mode != MODE_DIGITAL && mode != MODE_ANALOG && mode != MODE_DS2_NATIVE)
				{
					break;
				}

				memcpy(&pads[port][slot], &pdata->padData[port][slot], sizeof(PadFreezeData));
			}

			if (pdata->slot[port] < 4)
				slots[port] = pdata->slot[port];
		}
	}
	else if (mode == FreezeAction::Save)
	{
		if (data->size != sizeof(PadFullFreezeData))
			return 0;

		PadFullFreezeData* pdata = (PadFullFreezeData*)(data->data);

		// Tales of the Abyss - pad fix
		// - PCSX2 only saves port0 (save #1), then port1 (save #2)

		memset(pdata, 0, data->size);
		strncpy(pdata->format, "LinPad", sizeof(pdata->format));
		pdata->version = PAD_SAVE_STATE_VERSION;
		pdata->query = query;

		for (int port = 0; port < 2; port++)
		{
			for (int slot = 0; slot < 4; slot++)
			{
				pdata->padData[port][slot] = pads[port][slot];
			}

			pdata->slot[port] = slots[port];
		}
	}
	else
	{
		return -1;
	}

	return 0;
}

u8 PADstartPoll(int pad)
{
	return pad_start_poll(pad);
}

u8 PADpoll(u8 value)
{
	return pad_poll(value);
}

std::string PAD::GetConfigSection(u32 pad_index)
{
	return fmt::format("Pad{}", pad_index + 1);
}

void PAD::LoadConfig(const SettingsInterface& si)
{
	PAD::s_macro_buttons = {};

	EmuConfig.MultitapPort0_Enabled = si.GetBoolValue("Pad", "MultitapPort1", false);
	EmuConfig.MultitapPort1_Enabled = si.GetBoolValue("Pad", "MultitapPort2", false);

	// This is where we would load controller types, if onepad supported them.
	for (u32 i = 0; i < NUM_CONTROLLER_PORTS; i++)
	{
		const std::string section(GetConfigSection(i));
		const std::string type(si.GetStringValue(section.c_str(), "Type", GetDefaultPadType(i)));

		const ControllerInfo* ci = GetControllerInfo(type);
		if (!ci)
		{
			g_key_status.SetType(i, ControllerType::NotConnected);
			continue;
		}

		g_key_status.SetType(i, ci->type);

		const float axis_deadzone = si.GetFloatValue(section.c_str(), "Deadzone", DEFAULT_STICK_DEADZONE);
		const float axis_scale = si.GetFloatValue(section.c_str(), "AxisScale", DEFAULT_STICK_SCALE);
		g_key_status.SetAxisScale(i, axis_deadzone, axis_scale);

		if (ci->vibration_caps != VibrationCapabilities::NoVibration)
		{
			const float large_motor_scale = si.GetFloatValue(section.c_str(), "LargeMotorScale", DEFAULT_MOTOR_SCALE);
			const float small_motor_scale = si.GetFloatValue(section.c_str(), "SmallMotorScale", DEFAULT_MOTOR_SCALE);
			g_key_status.SetVibrationScale(i, 0, large_motor_scale);
			g_key_status.SetVibrationScale(i, 1, small_motor_scale);
		}

		const float pressure_modifier = si.GetFloatValue(section.c_str(), "PressureModifier", 1.0f);
		g_key_status.SetPressureModifier(i, pressure_modifier);

		LoadMacroButtonConfig(si, i, type, section);
	}
}

const char* PAD::GetDefaultPadType(u32 pad)
{
	return (pad == 0) ? "DualShock2" : "None";
}

void PAD::SetDefaultConfig(SettingsInterface& si)
{
	si.ClearSection("InputSources");
	si.ClearSection("Hotkeys");
	si.ClearSection("Pad");

	// PCSX2 Controller Settings - Global Settings
	si.SetBoolValue("InputSources", "SDL", true);
	si.SetBoolValue("InputSources", "SDLControllerEnhancedMode", false);
	si.SetBoolValue("InputSources", "XInput", false);
	si.SetBoolValue("InputSources", "RawInput", false);
	si.SetBoolValue("Pad", "MultitapPort1", false);
	si.SetBoolValue("Pad", "MultitapPort2", false);
	si.SetFloatValue("Pad", "PointerXScale", 8.0f);
	si.SetFloatValue("Pad", "PointerYScale", 8.0f);
	si.SetBoolValue("Pad", "PointerXInvert", false);
	si.SetBoolValue("Pad", "PointerYInvert", false);

	// PCSX2 Controller Settings - Default pad types and parameters.
	for (u32 i = 0; i < NUM_CONTROLLER_PORTS; i++)
	{
		const std::string section(GetConfigSection(i));
		si.ClearSection(section.c_str());
		si.SetStringValue(section.c_str(), "Type", GetDefaultPadType(i));
		si.SetFloatValue(section.c_str(), "Deadzone", DEFAULT_STICK_DEADZONE);
		si.SetFloatValue(section.c_str(), "AxisScale", DEFAULT_STICK_SCALE);
		si.SetFloatValue(section.c_str(), "LargeMotorScale", DEFAULT_MOTOR_SCALE);
		si.SetFloatValue(section.c_str(), "SmallMotorScale", DEFAULT_MOTOR_SCALE);
		si.SetFloatValue(section.c_str(), "PressureModifier", DEFAULT_PRESSURE_MODIFIER);
	}

	// PCSX2 Controller Settings - Controller 1 / Controller 2 / ...
	// Use the automapper to set this up.
	MapController(si, 0, InputManager::GetGenericBindingMapping("Keyboard"));

	// PCSX2 Controller Settings - Hotkeys

	// PCSX2 Controller Settings - Hotkeys - General
	si.SetStringValue("Hotkeys", "ToggleFullscreen", "Keyboard/Alt & Keyboard/Return");

	// PCSX2 Controller Settings - Hotkeys - Graphics
	si.SetStringValue("Hotkeys", "CycleAspectRatio", "Keyboard/F6");
	si.SetStringValue("Hotkeys", "CycleInterlaceMode", "Keyboard/F5");
	si.SetStringValue("Hotkeys", "CycleMipmapMode", "Keyboard/Insert");
	//	si.SetStringValue("Hotkeys", "DecreaseUpscaleMultiplier", "Keyboard"); TBD
	//	si.SetStringValue("Hotkeys", "IncreaseUpscaleMultiplier", "Keyboard"); TBD
	//  si.SetStringValue("Hotkeys", "ReloadTextureReplacements", "Keyboard"); TBD
	si.SetStringValue("Hotkeys", "GSDumpMultiFrame", "Keyboard/Control & Keyboard/Shift & Keyboard/F8");
	si.SetStringValue("Hotkeys", "Screenshot", "Keyboard/F8");
	si.SetStringValue("Hotkeys", "GSDumpSingleFrame", "Keyboard/Shift & Keyboard/F8");
	si.SetStringValue("Hotkeys", "ToggleSoftwareRendering", "Keyboard/F9");
	//  si.SetStringValue("Hotkeys", "ToggleTextureDumping", "Keyboard"); TBD
	//  si.SetStringValue("Hotkeys", "ToggleTextureReplacements", "Keyboard"); TBD
	si.SetStringValue("Hotkeys", "ZoomIn", "Keyboard/Control & Keyboard/Plus");
	si.SetStringValue("Hotkeys", "ZoomOut", "Keyboard/Control & Keyboard/Minus");
	// Missing hotkey for resetting zoom back to 100 with Keyboard/Control & Keyboard/Asterisk

	// PCSX2 Controller Settings - Hotkeys - Input Recording
	si.SetStringValue("Hotkeys", "InputRecToggleMode", "Keyboard/Shift & Keyboard/R");

	// PCSX2 Controller Settings - Hotkeys - Save States
	si.SetStringValue("Hotkeys", "LoadStateFromSlot", "Keyboard/F3");
	si.SetStringValue("Hotkeys", "SaveStateToSlot", "Keyboard/F1");
	si.SetStringValue("Hotkeys", "NextSaveStateSlot", "Keyboard/F2");
	si.SetStringValue("Hotkeys", "PreviousSaveStateSlot", "Keyboard/Shift & Keyboard/F2");

	// PCSX2 Controller Settings - Hotkeys - System
	//	si.SetStringValue("Hotkeys", "DecreaseSpeed", "Keyboard"); TBD
	//  si.SetStringValue("Hotkeys", "FrameAdvance", "Keyboard"); TBD
	//	si.SetStringValue("Hotkeys", "IncreaseSpeed", "Keyboard"); TBD
	//  si.SetStringValue("Hotkeys", "ResetVM", "Keyboard"); TBD
	si.SetStringValue("Hotkeys", "ShutdownVM", "Keyboard/Escape");
	si.SetStringValue("Hotkeys", "ToggleFrameLimit", "Keyboard/F4");
	si.SetStringValue("Hotkeys", "TogglePause", "Keyboard/Space");
	si.SetStringValue("Hotkeys", "ToggleSlowMotion", "Keyboard/Shift & Keyboard/Backtab");
	si.SetStringValue("Hotkeys", "ToggleTurbo", "Keyboard/Tab");
	si.SetStringValue("Hotkeys", "HoldTurbo", "Keyboard/Period");
}

void PAD::Update()
{
	Pad::rumble_all();
	UpdateMacroButtons();
}

static const PAD::ControllerBindingInfo s_dualshock2_binds[] = {
	{"Up", "D-Pad Up", PAD::ControllerBindingType::Button, GenericInputBinding::DPadUp},
	{"Right", "D-Pad Right", PAD::ControllerBindingType::Button, GenericInputBinding::DPadRight},
	{"Down", "D-Pad Down", PAD::ControllerBindingType::Button, GenericInputBinding::DPadDown},
	{"Left", "D-Pad Left", PAD::ControllerBindingType::Button, GenericInputBinding::DPadLeft},
	{"Triangle", "Triangle", PAD::ControllerBindingType::Button, GenericInputBinding::Triangle},
	{"Circle", "Circle", PAD::ControllerBindingType::Button, GenericInputBinding::Circle},
	{"Cross", "Cross", PAD::ControllerBindingType::Button, GenericInputBinding::Cross},
	{"Square", "Square", PAD::ControllerBindingType::Button, GenericInputBinding::Square},
	{"Select", "Select", PAD::ControllerBindingType::Button, GenericInputBinding::Select},
	{"Start", "Start", PAD::ControllerBindingType::Button, GenericInputBinding::Start},
	{"L1", "L1 (Left Bumper)", PAD::ControllerBindingType::Button, GenericInputBinding::L1},
	{"L2", "L2 (Left Trigger)", PAD::ControllerBindingType::HalfAxis, GenericInputBinding::L2},
	{"R1", "R1 (Right Bumper)", PAD::ControllerBindingType::Button, GenericInputBinding::R1},
	{"R2", "R2 (Right Trigger)", PAD::ControllerBindingType::HalfAxis, GenericInputBinding::R2},
	{"L3", "L3 (Left Stick Button)", PAD::ControllerBindingType::Button, GenericInputBinding::L3},
	{"R3", "R3 (Right Stick Button)", PAD::ControllerBindingType::Button, GenericInputBinding::R3},
	{"Analog", "Analog Toggle", PAD::ControllerBindingType::Button, GenericInputBinding::System},
	{"Pressure", "Apply Pressure", PAD::ControllerBindingType::Button, GenericInputBinding::Unknown},
	{"LUp", "Left Stick Up", PAD::ControllerBindingType::HalfAxis, GenericInputBinding::LeftStickUp},
	{"LRight", "Left Stick Right", PAD::ControllerBindingType::HalfAxis, GenericInputBinding::LeftStickRight},
	{"LDown", "Left Stick Down", PAD::ControllerBindingType::HalfAxis, GenericInputBinding::LeftStickDown},
	{"LLeft", "Left Stick Left", PAD::ControllerBindingType::HalfAxis, GenericInputBinding::LeftStickLeft},
	{"RUp", "Right Stick Up", PAD::ControllerBindingType::HalfAxis, GenericInputBinding::RightStickUp},
	{"RRight", "Right Stick Right", PAD::ControllerBindingType::HalfAxis, GenericInputBinding::RightStickRight},
	{"RDown", "Right Stick Down", PAD::ControllerBindingType::HalfAxis, GenericInputBinding::RightStickDown},
	{"RLeft", "Right Stick Left", PAD::ControllerBindingType::HalfAxis, GenericInputBinding::RightStickLeft},
	{"LargeMotor", "Large (Low Frequency) Motor", PAD::ControllerBindingType::Motor, GenericInputBinding::LargeMotor},
	{"SmallMotor", "Small (High Frequency) Motor", PAD::ControllerBindingType::Motor, GenericInputBinding::SmallMotor},
};

static const PAD::ControllerInfo s_controller_info[] = {
	{"None", "Not Connected", nullptr, 0, PAD::ControllerType::NotConnected, PAD::VibrationCapabilities::NoVibration},
	{"DualShock2", "DualShock 2", s_dualshock2_binds, std::size(s_dualshock2_binds), PAD::ControllerType::DualShock2, PAD::VibrationCapabilities::LargeSmallMotors},
};

const PAD::ControllerInfo* PAD::GetControllerInfo(ControllerType type)
{
	for (const ControllerInfo& info : s_controller_info)
	{
		if (type == info.type)
			return &info;
	}

	return nullptr;
}

const PAD::ControllerInfo* PAD::GetControllerInfo(const std::string_view& name)
{
	for (const ControllerInfo& info : s_controller_info)
	{
		if (name == info.name)
			return &info;
	}

	return nullptr;
}

std::vector<std::pair<std::string, std::string>> PAD::GetControllerTypeNames()
{
	std::vector<std::pair<std::string, std::string>> ret;
	for (const ControllerInfo& info : s_controller_info)
		ret.emplace_back(info.name, info.display_name);

	return ret;
}

std::vector<std::string> PAD::GetControllerBinds(const std::string_view& type)
{
	std::vector<std::string> ret;

	const ControllerInfo* info = GetControllerInfo(type);
	if (info)
	{
		for (u32 i = 0; i < info->num_bindings; i++)
		{
			const ControllerBindingInfo& bi = info->bindings[i];
			if (bi.type == ControllerBindingType::Unknown || bi.type == ControllerBindingType::Motor)
				continue;

			ret.emplace_back(info->bindings[i].name);
		}
	}

	return ret;
}

void PAD::ClearPortBindings(SettingsInterface& si, u32 port)
{
	const std::string section(StringUtil::StdStringFromFormat("Pad%u", port + 1));
	const std::string type(si.GetStringValue(section.c_str(), "Type", GetDefaultPadType(port)));

	const ControllerInfo* info = GetControllerInfo(type);
	if (!info)
		return;

	for (u32 i = 0; i < info->num_bindings; i++)
		si.DeleteValue(section.c_str(), info->bindings[i].name);
}

void PAD::CopyConfiguration(SettingsInterface* dest_si, const SettingsInterface& src_si,
	bool copy_pad_config, bool copy_pad_bindings, bool copy_hotkey_bindings)
{
	if (copy_pad_config)
	{
		dest_si->CopyBoolValue(src_si, "Pad", "MultitapPort1");
		dest_si->CopyBoolValue(src_si, "Pad", "MultitapPort2");
	}

	for (u32 port = 0; port < NUM_CONTROLLER_PORTS; port++)
	{
		const std::string section(fmt::format("Pad{}", port + 1));
		const std::string type(src_si.GetStringValue(section.c_str(), "Type", GetDefaultPadType(port)));
		if (copy_pad_config)
			dest_si->SetStringValue(section.c_str(), "Type", type.c_str());

		const ControllerInfo* info = GetControllerInfo(type);
		if (!info)
			return;

		if (copy_pad_bindings)
		{
			for (u32 i = 0; i < info->num_bindings; i++)
			{
				const ControllerBindingInfo& bi = info->bindings[i];
				dest_si->CopyStringListValue(src_si, section.c_str(), bi.name);
			}

			for (u32 i = 0; i < NUM_MACRO_BUTTONS_PER_CONTROLLER; i++)
			{
				dest_si->CopyStringListValue(src_si, section.c_str(), fmt::format("Macro{}", i + 1).c_str());
				dest_si->CopyStringValue(src_si, section.c_str(), fmt::format("Macro{}Binds", i + 1).c_str());
				dest_si->CopyUIntValue(src_si, section.c_str(), fmt::format("Macro{}Frequency", i + 1).c_str());
			}
		}

		if (copy_pad_config)
		{
			dest_si->CopyFloatValue(src_si, section.c_str(), "AxisScale");

			if (info->vibration_caps != VibrationCapabilities::NoVibration)
			{
				dest_si->CopyFloatValue(src_si, section.c_str(), "LargeMotorScale");
				dest_si->CopyFloatValue(src_si, section.c_str(), "SmallMotorScale");
			}
		}
	}

	if (copy_hotkey_bindings)
	{
		std::vector<const HotkeyInfo*> hotkeys(InputManager::GetHotkeyList());
		for (const HotkeyInfo* hki : hotkeys)
			dest_si->CopyStringListValue(src_si, "Hotkeys", hki->name);
	}
}

PAD::VibrationCapabilities PAD::GetControllerVibrationCapabilities(const std::string_view& type)
{
	const ControllerInfo* info = GetControllerInfo(type);
	return info ? info->vibration_caps : VibrationCapabilities::NoVibration;
}

static u32 TryMapGenericMapping(SettingsInterface& si, const std::string& section,
	const GenericInputBindingMapping& mapping, GenericInputBinding generic_name,
	const char* bind_name)
{
	// find the mapping it corresponds to
	const std::string* found_mapping = nullptr;
	for (const std::pair<GenericInputBinding, std::string>& it : mapping)
	{
		if (it.first == generic_name)
		{
			found_mapping = &it.second;
			break;
		}
	}

	if (found_mapping)
	{
		Console.WriteLn("(MapController) Map %s/%s to '%s'", section.c_str(), bind_name, found_mapping->c_str());
		si.SetStringValue(section.c_str(), bind_name, found_mapping->c_str());
		return 1;
	}
	else
	{
		si.DeleteValue(section.c_str(), bind_name);
		return 0;
	}
}


bool PAD::MapController(SettingsInterface& si, u32 controller,
	const std::vector<std::pair<GenericInputBinding, std::string>>& mapping)
{
	const std::string section(StringUtil::StdStringFromFormat("Pad%u", controller + 1));
	const std::string type(si.GetStringValue(section.c_str(), "Type", GetDefaultPadType(controller)));
	const ControllerInfo* info = GetControllerInfo(type);
	if (!info)
		return false;

	u32 num_mappings = 0;
	for (u32 i = 0; i < info->num_bindings; i++)
	{
		const ControllerBindingInfo& bi = info->bindings[i];
		if (bi.generic_mapping == GenericInputBinding::Unknown)
			continue;

		num_mappings += TryMapGenericMapping(si, section, mapping, bi.generic_mapping, bi.name);
	}
	if (info->vibration_caps == VibrationCapabilities::LargeSmallMotors)
	{
		num_mappings += TryMapGenericMapping(si, section, mapping, GenericInputBinding::SmallMotor, "SmallMotor");
		num_mappings += TryMapGenericMapping(si, section, mapping, GenericInputBinding::LargeMotor, "LargeMotor");
	}
	else if (info->vibration_caps == VibrationCapabilities::SingleMotor)
	{
		if (TryMapGenericMapping(si, section, mapping, GenericInputBinding::LargeMotor, "Motor") == 0)
			num_mappings += TryMapGenericMapping(si, section, mapping, GenericInputBinding::SmallMotor, "Motor");
		else
			num_mappings++;
	}

	return (num_mappings > 0);
}

void PAD::SetControllerState(u32 controller, u32 bind, float value)
{
	if (controller >= NUM_CONTROLLER_PORTS || bind > MAX_KEYS)
		return;

	g_key_status.Set(controller, bind, value);
}

void PAD::LoadMacroButtonConfig(const SettingsInterface& si, u32 pad, const std::string_view& type, const std::string& section)
{
	// lazily initialized
	std::vector<std::string> binds;

	for (u32 i = 0; i < NUM_MACRO_BUTTONS_PER_CONTROLLER; i++)
	{
		std::string binds_string;
		if (!si.GetStringValue(section.c_str(), StringUtil::StdStringFromFormat("Macro%uBinds", i + 1).c_str(), &binds_string))
			continue;

		const u32 frequency = si.GetUIntValue(section.c_str(), StringUtil::StdStringFromFormat("Macro%uFrequency", i + 1).c_str(), 0u);
		if (binds.empty())
			binds = GetControllerBinds(type);

		// convert binds
		std::vector<u32> bind_indices;
		std::vector<std::string_view> buttons_split(StringUtil::SplitString(binds_string, '&', true));
		if (buttons_split.empty())
			continue;
		for (const std::string_view& button : buttons_split)
		{
			auto it = std::find(binds.begin(), binds.end(), button);
			if (it == binds.end())
			{
				Console.Error("Invalid bind '%.*s' in macro button %u for pad %u", static_cast<int>(button.size()), button.data(), pad, i);
				continue;
			}

			bind_indices.push_back(static_cast<u32>(std::distance(binds.begin(), it)));
		}
		if (bind_indices.empty())
			continue;

		s_macro_buttons[pad][i].buttons = std::move(bind_indices);
		s_macro_buttons[pad][i].toggle_frequency = frequency;
	}
}

void PAD::SetMacroButtonState(u32 pad, u32 index, bool state)
{
	if (pad >= NUM_CONTROLLER_PORTS || index >= NUM_MACRO_BUTTONS_PER_CONTROLLER)
		return;

	MacroButton& mb = s_macro_buttons[pad][index];
	if (mb.buttons.empty() || mb.trigger_state == state)
		return;

	mb.toggle_counter = mb.toggle_frequency;
	mb.trigger_state = state;
	if (mb.toggle_state != state)
	{
		mb.toggle_state = state;
		ApplyMacroButton(pad, mb);
	}
}

std::vector<std::string> PAD::GetInputProfileNames()
{
	FileSystem::FindResultsArray results;
	FileSystem::FindFiles(EmuFolders::InputProfiles.c_str(), "*.ini",
		FILESYSTEM_FIND_FILES | FILESYSTEM_FIND_HIDDEN_FILES | FILESYSTEM_FIND_RELATIVE_PATHS,
		&results);

	std::vector<std::string> ret;
	ret.reserve(results.size());
	for (FILESYSTEM_FIND_DATA& fd : results)
		ret.emplace_back(Path::GetFileTitle(fd.FileName));
	return ret;
}

void PAD::ApplyMacroButton(u32 pad, const MacroButton& mb)
{
	const float value = mb.toggle_state ? 1.0f : 0.0f;
	for (const u32 btn : mb.buttons)
		g_key_status.Set(pad, btn, value);
}

void PAD::UpdateMacroButtons()
{
	for (u32 pad = 0; pad < NUM_CONTROLLER_PORTS; pad++)
	{
		for (u32 index = 0; index < NUM_MACRO_BUTTONS_PER_CONTROLLER; index++)
		{
			MacroButton& mb = s_macro_buttons[pad][index];
			if (!mb.trigger_state || mb.toggle_frequency == 0)
				continue;

			mb.toggle_counter--;
			if (mb.toggle_counter > 0)
				continue;

			mb.toggle_counter = mb.toggle_frequency;
			mb.toggle_state = !mb.toggle_state;
			ApplyMacroButton(pad, mb);
		}
	}
}
