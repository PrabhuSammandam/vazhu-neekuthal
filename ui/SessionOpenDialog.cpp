#include "SessionOpenDialog.h"
#include <wx/sizer.h>
#include <wx/app.h>

wxDEFINE_EVENT(wxEVT_OPEN_SESSION, wxCommandEvent);

SessionOpenDialog::SessionOpenDialog(wxWindow *parent, SessionInfoList_t &sessionInfoList)
    : wxDialog{parent, -1, "Open Session", wxDefaultPosition, wxSize{800, 400}}, _sessionInfoList{sessionInfoList} {
   createUI();
   _searchCtrl->Bind(wxEVT_TEXT, &SessionOpenDialog::onSearchText, this);
   _searchCtrl->Bind(wxEVT_CHAR, &SessionOpenDialog::onSearchKeyDown, this);
   _listViewSessions->Bind(wxEVT_LIST_ITEM_ACTIVATED, &SessionOpenDialog::onListItemActivated, this);
   _listViewSessions->AppendColumn("");
   _listViewSessions->AppendColumn("");
   _listViewSessions->SetColumnWidth(0, 200);
   _listViewSessions->SetColumnWidth(1, 600);
   _listViewSessions->EnableAlternateRowColours(true);

   for (auto *session : sessionInfoList) {
      auto index = _listViewSessions->InsertItem(_listViewSessions->GetItemCount(), session->_sessionName);
      _listViewSessions->SetItem(index, 1, session->_sessionFullPath);
   }
}

void SessionOpenDialog::createUI() {
   auto *boxSizer = new wxBoxSizer{wxVERTICAL};

   _searchCtrl       = new wxTextCtrl{this, -1};
   _listViewSessions = new wxListView{this, -1, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_NO_HEADER | wxLC_VRULES | wxLC_HRULES};

   boxSizer->Add(_searchCtrl, wxSizerFlags().Expand());
   boxSizer->Add(_listViewSessions, wxSizerFlags(2).Expand());

   SetSizer(boxSizer);
}

void SessionOpenDialog::onSearchText(wxCommandEvent &textEvent) {
   auto searchText = textEvent.GetString().Lower();
   _listViewSessions->DeleteAllItems();

   if (searchText.empty()) {
      for (auto *session : _sessionInfoList) {
         auto index = _listViewSessions->InsertItem(_listViewSessions->GetItemCount(), session->_sessionName);
         _listViewSessions->SetItem(index, 1, session->_sessionFullPath);
      }
   } else {
      for (auto *session : _sessionInfoList) {
         if (session->_sessionName.rfind(searchText, 0) == std::string::npos) {
            continue;
         }
         auto index = _listViewSessions->InsertItem(_listViewSessions->GetItemCount(), session->_sessionName);
         _listViewSessions->SetItem(index, 1, session->_sessionFullPath);
      }
   }
}

void SessionOpenDialog::onSearchKeyDown(wxKeyEvent &keyEvent) { //   std::cout << "selected item = " << textEvent.GetKeyCode() << " , key up = " << WXK_DOWN << std::endl;
   if (keyEvent.GetKeyCode() != WXK_UP) {
      if (keyEvent.GetKeyCode() != WXK_DOWN) {
         if (keyEvent.GetKeyCode() != WXK_RETURN) {
            keyEvent.Skip();
            return;
         }
      }
   }
   keyEvent.Skip(false);
   // std::cout << "list processing " << textEvent.GetKeyCode() << std::endl;

   if (_listViewSessions->GetItemCount() == 0) {
      return;
   }

   int lastItemIndex = _listViewSessions->GetItemCount() - 1;
   int selectedItem  = _listViewSessions->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

   if (keyEvent.GetKeyCode() == WXK_RETURN) {
      if (selectedItem != -1) {
         _sessionName = _listViewSessions->GetItemText(selectedItem, 0);
         EndModal(wxID_OK);
      }
      return;
   }

   int index = 0;
   if (keyEvent.GetKeyCode() == WXK_UP) {
      if (selectedItem == -1 || selectedItem == 0) {
         index = lastItemIndex;
      } else {
         index = selectedItem - 1;
      }
   } else if (keyEvent.GetKeyCode() == WXK_DOWN) {
      if (selectedItem == -1 || selectedItem == lastItemIndex) {
         index = 0;
      } else {
         index = selectedItem + 1;
      }
   }
   _listViewSessions->Select(index);
   _listViewSessions->EnsureVisible(index);
}

void SessionOpenDialog::onListItemActivated(wxListEvent &listEvent) {
   auto index   = listEvent.GetItem().GetId();
   _sessionName = _listViewSessions->GetItemText(index, 0);
   EndModal(wxID_OK);
}
