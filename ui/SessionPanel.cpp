#include "SessionPanel.h"
#include "ID.h"
#include "MainFrame.h"
#include "wx/arrstr.h"
#include "wx/filefn.h"
#include "wx/filename.h"
#include "wx/gdicmn.h"
#include "wx/string.h"
#include "wx/treebase.h"
#include <ostream>
#include <vector>
#include <wx/dir.h>
#include <wx/fileconf.h>
#include <wx/listctrl.h>
#include <wx/stdpaths.h>

SessionPanel::SessionPanel(wxWindow *parent, MainFrame *mainFrame, wxWindowID id) : wxPanel{parent, id}, _mainFrame{mainFrame} {
   createUI();
   connectEvents();
}

void SessionPanel::createUI() {
   auto *boxSizer = new wxBoxSizer{wxVERTICAL};

   _treeCtrlSessions =
       new clTreeListCtrl{this, TREE_LIST_BREAKPOINTS, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxTR_COLUMN_LINES | wxTR_ROW_LINES | wxTR_FULL_ROW_HIGHLIGHT};
   _treeCtrlSessions->SetFont(wxFont{"Noto Sans Mono"});

   boxSizer->Add(_treeCtrlSessions, wxSizerFlags(2).Expand().Border());

   _treeCtrlSessions->AddColumn("Name", 200, wxLIST_FORMAT_LEFT);
   _treeCtrlSessions->AddColumn("Value", 1000, wxLIST_FORMAT_LEFT);
   _treeCtrlSessions->AddRoot("root");

   this->SetAutoLayout(true);
   this->SetSizer(boxSizer);
}

void SessionPanel::connectEvents() {
   _treeCtrlSessions->Bind(wxEVT_TREE_ITEM_ACTIVATED, &SessionPanel::onTreeItemActivated, this);
   _treeCtrlSessions->Bind(wxEVT_TREE_ITEM_MENU, &SessionPanel::onTreeItemMenu, this);
}

void SessionPanel::onTreeItemActivated(wxTreeEvent &e) {
   auto selectedItem = e.GetItem();

   auto *itemData = dynamic_cast<SessionInfoUserData *>(_treeCtrlSessions->GetItemData(selectedItem));
   if (itemData != nullptr) {
      _mainFrame->runSession(itemData->_sessionInfo);
   } else {
      e.Skip(false);
   }
}

void SessionPanel::onTreeItemMenu(wxTreeEvent &event) {
   wxTreeItemId itemId = event.GetItem();

   if (!itemId.IsOk()) {
      return;
   }

   wxPoint clientpt = event.GetPoint();
   wxMenu  menu;

   menu.Append(TREE_LIST_SESSIONS_OPEN, "Open");
   Bind(wxEVT_MENU, &SessionPanel::onOpenSession, this, TREE_LIST_SESSIONS_OPEN);

   menu.Append(TREE_LIST_SESSIONS_CLOSE, "Close");
   Bind(wxEVT_MENU, &SessionPanel::onCloseSession, this, TREE_LIST_SESSIONS_CLOSE);

   menu.Append(TREE_LIST_SESSIONS_EDIT, "Edit");
   Bind(wxEVT_MENU, &SessionPanel::onEditSession, this, TREE_LIST_SESSIONS_EDIT);

   menu.Append(TREE_LIST_SESSIONS_DELETE, "Delete");
   Bind(wxEVT_MENU, &SessionPanel::onDeleteSession, this, TREE_LIST_SESSIONS_DELETE);

   PopupMenu(&menu, clientpt);
   event.Skip();
}

void SessionPanel::onOpenSession(wxCommandEvent & /*e*/) {
   auto            itemId         = _treeCtrlSessions->GetSelection();
   auto           *itemData       = dynamic_cast<SessionInfoUserData *>(_treeCtrlSessions->GetItemData(itemId));
   GdbSessionInfo *sessionInfoObj = (itemData) != nullptr ? itemData->_sessionInfo : nullptr;

   _mainFrame->openSession(sessionInfoObj);
}

void SessionPanel::onCloseSession(wxCommandEvent & /*e*/) {
   auto            itemId         = _treeCtrlSessions->GetSelection();
   auto           *itemData       = dynamic_cast<SessionInfoUserData *>(_treeCtrlSessions->GetItemData(itemId));
   GdbSessionInfo *sessionInfoObj = (itemData) != nullptr ? itemData->_sessionInfo : nullptr;

   _mainFrame->closeSession(sessionInfoObj);
}

void SessionPanel::onEditSession(wxCommandEvent & /*e*/) {
   auto            itemId         = _treeCtrlSessions->GetSelection();
   auto           *itemData       = dynamic_cast<SessionInfoUserData *>(_treeCtrlSessions->GetItemData(itemId));
   GdbSessionInfo *sessionInfoObj = (itemData) != nullptr ? itemData->_sessionInfo : nullptr;

   _mainFrame->editSession(sessionInfoObj);
}

void SessionPanel::onDeleteSession(wxCommandEvent & /*e*/) {
   auto  itemId   = _treeCtrlSessions->GetSelection();
   auto *itemData = dynamic_cast<SessionInfoUserData *>(_treeCtrlSessions->GetItemData(itemId));

   wxRemoveFile(itemData->_filePath);
   loadSessions();
}

void SessionPanel::loadSessions() {
   auto userDataDir = wxStandardPaths::Get().GetUserDataDir();

   if (!wxDir::Exists(userDataDir)) {
      wxDir::Make(userDataDir);
   }

   auto rootItem = _treeCtrlSessions->GetRootItem();

   _treeCtrlSessions->DeleteChildren(rootItem);

   for (auto *si : _sessionInfoList) {
      delete si;
   }
   _sessionInfoList.clear();

   wxArrayString sessionFileList;

   wxDir::GetAllFiles(userDataDir, &sessionFileList, "*.vz");

   auto execRootItem         = _treeCtrlSessions->AppendItem(rootItem, "Local Executables");
   auto coredumpRootItem     = _treeCtrlSessions->AppendItem(rootItem, "Core Dumps");
   auto remoteAttachRootItem = _treeCtrlSessions->AppendItem(rootItem, "Remote Attach");

   for (const auto &file : sessionFileList) {
      wxFileName fn{file};
      auto       name = wxFileName::StripExtension(fn.GetName());

      wxFileConfig fc{wxEmptyString, wxEmptyString, fn.GetFullPath(), wxEmptyString, wxCONFIG_USE_LOCAL_FILE};
      auto        *sessionInfoObj      = new GdbSessionInfo{};
      sessionInfoObj->_sessionFullPath = fn.GetFullPath();

      sessionInfoObj->loadFromConfig(fc);
      _sessionInfoList.push_back(sessionInfoObj);

      wxTreeItemId tempRootItem;
      switch (sessionInfoObj->_sessionType) {
      case GdbSessionInfo::GDB_SESSION_TYPE_EXE_RUN: {
      case GdbSessionInfo::GDB_SESSION_TYPE_ATTACH_PID:
         tempRootItem = execRootItem;
      } break;
      case GdbSessionInfo::GDB_SESSION_TYPE_COREDUMP: {
         tempRootItem = coredumpRootItem;
      } break;
      case GdbSessionInfo::GDB_SESSION_TYPE_REMOTE: {
         tempRootItem = remoteAttachRootItem;
      } break;
      }

      auto newItem = _treeCtrlSessions->AppendItem(tempRootItem, name, -1, -1, new SessionInfoUserData{sessionInfoObj, fn.GetFullPath()});

      auto tempItem = _treeCtrlSessions->AppendItem(newItem, "Gdb");
      _treeCtrlSessions->SetItemText(tempItem, 1, sessionInfoObj->_gdbExePath);

      tempItem = _treeCtrlSessions->AppendItem(newItem, "Gdb Arguments");
      _treeCtrlSessions->SetItemText(tempItem, 1, sessionInfoObj->_gdbArgs);

      if (sessionInfoObj->_sessionType == GdbSessionInfo::GDB_SESSION_TYPE_EXE_RUN || sessionInfoObj->_sessionType == GdbSessionInfo::GDB_SESSION_TYPE_REMOTE) {

         tempItem = _treeCtrlSessions->AppendItem(newItem, "Exec File");
         _treeCtrlSessions->SetItemText(tempItem, 1, sessionInfoObj->_exePath);

         tempItem = _treeCtrlSessions->AppendItem(newItem, "Exec Args");
         _treeCtrlSessions->SetItemText(tempItem, 1, sessionInfoObj->_exeArgs);

         tempItem = _treeCtrlSessions->AppendItem(newItem, "Exec Working Dir");
         _treeCtrlSessions->SetItemText(tempItem, 1, sessionInfoObj->_exeWorkingDir);
      }

      if (sessionInfoObj->_sessionType == GdbSessionInfo::GDB_SESSION_TYPE_REMOTE) {
         tempItem = _treeCtrlSessions->AppendItem(newItem, "Remote Host");
         _treeCtrlSessions->SetItemText(tempItem, 1, sessionInfoObj->_remoteHost);

         tempItem  = _treeCtrlSessions->AppendItem(newItem, "Remote Port");
         auto port = wxString::Format("%d", (int)sessionInfoObj->_remotePort);
         _treeCtrlSessions->SetItemText(tempItem, 1, port);

         tempItem = _treeCtrlSessions->AppendItem(newItem, "Remote Process");
         _treeCtrlSessions->SetItemText(tempItem, 1, sessionInfoObj->_remoteProcess);
      }

      if (sessionInfoObj->_sessionType == GdbSessionInfo::GDB_SESSION_TYPE_EXE_RUN) {
         tempItem = _treeCtrlSessions->AppendItem(newItem, "Break At main");
         _treeCtrlSessions->SetItemText(tempItem, 1, sessionInfoObj->_isBreakAtMain ? "YES" : "NO");
      }
   }

   _treeCtrlSessions->Expand(execRootItem);
   _treeCtrlSessions->Expand(coredumpRootItem);
   _treeCtrlSessions->Expand(remoteAttachRootItem);
}

auto SessionPanel::getSessionPath(const wxString &sessionName) -> wxString {
   for (auto *session : _sessionInfoList) {
      if (session->_sessionName == sessionName) {
         return session->_sessionFullPath;
      }
   }
   return "";
}

auto SessionPanel::getSessionObjByName(const wxString &sessionName) -> GdbSessionInfo * {
   for (auto *session : _sessionInfoList) {
      if (session->_sessionName == sessionName) {
         return session;
      }
   }
   return nullptr;
}
