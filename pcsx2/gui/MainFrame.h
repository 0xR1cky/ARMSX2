/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#include "App.h"
#include "AppSaveStates.h"

#include <wx/image.h>
#include <wx/docview.h>

struct PluginMenuAddition
{
	wxString Text;
	wxString HelpText;
	PS2E_MenuItemStyle Flags;

	wxMenuItem* Item;
	int ItemId;

	// Optional user data pointer (or typecast integer value)
	void* UserPtr;

	void(PS2E_CALLBACK* OnClicked)(PS2E_THISPTR* thisptr, void* userptr);
};

// --------------------------------------------------------------------------------------
//  PerPluginMenuInfo
// --------------------------------------------------------------------------------------
class PerPluginMenuInfo
{
protected:
	typedef std::vector<PluginMenuAddition> MenuItemAddonList;

	// A list of menu items belonging to this plugin's menu.
	MenuItemAddonList m_PluginMenuItems;

public:
	wxMenu& MyMenu;
	PluginsEnum_t PluginId;

public:
	PerPluginMenuInfo()
		: MyMenu(*new wxMenu())
		, PluginId(PluginId_Count)
	{
	}

	virtual ~PerPluginMenuInfo() = default;

	void Populate(PluginsEnum_t pid);
	void OnUnloaded();
	void OnLoaded();

	operator wxMenu*() { return &MyMenu; }
	operator const wxMenu*() const { return &MyMenu; }
};

// --------------------------------------------------------------------------------------
//  InvokeMenuCommand_OnSysStateUnlocked
// --------------------------------------------------------------------------------------
class InvokeMenuCommand_OnSysStateUnlocked
	: public IEventListener_SysState,
	  public BaseDeletableObject
{
protected:
	MenuIdentifiers m_menu_cmd;

public:
	InvokeMenuCommand_OnSysStateUnlocked(MenuIdentifiers menu_command)
	{
		m_menu_cmd = menu_command;
	}

	virtual ~InvokeMenuCommand_OnSysStateUnlocked() = default;

	virtual void SaveStateAction_OnCreateFinished()
	{
		wxGetApp().PostMenuAction(m_menu_cmd);
	}
};

// --------------------------------------------------------------------------------------
//  MainEmuFrame
// --------------------------------------------------------------------------------------
class MainEmuFrame : public wxFrame,
					 public EventListener_Plugins,
					 public EventListener_CoreThread,
					 public EventListener_AppStatus
{
	typedef wxFrame _parent;

protected:
	bool m_RestartEmuOnDelete;

	wxStatusBar& m_statusbar;
	wxStaticBitmap* m_background;

	wxMenuBar& m_menubar;

	wxMenu& m_menuCDVD;
	wxMenu& m_menuSys;
	wxMenu& m_menuConfig;
	wxMenu& m_menuWindow;

	wxMenu& m_menuCapture;
	wxMenu& m_submenuVideoCapture;

#ifndef DISABLE_RECORDING
	wxMenu& m_menuRecording;
#endif
	wxMenu& m_menuHelp;

	wxMenu& m_LoadStatesSubmenu;
	wxMenu& m_SaveStatesSubmenu;
	wxMenu& m_GameSettingsSubmenu;

	wxMenuItem* m_menuItem_RecentIsoMenu;
	wxMenuItem* m_menuItem_DriveListMenu;
	wxMenuItem& m_MenuItem_Console;
#if defined(__unix__)
	wxMenuItem& m_MenuItem_Console_Stdio;
#endif

	PerPluginMenuInfo m_PluginMenuPacks[PluginId_Count];

	bool m_capturingVideo;

	virtual void DispatchEvent(const PluginEventType& plugin_evt);
	virtual void DispatchEvent(const CoreThreadStatus& status);
	virtual void AppStatusEvent_OnSettingsApplied();

public:
	MainEmuFrame(wxWindow* parent, const wxString& title);
	virtual ~MainEmuFrame();

	void OnLogBoxHidden();

	bool IsPaused() const { return GetMenuBar()->IsChecked(MenuId_Sys_SuspendResume); }
	void UpdateCdvdSrcSelection();
	void RemoveCdvdMenu();
	void EnableMenuItem(int id, bool enable);
	void CheckMenuItem(int id, bool checked);
	void SetMenuItemLabel(int id, wxString str);
	void EnableCdvdPluginSubmenu(bool isEnable = true);

	void CreateCdvdMenu();
	void CreatePcsx2Menu();
	void CreateConfigMenu();
	void CreateWindowsMenu();
	void CreateCaptureMenu();
	void CreateRecordMenu();
	void CreateHelpMenu();

	bool Destroy();

	void ApplyConfigToGui(AppConfig& configToApply, int flags = 0);
	void CommitPreset_noTrigger();
	void AppendKeycodeNamesToMenuOptions();
	void UpdateStatusBar();
#ifndef DISABLE_RECORDING
	void initializeRecordingMenuItem(MenuIdentifiers menuId, wxString keyCodeStr, bool enable = true);
	void enableRecordingMenuItem(MenuIdentifiers menuId, bool enable);
#endif

protected:
	void DoGiveHelp(const wxString& text, bool show);

	//Apply here is from config to GUI.
	void ApplySettings();
	void ApplyCoreStatus();

	void InitLogBoxPosition(AppConfig::ConsoleLogOptions& conf);

	void OnCloseWindow(wxCloseEvent& evt);
	void OnMoveAround(wxMoveEvent& evt);
	void OnFocus(wxFocusEvent& evt);
	void OnActivate(wxActivateEvent& evt);

	void Menu_SysSettings_Click(wxCommandEvent& event);
	void Menu_AudioSettings_Click(wxCommandEvent& event);
	void Menu_McdSettings_Click(wxCommandEvent& event);
	void Menu_WindowSettings_Click(wxCommandEvent& event);
	void Menu_GSSettings_Click(wxCommandEvent& event);
	void Menu_SelectPluginsBios_Click(wxCommandEvent& event);
	void Menu_ResetAllSettings_Click(wxCommandEvent& event);

	void Menu_IsoBrowse_Click(wxCommandEvent& event);
	void Menu_IsoClear_Click(wxCommandEvent& event);
	void Menu_EnableBackupStates_Click(wxCommandEvent& event);
	void Menu_EnablePatches_Click(wxCommandEvent& event);
	void Menu_EnableCheats_Click(wxCommandEvent& event);
	void Menu_EnableIPC_Click(wxCommandEvent& event);
	void Menu_EnableWideScreenPatches_Click(wxCommandEvent& event);
#ifndef DISABLE_RECORDING
	void Menu_EnableRecordingTools_Click(wxCommandEvent& event);
#endif
	void Menu_EnableHostFs_Click(wxCommandEvent& event);

	void Menu_BootCdvd_Click(wxCommandEvent& event);
	void Menu_FastBoot_Click(wxCommandEvent& event);

	void Menu_OpenELF_Click(wxCommandEvent& event);
	void Menu_CdvdSource_Click(wxCommandEvent& event);
	void Menu_LoadStates_Click(wxCommandEvent& event);
	void Menu_SaveStates_Click(wxCommandEvent& event);
	void Menu_LoadStateFromFile_Click(wxCommandEvent& event);
	void Menu_SaveStateToFile_Click(wxCommandEvent& event);
	void Menu_Exit_Click(wxCommandEvent& event);

	void Menu_SuspendResume_Click(wxCommandEvent& event);
	void Menu_SysShutdown_Click(wxCommandEvent& event);

	void Menu_ConfigPlugin_Click(wxCommandEvent& event);

	void Menu_MultitapToggle_Click(wxCommandEvent& event);

	void Menu_Debug_Open_Click(wxCommandEvent& event);
	void Menu_Debug_MemoryDump_Click(wxCommandEvent& event);
	void Menu_Debug_CreateBlockdump_Click(wxCommandEvent& event);
	void Menu_Ask_On_Boot_Click(wxCommandEvent& event);

	void Menu_ShowConsole(wxCommandEvent& event);
	void Menu_ChangeLang(wxCommandEvent& event);
	void Menu_ShowConsole_Stdio(wxCommandEvent& event);

	void Menu_GetStarted(wxCommandEvent& event);
	void Menu_Compatibility(wxCommandEvent& event);
	void Menu_Forums(wxCommandEvent& event);
	void Menu_Website(wxCommandEvent& event);
	void Menu_Github(wxCommandEvent& event);
	void Menu_Wiki(wxCommandEvent& event);
	void Menu_ShowAboutBox(wxCommandEvent& event);

	void Menu_Capture_Video_Record_Click(wxCommandEvent& event);
	void Menu_Capture_Video_Stop_Click(wxCommandEvent& event);
	void VideoCaptureUpdate();
	void Menu_Capture_Screenshot_Screenshot_Click(wxCommandEvent& event);

#ifndef DISABLE_RECORDING
	void Menu_Recording_New_Click(wxCommandEvent& event);
	void Menu_Recording_Play_Click(wxCommandEvent& event);
	void Menu_Recording_Stop_Click(wxCommandEvent& event);
	void Menu_Recording_TogglePause_Click(wxCommandEvent &event);
	void Menu_Recording_FrameAdvance_Click(wxCommandEvent &event);
	void Menu_Recording_ToggleRecordingMode_Click(wxCommandEvent &event);
	void Menu_Recording_VirtualPad_Open_Click(wxCommandEvent &event);
#endif

	void _DoBootCdvd();
	bool _DoSelectIsoBrowser(wxString& dest);
	bool _DoSelectELFBrowser();

	// ------------------------------------------------------------------------
	//     MainEmuFram Internal API for Populating Main Menu Contents
	// ------------------------------------------------------------------------

	wxMenu* MakeStatesSubMenu(int baseid, int loadBackupId = -1) const;

	void ConnectMenus();

	friend class Pcsx2App;
};

extern int GetPluginMenuId_Settings(PluginsEnum_t pid);
