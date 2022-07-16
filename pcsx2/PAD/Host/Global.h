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

#include "common/Pcsx2Defs.h"

enum gamePadValues
{
	PAD_UP,       // Directional pad ↑
	PAD_RIGHT,    // Directional pad →
	PAD_DOWN,     // Directional pad ↓
	PAD_LEFT,     // Directional pad ←
	PAD_TRIANGLE, // Triangle button ▲
	PAD_CIRCLE,   // Circle button ●
	PAD_CROSS,    // Cross button ✖
	PAD_SQUARE,   // Square button ■
	PAD_SELECT,   // Select button
	PAD_START,    // Start button
	PAD_L1,       // L1 button
	PAD_L2,       // L2 button
	PAD_R1,       // R1 button
	PAD_R2,       // R2 button
	PAD_L3,       // Left joystick button (L3)
	PAD_R3,       // Right joystick button (R3)
	PAD_ANALOG,   // Analog mode toggle
	PAD_PRESSURE, // Pressure modifier
	PAD_L_UP,     // Left joystick (Up) ↑
	PAD_L_RIGHT,  // Left joystick (Right) →
	PAD_L_DOWN,   // Left joystick (Down) ↓
	PAD_L_LEFT,   // Left joystick (Left) ←
	PAD_R_UP,     // Right joystick (Up) ↑
	PAD_R_RIGHT,  // Right joystick (Right) →
	PAD_R_DOWN,   // Right joystick (Down) ↓
	PAD_R_LEFT,   // Right joystick (Left) ←
	MAX_KEYS,
};

static inline bool IsAnalogKey(int index)
{
	return ((index >= PAD_L_UP) && (index <= PAD_R_LEFT));
}

static inline bool IsTriggerKey(int index)
{
	return (index == PAD_L2 || index == PAD_R2);
}
