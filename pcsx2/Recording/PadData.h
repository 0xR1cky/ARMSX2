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

class PadData
{
public:
	/// Create a struct containing the PAD data from the global PAD state
	/// see - `g_key_status`
	PadData(const int port, const int slot);
	PadData(const int port, const int slot, const std::array<u8, 18> data);

	/// Constants
	static constexpr u8 ANALOG_VECTOR_NEUTRAL = 127;

	int m_ext_port;
	int m_port;
	int m_slot;

	// Analog Sticks - <x, y> - 0-255 (127 center)
	std::tuple<u8, u8> m_rightAnalog = {ANALOG_VECTOR_NEUTRAL, ANALOG_VECTOR_NEUTRAL};
	std::tuple<u8, u8> m_leftAnalog = {ANALOG_VECTOR_NEUTRAL, ANALOG_VECTOR_NEUTRAL};

	u8 m_compactPressFlagsGroupOne = 255;
	u8 m_compactPressFlagsGroupTwo = 255;

	// Buttons <pressed, pressure (0-255)>
	std::tuple<bool, u8> m_circle = {false, 0};
	std::tuple<bool, u8> m_cross = {false, 0};
	std::tuple<bool, u8> m_square = {false, 0};
	std::tuple<bool, u8> m_triangle = {false, 0};

	std::tuple<bool, u8> m_down = {false, 0};
	std::tuple<bool, u8> m_left = {false, 0};
	std::tuple<bool, u8> m_right = {false, 0};
	std::tuple<bool, u8> m_up = {false, 0};

	std::tuple<bool, u8> m_l1 = {false, 0};
	std::tuple<bool, u8> m_l2 = {false, 0};
	std::tuple<bool, u8> m_r1 = {false, 0};
	std::tuple<bool, u8> m_r2 = {false, 0};

	// Buttons <pressed>
	bool m_start = false;
	bool m_select = false;
	bool m_l3 = false;
	bool m_r3 = false;

	// Overrides the actual controller's state with the the values in this struct
	void OverrideActualController() const;

	// Prints current PadData to the Controller Log filter which is disabled by default
	void LogPadData() const;
};

