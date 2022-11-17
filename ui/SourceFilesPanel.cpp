#include "SourceFilesPanel.h"
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/srchctrl.h>
#include <wx/filename.h>

wxDEFINE_EVENT(wxEVT_SOURCE_FILE_OPEN, wxCommandEvent);

SourceFilesPanel::SourceFilesPanel(wxWindow *parent, wxWindowID windowId) : wxPanel{parent, windowId} {
   createUI();
   _searchCtrl->Bind(wxEVT_TEXT, &SourceFilesPanel::onSearchText, this);
   _searchCtrl->Bind(wxEVT_CHAR, &SourceFilesPanel::onSearchKeyDown, this);
   _listView->Bind(wxEVT_LIST_ITEM_ACTIVATED, &SourceFilesPanel::onListItemActivated, this);
}

void SourceFilesPanel::createUI() {
   auto *boxSizer = new wxBoxSizer{wxVERTICAL};

   _searchCtrl = new wxTextCtrl{this, -1};
   _listView   = new wxListView{this, -1, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL};
   _listView->InsertColumn(0, "Name");
   _listView->InsertColumn(1, "FullName");
   _listView->SetColumnWidth(0, 200);
   _listView->SetColumnWidth(1, 1000);

   boxSizer->Add(_searchCtrl, wxSizerFlags().Expand());
   boxSizer->Add(_listView, wxSizerFlags(2).Expand());

   this->SetAutoLayout(true);
   this->SetSizer(boxSizer);
}

void SourceFilesPanel::onSearchText(wxCommandEvent &textEvent) {
   auto searchText = textEvent.GetString().Lower();
   _listView->DeleteAllItems();

   if (searchText.empty()) {
      for (auto *sourceFile : _sourceFilesList) {
         if (!sourceFile->_isValid) {
            continue;
         }
         long index = _listView->InsertItem(_listView->GetItemCount(), sourceFile->_fileName);
         _listView->SetItem(index, 1, sourceFile->_fullName);
      }
   } else {
      for (auto *sourceFile : _sourceFilesList) {
         if (!sourceFile->_isValid || sourceFile->_fileName.rfind(searchText, 0) == std::string::npos) {
            continue;
         }
         long index = _listView->InsertItem(_listView->GetItemCount(), sourceFile->_fileName);
         _listView->SetItem(index, 1, sourceFile->_fullName);
      }
   }

   std::cout << "Search text = " << textEvent.GetString() << std::endl;
}

void SourceFilesPanel::setFiles(SourceFileList_t &sourceFilesList) {
   _listView->DeleteAllItems();

   std::for_each(_sourceFilesList.begin(), _sourceFilesList.end(), [](auto *sourceFile) { delete sourceFile; });
   _sourceFilesList.clear();

   _sourceFilesList = std::move(sourceFilesList);

   for (auto *sourceFile : _sourceFilesList) {

      if (!wxFileExists(sourceFile->_fullName)) {
         continue;
      }

      wxFileName fileName{sourceFile->_fullName};

      sourceFile->_isValid  = true;
      sourceFile->_fileName = fileName.GetFullName().Lower();

      long index = _listView->InsertItem(_listView->GetItemCount(), sourceFile->_fileName);

      _listView->SetItem(index, 1, sourceFile->_fullName);
   }
}

void SourceFilesPanel::onListItemActivated(wxListEvent &listEvent) {
   auto index = listEvent.GetItem().GetId();

   //   std::cout << "selected item = " << _listView->GetItemText(index, 1) << std::endl;

   wxCommandEvent event{wxEVT_SOURCE_FILE_OPEN};

   event.SetString(_listView->GetItemText(index, 1));
   GetEventHandler()->ProcessEvent(event);
   _searchCtrl->Clear();
}
void SourceFilesPanel::setSearchFocus() { _searchCtrl->SetFocus(); }

void SourceFilesPanel::onSearchKeyDown(wxKeyEvent &keyEvent) {
   //   std::cout << "selected item = " << textEvent.GetKeyCode() << " , key up = " << WXK_DOWN << std::endl;
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

   if (_listView->GetItemCount() == 0) {
      return;
   }

   int lastItemIndex = _listView->GetItemCount() - 1;
   int selectedItem  = _listView->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

   if (keyEvent.GetKeyCode() == WXK_RETURN) {
      if (selectedItem != -1) {
         wxCommandEvent event{wxEVT_SOURCE_FILE_OPEN};

         event.SetString(_listView->GetItemText(selectedItem, 1));
         GetEventHandler()->ProcessEvent(event);
         _searchCtrl->Clear();
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
   _listView->Select(index);
   _listView->EnsureVisible(index);
}
