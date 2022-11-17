#pragma once

#include "GdbModels.h"
#include "wx/event.h"
#include <wx/listctrl.h>
#include <wx/panel.h>

class wxSearchCtrl;

class SourceFilesPanel : public wxPanel {
 public:
   explicit SourceFilesPanel(wxWindow *parent, wxWindowID windowId = wxID_ANY);

   void setFiles(SourceFileList_t &sourceFilesList);
   void setSearchFocus();

 private:
   void createUI();
   void onSearchText(wxCommandEvent &textEvent);
   void onSearchKeyDown(wxKeyEvent &textEvent);
   void onListItemActivated(wxListEvent &listEvent);

   wxTextCtrl      *_searchCtrl = nullptr;
   wxListView      *_listView   = nullptr;
   SourceFileList_t _sourceFilesList;
};

wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_SOURCE_FILE_OPEN, wxCommandEvent);
