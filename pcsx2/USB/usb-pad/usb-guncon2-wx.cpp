/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2020  PCSX2 Dev Team
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
#include "AppConfig.h"
#include "padproxy.h"
#include "USB/shared/inifile_usb.h"
#include "USB/usb-hid/hidproxy.h"
#include "USB/usb-hid/usb-hid.h"
#include "usb-guncon2-wx.h"

#ifndef countof
#define countof(a) (sizeof(a) / sizeof(a[0]))
#endif

using namespace usb_pad;

Dialog::Dialog(int port, const std::string& api)
	: wxDialog(nullptr, wxID_ANY, "Guncon2 Config", wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER)
	, m_port(port)
	, m_api(api)
{
	wxStaticText* staticText;
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* bSizer1 = new wxBoxSizer(wxVERTICAL);
	wxGridSizer* gSizer2 = new wxGridSizer(0, 2, 5, 5);
	wxStaticBoxSizer* sbSizer1 = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Mouse")), wxVERTICAL);
	wxBoxSizer* bSizer2 = new wxBoxSizer(wxVERTICAL);

	wxFlexGridSizer* fgSizer4 = new wxFlexGridSizer(0, 4, 0, 0);
	fgSizer4->SetFlexibleDirection(wxBOTH);
	fgSizer4->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	staticText = new wxStaticText(sbSizer1->GetStaticBox(), wxID_ANY, wxT("Sensitivity"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer4->Add(staticText, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	m_spinCtrlSens = new wxSpinCtrlDouble(sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999, 100.000000, 0.1);
	m_spinCtrlSens->SetDigits(3);
	fgSizer4->Add(m_spinCtrlSens, 0, wxALL | wxEXPAND, 5);

	staticText = new wxStaticText(sbSizer1->GetStaticBox(), wxID_ANY, wxT("Reload"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer4->Add(staticText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	wxString m_choiceReloadChoices[] = {wxT("Manual"), wxT("Semi")};
	m_choiceReload = new wxChoice(sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, countof(m_choiceReloadChoices), m_choiceReloadChoices, 0);
	m_choiceReload->SetSelection(0);
	fgSizer4->Add(m_choiceReload, 0, wxALL, 5);

	staticText = new wxStaticText(sbSizer1->GetStaticBox(), wxID_ANY, wxT("Threshold"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer4->Add(staticText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	m_spinCtrlThres = new wxSpinCtrl(sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 512, 512);
	fgSizer4->Add(m_spinCtrlThres, 0, wxALL | wxEXPAND, 5);

	staticText = new wxStaticText(sbSizer1->GetStaticBox(), wxID_ANY, wxT("Crosshair"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer4->Add(staticText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	wxString m_choice2Choices[] = {wxT("Visible"), wxT("Hidden")};
	m_choiceCHair = new wxChoice(sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, countof(m_choice2Choices), m_choice2Choices, 0);
	m_choiceCHair->SetSelection(0);
	fgSizer4->Add(m_choiceCHair, 0, wxALL, 5);

	staticText = new wxStaticText(sbSizer1->GetStaticBox(), wxID_ANY, wxT("Deadzone"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer4->Add(staticText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	m_spinCtrlDead = new wxSpinCtrl(sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 512, 0);
	fgSizer4->Add(m_spinCtrlDead, 0, wxALL | wxEXPAND, 5);

	staticText = new wxStaticText(sbSizer1->GetStaticBox(), wxID_ANY, _("Model"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer4->Add(staticText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	wxString m_choice3Choices[] = {wxT("Namco")};
	m_choiceModel = new wxChoice(sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, countof(m_choice3Choices), m_choice3Choices, 0);
	m_choiceModel->SetSelection(0);
	fgSizer4->Add(m_choiceModel, 0, wxALL, 5);


	bSizer2->Add(fgSizer4, 0, 0, 5);

	wxFlexGridSizer* fgSizer5;
	fgSizer5 = new wxFlexGridSizer(0, 4, 0, 0);
	fgSizer5->SetFlexibleDirection(wxBOTH);
	fgSizer5->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	const wxString ms_btns[] = {
		_("None"),
		_("Reload"),
		_("Trigger"),
		_("A"),
		_("B"),
		_("C"),
		_("Start"),
		_("Select"),
		_("D-Up"),
		_("D-Down"),
		_("D-Left"),
		_("D-Right"),
		_("A + Select"),
		_("B + Select"),
		_("D-Up + Select"),
		_("D-Down + Select"),
		_("D-Left + Select"),
		_("D-Right + Select"),
	};

	staticText = new wxStaticText(sbSizer1->GetStaticBox(), wxID_ANY, wxT("Left"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer5->Add(staticText, 0, wxALL, 5);

	m_choiceMLeft = new wxChoice(sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, countof(ms_btns), ms_btns, 0);
	m_choiceMLeft->SetSelection(0);
	fgSizer5->Add(m_choiceMLeft, 0, wxALL, 5);

	staticText = new wxStaticText(sbSizer1->GetStaticBox(), wxID_ANY, wxT("Aux 2"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer5->Add(staticText, 0, wxALL, 5);

	m_choiceMAux2 = new wxChoice(sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, countof(ms_btns), ms_btns, 0);
	m_choiceMAux2->SetSelection(0);
	fgSizer5->Add(m_choiceMAux2, 0, wxALL, 5);

	staticText = new wxStaticText(sbSizer1->GetStaticBox(), wxID_ANY, wxT("Right"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer5->Add(staticText, 0, wxALL, 5);

	m_choiceMRight = new wxChoice(sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, countof(ms_btns), ms_btns, 0);
	m_choiceMRight->SetSelection(0);
	fgSizer5->Add(m_choiceMRight, 0, wxALL, 5);

	staticText = new wxStaticText(sbSizer1->GetStaticBox(), wxID_ANY, wxT("Wheel up"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer5->Add(staticText, 0, wxALL, 5);

	m_choiceWheelUp = new wxChoice(sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, countof(ms_btns), ms_btns, 0);
	m_choiceWheelUp->SetSelection(0);
	fgSizer5->Add(m_choiceWheelUp, 0, wxALL, 5);

	staticText = new wxStaticText(sbSizer1->GetStaticBox(), wxID_ANY, wxT("Middle"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer5->Add(staticText, 0, wxALL, 5);

	m_choiceMMid = new wxChoice(sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, countof(ms_btns), ms_btns, 0);
	m_choiceMMid->SetSelection(0);
	fgSizer5->Add(m_choiceMMid, 0, wxALL, 5);

	staticText = new wxStaticText(sbSizer1->GetStaticBox(), wxID_ANY, wxT("Wheel down"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer5->Add(staticText, 0, wxALL, 5);

	m_choiceWheelDn = new wxChoice(sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, countof(ms_btns), ms_btns, 0);
	m_choiceWheelDn->SetSelection(0);
	fgSizer5->Add(m_choiceWheelDn, 0, wxALL, 5);

	staticText = new wxStaticText(sbSizer1->GetStaticBox(), wxID_ANY, wxT("Aux 1"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer5->Add(staticText, 0, wxALL, 5);

	m_choiceMAux1 = new wxChoice(sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, countof(ms_btns), ms_btns, 0);
	m_choiceMAux1->SetSelection(0);
	fgSizer5->Add(m_choiceMAux1, 0, wxALL, 5);


	bSizer2->Add(fgSizer5, 1, wxEXPAND, 5);


	sbSizer1->Add(bSizer2, 1, wxEXPAND, 5);


	gSizer2->Add(sbSizer1, 0, wxEXPAND, 5);

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, wxT("Lightgun")), wxVERTICAL);

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer(0, 2, 0, 0);
	fgSizer3->SetFlexibleDirection(wxBOTH);
	fgSizer3->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	staticText = new wxStaticText(sbSizer3->GetStaticBox(), wxID_ANY, wxT("Left"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer3->Add(staticText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	m_spinCtrlLeft = new wxSpinCtrl(sbSizer3->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 65534, 1);
	fgSizer3->Add(m_spinCtrlLeft, 0, wxALL | wxEXPAND, 5);

	staticText = new wxStaticText(sbSizer3->GetStaticBox(), wxID_ANY, wxT("Top"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer3->Add(staticText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	m_spinCtrlTop = new wxSpinCtrl(sbSizer3->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 65534, 1);
	fgSizer3->Add(m_spinCtrlTop, 0, wxALL | wxEXPAND, 5);

	staticText = new wxStaticText(sbSizer3->GetStaticBox(), wxID_ANY, wxT("Right"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer3->Add(staticText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	m_spinCtrlRight = new wxSpinCtrl(sbSizer3->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 65534, 65534);
	fgSizer3->Add(m_spinCtrlRight, 0, wxALL | wxEXPAND, 5);

	staticText = new wxStaticText(sbSizer3->GetStaticBox(), wxID_ANY, wxT("Bottom"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer3->Add(staticText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	m_spinCtrlBot = new wxSpinCtrl(sbSizer3->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 65534, 65534);
	fgSizer3->Add(m_spinCtrlBot, 0, wxALL | wxEXPAND, 5);


	sbSizer3->Add(fgSizer3, 1, wxEXPAND, 5);


	gSizer2->Add(sbSizer3, 1, wxEXPAND, 5);

	wxStaticBoxSizer* sbSizer31;
	sbSizer31 = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, wxEmptyString), wxVERTICAL);

	m_checkBoxKbd = new wxCheckBox(sbSizer31->GetStaticBox(), wxID_ANY, wxT("Use keyboard as D-Pad (WASD)"), wxDefaultPosition, wxDefaultSize, 0);
	sbSizer31->Add(m_checkBoxKbd, 0, wxALL, 5);

	m_checkBoxStart = new wxCheckBox(sbSizer31->GetStaticBox(), wxID_ANY, wxT("START = A +B + Trigger"), wxDefaultPosition, wxDefaultSize, 0);
	sbSizer31->Add(m_checkBoxStart, 0, wxALL, 5);

	m_checkBoxCalib = new wxCheckBox(sbSizer31->GetStaticBox(), wxID_ANY, wxT("Mouse calibration hack"), wxDefaultPosition, wxDefaultSize, 0);
	sbSizer31->Add(m_checkBoxCalib, 0, wxALL, 5);

	m_checkBoxAbsCoords = new wxCheckBox(sbSizer31->GetStaticBox(), wxID_ANY, wxT("Convert absolute coords to window"), wxDefaultPosition, wxDefaultSize, 0);
	sbSizer31->Add(m_checkBoxAbsCoords, 0, wxALL, 5);

	wxFlexGridSizer* fgSizer41;
	fgSizer41 = new wxFlexGridSizer(0, 2, 0, 0);
	fgSizer41->SetFlexibleDirection(wxBOTH);
	fgSizer41->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	staticText = new wxStaticText(sbSizer31->GetStaticBox(), wxID_ANY, wxT("Alignment"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer41->Add(staticText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	wxArrayString m_choiceProfileChoices;
	m_choiceProfile = new wxChoice(sbSizer31->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceProfileChoices, 0);
	m_choiceProfile->SetSelection(0);
	fgSizer41->Add(m_choiceProfile, 0, wxALL, 5);

	m_buttonProfEdit = new wxButton(sbSizer31->GetStaticBox(), wxID_ANY, wxT("Edit"), wxDefaultPosition, wxDefaultSize, 0);
	fgSizer41->Add(m_buttonProfEdit, 0, wxALL, 5);

	m_buttonProfDef = new wxButton(sbSizer31->GetStaticBox(), wxID_ANY, wxT("Default"), wxDefaultPosition, wxDefaultSize, 0);
	fgSizer41->Add(m_buttonProfDef, 0, wxALL, 5);


	sbSizer31->Add(fgSizer41, 1, wxEXPAND, 5);


	gSizer2->Add(sbSizer31, 1, wxEXPAND, 5);

	wxFlexGridSizer* fgSizer6;
	fgSizer6 = new wxFlexGridSizer(0, 2, 0, 0);
	fgSizer6->SetFlexibleDirection(wxBOTH);
	fgSizer6->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	staticText = new wxStaticText(this, wxID_ANY, wxT("Aiming scale X"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer6->Add(staticText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	m_spinCtrlAimScaleX = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 100, 0.1);
	m_spinCtrlAimScaleX->SetDigits(2);
	fgSizer6->Add(m_spinCtrlAimScaleX, 0, wxALL | wxEXPAND, 5);

	staticText = new wxStaticText(this, wxID_ANY, wxT("Aiming scale Y"), wxDefaultPosition, wxDefaultSize, 0);
	staticText->Wrap(-1);
	fgSizer6->Add(staticText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	m_spinCtrlAimScaleY = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 100, 0.1);
	m_spinCtrlAimScaleY->SetDigits(2);
	fgSizer6->Add(m_spinCtrlAimScaleY, 0, wxALL | wxEXPAND, 5);

	m_buttonAPI = new wxButton(this, wxID_ANY, wxT("Configure mouse"), wxDefaultPosition, wxDefaultSize, 0);
	fgSizer6->Add(m_buttonAPI, 0, wxALL | wxEXPAND, 5);

	m_buttonAPI2 = new wxButton(this, wxID_ANY, wxT("Configure keyboard"), wxDefaultPosition, wxDefaultSize, 0);
	fgSizer6->Add(m_buttonAPI2, 0, wxALL | wxEXPAND, 5);


	gSizer2->Add(fgSizer6, 1, wxEXPAND, 5);


	bSizer1->Add(gSizer2, 1, wxEXPAND, 5);

	m_sdbSizer2 = new wxStdDialogButtonSizer();
	m_sdbSizer2OK = new wxButton(this, wxID_OK);
	m_sdbSizer2->AddButton(m_sdbSizer2OK);
	m_sdbSizer2Cancel = new wxButton(this, wxID_CANCEL);
	m_sdbSizer2->AddButton(m_sdbSizer2Cancel);
	m_sdbSizer2->Realize();

	bSizer1->Add(m_sdbSizer2, 0, wxEXPAND, 5);


	this->SetSizer(bSizer1);
	this->Layout();
	bSizer1->Fit(this);

	this->Centre(wxBOTH);

	// Connect Events
	m_buttonProfEdit->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Dialog::EditProfiles), NULL, this);
	m_buttonProfDef->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Dialog::LoadDefaultProfiles), NULL, this);
	m_buttonAPI->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Dialog::ConfigureApi), NULL, this);
	m_buttonAPI2->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Dialog::ConfigureApi2), NULL, this);
	m_sdbSizer2OK->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Dialog::OnOkClicked), NULL, this);

	Load();
}

Dialog::~Dialog()
{
	// Disconnect Events
	m_buttonProfEdit->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Dialog::EditProfiles), NULL, this);
	m_buttonProfDef->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Dialog::LoadDefaultProfiles), NULL, this);
	m_buttonAPI->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Dialog::ConfigureApi), NULL, this);
	m_buttonAPI2->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Dialog::ConfigureApi2), NULL, this);
	m_sdbSizer2OK->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Dialog::OnOkClicked), NULL, this);
}

void Dialog::Load()
{
	const_cast<PadConfig&>(Config).Load(m_port);

	auto& s = Config.Port[m_port].Guncon2;
	m_spinCtrlSens->SetValue(s.Sensitivity.ToDouble());
	m_spinCtrlThres->SetValue(s.Threshold);
	m_spinCtrlDead->SetValue(s.Deadzone);

	m_choiceReload->SetSelection(s.Reload);
	m_choiceCHair->SetSelection(s.Cursor);
	m_choiceModel->SetSelection(s.Model);

	m_choiceMLeft->SetSelection(s.Left);
	m_choiceMRight->SetSelection(s.Right);
	m_choiceMMid->SetSelection(s.Middle);

	m_choiceMAux1->SetSelection(s.Aux_1);
	m_choiceMAux2->SetSelection(s.Aux_2);
	m_choiceWheelUp->SetSelection(s.Wheel_up);
	m_choiceWheelDn->SetSelection(s.Wheel_dn);

	m_spinCtrlLeft->SetValue(s.Lightgun_left);
	m_spinCtrlTop->SetValue(s.Lightgun_top);
	m_spinCtrlRight->SetValue(s.Lightgun_right);
	m_spinCtrlBot->SetValue(s.Lightgun_bottom);

	m_checkBoxKbd->SetValue(s.Keyboard_Dpad);
	m_checkBoxStart->SetValue(s.Start_hotkey);
	m_checkBoxCalib->SetValue(s.Calibration);
	m_checkBoxAbsCoords->SetValue(s.Abs2Window);

	m_spinCtrlAimScaleX->SetValue(s.Aiming_scale_X.ToDouble());
	m_spinCtrlAimScaleY->SetValue(s.Aiming_scale_Y.ToDouble());

	m_presets = GetGuncon2Presets(m_port, false);
	m_choiceProfile->Clear();

	for (const auto& p : m_presets)
	{
		m_choiceProfile->Append(p.name);
		if (s.Preset == p.id)
			m_choiceProfile->SetSelection(m_choiceProfile->GetCount() - 1);
	}
}

void Dialog::Save()
{
	auto& s = const_cast<PadConfig&>(Config).Port[m_port].Guncon2;
	s.Sensitivity = m_spinCtrlSens->GetValue();
	s.Threshold = m_spinCtrlThres->GetValue();
	s.Deadzone = m_spinCtrlDead->GetValue();

	s.Reload = m_choiceReload->GetSelection();
	s.Cursor = m_choiceCHair->GetSelection();
	s.Model = m_choiceModel->GetSelection();

	s.Left = m_choiceMLeft->GetSelection();
	s.Right = m_choiceMRight->GetSelection();
	s.Middle = m_choiceMMid->GetSelection();

	s.Aux_1 = m_choiceMAux1->GetSelection();
	s.Aux_2 = m_choiceMAux2->GetSelection();
	s.Wheel_up = m_choiceWheelUp->GetSelection();
	s.Wheel_dn = m_choiceWheelDn->GetSelection();

	s.Lightgun_left = m_spinCtrlLeft->GetValue();
	s.Lightgun_top = m_spinCtrlTop->GetValue();
	s.Lightgun_right = m_spinCtrlRight->GetValue();
	s.Lightgun_bottom = m_spinCtrlBot->GetValue();

	s.Keyboard_Dpad = m_checkBoxKbd->GetValue();
	s.Start_hotkey = m_checkBoxStart->GetValue();
	s.Calibration = m_checkBoxCalib->GetValue();
	s.Abs2Window = m_checkBoxAbsCoords->GetValue();

	s.Aiming_scale_X = m_spinCtrlAimScaleX->GetValue();
	s.Aiming_scale_Y = m_spinCtrlAimScaleY->GetValue();

	const auto preset = m_choiceProfile->GetSelection();
	if (preset != wxNOT_FOUND)
	{
		if (preset < static_cast<int>(m_presets.size()))
			s.Preset = m_presets[preset].id;
		else
			s.Preset = "custom";
	}

	const_cast<PadConfig&>(Config).Save(m_port);
}

void Dialog::EditProfiles(wxCommandEvent& event) { event.Skip(); }
void Dialog::LoadDefaultProfiles(wxCommandEvent& event) { event.Skip(); }

void Dialog::OnOkClicked(wxCommandEvent& event)
{
	event.Skip();
	Save();
	EndModal(wxID_OK);
}

void Dialog::ConfigureApi(wxCommandEvent& event)
{
	event.Skip();
	auto proxy = usb_hid::RegisterUsbHID::instance().Proxy(m_api);
	std::string hid_type(Guncon2Device::TypeName());
	hid_type += "_ms";
	if (proxy)
		proxy->Configure(m_port, hid_type.c_str(), HID_MOUSE, nullptr);
}

void Dialog::ConfigureApi2(wxCommandEvent& event)
{
	event.Skip();
	auto proxy = usb_hid::RegisterUsbHID::instance().Proxy(m_api);
	std::string hid_type(Guncon2Device::TypeName());
	hid_type += "_kbd";
	if (proxy)
		proxy->Configure(m_port, hid_type.c_str(), HID_KEYBOARD, nullptr);
}
