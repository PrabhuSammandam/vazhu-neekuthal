#include "MainFrame.h"
#include "ui/BreakPointsPanel.h"
#include "CustomEvents.h"
#include "GdbMgr.h"
#include "GdbSessionInfo.h"
#include "ID.h"
#include "ui/LocalVariablesPanel.h"
#include "ui/StackPanel.h"
#include "jaeCppTextDocument.h"
#include "ui/ExecSessionDialog.h"
#include "ui/RemoteAttachSessionDialog.h"
#include "ui/SessionPanel.h"
#include "wx/accel.h"
#include "wx/app.h"
#include "wx/datetime.h"
#include "wx/file.h"
#include "wx/filefn.h"
#include "wx/filename.h"
#include "wx/process.h"
#include "wx/stdpaths.h"
#include "wx/string.h"
#include "wx/utils.h"
#include "wx/wfstream.h"
#include <iostream>
#include <ostream>
#include <wx/aui/aui.h>
#include <wx/aui/auibook.h>
#include <wx/aui/framemanager.h>
#include <wx/dir.h>
#include <wx/event.h>
#include <wx/fileconf.h>
#include <wx/gtk/frame.h>
#include <wx/gtk/menu.h>
#include <wx/gtk/window.h>
#include <wx/msgdlg.h>
#include <wx/stc/stc.h>
#include <wx/stdpaths.h>

void MainFrame::createSessionMenu(wxMenuBar *menuBar) {
   auto *sessionMenu = new wxMenu{};

   auto *newSubMenu = new wxMenu{};
   newSubMenu->Append(MENU_SESSION_ID_NEW_EXEC, _("&Exec"));
   Bind(wxEVT_MENU, &MainFrame::onSessionNewExec, this, MENU_SESSION_ID_NEW_EXEC);

   newSubMenu->Append(MENU_SESSION_ID_NEW_LOCAL_ATTACH, _("&Attach"));
   Bind(wxEVT_MENU, &MainFrame::onSessionNewLocalAtach, this, MENU_SESSION_ID_NEW_LOCAL_ATTACH);

   newSubMenu->Append(MENU_SESSION_ID_NEW_COREDUMP, _("&Coredump"));
   Bind(wxEVT_MENU, &MainFrame::onSessionNewCoredump, this, MENU_SESSION_ID_NEW_COREDUMP);

   newSubMenu->Append(MENU_SESSION_ID_NEW_REMOTE_ATTACH, _("&Remote Attach"));
   Bind(wxEVT_MENU, &MainFrame::onSessionNewRemoteAttach, this, MENU_SESSION_ID_NEW_REMOTE_ATTACH);

   sessionMenu->AppendSubMenu(newSubMenu, _("&New"));

   sessionMenu->Append(MENU_SESSION_ID_OPEN, _("&Open"));
   Bind(wxEVT_MENU, &MainFrame::onSessionOpen, this, MENU_SESSION_ID_OPEN);

   auto *tempMenuItem = sessionMenu->Append(MENU_SESSION_ID_CLOSE, _("&Close"));
   Bind(wxEVT_MENU, &MainFrame::onSessionClose, this, MENU_SESSION_ID_CLOSE);
   tempMenuItem->Enable(false);

   sessionMenu->AppendSeparator();
   sessionMenu->Append(wxID_EXIT, _("E&xit"));
   this->Bind(
       wxEVT_MENU, [this](wxCommandEvent & /*unused*/) -> void { this->Close(); }, wxID_EXIT);

   menuBar->Append(sessionMenu, _("&Session"));
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

void MainFrame::onSessionNewExec(wxCommandEvent & /*e*/) {
   ExecSessionDialog dlg{this};
   auto              data = dlg.getData();

   if (dlg.ShowModal() == wxID_OK) {
      saveSession(dlg.getData(), false);
      _sessionPanel->loadSessions();
   }
}

void MainFrame::onSessionNewLocalAtach(wxCommandEvent &e) {}

void MainFrame::onSessionNewCoredump(wxCommandEvent &e) {}

void MainFrame::onSessionNewRemoteAttach(wxCommandEvent &e) {
   RemoteAttachSessionDialog dlg{this};

   if (dlg.ShowModal() == wxID_OK) {
      saveSession(dlg.getData(), false);
      _sessionPanel->loadSessions();
   }
}

void MainFrame::onSessionOpen(wxCommandEvent & /*e*/) {
   auto *treeListSessions    = _sessionPanel->getListCtrl();
   auto  selectedSessionItem = treeListSessions->GetSelection();

   if (selectedSessionItem.IsOk() == false) {
      wxMessageBox("Select the session in the Session Panel", "Session", wxOK);
      return;
   }

   auto *itemData = dynamic_cast<SessionInfoUserData *>(treeListSessions->GetItemData(selectedSessionItem));

   if (itemData != nullptr) {
      openSession(itemData->_sessionInfo);
   }
}

void MainFrame::onSessionClose(wxCommandEvent & /*e*/) {
   GdbSessionInfo sessionInfo;
   closeSession(&sessionInfo);
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

void MainFrame::openSession(GdbSessionInfo *sessionInfoObj) {
   GdbSessionInfo sessionInfo = *sessionInfoObj;
   sessionInfo._gdbArgs       = " --interpreter=mi3 " + sessionInfo._gdbArgs;

   if (sessionInfo._sessionType == GdbSessionInfo::GDB_SESSION_TYPE_EXE_RUN) {
      _gdbMgr->initExeSession(sessionInfo);
   } else if (sessionInfo._sessionType == GdbSessionInfo::GDB_SESSION_TYPE_COREDUMP) {
      _gdbMgr->initCoredumpSession(sessionInfo);
   } else if (sessionInfo._sessionType == GdbSessionInfo::GDB_SESSION_TYPE_ATTACH_PID) {
      _gdbMgr->initAttachPidSession(sessionInfo);
   } else if (sessionInfo._sessionType == GdbSessionInfo::GDB_SESSION_TYPE_REMOTE) {
      _gdbMgr->initRemoteSession(sessionInfo);
   }

   setMenuEnabled(MENU_SESSION_ID_CLOSE, true);
   setMenuEnabled(MENU_DEBUG_ID_RUN, true);

   SetTitle("Vazhu-Neekuthal - " + sessionInfo._exePath);
}

void MainFrame::runSession(GdbSessionInfo *sessionInfoObj) {
   openSession(sessionInfoObj);
   _gdbMgr->gdbRun();
}

void MainFrame::closeSession(GdbSessionInfo * /*sessionInfoObj*/) {
   if (!_gdbMgr->_sessionInfo._isValid) {
      return;
   }
   _gdbMgr->quit();

   setMenuEnabled(MENU_SESSION_ID_CLOSE, false);
}

void MainFrame::editSession(GdbSessionInfo *sessionInfoObj) {
   GdbSessionInfo newSessionInfo;

   if (sessionInfoObj->_sessionType == GdbSessionInfo::GDB_SESSION_TYPE_EXE_RUN) {
      ExecSessionDialog dlg{this};
      dlg.setData(*sessionInfoObj);

      if (dlg.ShowModal() != wxID_OK) {
         return;
      }
      newSessionInfo = dlg.getData();
   } else if (sessionInfoObj->_sessionType == GdbSessionInfo::GDB_SESSION_TYPE_REMOTE) {
      RemoteAttachSessionDialog dlg{this};
      dlg.setData(*sessionInfoObj);

      if (dlg.ShowModal() != wxID_OK) {
         return;
      }
      newSessionInfo = dlg.getData();
   }

   auto userDataDir = wxStandardPaths::Get().GetUserDataDir();

   if (newSessionInfo._sessionName != sessionInfoObj->_sessionName) {
      auto sessionFileName = userDataDir + "/" + sessionInfoObj->_sessionName + ".vz";
      wxRemoveFile(sessionFileName);
   }
   auto  sessionFileName = userDataDir + "/" + newSessionInfo._sessionName + ".vz";
   auto *fc              = new wxFileConfig{wxEmptyString, wxEmptyString, sessionFileName, wxEmptyString, wxCONFIG_USE_LOCAL_FILE};

   newSessionInfo.saveToConfig(*fc);
   fc->Flush();
   delete fc;

   _sessionPanel->loadSessions();
}

void MainFrame::deleteSession(GdbSessionInfo *sessionInfoObj) {}

void MainFrame::saveSession(GdbSessionInfo &data, bool isEdited) {
   auto       userDataDir     = wxStandardPaths::Get().GetUserDataDir();
   auto       sessionFileName = userDataDir + "/" + data._sessionName + ".vz";
   wxFileName fn{sessionFileName};

   if (fn.Exists()) {
      wxMessageBox("Session name already exists", "Session", wxOK);
      return;
   }

   auto *fc = new wxFileConfig{wxEmptyString, wxEmptyString, fn.GetFullPath(), wxEmptyString, wxCONFIG_USE_LOCAL_FILE};

   data.saveToConfig(*fc);

   delete fc;
}
