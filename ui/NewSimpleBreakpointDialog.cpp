#include <wx/wx.h>
#include <wx/filepicker.h>
#include "ID.h"
#include "NewSimpleBreakpointDialog.h"
#include <wx/checkbox.h>
#include <wx/valgen.h>

NewSimpleBreakpointDialog::NewSimpleBreakpointDialog(wxWindow *parent) : wxDialog{parent, DIALOG_NEW_SIMPLE_BREAKPOINT, "New Simple Breakpoint"} {
   auto *tb            = new wxBoxSizer{wxVERTICAL};
   auto *buttonSizer   = new wxStdDialogButtonSizer{};
   auto *flexGridSizer = new wxFlexGridSizer{3, 3, 2, 2};
   auto *horz1Sizer1   = new wxBoxSizer{wxHORIZONTAL};

   tb->Add(flexGridSizer, 0, wxEXPAND, 5);
   tb->Add(horz1Sizer1, 0, wxEXPAND, 5);
   tb->Add(buttonSizer, 0, wxALL | wxALIGN_RIGHT, 5);

   // row 0
   flexGridSizer->Add(new wxStaticText{this, wxID_ANY, "LineSpec Location:"}, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);
   _textCtrl_name = new wxTextCtrl{this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, wxTextValidator{wxFILTER_NONE, &_data._name}};
   _textCtrl_name->SetMinSize({200, 32});
   flexGridSizer->Add(_textCtrl_name, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 2);
   flexGridSizer->AddSpacer(0);
   flexGridSizer->AddGrowableCol(1); // make text control expandable

   // row 1
   _checkTempBreakpoint     = new wxCheckBox(this, wxID_ANY, "&Temp", wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator{&_data._isTemp});
   _checkDisabledBreakpoint = new wxCheckBox(this, wxID_ANY, "&Disabled", wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator{&_data._isDisabled});

   horz1Sizer1->Add(_checkTempBreakpoint);
   horz1Sizer1->Add(_checkDisabledBreakpoint);

   auto *buttonOk = new wxButton(this, wxID_OK, "OK");
   buttonOk->SetDefault();
   buttonSizer->AddButton(buttonOk);
   buttonSizer->AddButton(new wxButton(this, wxID_CANCEL, "Cancel"));
   buttonSizer->Realize();

   Bind(
       wxEVT_BUTTON,
       [this](wxCommandEvent &e) {
          if (_textCtrl_name->IsEmpty()) {
             wxMessageBox("Breakpoint specification Empty", "Error", wxOK | wxICON_ERROR);
          } else {
             auto newWorkspaceName = _textCtrl_name->GetValue();
             e.Skip();
          }
       },
       wxID_OK);
   SetSizerAndFit(tb);
}
