/*  PCSX2 - PS2 Emulator for PCs
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

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "PAD/Host/Global.h"
#include "SaveState.h"

class SettingsInterface;
struct WindowInfo;
enum class GenericInputBinding : u8;

s32 PADinit();
void PADshutdown();
s32 PADopen(const WindowInfo& wi);
void PADclose();
s32 PADsetSlot(u8 port, u8 slot);
s32 PADfreeze(FreezeAction mode, freezeData* data);
u8 PADstartPoll(int pad);
u8 PADpoll(u8 value);

namespace PAD
{
	enum class VibrationCapabilities
	{
		NoVibration,
		LargeSmallMotors,
		SingleMotor,
		Count
	};

	/// Number of macro buttons per controller.
	static constexpr u32 NUM_MACRO_BUTTONS_PER_CONTROLLER = 4;

	/// Reloads configuration.
	void LoadConfig(const SettingsInterface& si);

	/// Restores default configuration.
	void SetDefaultConfig(SettingsInterface& si);

	/// Updates vibration and other internal state. Called at the *end* of a frame.
	void Update();

	/// Returns a list of controller type names.
	std::vector<std::string> GetControllerTypeNames();

	/// Returns the list of binds for the specified controller type.
	std::vector<std::string> GetControllerBinds(const std::string_view& type);

	/// Returns the vibration configuration for the specified controller type.
	VibrationCapabilities GetControllerVibrationCapabilities(const std::string_view& type);

	/// Performs automatic controller mapping with the provided list of generic mappings.
	bool MapController(SettingsInterface& si, u32 controller,
		const std::vector<std::pair<GenericInputBinding, std::string>>& mapping);

	/// Sets the specified bind on a controller to the specified pressure (normalized to 0..1).
	void SetControllerState(u32 controller, u32 bind, float value);

	/// Sets the state of the specified macro button.
	void SetMacroButtonState(u32 pad, u32 index, bool state);
} // namespace PAD
