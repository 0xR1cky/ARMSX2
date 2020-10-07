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

#include "PrecompiledHeader.h"
#include "MainFrame.h"
#include "AppSaveStates.h"
#include "ConsoleLogger.h"
#include "MSWstuff.h"

#include "Dialogs/ModalPopups.h"
#include "IsoDropTarget.h"

#include <wx/iconbndl.h>

#include <unordered_map>
#include "AppAccelerators.h"

#include "svnrev.h"
#include "Saveslots.h"

// ------------------------------------------------------------------------
wxMenu* MainEmuFrame::MakeStatesSubMenu(int baseid, int loadBackupId) const
{
	wxMenu* mnuSubstates = new wxMenu();

	for (int i = 0; i < 10; i++)
	{
		// Will be changed once an iso is loaded.
		mnuSubstates->Append(baseid + i + 1, wxsFormat(_("Slot %d"), i));
	}

	if (loadBackupId >= 0)
	{
		mnuSubstates->AppendSeparator();

		wxMenuItem* m = mnuSubstates->Append(loadBackupId, _("Backup"));
		m->Enable(false);
	}

	mnuSubstates->Append(baseid - 1, _("File..."));
	return mnuSubstates;
}

void MainEmuFrame::UpdateStatusBar()
{
	wxString temp(wxEmptyString);

	if (g_Conf->EnableFastBoot)
		temp += "Fast Boot - ";

	if (g_Conf->CdvdSource == CDVD_SourceType::Iso)
		temp += "Load: '" + wxFileName(g_Conf->CurrentIso).GetFullName() + "' ";

	m_statusbar.SetStatusText(temp, 0);
	m_statusbar.SetStatusText(CDVD_SourceLabels[enum_cast(g_Conf->CdvdSource)], 1);

#ifdef __M_X86_64
	m_statusbar.SetStatusText("x64", 2);
#else
	m_statusbar.SetStatusText("x32", 2);
#endif
}

void MainEmuFrame::UpdateCdvdSrcSelection()
{
	MenuIdentifiers cdsrc = MenuId_Src_Iso;

	switch (g_Conf->CdvdSource)
	{
		case CDVD_SourceType::Iso:
			cdsrc = MenuId_Src_Iso;
			break;
		case CDVD_SourceType::Disc:
			cdsrc = MenuId_Src_Disc;
			break;
		case CDVD_SourceType::NoDisc:
			cdsrc = MenuId_Src_NoDisc;
			break;

			jNO_DEFAULT
	}
	sMenuBar.Check(cdsrc, true);
	UpdateStatusBar();
}

bool MainEmuFrame::Destroy()
{
	// Sigh: wxWidgets doesn't issue Destroy() calls for children windows when the parent
	// is destroyed (it just deletes them, quite suddenly).  So let's do it for them, since
	// our children have configuration stuff they like to do when they're closing.

	for (
		wxWindowList::const_iterator
			i = wxTopLevelWindows.begin(),
			end = wxTopLevelWindows.end();
		i != end; ++i)
	{
		wxTopLevelWindow* const win = wx_static_cast(wxTopLevelWindow*, *i);
		if (win == this)
			continue;
		if (win->GetParent() != this)
			continue;

		win->Destroy();
	}

	return _parent::Destroy();
}

// ------------------------------------------------------------------------
//     MainFrame OnEvent Handlers
// ------------------------------------------------------------------------

// Close out the console log windows along with the main emu window.
// Note: This event only happens after a close event has occurred and was *not* veto'd.  Ie,
// it means it's time to provide an unconditional closure of said window.
//
void MainEmuFrame::OnCloseWindow(wxCloseEvent& evt)
{
	if (IsBeingDeleted())
		return;

	CoreThread.Suspend();

	//bool isClosing = false;

	if (!evt.CanVeto())
	{
		// Mandatory destruction...
		//isClosing = true;
	}
	else
	{
		// TODO : Add confirmation prior to exit here!
		// Problem: Suspend is often slow because it needs to wait until the current EE frame
		// has finished processing (if the GS or logging has incurred severe overhead this makes
		// closing PCSX2 difficult).  A non-blocking suspend with modal dialog might suffice
		// however. --air

		//evt.Veto( true );
	}

	sApp.OnMainFrameClosed(GetId());

	RemoveCdvdMenu();

	RemoveEventHandler(&wxGetApp().GetRecentIsoManager());
	wxGetApp().PostIdleAppMethod(&Pcsx2App::PrepForExit);

	evt.Skip();
}

void MainEmuFrame::OnMoveAround(wxMoveEvent& evt)
{
	if (IsBeingDeleted() || !IsVisible() || IsIconized())
		return;

	// Uncomment this when doing logger stress testing (and then move the window around
	// while the logger spams itself)
	// ... makes for a good test of the message pump's responsiveness.
	if (EnableThreadedLoggingTest)
		Console.Warning("Threaded Logging Test!  (a window move event)");

	// evt.GetPosition() returns the client area position, not the window frame position.
	// So read the window's screen-relative position directly.
	g_Conf->MainGuiPosition = GetScreenPosition();

	// wxGTK note: X sends gratuitous amounts of OnMove messages for various crap actions
	// like selecting or deselecting a window, which muck up docking logic.  We filter them
	// out using 'lastpos' here. :)

	static wxPoint lastpos(wxDefaultCoord, wxDefaultCoord);
	if (lastpos == evt.GetPosition())
		return;
	lastpos = evt.GetPosition();

	if (g_Conf->ProgLogBox.AutoDock)
	{
		if (ConsoleLogFrame* proglog = wxGetApp().GetProgramLog())
		{
			if (!proglog->IsMaximized())
			{
				g_Conf->ProgLogBox.DisplayPosition = GetRect().GetTopRight();
				proglog->SetPosition(g_Conf->ProgLogBox.DisplayPosition);
			}
		}
	}

	evt.Skip();
}

void MainEmuFrame::OnLogBoxHidden()
{
	g_Conf->ProgLogBox.Visible = false;
	m_MenuItem_Console.Check(false);
}

// ------------------------------------------------------------------------
void MainEmuFrame::ConnectMenus()
{
	// System
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_BootCdvd_Click, this, MenuId_Boot_CDVD);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_FastBoot_Click, this, MenuId_Config_FastBoot);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_OpenELF_Click, this, MenuId_Boot_ELF);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_SuspendResume_Click, this, MenuId_Sys_SuspendResume);

	Bind(wxEVT_MENU, &MainEmuFrame::Menu_LoadStates_Click, this, MenuId_State_Load01 + 1, MenuId_State_Load01 + 10);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_LoadStates_Click, this, MenuId_State_LoadBackup);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_LoadStateFromFile_Click, this, MenuId_State_LoadFromFile);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_SaveStates_Click, this, MenuId_State_Save01 + 1, MenuId_State_Save01 + 10);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_SaveStateToFile_Click, this, MenuId_State_SaveToFile);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_EnableBackupStates_Click, this, MenuId_EnableBackupStates);

	Bind(wxEVT_MENU, &MainEmuFrame::Menu_EnablePatches_Click, this, MenuId_EnablePatches);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_EnableCheats_Click, this, MenuId_EnableCheats);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_EnableIPC_Click, this, MenuId_EnableIPC);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_EnableWideScreenPatches_Click, this, MenuId_EnableWideScreenPatches);
#ifndef DISABLE_RECORDING
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_EnableRecordingTools_Click, this, MenuId_EnableInputRecording);
#endif
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_EnableHostFs_Click, this, MenuId_EnableHostFs);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_SysShutdown_Click, this, MenuId_Sys_Shutdown);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Exit_Click, this, MenuId_Exit);

	// CDVD
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_IsoBrowse_Click, this, MenuId_IsoBrowse);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_IsoClear_Click, this, MenuId_IsoClear);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_CdvdSource_Click, this, MenuId_Src_Iso);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_CdvdSource_Click, this, MenuId_Src_Disc);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_CdvdSource_Click, this, MenuId_Src_NoDisc);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Ask_On_Boot_Click, this, MenuId_Ask_On_Booting);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Debug_CreateBlockdump_Click, this, MenuId_Debug_CreateBlockdump);

	// Config
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_SysSettings_Click, this, MenuId_Config_SysSettings);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_McdSettings_Click, this, MenuId_Config_McdSettings);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_SelectPluginsBios_Click, this, MenuId_Config_BIOS);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_AudioSettings_Click, this, MenuId_Config_SPU2);

	Bind(wxEVT_MENU, &MainEmuFrame::Menu_GSSettings_Click, this, MenuId_Video_CoreSettings);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_WindowSettings_Click, this, MenuId_Video_WindowSettings);
	for (int i = 0; i < PluginId_Count; ++i)
		Bind(wxEVT_MENU, &MainEmuFrame::Menu_ConfigPlugin_Click, this, MenuId_PluginBase_Settings + i * PluginMenuId_Interval);

	Bind(wxEVT_MENU, &MainEmuFrame::Menu_MultitapToggle_Click, this, MenuId_Config_Multitap0Toggle);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_MultitapToggle_Click, this, MenuId_Config_Multitap1Toggle);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_ResetAllSettings_Click, this, MenuId_Config_ResetAll);

	// Misc
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_ShowConsole, this, MenuId_Console);
#if defined(__unix__)
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_ShowConsole_Stdio, this, MenuId_Console_Stdio);
#endif

	Bind(wxEVT_MENU, &MainEmuFrame::Menu_GetStarted, this, MenuId_Help_GetStarted);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Compatibility, this, MenuId_Help_Compatibility);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Forums, this, MenuId_Help_Forums);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Website, this, MenuId_Help_Website);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Github, this, MenuId_Help_Github);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Wiki, this, MenuId_Help_Wiki);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_ShowAboutBox, this, MenuId_About);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_ChangeLang, this, MenuId_ChangeLang);

	// Debug
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Debug_Open_Click, this, MenuId_Debug_Open);

	// Capture
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Capture_Video_Record_Click, this, MenuId_Capture_Video_Record);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Capture_Video_Stop_Click, this, MenuId_Capture_Video_Stop);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Capture_Screenshot_Screenshot_Click, this, MenuId_Capture_Screenshot);

#ifndef DISABLE_RECORDING
	// Recording
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Recording_New_Click, this, MenuId_Recording_New);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Recording_Play_Click, this, MenuId_Recording_Play);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Recording_Stop_Click, this, MenuId_Recording_Stop);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Recording_TogglePause_Click, this, MenuId_Recording_TogglePause);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Recording_FrameAdvance_Click, this, MenuId_Recording_FrameAdvance);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Recording_ToggleRecordingMode_Click, this, MenuId_Recording_ToggleRecordingMode);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Recording_VirtualPad_Open_Click, this, MenuId_Recording_VirtualPad_Port0);
	Bind(wxEVT_MENU, &MainEmuFrame::Menu_Recording_VirtualPad_Open_Click, this, MenuId_Recording_VirtualPad_Port1);
#endif
}

void MainEmuFrame::InitLogBoxPosition(AppConfig::ConsoleLogOptions& conf)
{
	conf.DisplaySize.Set(
		std::min(std::max(conf.DisplaySize.GetWidth(), 160), wxGetDisplayArea().GetWidth()),
		std::min(std::max(conf.DisplaySize.GetHeight(), 160), wxGetDisplayArea().GetHeight()));

	if (conf.AutoDock)
	{
		conf.DisplayPosition = GetScreenPosition() + wxSize(GetSize().x, 0);
	}
	else if (conf.DisplayPosition != wxDefaultPosition)
	{
		if (!wxGetDisplayArea().Contains(wxRect(conf.DisplayPosition, conf.DisplaySize)))
			conf.DisplayPosition = wxDefaultPosition;
	}
}

void MainEmuFrame::DispatchEvent(const PluginEventType& plugin_evt)
{
	if (!pxAssertMsg(GetMenuBar() != NULL, "Mainframe menu bar is NULL!"))
		return;

	//ApplyCoreStatus();

	if (plugin_evt == CorePlugins_Unloaded)
	{
		for (int i = 0; i < PluginId_Count; ++i)
			m_PluginMenuPacks[i].OnUnloaded();
	}
	else if (plugin_evt == CorePlugins_Loaded)
	{
		for (int i = 0; i < PluginId_Count; ++i)
			m_PluginMenuPacks[i].OnLoaded();
	}
}

void MainEmuFrame::DispatchEvent(const CoreThreadStatus& status)
{
	if (!pxAssertMsg(GetMenuBar() != NULL, "Mainframe menu bar is NULL!"))
		return;
	ApplyCoreStatus();
}

void MainEmuFrame::AppStatusEvent_OnSettingsApplied()
{
	ApplySettings();
}

int GetPluginMenuId_Settings(PluginsEnum_t pid)
{
	return MenuId_PluginBase_Settings + ((int)pid * PluginMenuId_Interval);
}

static int GetPluginMenuId_Name(PluginsEnum_t pid)
{
	return MenuId_PluginBase_Name + ((int)pid * PluginMenuId_Interval);
}

void MainEmuFrame::CreatePcsx2Menu()
{
	// ------------------------------------------------------------------------
	// Some of the items in the System menu are configured by the UpdateCoreStatus() method.

	m_menuSys.Append(MenuId_Boot_CDVD, _("Initializing..."));

	m_menuSys.Append(MenuId_Sys_SuspendResume, _("Initializing..."));

	m_menuSys.Append(MenuId_Sys_Shutdown, _("Shut&down"),
					 _("Wipes all internal VM states and shuts down plugins."));
	m_menuSys.FindItem(MenuId_Sys_Shutdown)->Enable(false);

	m_menuSys.Append(MenuId_Boot_ELF, _("&Run ELF..."),
					 _("For running raw PS2 binaries directly"));

	m_menuSys.AppendSeparator();

	m_menuSys.Append(MenuId_Config_FastBoot, _("Fast Boot"),
					 _("Skips PS2 splash screens when booting from ISO or DVD media"), wxITEM_CHECK);

	m_menuSys.AppendCheckItem(MenuId_Debug_CreateBlockdump, _("Create &Blockdump"), _("Creates a block dump for debugging purposes."));

	m_menuSys.Append(MenuId_GameSettingsSubMenu, _("&Game Settings"), &m_GameSettingsSubmenu);

	m_GameSettingsSubmenu.Append(MenuId_EnablePatches, _("Automatic &Gamefixes"),
								 _("Automatically applies needed Gamefixes to known problematic games"), wxITEM_CHECK);

	m_GameSettingsSubmenu.Append(MenuId_EnableCheats, _("Enable &Cheats"),
								 wxEmptyString, wxITEM_CHECK);

	m_GameSettingsSubmenu.Append(MenuId_EnableIPC, _("Enable &IPC"),
								 wxEmptyString, wxITEM_CHECK);

	m_GameSettingsSubmenu.Append(MenuId_EnableWideScreenPatches, _("Enable &Widescreen Patches"),
								 _("Enabling Widescreen Patches may occasionally cause issues."), wxITEM_CHECK);

#ifndef DISABLE_RECORDING
	m_GameSettingsSubmenu.Append(MenuId_EnableInputRecording, _("Enable &Input Recording"),
								 wxEmptyString, wxITEM_CHECK);
#endif

	if (IsDebugBuild || IsDevBuild)
		m_GameSettingsSubmenu.Append(MenuId_EnableHostFs, _("Enable &Host Filesystem"),
									 wxEmptyString, wxITEM_CHECK);

	m_menuSys.AppendSeparator();

	m_menuSys.Append(MenuId_Sys_LoadStates, _("&Load state"), &m_LoadStatesSubmenu);
	m_menuSys.Append(MenuId_Sys_SaveStates, _("&Save state"), &m_SaveStatesSubmenu);

	m_menuSys.Append(MenuId_EnableBackupStates, _("&Backup before save"), wxEmptyString, wxITEM_CHECK);

	m_menuSys.AppendSeparator();

	m_menuSys.Append(MenuId_Exit, _("E&xit"),
					 AddAppName(_("Closing %s may be hazardous to your health")));
}

void MainEmuFrame::CreateCdvdMenu()
{
	// ------------------------------------------------------------------------
	wxMenu& isoRecents(wxGetApp().GetRecentIsoMenu());
	wxMenu& driveList(wxGetApp().GetDriveListMenu());

	m_menuItem_RecentIsoMenu = m_menuCDVD.AppendSubMenu(&isoRecents, _("ISO &Selector"));
	m_menuItem_DriveListMenu = m_menuCDVD.AppendSubMenu(&driveList, _("D&rive Selector"));

	m_menuCDVD.AppendSeparator();
	m_menuCDVD.Append(MenuId_Src_Iso, _("&ISO"), _("Makes the specified ISO image the CDVD source."), wxITEM_RADIO);
	m_menuCDVD.Append(MenuId_Src_Disc, _("&Disc"), _("Uses a disc drive as the CDVD source."), wxITEM_RADIO);
	m_menuCDVD.Append(MenuId_Src_NoDisc, _("&No disc"), _("Use this to boot into your virtual PS2's BIOS configuration."), wxITEM_RADIO);

#if defined(__FREEBSD__) || defined(__APPLE__)
	m_menuItem_DriveListMenu->Enable(false);
	m_menuCDVD.Enable(MenuId_Src_Disc, false);
#endif
}


void MainEmuFrame::CreateConfigMenu()
{
	m_menuConfig.Append(MenuId_Config_SysSettings, _("Emulation &Settings..."));
	m_menuConfig.Append(MenuId_Config_McdSettings, _("&Memory Cards..."));
	m_menuConfig.Append(MenuId_Config_BIOS, _("&Plugin/BIOS Selector..."));
	m_menuConfig.Append(MenuId_Config_SPU2, _("&Audio Settings..."));

	m_menuConfig.AppendSeparator();

	m_menuConfig.Append(MenuId_Config_GS, _("&Video (GS)"), m_PluginMenuPacks[PluginId_GS]);
	m_menuConfig.Append(MenuId_Config_PAD, _("&Controllers (PAD)"), m_PluginMenuPacks[PluginId_PAD]);
	m_menuConfig.Append(MenuId_Config_DEV9, _("&Dev9"), m_PluginMenuPacks[PluginId_DEV9]);
	m_menuConfig.Append(MenuId_Config_USB, _("&USB"), m_PluginMenuPacks[PluginId_USB]);

	m_menuConfig.AppendSeparator();
	m_menuConfig.Append(MenuId_Config_Multitap0Toggle, _("Multitap &1"), wxEmptyString, wxITEM_CHECK);
	m_menuConfig.Append(MenuId_Config_Multitap1Toggle, _("Multitap &2"), wxEmptyString, wxITEM_CHECK);

	m_menuConfig.AppendSeparator();

	m_menuConfig.Append(MenuId_ChangeLang, L"Change &Language..."); // Always in English
	m_menuConfig.Append(MenuId_Config_ResetAll, _("C&lear All Settings..."),
						AddAppName(_("Clears all %s settings and re-runs the startup wizard.")));
}

void MainEmuFrame::CreateWindowsMenu()
{
	m_menuWindow.Append(MenuId_Debug_Open, _("&Show Debug"), wxEmptyString, wxITEM_CHECK);

	m_menuWindow.Append(&m_MenuItem_Console);
#if defined(__unix__)
	m_menuWindow.AppendSeparator();
	m_menuWindow.Append(&m_MenuItem_Console_Stdio);
#endif
}

void MainEmuFrame::CreateCaptureMenu()
{
	m_menuCapture.Append(MenuId_Capture_Video, _("Video"), &m_submenuVideoCapture);
	m_submenuVideoCapture.Append(MenuId_Capture_Video_Record, _("Start Screenrecorder"));
	m_submenuVideoCapture.Append(MenuId_Capture_Video_Stop, _("Stop Screenrecorder"))->Enable(false);

	m_menuCapture.Append(MenuId_Capture_Screenshot, _("Screenshot"));
}

void MainEmuFrame::CreateRecordMenu()
{
#ifndef DISABLE_RECORDING
	m_menuRecording.Append(MenuId_Recording_New, _("New"), _("Create a new input recording."));
	m_menuRecording.Append(MenuId_Recording_Stop, _("Stop"), _("Stop the active input recording."))->Enable(false);
	m_menuRecording.Append(MenuId_Recording_Play, _("Play"), _("Playback an existing input recording."));
	m_menuRecording.AppendSeparator();
	m_menuRecording.Append(MenuId_Recording_TogglePause, _("Toggle Pause"), _("Pause or resume emulation on the fly."))->Enable(false);
	m_menuRecording.Append(MenuId_Recording_FrameAdvance, _("Frame Advance"), _("Advance emulation forward by a single frame at a time."))->Enable(false);
	m_menuRecording.Append(MenuId_Recording_ToggleRecordingMode, _("Toggle Recording Mode"), _("Save/playback inputs to/from the recording file."))->Enable(false);
	m_menuRecording.AppendSeparator();
	m_menuRecording.Append(MenuId_Recording_VirtualPad_Port0, _("Virtual Pad (Port 1)"));
	m_menuRecording.Append(MenuId_Recording_VirtualPad_Port1, _("Virtual Pad (Port 2)"));
#endif
}

void MainEmuFrame::CreateHelpMenu()
{
	m_menuHelp.Append(MenuId_Help_GetStarted, _("&Getting Started"));
	m_menuHelp.Append(MenuId_Help_Compatibility, _("&Compatibility"));
	m_menuHelp.AppendSeparator();
	m_menuHelp.Append(MenuId_Help_Website, _("&Website"));
	m_menuHelp.Append(MenuId_Help_Wiki, _("&Wiki"));
	m_menuHelp.Append(MenuId_Help_Forums, _("&Support Forums"));
	m_menuHelp.Append(MenuId_Help_Github, _("&Github Repository"));
	m_menuHelp.AppendSeparator();
	m_menuHelp.Append(MenuId_About, _("&About..."));
}

// ------------------------------------------------------------------------
MainEmuFrame::MainEmuFrame(wxWindow* parent, const wxString& title)
	: wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX | wxRESIZE_BORDER))

	, m_statusbar(*CreateStatusBar(2, 0))
	, m_background(new wxStaticBitmap(this, wxID_ANY, wxGetApp().GetLogoBitmap()))

	// All menu components must be created on the heap!

	, m_menubar(*new wxMenuBar())

	, m_menuCDVD(*new wxMenu())
	, m_menuSys(*new wxMenu())
	, m_menuConfig(*new wxMenu())
	, m_menuWindow(*new wxMenu())
	, m_menuCapture(*new wxMenu())
	, m_submenuVideoCapture(*new wxMenu())
#ifndef DISABLE_RECORDING
	, m_menuRecording(*new wxMenu())
#endif
	, m_menuHelp(*new wxMenu())
	, m_LoadStatesSubmenu(*MakeStatesSubMenu(MenuId_State_Load01, MenuId_State_LoadBackup))
	, m_SaveStatesSubmenu(*MakeStatesSubMenu(MenuId_State_Save01))
	, m_GameSettingsSubmenu(*new wxMenu())

	, m_MenuItem_Console(*new wxMenuItem(&m_menuWindow, MenuId_Console, _("&Show Console"), wxEmptyString, wxITEM_CHECK))
#if defined(__unix__)
	, m_MenuItem_Console_Stdio(*new wxMenuItem(&m_menuWindow, MenuId_Console_Stdio, _("&Console to Stdio"), wxEmptyString, wxITEM_CHECK))
#endif

{
	m_RestartEmuOnDelete = false;

	for (int i = 0; i < PluginId_Count; ++i)
		m_PluginMenuPacks[i].Populate((PluginsEnum_t)i);

	// ------------------------------------------------------------------------
	// Initial menubar setup.  This needs to be done first so that the menu bar's visible size
	// can be factored into the window size (which ends up being background+status+menus)

	m_menubar.Append(&m_menuSys, _("&PCSX2"));
	m_menubar.Append(&m_menuCDVD, _("CD&VD"));
	m_menubar.Append(&m_menuConfig, _("&Config"));
	m_menubar.Append(&m_menuWindow, _("&Window"));
	m_menubar.Append(&m_menuCapture, _("&Capture"));

	SetMenuBar(&m_menubar);

#ifndef DISABLE_RECORDING
	// Append the Recording options if previously enabled and setting has been picked up from ini
	if (g_Conf->EmuOptions.EnableRecordingTools)
	{
		m_menubar.Append(&m_menuRecording, _("&Input Record"));
	}
#endif
	m_menubar.Append(&m_menuHelp, _("&Help"));

	// ------------------------------------------------------------------------

	// The background logo and its window size are different on Windows. Use the
	// background logo size, which is what it'll eventually be resized to.
	wxSize backsize(m_background->GetBitmap().GetWidth(), m_background->GetBitmap().GetHeight());

	wxString wintitle;
	if (PCSX2_isReleaseVersion)
	{
		// stable releases, with a simple title.
		wintitle.Printf(L"%s  %d.%d.%d", pxGetAppName().c_str(), PCSX2_VersionHi, PCSX2_VersionMid, PCSX2_VersionLo);
	}
	else
	{
		// beta / development editions, which feature revision number and compile date.
		if (strlen(GIT_REV) > 5)
		{
			wintitle.Printf(L"%s %s", pxGetAppName().c_str(), GIT_REV);
		}
		else
		{
			wintitle.Printf(L"%s  %d.%d.%d-%lld%s (git)", pxGetAppName().c_str(), PCSX2_VersionHi, PCSX2_VersionMid,
							PCSX2_VersionLo, SVN_REV, SVN_MODS ? L"m" : wxEmptyString);
		}
	}

	SetTitle(wintitle);

	// Ideally the __WXMSW__ port should use the embedded IDI_ICON2 icon, because wxWidgets sucks and
	// loses the transparency information when loading bitmaps into icons.  But for some reason
	// I cannot get it to work despite following various examples to the letter.
	SetIcons(wxGetApp().GetIconBundle());

	int m_statusbar_widths[] = {(int)-20, (int)-3, (int)-2};
	m_statusbar.SetFieldsCount(3);
	m_statusbar.SetStatusWidths(3, m_statusbar_widths);
	m_statusbar.SetStatusText(wxEmptyString, 0);

	wxBoxSizer& joe(*new wxBoxSizer(wxVERTICAL));
	joe.Add(m_background);
	SetSizerAndFit(&joe);

	// Makes no sense, but this is needed for the window size to be correct for
	// 200% DPI on Windows. The SetSizerAndFit is supposed to be doing the exact
	// same thing.
	GetSizer()->SetSizeHints(this);

	// Use default window position if the configured windowpos is invalid (partially offscreen)
	if (g_Conf->MainGuiPosition == wxDefaultPosition || !pxIsValidWindowPosition(*this, g_Conf->MainGuiPosition))
		g_Conf->MainGuiPosition = GetScreenPosition();
	else
		SetPosition(g_Conf->MainGuiPosition);

	// Updating console log positions after the main window has been fitted to its sizer ensures
	// proper docked positioning, since the main window's size is invalid until after the sizer
	// has been set/fit.

	InitLogBoxPosition(g_Conf->ProgLogBox);
	CreatePcsx2Menu();
	CreateCdvdMenu();
	CreateConfigMenu();
	CreateWindowsMenu();
	CreateCaptureMenu();
#ifndef DISABLE_RECORDING
	CreateRecordMenu();
#endif
	CreateHelpMenu();

	m_MenuItem_Console.Check(g_Conf->ProgLogBox.Visible);

	ConnectMenus();
	Bind(wxEVT_MOVE, &MainEmuFrame::OnMoveAround, this);
	Bind(wxEVT_CLOSE_WINDOW, &MainEmuFrame::OnCloseWindow, this);
	Bind(wxEVT_SET_FOCUS, &MainEmuFrame::OnFocus, this);
	Bind(wxEVT_ACTIVATE, &MainEmuFrame::OnActivate, this);

	PushEventHandler(&wxGetApp().GetRecentIsoManager());
	SetDropTarget(new IsoDropTarget(this));

	ApplyCoreStatus();
	ApplySettings();
	AppendKeycodeNamesToMenuOptions();
}

MainEmuFrame::~MainEmuFrame()
{
	try
	{
		if (m_RestartEmuOnDelete)
		{
			sApp.SetExitOnFrameDelete(false);
			sApp.PostAppMethod(&Pcsx2App::DetectCpuAndUserMode);
			sApp.WipeUserModeSettings();
		}
	}
	DESTRUCTOR_CATCHALL
}

void MainEmuFrame::DoGiveHelp(const wxString& text, bool show)
{
	_parent::DoGiveHelp(text, show);
	wxGetApp().GetProgramLog()->DoGiveHelp(text, show);
}

// ----------------------------------------------------------------------------
// OnFocus / OnActivate : Special implementation to "connect" the console log window
// with the main frame window.  When one is clicked, the other is assured to be brought
// to the foreground with it.  (Currently only MSW only, as wxWidgets appears to have no
// equivalent to this).  Both OnFocus and OnActivate are handled because Focus events do
// not propagate up the window hierarchy, and on Activate events don't always get sent
// on the first focusing event after PCSX2 starts.

void MainEmuFrame::OnFocus(wxFocusEvent& evt)
{
	if (ConsoleLogFrame* logframe = wxGetApp().GetProgramLog())
		MSW_SetWindowAfter(logframe->GetHandle(), GetHandle());

	evt.Skip();
}

void MainEmuFrame::OnActivate(wxActivateEvent& evt)
{
	if (ConsoleLogFrame* logframe = wxGetApp().GetProgramLog())
		MSW_SetWindowAfter(logframe->GetHandle(), GetHandle());

	evt.Skip();
}
// ----------------------------------------------------------------------------

void MainEmuFrame::ApplyCoreStatus()
{
	wxMenuBar& menubar(*GetMenuBar());

	// [TODO] : Ideally each of these items would bind a listener instance to the AppCoreThread
	// dispatcher, and modify their states accordingly.  This is just a hack (for now) -- air

	if (wxMenuItem* susres = menubar.FindItem(MenuId_Sys_SuspendResume))
	{
		if (!CoreThread.IsClosing())
		{
			susres->Enable();
			susres->SetItemLabel(_("Paus&e"));
			susres->SetHelp(_("Safely pauses emulation and preserves the PS2 state."));
		}
		else
		{
			bool ActiveVM = SysHasValidState();
			susres->Enable(ActiveVM);
			if (ActiveVM)
			{
				susres->SetItemLabel(_("R&esume"));
				susres->SetHelp(_("Resumes the suspended emulation state."));
			}
			else
			{
				susres->SetItemLabel(_("Pause/Resume"));
				susres->SetHelp(_("No emulation state is active; cannot suspend or resume."));
			}
		}
	}

	const CDVD_SourceType Source = g_Conf->CdvdSource;

	wxMenuItem* cdvd_menu = menubar.FindItem(MenuId_Boot_CDVD);

	wxString label;
	wxString help_text = _("Use fast boot to skip PS2 startup and splash screens");

	switch (Source)
	{
		case CDVD_SourceType::Iso:
			label = _("Boot ISO");
			break;
		case CDVD_SourceType::Disc:
			label = _("Boot CDVD");
			break;
		case CDVD_SourceType::NoDisc:
			label = _("Boot Bios");
			break;
		default:
			label = _("Boot Bios");
			break;
	}

	cdvd_menu->SetItemLabel(label);
	cdvd_menu->SetHelp(help_text);
}

//Apply a config to the menu such that the menu reflects it properly
void MainEmuFrame::ApplySettings()
{
	ApplyConfigToGui(*g_Conf);
}

//MainEmuFrame needs to be aware which items are affected by presets if AppConfig::APPLY_FLAG_FROM_PRESET is on.
//currently only EnablePatches is affected when the settings come from a preset.
void MainEmuFrame::ApplyConfigToGui(AppConfig& configToApply, int flags)
{
	wxMenuBar& menubar(*GetMenuBar());

	menubar.Check(MenuId_EnablePatches, configToApply.EmuOptions.EnablePatches);
	menubar.Enable(MenuId_EnablePatches, !configToApply.EnablePresets);

	if (!(flags & AppConfig::APPLY_FLAG_FROM_PRESET))
	{ //these should not be affected by presets
		menubar.Check(MenuId_EnableBackupStates, configToApply.EmuOptions.BackupSavestate);
		menubar.Check(MenuId_EnableCheats, configToApply.EmuOptions.EnableCheats);
		menubar.Check(MenuId_EnableIPC, configToApply.EmuOptions.EnableIPC);
		menubar.Check(MenuId_EnableWideScreenPatches, configToApply.EmuOptions.EnableWideScreenPatches);
#ifndef DISABLE_RECORDING
		menubar.Check(MenuId_EnableInputRecording, configToApply.EmuOptions.EnableRecordingTools);
#endif
		menubar.Check(MenuId_EnableHostFs, configToApply.EmuOptions.HostFs);
		menubar.Check(MenuId_Debug_CreateBlockdump, configToApply.EmuOptions.CdvdDumpBlocks);
#if defined(__unix__)
		menubar.Check(MenuId_Console_Stdio, configToApply.EmuOptions.ConsoleToStdio);
#endif

		menubar.Check(MenuId_Config_Multitap0Toggle, configToApply.EmuOptions.MultitapPort0_Enabled);
		menubar.Check(MenuId_Config_Multitap1Toggle, configToApply.EmuOptions.MultitapPort1_Enabled);
		menubar.Check(MenuId_Config_FastBoot, configToApply.EnableFastBoot);
	}

	UpdateCdvdSrcSelection(); //shouldn't be affected by presets but updates from g_Conf anyway and not from configToApply, so no problem here.
}

//write pending preset settings from the gui to g_Conf,
//	without triggering an overall "settingsApplied" event.
void MainEmuFrame::CommitPreset_noTrigger()
{
	wxMenuBar& menubar(*GetMenuBar());
	g_Conf->EmuOptions.EnablePatches = menubar.IsChecked(MenuId_EnablePatches);
}

static void AppendShortcutToMenuOption(wxMenuItem& item, wxString keyCodeStr)
{
	wxString text = item.GetItemLabel();
	const size_t tabPos = text.rfind(L'\t');
	item.SetItemLabel(text.Mid(0, tabPos) + L"\t" + keyCodeStr);
}

void MainEmuFrame::AppendKeycodeNamesToMenuOptions()
{

	AppendShortcutToMenuOption(*m_menuSys.FindChildItem(MenuId_Sys_LoadStates), wxGetApp().GlobalAccels->findKeycodeWithCommandId("States_DefrostCurrentSlot").toTitleizedString());
	AppendShortcutToMenuOption(*m_menuSys.FindChildItem(MenuId_Sys_SaveStates), wxGetApp().GlobalAccels->findKeycodeWithCommandId("States_FreezeCurrentSlot").toTitleizedString());
}

#ifndef DISABLE_RECORDING
void MainEmuFrame::initializeRecordingMenuItem(MenuIdentifiers menuId, wxString keyCodeStr, bool enable)
{
	wxMenuItem& item = *m_menuRecording.FindChildItem(menuId);
	wxString text = item.GetItemLabel();
	const size_t tabPos = text.rfind(L'\t');
	item.SetItemLabel(text.Mid(0, tabPos) + L"\t" + keyCodeStr);
	item.Enable(enable);
}

void MainEmuFrame::enableRecordingMenuItem(MenuIdentifiers menuId, bool enable)
{
	wxMenuItem& item = *m_menuRecording.FindChildItem(menuId);
	item.Enable(enable);
}
#endif

// ------------------------------------------------------------------------
//   "Extensible" Plugin Menus
// ------------------------------------------------------------------------

void PerPluginMenuInfo::Populate(PluginsEnum_t pid)
{
	if (!pxAssert(pid < PluginId_Count))
		return;

	PluginId = pid;

	MyMenu.Append(GetPluginMenuId_Name(PluginId), _("No plugin loaded"))->Enable(false);
	MyMenu.AppendSeparator();

	if (PluginId == PluginId_GS)
	{
		MyMenu.Append(MenuId_Video_CoreSettings, _("&Core GS Settings..."),
					  _("Modify hardware emulation settings regulated by the PCSX2 core virtual machine."));

		MyMenu.Append(MenuId_Video_WindowSettings, _("&Window Settings..."),
					  _("Modify window and appearance options, including aspect ratio."));

		MyMenu.AppendSeparator();
	}

	// Populate options from the plugin here.

	MyMenu.Append(GetPluginMenuId_Settings(PluginId), _("&Plugin Settings..."),
				  wxsFormat(_("Opens the %s plugin's advanced settings dialog."), tbl_PluginInfo[pid].GetShortname().c_str()));
}

// deletes menu items belonging to (created by) the plugin.  Leaves menu items created
// by the PCSX2 core intact.
void PerPluginMenuInfo::OnUnloaded()
{
	// Delete any menu options added by plugins (typically a plugin will have already
	// done its own proper cleanup when the plugin was shutdown or unloaded, but lets
	// not trust them, shall we?)

	MenuItemAddonList& curlist(m_PluginMenuItems);
	for (uint mx = 0; mx < curlist.size(); ++mx)
		MyMenu.Delete(curlist[mx].Item);

	curlist.clear();

	MyMenu.SetLabel(GetPluginMenuId_Name(PluginId), _("No plugin loaded"));
	MyMenu.Enable(GetPluginMenuId_Settings(PluginId), false);
}

void PerPluginMenuInfo::OnLoaded()
{
	if (!CorePlugins.IsLoaded(PluginId))
		return;
	MyMenu.SetLabel(GetPluginMenuId_Name(PluginId),
					CorePlugins.GetName(PluginId) + L" " + CorePlugins.GetVersion(PluginId));
	MyMenu.Enable(GetPluginMenuId_Settings(PluginId), true);
}
