#include "RemoteAttachSessionDialog.h"
#include "ID.h"
#include "wx/msgdlg.h"
#include "wx/string.h"
#include "wx/valtext.h"
#include <cstdarg>
#include <wx/filepicker.h>
#include <wx/valgen.h>
#include <wx/wx.h>

RemoteAttachSessionDialog::RemoteAttachSessionDialog(wxWindow *parent) : wxDialog{parent, DIALOG_SESSION, "Remote Attach Session"} {
   auto *tb            = new wxBoxSizer{wxVERTICAL};
   auto *flexGridSizer = new wxFlexGridSizer{9, 3, 2, 2};
   auto *buttonSizer   = new wxStdDialogButtonSizer{};

   // row 0
   auto *tempStaticRoot  = new wxStaticText{this, wxID_ANY, "Session Name"};
   _textCtrl_sessionName = new wxTextCtrl{this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, wxTextValidator{wxFILTER_NONE, &_data._sessionName}};

   flexGridSizer->Add(tempStaticRoot, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);                   // column 0
   flexGridSizer->Add(_textCtrl_sessionName, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 2); // column 1
   flexGridSizer->AddSpacer(0);                                                                 // column 2

   // row 1
   tempStaticRoot = new wxStaticText{this, wxID_ANY, "Gdb"};
   _textCtrl_gdb  = new wxTextCtrl{this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, wxTextValidator{wxFILTER_NONE, &_data._gdbExePath}};
   _textCtrl_gdb->SetMinSize({400, 32});

   auto *filePicker = new wxFilePickerCtrl{this, wxID_ANY};
   filePicker->Bind(wxEVT_FILEPICKER_CHANGED, [this](wxFileDirPickerEvent &e) {
      _textCtrl_gdb->Clear();
      _textCtrl_gdb->AppendText(e.GetPath());
   });
   filePicker->SetPath(_data._gdbExePath);

   flexGridSizer->Add(tempStaticRoot, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);           // column 0
   flexGridSizer->Add(_textCtrl_gdb, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 2); // column 1
   flexGridSizer->Add(filePicker, 0, wxALL, 2);

   // row 2
   tempStaticRoot    = new wxStaticText{this, wxID_ANY, "Gdb Arguments"};
   _textCtrl_gdbArgs = new wxTextCtrl{this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, wxTextValidator{wxFILTER_NONE, &_data._gdbArgs}};

   flexGridSizer->Add(tempStaticRoot, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);               // column 0
   flexGridSizer->Add(_textCtrl_gdbArgs, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 2); // column 1
   flexGridSizer->AddSpacer(0);                                                             // column 2

   // row 3
   tempStaticRoot     = new wxStaticText{this, wxID_ANY, "Exec File"};
   _textCtrl_execFile = new wxTextCtrl{this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, wxTextValidator{wxFILTER_NONE, &_data._exePath}};

   filePicker = new wxFilePickerCtrl{this, wxID_ANY};
   filePicker->Bind(wxEVT_FILEPICKER_CHANGED, [this](wxFileDirPickerEvent &e) {
      _textCtrl_execFile->Clear();
      _textCtrl_execFile->AppendText(e.GetPath());
   });
   filePicker->SetPath(_data._exePath);

   flexGridSizer->Add(tempStaticRoot, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);                // column 0
   flexGridSizer->Add(_textCtrl_execFile, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 2); // column 1
   flexGridSizer->Add(filePicker, 0, wxALL, 2);

   // row 4
   tempStaticRoot     = new wxStaticText{this, wxID_ANY, "Exec Args"};
   _textCtrl_execArgs = new wxTextCtrl{this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, wxTextValidator{wxFILTER_NONE, &_data._exeArgs}};

   flexGridSizer->Add(tempStaticRoot, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);                // column 0
   flexGridSizer->Add(_textCtrl_execArgs, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 2); // column 1
   flexGridSizer->AddSpacer(0);                                                              // column 2

   // row 5
   tempStaticRoot           = new wxStaticText{this, wxID_ANY, "Exec Working Dir"};
   _textCtrl_execWorkingDir = new wxTextCtrl{this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, wxTextValidator{wxFILTER_NONE, &_data._exeWorkingDir}};
   auto *dirPicker          = new wxDirPickerCtrl{this, wxID_ANY};
   dirPicker->Bind(wxEVT_DIRPICKER_CHANGED, [this](wxFileDirPickerEvent &e) {
      _textCtrl_execWorkingDir->Clear();
      _textCtrl_execWorkingDir->AppendText(e.GetPath());
   });
   dirPicker->SetPath(_data._exeWorkingDir);

   flexGridSizer->Add(tempStaticRoot, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);                      // column 0
   flexGridSizer->Add(_textCtrl_execWorkingDir, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 2); // column 1
   flexGridSizer->Add(dirPicker, 0, wxALL, 2);

   // row 6
   tempStaticRoot = new wxStaticText{this, wxID_ANY, "Remote Host"};
   _textCtrl_host = new wxTextCtrl{this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, wxTextValidator{wxFILTER_NONE, &_data._remoteHost}};

   flexGridSizer->Add(tempStaticRoot, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);            // column 0
   flexGridSizer->Add(_textCtrl_host, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 2); // column 1
   flexGridSizer->AddSpacer(0);                                                          // column 2

   // row 7
   tempStaticRoot = new wxStaticText{this, wxID_ANY, "Remote Port"};
   _textCtrl_port = new wxTextCtrl{this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator{(int *)&_data._remotePort}};

   flexGridSizer->Add(tempStaticRoot, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);            // column 0
   flexGridSizer->Add(_textCtrl_port, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 2); // column 1
   flexGridSizer->AddSpacer(0);                                                          // column 2

   // row 8
   tempStaticRoot          = new wxStaticText{this, wxID_ANY, "Remote Process to Attach"};
   _textCtrl_remoteProcess = new wxTextCtrl{this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, wxTextValidator{wxFILTER_NONE, &_data._remoteProcess}};

   flexGridSizer->Add(tempStaticRoot, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);                     // column 0
   flexGridSizer->Add(_textCtrl_remoteProcess, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 2); // column 1
   flexGridSizer->AddSpacer(0);                                                                   // column 2

   tb->Add(flexGridSizer, 0, wxEXPAND, 5);
   tb->Add(buttonSizer, 0, wxALL | wxALIGN_RIGHT, 5);

   auto *buttonOk = new wxButton(this, wxID_OK, "OK");
   buttonOk->SetDefault();
   buttonSizer->AddButton(buttonOk);
   buttonSizer->AddButton(new wxButton(this, wxID_CANCEL, "Cancel"));
   buttonSizer->Realize();

   Bind(
       wxEVT_BUTTON,
       [this](wxCommandEvent &e) {
          if (_textCtrl_sessionName->IsEmpty()) {
             wxMessageBox("Empty session name", "Session Name", wxOK);
             return;
          }
          if (_textCtrl_gdb->IsEmpty()) {
             wxMessageBox("Empty gdb program", "GDB Program Name", wxOK);
             return;
          }
          if (_textCtrl_execFile->IsEmpty()) {
             wxMessageBox("Empty exec file", "Exec Name", wxOK);
             return;
          }
          if (_textCtrl_host->IsEmpty()) {
             wxMessageBox("Empty Host", "Host", wxOK);
             return;
          }
          if (_textCtrl_port->IsEmpty()) {
             wxMessageBox("Empty Port", "Port", wxOK);
             return;
          }
          if (_textCtrl_remoteProcess->IsEmpty()) {
             wxMessageBox("Empty Remote Process", "Remote Process", wxOK);
             return;
          }
          e.Skip();
       },
       wxID_OK);
   SetSizerAndFit(tb);
}

auto RemoteAttachSessionDialog::TransferDataToWindow() -> bool {
   _textCtrl_sessionName->SetValue(_data._sessionName);
   _textCtrl_gdb->SetValue(_data._gdbExePath);
   _textCtrl_gdbArgs->SetValue(_data._gdbArgs);
   _textCtrl_execFile->SetValue(_data._exePath);
   _textCtrl_execArgs->SetValue(_data._exeArgs);
   _textCtrl_execWorkingDir->SetValue(_data._exeWorkingDir);
   _textCtrl_host->SetValue(_data._remoteHost);
   _textCtrl_port->SetValue(wxString::Format("%l", _data._remotePort));
   _textCtrl_remoteProcess->SetValue(_data._remoteProcess);
   return true;
}

auto RemoteAttachSessionDialog::TransferDataFromWindow() -> bool {
   _data._sessionType   = GdbSessionInfo::GDB_SESSION_TYPE_REMOTE;
   _data._sessionName   = _textCtrl_sessionName->GetValue();
   _data._gdbExePath    = _textCtrl_gdb->GetValue();
   _data._gdbArgs       = _textCtrl_gdbArgs->GetValue();
   _data._exePath       = _textCtrl_execFile->GetValue();
   _data._exeArgs       = _textCtrl_execArgs->GetValue();
   _data._exeWorkingDir = _textCtrl_execWorkingDir->GetValue();
   _data._remoteHost    = _textCtrl_host->GetValue();
   _textCtrl_port->GetValue().ToLong(&_data._remotePort);
   _data._remoteProcess = _textCtrl_remoteProcess->GetValue();
   return true;
}
