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
#include "App.h"
#include "AppAccelerators.h"
#include "Dialogs/ConfigurationDialog.h"
#include "ConfigurationPanels.h"

#include <wx/spinctrl.h>
#include "fmt/core.h"

using namespace pxSizerFlags;

// --------------------------------------------------------------------------------------
//  FramelimiterPanel Implementations
// --------------------------------------------------------------------------------------

Panels::FramelimiterPanel::FramelimiterPanel( wxWindow* parent )
	: BaseApplicableConfigPanel_SpecificConfig( parent )
{
	//  Implement custom hotkeys (F4) with translatable string intact + not blank in GUI. 
	m_check_LimiterDisable	= new pxCheckBox( this, _("Disable Framelimiting") + wxString(" (") +  wxGetApp().GlobalAccels->findKeycodeWithCommandId("Framelimiter_MasterToggle").toTitleizedString()+ wxString(")"),
		_("Uncaps FPS. Useful for running benchmarks.") );

	m_check_LimiterDisable->SetToolTip( pxEt( L"Note that when Framelimiting is disabled, Turbo and SlowMotion modes will not be available either."
	) );

	m_spin_NominalPct = new wxSpinCtrl(this);
	m_spin_SlomoPct = new wxSpinCtrl(this);
	m_spin_TurboPct = new wxSpinCtrl(this);

	m_text_BaseNtsc		= CreateNumericalTextCtrl( this, 7 );
	m_text_BasePal		= CreateNumericalTextCtrl( this, 7 );

	m_spin_NominalPct	->SetRange( 10,  1000 );
	m_spin_SlomoPct		->SetRange(  5,  1000 );
	m_spin_TurboPct		->SetRange( 10,  1000 );

	// ------------------------------------------------------------
	// Sizers and Layouts

	*this += m_check_LimiterDisable | StdExpand();

	wxFlexGridSizer& s_spins( *new wxFlexGridSizer( 5 ) );
	s_spins.AddGrowableCol( 0 );

	s_spins += Label(_("Base Framerate Adjust:"))	| StdExpand();
	s_spins += 5;
	s_spins += m_spin_NominalPct					| pxBorder(wxTOP, 3);
	s_spins += Label(L"%")							| StdExpand();
	s_spins += 5;

	//  Implement custom hotkeys (Shift + Tab) with translatable string intact + not blank in GUI. 

	s_spins += Label(_("Slow Motion Adjust:") + wxString(" ") + fmt::format("({})", wxGetApp().GlobalAccels->findKeycodeWithCommandId("Framelimiter_SlomoToggle").toTitleizedString())) | StdExpand();
	s_spins += 5;
	s_spins += m_spin_SlomoPct						| pxBorder(wxTOP, 3);
	s_spins += Label(L"%")							| StdExpand();
	s_spins += 5;

	//  Implement custom hotkeys (Tab) with translatable string intact + not blank in GUI. 

	s_spins += Label(_("Turbo Adjust:") + wxString(" ") + fmt::format("({})", wxGetApp().GlobalAccels->findKeycodeWithCommandId("Framelimiter_TurboToggle").toTitleizedString())) | StdExpand();
	s_spins += 5;
	s_spins += m_spin_TurboPct						| pxBorder(wxTOP, 3);
	s_spins += Label(L"%") 							| StdExpand();
	s_spins += 5;

	wxFlexGridSizer& s_fps( *new wxFlexGridSizer( 5 ) );
	s_fps.AddGrowableCol( 0 );

	s_fps	+= Label(_("NTSC Framerate:"))	| StdExpand();
	s_fps	+= 5;
	s_fps	+= m_text_BaseNtsc				| pxBorder(wxTOP, 2).Right();
	s_fps	+= Label(_("FPS"))				| StdExpand();
	s_fps	+= 5;

	s_fps	+= Label(_("PAL Framerate:"))	| StdExpand();
	s_fps	+= 5;
	s_fps	+= m_text_BasePal				| pxBorder(wxTOP, 2).Right();
	s_fps	+= Label(_("FPS"))				| StdExpand();
	s_fps	+= 5;

	*this	+= s_spins	| pxExpand;
	*this	+= 5;
	*this	+= s_fps	| pxExpand;

	AppStatusEvent_OnSettingsApplied();
}

void Panels::FramelimiterPanel::AppStatusEvent_OnSettingsApplied()
{
	ApplyConfigToGui( *g_Conf );
}

void Panels::FramelimiterPanel::ApplyConfigToGui( AppConfig& configToApply, int flags )
{
	const AppConfig::FramerateOptions& appfps( configToApply.Framerate );
	const Pcsx2Config::GSOptions& gsconf( configToApply.EmuOptions.GS );

	if( ! (flags & AppConfig::APPLY_FLAG_FROM_PRESET) ){	//Presets don't control these: only change if config doesn't come from preset.
	
		m_check_LimiterDisable->SetValue( !gsconf.FrameLimitEnable );

		m_spin_TurboPct		->SetValue( appfps.TurboScalar.Raw );
		m_spin_SlomoPct		->SetValue( appfps.SlomoScalar.Raw );

		m_spin_TurboPct		->Enable( 1 );
		m_spin_SlomoPct		->Enable( 1 );
	}

	m_text_BaseNtsc		->ChangeValue( gsconf.FramerateNTSC.ToString() );
	m_text_BasePal		->ChangeValue( gsconf.FrameratePAL.ToString() );

	m_spin_NominalPct	->SetValue( appfps.NominalScalar.Raw );
	m_spin_NominalPct	->Enable(!configToApply.EnablePresets);

	// Vsync timing controls only on devel builds / via manual ini editing
#ifdef PCSX2_DEVBUILD
	m_text_BaseNtsc		->Enable(!configToApply.EnablePresets);
	m_text_BasePal		->Enable(!configToApply.EnablePresets);
#else
	m_text_BaseNtsc		->Enable( 0 );
	m_text_BasePal		->Enable( 0 );
#endif
}

void Panels::FramelimiterPanel::Apply()
{
	AppConfig::FramerateOptions& appfps( g_Conf->Framerate );
	Pcsx2Config::GSOptions& gsconf( g_Conf->EmuOptions.GS );

	gsconf.FrameLimitEnable	= !m_check_LimiterDisable->GetValue();

	appfps.NominalScalar.Raw	= m_spin_NominalPct	->GetValue();
	appfps.TurboScalar.Raw		= m_spin_TurboPct	->GetValue();
	appfps.SlomoScalar.Raw		= m_spin_SlomoPct	->GetValue();

	try {
		gsconf.FramerateNTSC	= Fixed100::FromString( m_text_BaseNtsc->GetValue() );
		gsconf.FrameratePAL		= Fixed100::FromString( m_text_BasePal->GetValue() );
	}
	catch( Exception::ParseError& )
	{
		throw Exception::CannotApplySettings( this )
			.SetDiagMsg(pxsFmt(
				L"Error while parsing either NTSC or PAL framerate settings.\n\tNTSC Input = %s\n\tPAL Input  = %s",
				WX_STR(m_text_BaseNtsc->GetValue()), WX_STR(m_text_BasePal->GetValue())
			) )
			.SetUserMsg(_t("Error while parsing either NTSC or PAL framerate settings.  Settings must be valid floating point numerics."));
	}

	appfps.SanityCheck();

	// If the user has a command line override specified, we need to disable it
	// so that their changes take effect
	wxGetApp().Overrides.ProfilingMode = false;
}

// --------------------------------------------------------------------------------------
//  FrameSkipPanel Implementations
// --------------------------------------------------------------------------------------

Panels::FrameSkipPanel::FrameSkipPanel( wxWindow* parent )
	: BaseApplicableConfigPanel_SpecificConfig( parent )
{
	const RadioPanelItem FrameskipOptions[] =
	{
		RadioPanelItem(
			_("Disabled [default]")
		),
		//  Implement custom hotkeys (Tab) with translatable string intact + not blank in GUI.  
		RadioPanelItem(
			_("Skip only on Turbo, to enable press") + fmt::format("{} ({})", " ", wxGetApp().GlobalAccels->findKeycodeWithCommandId("Framelimiter_TurboToggle").toTitleizedString())
		),
		//  Implement custom hotkeys (Shift + F4) with translatable string intact + not blank in GUI.  
		RadioPanelItem(
			_("Constant skipping") + fmt::format("{} ({})", " ", wxGetApp().GlobalAccels->findKeycodeWithCommandId("Frameskip_Toggle").toTitleizedString()),
			wxEmptyString,
			_("Normal and Turbo limit rates skip frames.  Slow motion mode will still disable frameskipping.")
		),
	};

	m_radio_SkipMode = new pxRadioPanel( this, FrameskipOptions );
	m_radio_SkipMode->Realize();

	m_spin_FramesToDraw = new wxSpinCtrl(this);
	m_spin_FramesToSkip = new wxSpinCtrl(this);

	// Set tooltips for spinners.


	// ------------------------------------------------------------
	// Sizers and Layouts

	*this += m_radio_SkipMode;

	wxFlexGridSizer& s_spins( *new wxFlexGridSizer( 4 ) );
	//s_spins.AddGrowableCol( 0 );

	s_spins += m_spin_FramesToDraw			| pxBorder(wxTOP, 2);
	s_spins += 10;
	s_spins += Label(_("Frames to Draw"))	| StdExpand();
	s_spins += 10;

	s_spins += m_spin_FramesToSkip			| pxBorder(wxTOP, 2);
	s_spins += 10;
	s_spins += Label(_("Frames to Skip"))	| StdExpand();
	s_spins += 10;

	*this	+= s_spins	| StdExpand();

	*this	+= Text( pxE( L"Notice: Due to PS2 hardware design, precise frame skipping is impossible. Enabling it will cause severe graphical errors in some games." )
	) | StdExpand();

	*this += 24; // Extends the right box to match the left one. Only works with (Windows) 100% dpi.

	AppStatusEvent_OnSettingsApplied();
}

void Panels::FrameSkipPanel::AppStatusEvent_OnSettingsApplied()
{
	ApplyConfigToGui( *g_Conf );
}

void Panels::FrameSkipPanel::ApplyConfigToGui( AppConfig& configToApply, int flags )
{
	const AppConfig::FramerateOptions& appfps( configToApply.Framerate );
	const Pcsx2Config::GSOptions& gsconf( configToApply.EmuOptions.GS );

	m_radio_SkipMode->SetSelection( appfps.SkipOnLimit ? 2 : (appfps.SkipOnTurbo ? 1 : 0) );

	m_spin_FramesToDraw->SetValue( gsconf.FramesToDraw );
	m_spin_FramesToDraw->Enable(!configToApply.EnablePresets);
	m_spin_FramesToSkip->SetValue( gsconf.FramesToSkip );
	m_spin_FramesToSkip->Enable(!configToApply.EnablePresets);

	this->Enable(!configToApply.EnablePresets);
}


void Panels::FrameSkipPanel::Apply()
{
	AppConfig::FramerateOptions& appfps( g_Conf->Framerate );
	Pcsx2Config::GSOptions& gsconf( g_Conf->EmuOptions.GS );

	gsconf.FramesToDraw = m_spin_FramesToDraw->GetValue();
	gsconf.FramesToSkip = m_spin_FramesToSkip->GetValue();

	switch( m_radio_SkipMode->GetSelection() )
	{
		case 0:
			appfps.SkipOnLimit = false;
			appfps.SkipOnTurbo = false;
			gsconf.FrameSkipEnable = false;
		break;

		case 1:
			appfps.SkipOnLimit = false;
			appfps.SkipOnTurbo = true;
			//gsconf.FrameSkipEnable = true;
		break;

		case 2:
			appfps.SkipOnLimit = true;
			appfps.SkipOnTurbo = true;
			gsconf.FrameSkipEnable = true;
		break;
	}

	appfps.SanityCheck();
}

// --------------------------------------------------------------------------------------
//  VideoPanel Implementation
// --------------------------------------------------------------------------------------

Panels::VideoPanel::VideoPanel( wxWindow* parent ) :
	BaseApplicableConfigPanel_SpecificConfig( parent )
{
	wxPanelWithHelpers* left	= new wxPanelWithHelpers( this, wxVERTICAL );
	wxPanelWithHelpers* right	= new wxPanelWithHelpers( this, wxVERTICAL );

	m_check_SynchronousGS = new pxCheckBox( left, _("Use Synchronized MTGS"),
		_t("For troubleshooting potential bugs in the MTGS only, as it is potentially very slow.")
	);

	m_spinner_VsyncQueue = new wxSpinCtrl(left);
	m_spinner_VsyncQueue->SetRange(0, 3);

	m_restore_defaults = new wxButton(right, wxID_DEFAULT, _("Restore Defaults"));

	m_spinner_VsyncQueue->SetToolTip( pxEt(L"Setting this to a lower value improves input lag, a value around 2 or 3 will slightly improve framerates. (Default is 2)"));
	m_check_SynchronousGS->SetToolTip( pxEt( L"Enable this if you think MTGS thread sync is causing crashes or graphical errors. For debugging to see if GS is running at the correct speed."));

	//GSWindowSettingsPanel* winpan = new GSWindowSettingsPanel( left );
	//winpan->AddFrame(_("Display/Window"));

	m_span = new FrameSkipPanel( right );
	m_span->AddFrame(_("Frame Skipping"));

	m_fpan = new FramelimiterPanel( left );
	m_fpan->AddFrame(_("Framelimiter"));

	wxFlexGridSizer* s_table = new wxFlexGridSizer( 2 );
	wxGridSizer* s_vsyncs = new wxGridSizer( 2 );
	s_table->AddGrowableCol( 0, 1 );
	s_table->AddGrowableCol( 1, 1 );

	*right		+= m_span		| pxExpand;
	*right		+= 5;
	*right		+= m_restore_defaults | StdButton();

	*left		+= m_fpan		| pxExpand;
	*left		+= 5;
	
	*s_vsyncs	+= Label(_("Vsyncs in MTGS Queue:")) | StdExpand();
	*s_vsyncs	+= m_spinner_VsyncQueue | pxBorder(wxTOP, -2).Right();
	*left		+= s_vsyncs | StdExpand();
	*left		+= 2;
	*left		+= m_check_SynchronousGS | StdExpand();

	*s_table	+= left		| StdExpand();
	*s_table	+= right	| StdExpand();

	*this		+= s_table	| pxExpand;

	Bind(wxEVT_BUTTON, &VideoPanel::Defaults_Click, this, wxID_DEFAULT);
	AppStatusEvent_OnSettingsApplied();
}

void Panels::VideoPanel::Defaults_Click(wxCommandEvent& evt)
{
	AppConfig config = *g_Conf;
	config.EmuOptions.GS = Pcsx2Config::GSOptions();
	config.Framerate = AppConfig::FramerateOptions();
	VideoPanel::ApplyConfigToGui(config);
	m_fpan->ApplyConfigToGui(config);
	m_span->ApplyConfigToGui(config);
	evt.Skip();
}

void Panels::VideoPanel::OnOpenWindowSettings( wxCommandEvent& evt )
{
	AppOpenDialog<Dialogs::ComponentsConfigDialog>( this );

	// don't evt.skip, this prevents the Apply button from being activated. :)
}

void Panels::VideoPanel::Apply()
{
	g_Conf->EmuOptions.GS.SynchronousMTGS	= m_check_SynchronousGS->GetValue();
	g_Conf->EmuOptions.GS.VsyncQueueSize = m_spinner_VsyncQueue->GetValue();
}

void Panels::VideoPanel::AppStatusEvent_OnSettingsApplied()
{
	ApplyConfigToGui(*g_Conf);
}

void Panels::VideoPanel::ApplyConfigToGui( AppConfig& configToApply, int flags ){
	
	m_check_SynchronousGS->SetValue( configToApply.EmuOptions.GS.SynchronousMTGS );
	m_spinner_VsyncQueue->SetValue( configToApply.EmuOptions.GS.VsyncQueueSize );
	m_check_SynchronousGS->Enable(!configToApply.EnablePresets);

	if( flags & AppConfig::APPLY_FLAG_MANUALLY_PROPAGATE )
	{
		m_span->ApplyConfigToGui( configToApply, true );
		m_fpan->ApplyConfigToGui( configToApply, true );
	}

	Layout();
}

