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

#include <wx/wx.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/wrapsizer.h>
#include <wx/statline.h>
#include <wx/filepicker.h>
#include "padconfig.h"

// class GSUIElementHolder
// {
// 	class GSwxChoice : public wxChoice
// 	{
// 	public:
// // 		const std::vector<GSSetting>& settings;
//
// 		GSwxChoice(
// 			wxWindow* parent,
// 			wxWindowID id,
// 			const wxPoint& pos,
// 			const wxSize& size,
// 			const wxArrayString& choices,
// // 			const std::vector<GSSetting>* settings,
// 			long style = 0,
// 			const wxValidator& validator = wxDefaultValidator,
// 			const wxString& name = wxChoiceNameStr)
// 			: wxChoice(parent, id, pos, size, choices, style, validator, name)
// // 			, settings(*settings)
// 		{
// 		}
// 	};
//
// 	struct UIElem
// 	{
// 		enum class Type
// 		{
// 			CheckBox,
// 			Choice,
// 			Spin,
// 			Slider,
// 			File,
// 			Directory,
// 		};
//
// 		Type type;
// 		wxControl* control;
// 		const char* config;
// 		wxCheckBox* prereq;
//
// 		UIElem(Type type, wxControl* control, const char* config, wxCheckBox* prereq)
// 			: type(type), control(control), config(config), prereq(prereq)
// 		{
// 		}
// 	};
//
// 	wxWindow* m_window;
// 	std::vector<UIElem> m_elems;
//
// 	void addWithLabel(wxControl* control, UIElem::Type type, wxSizer* sizer, const char* label, const char* config_name, int tooltip, wxCheckBox* prereq, wxSizerFlags flags = wxSizerFlags().Centre().Expand().Left());
//
// public:
// 	GSUIElementHolder(wxWindow* window);
// 	wxCheckBox* addCheckBox(wxSizer* sizer, const char* label, const char* config_name, int tooltip = -1, wxCheckBox* prereq = nullptr);
// 	wxChoice* addComboBoxAndLabel(wxSizer* sizer, const char* label, const char* config_name, const std::vector<GSSetting>* settings, int tooltip = -1, wxCheckBox* prereq = nullptr);
// 	wxSpinCtrl* addSpin(wxSizer* sizer, const char* config_name, int min, int max, int initial, int tooltip = -1, wxCheckBox* prereq = nullptr);
// 	wxSpinCtrl* addSpinAndLabel(wxSizer* sizer, const char* label, const char* config_name, int min, int max, int initial, int tooltip = -1, wxCheckBox* prereq = nullptr);
// // 	wxSlider* addSliderAndLabel(wxSizer* sizer, const char* label, const char* config_name, int min, int max, int initial, int tooltip = -1, wxCheckBox* prereq = nullptr);
// // 	wxFilePickerCtrl* addFilePickerAndLabel(wxSizer* sizer, const char* label, const char* config_name, int tooltip = -1, wxCheckBox* prereq = nullptr);
// // 	wxDirPickerCtrl* addDirPickerAndLabel(wxSizer* sizer, const char* label, const char* config_name, int tooltip = -1, wxCheckBox* prereq = nullptr);
//
// 	void Load();
// 	void Save();
// 	void Update();
// };

namespace usb_pad
{
	class Dialog : public wxDialog
	{

	private:
	protected:
		wxSpinCtrlDouble* m_spinCtrlSens;
		wxChoice* m_choiceReload;
		wxSpinCtrl* m_spinCtrlThres;
		wxChoice* m_choiceCHair;
		wxSpinCtrl* m_spinCtrlDead;
		wxChoice* m_choiceModel;
		wxChoice* m_choiceMLeft;
		wxChoice* m_choiceMAux2;
		wxChoice* m_choiceMRight;
		wxChoice* m_choiceWheelUp;
		wxChoice* m_choiceMMid;
		wxChoice* m_choiceWheelDn;
		wxChoice* m_choiceMAux1;
		wxSpinCtrl* m_spinCtrlLeft;
		wxSpinCtrl* m_spinCtrlTop;
		wxSpinCtrl* m_spinCtrlRight;
		wxSpinCtrl* m_spinCtrlBot;
		wxCheckBox* m_checkBoxKbd;
		wxCheckBox* m_checkBoxStart;
		wxCheckBox* m_checkBoxCalib;
		wxCheckBox* m_checkBoxAbsCoords;
		wxChoice* m_choiceProfile;
		wxButton* m_buttonProfEdit;
		wxButton* m_buttonProfDef;
		wxButton* m_buttonAPI;
		wxButton* m_buttonAPI2;
		wxSpinCtrlDouble* m_spinCtrlAimScaleX;
		wxSpinCtrlDouble* m_spinCtrlAimScaleY;
		wxStdDialogButtonSizer* m_sdbSizer2;
		wxButton* m_sdbSizer2OK;
		wxButton* m_sdbSizer2Cancel;

		// Virtual event handlers, overide them in your derived class
		virtual void EditProfiles(wxCommandEvent& event);
		virtual void LoadDefaultProfiles(wxCommandEvent& event);
		virtual void ConfigureApi(wxCommandEvent& event);
		virtual void ConfigureApi2(wxCommandEvent& event);
		virtual void OnOkClicked(wxCommandEvent& event);

		int m_port;
		std::string m_api;
		std::vector<Guncon2Preset> m_presets;

	public:
		Dialog(int port, const std::string& api);
		~Dialog();
		void Load();
		void Save();
	};
} // namespace usb_pad
