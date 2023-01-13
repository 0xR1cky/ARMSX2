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
#include "USB/deviceproxy.h"

namespace usb_lightgun
{
	class GunCon2Device final : public DeviceProxy
	{
	public:
		USBDevice* CreateDevice(SettingsInterface& si, u32 port, u32 subtype) const override;
		const char* Name() const override;
		const char* TypeName() const override;
		bool Freeze(USBDevice* dev, StateWrapper& sw) const override;
		void UpdateSettings(USBDevice* dev, SettingsInterface& si) const override;
		float GetBindingValue(const USBDevice* dev, u32 bind_index) const override;
		void SetBindingValue(USBDevice* dev, u32 bind_index, float value) const override;
		gsl::span<const InputBindingInfo> Bindings(u32 subtype) const override;
		gsl::span<const SettingInfo> Settings(u32 subtype) const override;
	};
} // namespace usb_lightgun
