#include "GoToLineDialog.h"
#include "wx/sizer.h"
#include "wx/valnum.h"
#include <wx/wx.h>

GoToLineDialog::GoToLineDialog(wxWindow *parent) : wxDialog{parent, wxID_ANY, "GoTo Line"} {
   auto *tb            = new wxBoxSizer{wxVERTICAL};
   auto *flexGridSizer = new wxFlexGridSizer{9, 3, 2, 2};
   auto *buttonSizer   = new wxStdDialogButtonSizer{};

   // row 0
   auto *tempStaticRoot = new wxStaticText{this, wxID_ANY, "Line No :"};
   _textCtrl_line       = new wxTextCtrl{this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, wxIntegerValidator<uint32_t>{}};

   flexGridSizer->Add(tempStaticRoot, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);            // column 0
   flexGridSizer->Add(_textCtrl_line, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 2); // column 1

   tb->Add(flexGridSizer, 0, wxEXPAND, 5);
   tb->Add(buttonSizer, 0, wxALL | wxALIGN_RIGHT, 5);

   auto *buttonOk = new wxButton(this, wxID_OK, "OK");
   buttonOk->SetDefault();
   buttonSizer->AddButton(buttonOk);
   buttonSizer->AddButton(new wxButton(this, wxID_CANCEL, "Cancel"));
   buttonSizer->Realize();

   SetSizerAndFit(tb);
   _textCtrl_line->SetFocus();
}

auto GoToLineDialog::TransferDataToWindow() -> bool { return true; }

auto GoToLineDialog::getLine() -> uint32_t {
   long value = 0;
   _textCtrl_line->GetValue().ToLong(&value);
   return value;
}
