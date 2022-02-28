#pragma once

#include "GdbSessionInfo.h"
#include <wx/dialog.h>

class wxTextCtrl;
class wxCheckBox;

class RemoteAttachSessionDialog : public wxDialog {
 public:
   explicit RemoteAttachSessionDialog(wxWindow *parent);
   auto getData() -> GdbSessionInfo & { return _data; }
   void setData(GdbSessionInfo &data) { _data = data; }
   auto TransferDataToWindow() -> bool override;
   auto TransferDataFromWindow() -> bool override;

 private:
   auto isValid() -> bool;

   GdbSessionInfo _data;
   wxTextCtrl *   _textCtrl_sessionName    = nullptr;
   wxTextCtrl *   _textCtrl_gdb            = nullptr;
   wxTextCtrl *   _textCtrl_gdbArgs        = nullptr;
   wxTextCtrl *   _textCtrl_execFile       = nullptr;
   wxTextCtrl *   _textCtrl_execArgs       = nullptr;
   wxTextCtrl *   _textCtrl_execWorkingDir = nullptr;
   wxTextCtrl *   _textCtrl_host           = nullptr;
   wxTextCtrl *   _textCtrl_port           = nullptr;
   wxTextCtrl *   _textCtrl_remoteProcess  = nullptr;
};
