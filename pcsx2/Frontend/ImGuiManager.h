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

struct ImFont;

union InputBindingKey;
enum class GenericInputBinding : u8;

namespace ImGuiManager
{
	/// Initializes ImGui, creates fonts, etc.
	bool Initialize();

	/// Frees all ImGui resources.
	void Shutdown();

	/// Updates internal state when the window is size.
	void WindowResized();

	/// Updates scaling of the on-screen elements.
	void UpdateScale();

	/// Call at the beginning of the frame to set up ImGui state.
	void NewFrame();

	/// Renders any on-screen display elements.
	void RenderOSD();

	/// Returns the scale of all on-screen elements.
	float GetGlobalScale();

	/// Returns the standard font for external drawing.
	ImFont* GetStandardFont();

	/// Returns the fixed-width font for external drawing.
	ImFont* GetFixedFont();

#ifdef PCSX2_CORE
	/// Called on the UI or CPU thread in response to mouse movement.
	void UpdateMousePosition(float x, float y);

	/// Called on the CPU thread in response to a mouse button press.
	/// Returns true if ImGui intercepted the event, and regular handlers should not execute.
	bool ProcessPointerButtonEvent(InputBindingKey key, float value);

	/// Called on the CPU thread in response to a mouse wheel movement.
	/// Returns true if ImGui intercepted the event, and regular handlers should not execute.
	bool ProcessPointerAxisEvent(InputBindingKey key, float value);

	/// Called on the CPU thread in response to a key press.
	/// Returns true if ImGui intercepted the event, and regular handlers should not execute.
	bool ProcessHostKeyEvent(InputBindingKey key, float value);

	/// Called on the CPU thread when any input event fires. Allows imgui to take over controller navigation.
	bool ProcessGenericInputEvent(GenericInputBinding key, float value);
#endif
} // namespace ImGuiManager

