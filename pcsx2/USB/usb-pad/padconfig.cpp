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

#include "PrecompiledHeader.h"

#include <wx/fileconf.h>
#include <fstream>

#include "AppConfig.h"
#include "Utilities/IniInterface.h"
#include "padconfig.h"

namespace usb_pad
{

#define PadIniEntry(varname) ini.Entry(wxT(#varname), Port[port].varname, Port[port].varname)

	const char guncon2_default_presets[] = "\
VERSION = 3.00\n\n\
; cmdline_id  game_name\n\
; sensitivity x-y  center x-y\n\
; model  width-height\n\
\n\
\n\
\n\
; 480p\n\
; - (mouse only) use calibration hack\n\
\n\
dino_stalker \"Dino Stalker (U)\"\n\
90.25 92.5 390 132\n\
namco 640 240\n\
\n\
\n\
\n\
; 480i\n\
\n\
endgame \"Endgame (U)\"\n\
89.25 93.5 422 141\n\
namco 640 240\n\
\n\
\n\
\n\
; 480i, 480p\n\
; - (mouse only) use calibration hack\n\
;\n\
; NOTE: Aim a little to the left of 'X' for calibration\n\
\n\
guncom2 \"(*) Guncom 2 (E)\"\n\
90.5 114.75 390 146\n\
namco 640 256\n\
\n\
\n\
\n\
; 480i\n\
\n\
gunfighter2 \"Gunfighter 2 - Jesse James (E)\"\n\
84.5 89.0 456 164\n\
namco 640 256\n\
\n\
\n\
\n\
; 480i\n\
; - (mouse only) use calibration hack\n\
\n\
gunvari_i \"Gunvari Collection (J) (480i)\"\n\
90.25 98.0 390 138\n\
namco 640 240\n\
\n\
\n\
\n\
; 480p\n\
; - (mouse only) use calibration hack\n\
\n\
gunvari \"Gunvari Collection (J) (480p)\"\n\
86.75 96.0 454 164\n\
namco 640 256\n\
\n\
\n\
\n\
; 480i, 480p\n\
;\n\
; NOTE: Aim a little to the left of center for calibration\n\
\n\
ninja_assault_e \"(*) Ninja Assault (E)\"\n\
90.25 94.5 390 169\n\
namco 640 256\n\
\n\
\n\
\n\
; 480i, 480p\n\
; - (mouse only) use calibration hack\n\
;\n\
; NOTE: Aim a little to the left of center for calibration\n\
\n\
ninja_assault \"(*) Ninja Assault (U)\"\n\
90.25 92.5 390 132\n\
namco 640 240\n\
\n\
\n\
\n\
; 480p\n\
; - (mouse only) use calibration hack\n\
;\n\
; - Calibration: hold down trigger after each shot and keep gun still\n\
\n\
re_survivor2 \"(*) Resident Evil Survivor 2 (E)\"\n\
84.75 96.0 454 164\n\
namco 640 240\n\
\n\
\n\
\n\
; 480p\n\
; - (mouse only) use calibration hack\n\
\n\
re_deadaim \"Resident Evil - Dead Aim (U)\"\n\
90.25 93.5 420 132\n\
namco 640 240\n\
\n\
\n\
\n\
; 480i\n\
; - (mouse only) use calibration hack\n\
; - options -> controller 2 = g-con 2 (shooting)\n\
\n\
starsky_hutch \"Starsky & Hutch (U)\"\n\
90.25 91.75 453 154\n\
namco 640 256\n\
\n\
\n\
\n\
; 480i, 480p\n\
; - (mouse only) use calibration hack\n\
\n\
time_crisis2 \"Time Crisis 2 (U)\"\n\
90.25 97.5 390 154\n\
namco 640 240\n\
\n\
\n\
\n\
; 480i, 480p\n\
; - (mouse only) use calibration hack\n\
\n\
time_crisis3 \"Time Crisis 3 (U)\"\n\
90.25 97.5 390 154\n\
namco 640 240\n\
\n\
\n\
\n\
; 480i\n\
; - (mouse only) use calibration hack\n\
\n\
time_crisis_zone_ui \"Time Crisis - Crisis Zone (U) (480i)\"\n\
90.25 99.0 390 153\n\
namco 640 240\n\
\n\
\n\
\n\
; 480p\n\
; - (mouse only) use calibration hack\n\
\n\
time_crisis_zone \"Time Crisis - Crisis Zone (U) (480p)\"\n\
94.5 104.75 423 407\n\
namco 768 768\n\
\n\
\n\
\n\
; 480p\n\
; - (mouse only) use calibration hack\n\
\n\
vampire_night \"Vampire Night (U)\"\n\
97.5 104.75 423 407\n\
namco 768 768\n\
\n\
\n\
\n\
; 480i\n\
; - (mouse only) use calibration hack\n\
\n\
virtua_cop_ei \"Virtua Cop - Elite Edition (E,J) (480i)\"\n\
88.75 100.0 454 164\n\
namco 640 256\n\
\n\
\n\
\n\
; 480p\n\
; - (mouse only) use calibration hack\n\
\n\
virtua_cop \"Virtua Cop - Elite Edition (E,J) (480p)\"\n\
85.75 92.0 456 164\n\
namco 640 256\n\
\n\
\n\
\n\
";

	static std::ifstream& getline(std::ifstream& ifs, std::string& s)
	{
		while (std::getline(ifs, s))
		{
			// remove comments
			s.erase(std::find(s.begin(), s.end(), ';'), s.end());
			if (s.empty())
				continue;
			break;
		}
		return ifs;
	}

	std::vector<Guncon2Preset> GetGuncon2Presets(int port, bool restore)
	{
		auto presetIni = GetSettingsFolder().Combine(wxString::Format("presets_guncon%d.ini", port)).GetFullPath();
		if (!wxFileExists(presetIni) || restore)
		{
#if _WIN32
			std::ofstream outfile(presetIni.wc_str());
#else
			std::ofstream outfile(presetIni.c_str());
#endif
			outfile << guncon2_default_presets;
		}

#if _WIN32
		std::ifstream infile(presetIni.wc_str());
#else
		std::ifstream infile(presetIni.c_str());
#endif
		std::vector<Guncon2Preset> presets;
		presets.push_back({"custom", "(-- Custom --)  use aiming values", 97.625, 94.625, 274, 168, 0});

		std::string line;
		if (getline(infile, line) && line != "VERSION = 3.00")
		{
			Console.WriteLn(Color_Red, "Invalid guncon2 presets version");
			return presets;
		}

		while (getline(infile, line))
		{
			Guncon2Preset preset;

			size_t quote_s = line.find('"');
			size_t quote_e = line.rfind('"');

			if (quote_s == std::string::npos || quote_s >= quote_e)
			{
				Console.WriteLn(Color_Red, "Guncon2 preset parse error, no profile name: '%s'", line.c_str());
				break;
			}
			preset.id = line.substr(0, quote_s);
			preset.name = line.substr(quote_s + 1, quote_e - quote_s - 1);

			if (getline(infile, line))
			{
				sscanf(line.c_str(), "%f %f %d %d",
					&preset.scale_x, &preset.scale_y,
					&preset.center_x, &preset.center_y);
			}
			if (getline(infile, line))
			{
				sscanf(line.c_str(), "%*s %d %d",
					/*&model,*/ &preset.width, &preset.height);
			}
			presets.emplace_back(preset);
		}

		return presets;
	}

	void Guncon2Config::LoadSave(IniInterface& ini)
	{
		ScopedIniGroup path(ini, wxString(L"Guncon2"));
		IniEntry(Sensitivity);
		IniEntry(Threshold);
		IniEntry(Deadzone);

		IniEntry(Left);
		IniEntry(Right);
		IniEntry(Middle);
		IniEntry(Aux_1);
		IniEntry(Aux_2);
		IniEntry(Wheel_up);
		IniEntry(Wheel_dn);

		IniEntry(Reload);
		IniEntry(Calibration);
		IniEntry(Cursor);

		IniEntry(MouseDevice);

		IniEntry(Lightgun_left);
		IniEntry(Lightgun_top);
		IniEntry(Lightgun_right);
		IniEntry(Lightgun_bottom);

		IniEntry(Model);
		IniEntry(Alignment);

		IniEntry(Aiming_scale_X);
		IniEntry(Aiming_scale_Y);
		IniEntry(Preset);

		IniEntry(Keyboard_Dpad);
		IniEntry(Start_hotkey);
		IniEntry(Abs2Window);
	}

	void PadConfig::LoadSave(IniInterface& ini, int port)
	{
		Port[port].Guncon2.LoadSave(ini);
	}

	void PadConfig::Load(int port /*, DeviceType type = DEVTYPE_ALL*/)
	{
		auto dstfile = GetSettingsFolder().Combine(wxString(L"wxUSB.ini")).GetFullPath();
		wxFileConfig cfg(wxEmptyString, wxEmptyString, dstfile, wxEmptyString, wxCONFIG_USE_RELATIVE_PATH);
		IniLoader loader(cfg);

		ScopedIniGroup path(loader, wxString::Format(L"Port%d", port));
		LoadSave(loader, port);
	}

	void PadConfig::Save(int port)
	{
		auto dstfile = GetSettingsFolder().Combine(wxString(L"wxUSB.ini")).GetFullPath();
		wxFileConfig cfg(wxEmptyString, wxEmptyString, dstfile, wxEmptyString, wxCONFIG_USE_RELATIVE_PATH);
		IniSaver saver(cfg);

		ScopedIniGroup path(saver, wxString::Format(L"Port%d", port));
		LoadSave(saver, port);
	}

	const PadConfig Config;

} // namespace usb_pad
