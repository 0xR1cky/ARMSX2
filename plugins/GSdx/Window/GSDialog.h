/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GSSetting.h"

class GSDialog
{
	int m_id;

	static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static UINT GetTooltipStructSize();

protected:
	HWND m_hWnd;

	virtual void OnInit() {}
	virtual bool OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
	virtual bool OnCommand(HWND hWnd, UINT id, UINT code);

public:
	GSDialog(UINT id);
	virtual ~GSDialog() {}

	int GetId() const {return m_id;}

	INT_PTR DoModal();

	std::string GetText(UINT id);
	int GetTextAsInt(UINT id);

	void SetText(UINT id, const char* str);
	void SetTextAsInt(UINT id, int i);

	void ComboBoxInit(UINT id, const std::vector<GSSetting>& settings, int32_t selectionValue, int32_t maxValue = INT32_MAX);
	int ComboBoxAppend(UINT id, const char* str, LPARAM data = 0, bool select = false);
	int ComboBoxAppend(UINT id, const wchar_t* str, LPARAM data = 0, bool select = false);
	bool ComboBoxGetSelData(UINT id, INT_PTR& data);
	void ComboBoxFixDroppedWidth(UINT id);

	void OpenFileDialog(UINT id, const char *title);

	void AddTooltip(UINT id);

	static void InitCommonControls();

private:
	int BoxAppend(HWND& hWnd, int item, LPARAM data = 0, bool select = false);
};
