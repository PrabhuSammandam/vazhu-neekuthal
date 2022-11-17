#pragma once

#include <wx/dialog.h>

class wxTextCtrl;
class wxCheckBox;

class GoToLineDialog : public wxDialog {
 public:
   explicit GoToLineDialog(wxWindow *parent);
   auto getLine() -> uint32_t;
   auto TransferDataToWindow() -> bool override;

   wxTextCtrl *_textCtrl_line = nullptr;
};
