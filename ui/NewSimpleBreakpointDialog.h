#pragma once

#include <wx/dialog.h>

class NewSimpleBreakpointDialogData {
 public:
   wxString _name;
   bool     _isTemp     = false;
   bool     _isDisabled = false;
};

class wxCheckBox;
class wxTextCtrl;

class NewSimpleBreakpointDialog : public wxDialog {
 public:
   explicit NewSimpleBreakpointDialog(wxWindow *parent);
   auto getData() -> NewSimpleBreakpointDialogData & { return _data; }

 private:
   auto isValid() -> bool;

   NewSimpleBreakpointDialogData _data;
   wxTextCtrl *                  _textCtrl_location       = nullptr;
   wxTextCtrl *                  _textCtrl_name           = nullptr;
   wxCheckBox *                  _checkTempBreakpoint     = nullptr;
   wxCheckBox *                  _checkDisabledBreakpoint = nullptr;
};
