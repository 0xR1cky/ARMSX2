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
#include <wx/string.h>

namespace usb_pad
{
	enum GunConMacros
	{
		GUNCON_NONE,
		GUNCON_RELOAD,
		GUNCON_TRIGGER,
		GUNCON_A,
		GUNCON_B,
		GUNCON_C,
		GUNCON_START,
		GUNCON_SELECT,
		GUNCON_DPAD_UP,
		GUNCON_DPAD_DOWN,
		GUNCON_DPAD_LEFT,
		GUNCON_DPAD_RIGHT,

		GUNCON_DPAD_A_SELECT,
		GUNCON_DPAD_B_SELECT,
		GUNCON_DPAD_UP_SELECT,
		GUNCON_DPAD_DOWN_SELECT,
		GUNCON_DPAD_LEFT_SELECT,
		GUNCON_DPAD_RIGHT_SELECT,
	};

	struct Guncon2Preset
	{
		std::string id;
		std::string name;

		float scale_x;
		float scale_y;
		int center_x;
		int center_y;

		int width;
		int height;
		int model;
	};

	struct Guncon2Config
	{
		Fixed100 Sensitivity = 100;
		int Threshold = 512;
		int Deadzone = 0;

		int Left = GunConMacros::GUNCON_TRIGGER;
		int Right = GunConMacros::GUNCON_A;
		int Middle = GunConMacros::GUNCON_B;
		int Aux_1 = GunConMacros::GUNCON_NONE;
		int Aux_2 = GunConMacros::GUNCON_NONE;
		int Wheel_up = GunConMacros::GUNCON_NONE;
		int Wheel_dn = GunConMacros::GUNCON_NONE;

		int Reload;
		bool Calibration;
		bool Cursor;

		wxString MouseDevice = L"SysMouse";

		int Lightgun_left = 1;
		int Lightgun_top = 1;
		int Lightgun_right = 65534;
		int Lightgun_bottom = 65534;

		int Model = 0;
		int Alignment = 0;

		Fixed100 Aiming_scale_X = 97.625;
		Fixed100 Aiming_scale_Y = 94.625;
		wxString Preset{"custom"};

		bool Keyboard_Dpad;
		bool Start_hotkey;
		bool Abs2Window;

		void LoadSave(IniInterface& conf);
	};

	struct PadConfig
	{
		PadConfig() {}
		struct Port
		{
			Guncon2Config Guncon2;
		} Port[2];
		void Load(int port);
		void Save(int port);
		void LoadSave(IniInterface& conf, int port);
	};

	std::vector<Guncon2Preset> GetGuncon2Presets(int port, bool restore = false);
	extern const PadConfig Config;

} //namespace usb_pad
