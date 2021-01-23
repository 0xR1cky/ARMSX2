/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2020  PCSX2 Dev Team
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
#include <stdlib.h>

#include <fstream>

//#include <winsock2.h>
#include "..\DEV9.h"
#include "AppConfig.h"

BOOL WritePrivateProfileInt(LPCWSTR lpAppName, LPCWSTR lpKeyName, int intvar, LPCWSTR lpFileName)
{
	return WritePrivateProfileString(lpAppName, lpKeyName, std::to_wstring(intvar).c_str(), lpFileName);
}
bool FileExists(std::wstring szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath.c_str());
	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
			!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void SaveConf()
{
	const std::wstring file(GetSettingsFolder().Combine(wxString("DEV9.cfg")).GetFullPath());
	DeleteFile(file.c_str());

	//Create file with UT16 BOM to allow PrivateProfile to save unicode data
	int bom = 0xFEFF;
	std::fstream nfile = std::fstream(file, std::ios::out | std::ios::binary);
	nfile.write((char*)&bom, 2);
	//Write header to avoid empty line
	nfile.write((char*)L"[DEV9]", 14);
	nfile.close();

	wchar_t wEth[sizeof(config.Eth)] = {0};
	mbstowcs(wEth, config.Eth, sizeof(config.Eth) - 1);
	WritePrivateProfileString(L"DEV9", L"Eth", wEth, file.c_str());
	WritePrivateProfileString(L"DEV9", L"Hdd", config.Hdd, file.c_str());

	WritePrivateProfileInt(L"DEV9", L"HddSize", config.HddSize, file.c_str());
	WritePrivateProfileInt(L"DEV9", L"ethEnable", config.ethEnable, file.c_str());
	WritePrivateProfileInt(L"DEV9", L"hddEnable", config.hddEnable, file.c_str());
}

void LoadConf()
{
	const std::wstring file(GetSettingsFolder().Combine(wxString("DEV9.cfg")).GetFullPath());
	if (FileExists(file.c_str()) == false)
		return;

	wchar_t wEth[sizeof(config.Eth)] = {0};
	mbstowcs(wEth, ETH_DEF, sizeof(config.Eth) - 1);
	GetPrivateProfileString(L"DEV9", L"Eth", wEth, wEth, sizeof(config.Eth), file.c_str());
	wcstombs(config.Eth, wEth, sizeof(config.Eth) - 1);

	GetPrivateProfileString(L"DEV9", L"Hdd", HDD_DEF, config.Hdd, sizeof(config.Hdd), file.c_str());

	config.HddSize = GetPrivateProfileInt(L"DEV9", L"HddSize", config.HddSize, file.c_str());
	config.ethEnable = GetPrivateProfileInt(L"DEV9", L"ethEnable", config.ethEnable, file.c_str());
	config.hddEnable = GetPrivateProfileInt(L"DEV9", L"hddEnable", config.hddEnable, file.c_str());
}
