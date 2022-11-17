#pragma once

#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include "ui/SessionPanel.h"

class wxListView;

class SessionOpenDialog : public wxDialog {
 public:
   explicit SessionOpenDialog(wxWindow *parent, SessionInfoList_t &sessionInfoList);

   auto getSessionName() { return _sessionName; }

 private:
   void createUI();
   void onSearchText(wxCommandEvent &textEvent);
   void onSearchKeyDown(wxKeyEvent &textEvent);
   void onListItemActivated(wxListEvent &listEvent);

   wxListView        *_listViewSessions = nullptr;
   wxTextCtrl        *_searchCtrl       = nullptr;
   SessionInfoList_t &_sessionInfoList;
   wxString           _sessionName;
};
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_OPEN_SESSION, wxCommandEvent);
