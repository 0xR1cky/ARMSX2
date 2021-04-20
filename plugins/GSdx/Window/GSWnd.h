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

#include "stdafx.h"
#include "GSdx.h"
#include "GSVector.h"

class GSWnd
{
protected:
	bool m_managed; // set true when we're attached to a 3rdparty window that's amanged by the emulator

public:
	GSWnd()
		: m_managed(false)
	{
	}
	virtual ~GSWnd() {}

	virtual bool Create(const std::string& title, int w, int h) = 0;
	virtual bool Attach(void* handle, bool managed = true) = 0;
	virtual void Detach() = 0;
	bool IsManaged() const { return m_managed; }

	virtual void* GetDisplay() = 0;
	virtual void* GetHandle() = 0;
	virtual GSVector4i GetClientRect() = 0;
	virtual bool SetWindowText(const char* title) = 0;

	virtual void AttachContext() {}
	virtual void DetachContext() {}

	virtual void Show() = 0;
	virtual void Hide() = 0;
	virtual void HideFrame() = 0;

	virtual void Flip() {}
	virtual void SetVSync(int vsync) {}
};

class GSWndGL : public GSWnd
{
protected:
	bool m_ctx_attached;
	std::atomic<bool> m_vsync_change_requested;
	std::atomic<int> m_vsync;

	bool IsContextAttached() const { return m_ctx_attached; }
	void PopulateGlFunction();
	virtual void PopulateWndGlFunction() = 0;
	void FullContextInit();
	virtual void CreateContext(int major, int minor) = 0;

	virtual void SetSwapInterval() = 0;
	virtual bool HasLateVsyncSupport() = 0;

public:
	GSWndGL()
		: m_ctx_attached(false)
		, m_vsync_change_requested(false)
		, m_vsync(0)
	{
	}
	virtual ~GSWndGL() {}

	virtual bool Create(const std::string& title, int w, int h) = 0;
	virtual bool Attach(void* handle, bool managed = true) = 0;
	virtual void Detach() = 0;

	virtual void* GetDisplay() = 0;
	virtual void* GetHandle() = 0;
	virtual GSVector4i GetClientRect() = 0;
	virtual bool SetWindowText(const char* title) = 0;

	virtual void AttachContext() = 0;
	virtual void DetachContext() = 0;
	virtual void* GetProcAddress(const char* name, bool opt = false) = 0;

	virtual void Show() = 0;
	virtual void Hide() = 0;
	virtual void HideFrame() = 0;
	virtual void Flip() = 0;
	virtual void SetVSync(int vsync) final;
};
