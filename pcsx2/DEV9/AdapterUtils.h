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

#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#elif defined(__POSIX__)
#include <sys/types.h>
#include <ifaddrs.h>
#endif

#include <string>
#include <optional>

#include "DEV9/PacketReader/IP/IP_Address.h"

namespace AdapterUtils
{
#ifdef _WIN32
	bool GetWin32Adapter(const std::string& name, PIP_ADAPTER_ADDRESSES adapter, std::unique_ptr<IP_ADAPTER_ADDRESSES[]>* buffer);
	bool GetWin32AdapterAuto(PIP_ADAPTER_ADDRESSES adapter, std::unique_ptr<IP_ADAPTER_ADDRESSES[]>* buffer);

	std::optional<PacketReader::IP::IP_Address> GetAdapterIP(PIP_ADAPTER_ADDRESSES adapter);
	//Mask
	std::vector<PacketReader::IP::IP_Address> GetGateways(PIP_ADAPTER_ADDRESSES adapter);
	std::vector<PacketReader::IP::IP_Address> GetDNS(PIP_ADAPTER_ADDRESSES adapter);
#elif defined(__POSIX__)
	bool GetIfAdapter(const std::string& name, ifaddrs* adapter, ifaddrs** buffer);
	bool GetIfAdapterAuto(ifaddrs* adapter, ifaddrs** buffer);

	std::optional<PacketReader::IP::IP_Address> GetAdapterIP(ifaddrs* adapter);
	//Mask
	std::vector<PacketReader::IP::IP_Address> GetGateways(ifaddrs* adapter);
	std::vector<PacketReader::IP::IP_Address> GetDNS(ifaddrs* adapter);
#endif
}; // namespace AdapterUtils
