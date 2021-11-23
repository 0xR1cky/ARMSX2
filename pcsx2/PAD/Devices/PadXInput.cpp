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

#include "PadXInput.h"
#include "../PS2/PadPS2.h"

#ifdef _WIN32
void XInput_Poll(PadPS2* pad)
{
	DWORD res = XInputGetState(0, &pad->state);

	if (res != ERROR_SUCCESS)
	{
		DevCon.Warning("%s Xinput error %d", __FUNCTION__, res);
		return;
	}

	WORD xinputButtons = pad->state.Gamepad.wButtons;

	pad->SetButton(PS2Button::SELECT, (xinputButtons & XINPUT_GAMEPAD_BACK ? 0xff : 0));
	pad->SetButton(PS2Button::L3, (xinputButtons & XINPUT_GAMEPAD_LEFT_THUMB ? 0xff : 0));
	pad->SetButton(PS2Button::R3, (xinputButtons & XINPUT_GAMEPAD_RIGHT_THUMB ? 0xff : 0));
	pad->SetButton(PS2Button::START, (xinputButtons & XINPUT_GAMEPAD_START ? 0xff : 0));
	pad->SetButton(PS2Button::UP, (xinputButtons & XINPUT_GAMEPAD_DPAD_UP ? 0xff : 0));
	pad->SetButton(PS2Button::RIGHT, (xinputButtons & XINPUT_GAMEPAD_DPAD_RIGHT ? 0xff : 0));
	pad->SetButton(PS2Button::DOWN, (xinputButtons & XINPUT_GAMEPAD_DPAD_DOWN ? 0xff : 0));
	pad->SetButton(PS2Button::LEFT, (xinputButtons & XINPUT_GAMEPAD_DPAD_LEFT ? 0xff : 0));
	pad->SetButton(PS2Button::L2, pad->state.Gamepad.bLeftTrigger);
	pad->SetButton(PS2Button::R2, pad->state.Gamepad.bRightTrigger);
	pad->SetButton(PS2Button::L1, (xinputButtons & XINPUT_GAMEPAD_LEFT_SHOULDER ? 0xff : 0));
	pad->SetButton(PS2Button::R1, (xinputButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER ? 0xff : 0));
	pad->SetButton(PS2Button::TRIANGLE, (xinputButtons & XINPUT_GAMEPAD_Y ? 0xff : 0));
	pad->SetButton(PS2Button::CIRCLE, (xinputButtons & XINPUT_GAMEPAD_B ? 0xff : 0));
	pad->SetButton(PS2Button::CROSS, (xinputButtons & XINPUT_GAMEPAD_A ? 0xff : 0));
	pad->SetButton(PS2Button::SQUARE, (xinputButtons & XINPUT_GAMEPAD_X ? 0xff : 0));

	pad->SetAnalog(PS2Analog::LEFT_X, Normalize<SHORT>(std::abs(pad->state.Gamepad.sThumbLX) > 5000 ? pad->state.Gamepad.sThumbLX : 0));
	pad->SetAnalog(PS2Analog::LEFT_Y, 0xff - Normalize<SHORT>(std::abs(pad->state.Gamepad.sThumbLY) > 5000 ? pad->state.Gamepad.sThumbLY : 0));
	pad->SetAnalog(PS2Analog::RIGHT_X, Normalize<SHORT>(std::abs(pad->state.Gamepad.sThumbRX) > 5000 ? pad->state.Gamepad.sThumbRX : 0));
	pad->SetAnalog(PS2Analog::RIGHT_Y, 0xff - Normalize<SHORT>(std::abs(pad->state.Gamepad.sThumbRY) > 5000 ? pad->state.Gamepad.sThumbRY : 0));
}
#endif
