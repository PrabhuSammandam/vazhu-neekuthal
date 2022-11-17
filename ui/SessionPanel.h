#pragma once

#include "wx/event.h"
#include "wx/panel.h"
#include "GdbSessionInfo.h"
#include "cl_treelistctrl.h"
#include "wx/string.h"
#include "wx/treebase.h"
#include <vector>

class wxListCtrl;
class wxWindow;
class MainFrame;

class SessionInfoUserData : public wxTreeItemData {
 public:
   SessionInfoUserData() : wxTreeItemData{} {}
   ~SessionInfoUserData() override = default;
   explicit SessionInfoUserData(GdbSessionInfo *sessionInfo, const wxString &filePath) : wxTreeItemData{}, _sessionInfo{sessionInfo}, _filePath{filePath} {}

   GdbSessionInfo *_sessionInfo = nullptr;
   wxString        _filePath    = "";
};

using SessionInfoList_t = std::vector<GdbSessionInfo *>;

class SessionPanel : public wxPanel {
 public:
   SessionPanel(wxWindow *parent, MainFrame *mainFrame, wxWindowID id = wxID_ANY);
   ~SessionPanel() override = default;

   auto getListCtrl() -> clTreeListCtrl * { return _treeCtrlSessions; }
   auto getSessionInfoList() -> SessionInfoList_t & { return _sessionInfoList; }
   void loadSessions();
   auto getSessionPath(const wxString &sessionName) -> wxString;
   auto getSessionList() -> SessionInfoList_t & { return _sessionInfoList; }
   auto getSessionObjByName(const wxString &sessionName) -> GdbSessionInfo *;

 private:
   void createUI();
   void connectEvents();

   void onTreeItemActivated(wxTreeEvent &e);
   void onTreeItemMenu(wxTreeEvent &e);
   void onOpenSession(wxCommandEvent &e);
   void onCloseSession(wxCommandEvent &e);
   void onEditSession(wxCommandEvent &e);
   void onDeleteSession(wxCommandEvent &e);

   clTreeListCtrl   *_treeCtrlSessions = nullptr;
   MainFrame        *_mainFrame        = nullptr;
   SessionInfoList_t _sessionInfoList;
};
