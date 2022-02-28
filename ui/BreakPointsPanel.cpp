#include "ui/BreakPointsPanel.h"
#include "CustomEvents.h"
#include "ID.h"
#include "MainFrame.h"
#include "cl_treelistctrl.h"
#include "wx/event.h"
#include "wx/filename.h"
#include "wx/gdicmn.h"
#include "wx/listbase.h"
#include "wx/log.h"
#include "wx/menu.h"
#include "wx/sizer.h"
#include "wx/treebase.h"
#include <vector>
#include "ui/NewSimpleBreakpointDialog.h"

BreakPointsListPanel::BreakPointsListPanel(wxWindow *parent, MainFrame *mainFrame, wxWindowID id) : wxPanel{parent, id}, _mainFrame{mainFrame} {
   _gdbMgr = (_mainFrame) != nullptr ? _mainFrame->getGdbMgr() : nullptr;
   createUI();

   _treeWidgetBreakpoints->Bind(wxEVT_TREE_ITEM_ACTIVATED, &BreakPointsListPanel::onBreakpointItemActivate, this);
   _treeWidgetBreakpoints->Bind(wxEVT_TREE_ITEM_MENU, &BreakPointsListPanel::onTreeItemMenu, this);
}

void BreakPointsListPanel::createUI() {
   auto *boxSizer = new wxBoxSizer{wxVERTICAL};

   _treeWidgetBreakpoints =
       new clTreeListCtrl{this, TREE_LIST_BREAKPOINTS, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxTR_COLUMN_LINES | wxTR_ROW_LINES | wxTR_FULL_ROW_HIGHLIGHT};

   boxSizer->Add(_treeWidgetBreakpoints, wxSizerFlags(2).Expand().Border());

   _treeWidgetBreakpoints->AddColumn("No", 50, wxALIGN_LEFT);
   _treeWidgetBreakpoints->AddColumn("Times", 50, wxALIGN_LEFT);
   _treeWidgetBreakpoints->AddColumn("Line", 60, wxALIGN_LEFT);
   _treeWidgetBreakpoints->AddColumn("Function", 300, wxALIGN_LEFT);
   _treeWidgetBreakpoints->AddColumn("Fullname", 1000, wxALIGN_LEFT);
   _treeWidgetBreakpoints->AddColumn("Address", 120, wxALIGN_LEFT);
   _treeWidgetBreakpoints->AddRoot("root");

   this->SetAutoLayout(true);
   this->SetSizer(boxSizer);
}

void BreakPointsListPanel::onBreakpointItemActivate(wxTreeEvent &e) {
   auto activatedItem = e.GetItem();

   if (activatedItem.IsOk()) {
      auto *itemData = dynamic_cast<BreakpointInfoUserData *>(_treeWidgetBreakpoints->GetItemData(activatedItem));

      if (itemData != nullptr) {
         auto *breakpointInfo = _gdbMgr->getBreakpointList()[itemData->_index];

         if (breakpointInfo != nullptr) {
            _mainFrame->openFile(breakpointInfo->m_fullname, breakpointInfo->m_lineNo);
         }
      }
   }
}

void BreakPointsListPanel::onTreeItemMenu(wxTreeEvent &event) {
   if (!_gdbMgr->isValidSession()) {
      return;
   }

   wxPoint      clientpt = event.GetPoint();
   wxMenu       menu;
   wxTreeItemId itemId = event.GetItem();

   menu.Append(TREE_LIST_BREAKPOINTS_CONTEXT_MENU_NEW_BREAKPOINT, "New Breakpoint");
   Bind(wxEVT_MENU, &BreakPointsListPanel::onNewBreakpoint, this, TREE_LIST_BREAKPOINTS_CONTEXT_MENU_NEW_BREAKPOINT);

   if (itemId.IsOk()) {
      auto *      itemData = dynamic_cast<BreakpointInfoUserData *>(_treeWidgetBreakpoints->GetItemData(itemId));
      BreakPoint *bpObj    = (itemData) != nullptr ? itemData->_bp : nullptr;

      menu.Append(TREE_LIST_BREAKPOINTS_CONTEXT_MENU_GOTO, "Go To");
      menu.Append(TREE_LIST_BREAKPOINTS_CONTEXT_MENU_ENABLE, "Enable");
      menu.Append(TREE_LIST_BREAKPOINTS_CONTEXT_MENU_ENABLE_ALL, "Enable All");
      menu.Append(TREE_LIST_BREAKPOINTS_CONTEXT_MENU_DISABLE, "Disable");
      menu.Append(TREE_LIST_BREAKPOINTS_CONTEXT_MENU_DISABLE_ALL, "Disable All");
      menu.Append(TREE_LIST_BREAKPOINTS_CONTEXT_MENU_DELETE, "Delete");
      menu.Append(TREE_LIST_BREAKPOINTS_CONTEXT_MENU_DELETE_ALL, "Delete All");

      Bind(
          wxEVT_MENU, [this, bpObj](wxCommandEvent & /*e*/) { _mainFrame->openFile(bpObj->m_fullname, bpObj->m_lineNo); }, TREE_LIST_BREAKPOINTS_CONTEXT_MENU_GOTO);
      Bind(wxEVT_MENU, &BreakPointsListPanel::onEnableBreakpoint, this, TREE_LIST_BREAKPOINTS_CONTEXT_MENU_ENABLE);
      Bind(wxEVT_MENU, &BreakPointsListPanel::onEnableAllBreakpoint, this, TREE_LIST_BREAKPOINTS_CONTEXT_MENU_ENABLE_ALL);
      Bind(wxEVT_MENU, &BreakPointsListPanel::onDisableBreakpoint, this, TREE_LIST_BREAKPOINTS_CONTEXT_MENU_DISABLE);
      Bind(wxEVT_MENU, &BreakPointsListPanel::onDisableAllBreakpoint, this, TREE_LIST_BREAKPOINTS_CONTEXT_MENU_DISABLE_ALL);
      Bind(wxEVT_MENU, &BreakPointsListPanel::onRemoveBreakpoint, this, TREE_LIST_BREAKPOINTS_CONTEXT_MENU_DELETE);
      Bind(wxEVT_MENU, &BreakPointsListPanel::onRemoveAllBreakpoint, this, TREE_LIST_BREAKPOINTS_CONTEXT_MENU_DELETE_ALL);
   } else {
   }
   PopupMenu(&menu, clientpt);
   event.Skip();
}

void BreakPointsListPanel::onNewBreakpoint(wxCommandEvent & /*event*/) {
   if (!_gdbMgr->isValidSession()) {
      return;
   }
   NewSimpleBreakpointDialog d{this};

   d.getData()._name = "";

   if (d.ShowModal() != wxID_OK) {
      return;
   }

   _gdbMgr->gdbSetBreakpoint(d.getData()._name, d.getData()._isTemp, d.getData()._isDisabled);
}

void BreakPointsListPanel::onEnableBreakpoint(wxCommandEvent & /*event*/) {
   wxArrayTreeItemIds        selectionArray;
   auto                      count = _treeWidgetBreakpoints->GetSelections(selectionArray);
   std::vector<BreakPoint *> bpList;

   for (auto i = 0; i < (int)count; i++) {
      auto item = selectionArray.Item(i);

      if (!item.IsOk()) {
         continue;
      }
      auto *itemData = dynamic_cast<BreakpointInfoUserData *>(_treeWidgetBreakpoints->GetItemData(item));

      if (itemData != nullptr) {
         bpList.push_back(itemData->_bp);
      }
   }
   _gdbMgr->gdbEnableBreakpoint(bpList);
   // saveBreakpoints();
}

void BreakPointsListPanel::onDisableBreakpoint(wxCommandEvent & /*event*/) {
   wxArrayTreeItemIds        selectionArray;
   auto                      count = _treeWidgetBreakpoints->GetSelections(selectionArray);
   std::vector<BreakPoint *> bpList;

   for (auto i = 0; i < (int)count; i++) {
      auto item = selectionArray.Item(i);

      if (!item.IsOk()) {
         continue;
      }
      auto *itemData = dynamic_cast<BreakpointInfoUserData *>(_treeWidgetBreakpoints->GetItemData(item));

      if (itemData != nullptr) {
         bpList.push_back(itemData->_bp);
      }
   }
   _gdbMgr->gdbDisableBreakpoint(bpList);

   // saveBreakpoints();
}

void BreakPointsListPanel::onEnableAllBreakpoint(wxCommandEvent & /*event*/) {
   _gdbMgr->gdbEnableAllBreakpoints();
   // saveBreakpoints();
}

void BreakPointsListPanel::onDisableAllBreakpoint(wxCommandEvent & /*event*/) {
   _gdbMgr->gdbDisableAllBreakpoints();
   // saveBreakpoints();
}

void BreakPointsListPanel::onRemoveAllBreakpoint(wxCommandEvent & /*event*/) {
   _gdbMgr->gdbRemoveAllBreakpoints();
   // saveBreakpoints();
}

void BreakPointsListPanel::onRemoveBreakpoint(wxCommandEvent & /*event*/) {
   auto selectedItem = _treeWidgetBreakpoints->GetSelection();
   if (!selectedItem.IsOk()) {
      return;
   }
   auto *itemData = dynamic_cast<BreakpointInfoUserData *>(_treeWidgetBreakpoints->GetItemData(selectedItem));
   if (itemData != nullptr) {
      _gdbMgr->gdbRemoveBreakpoint(itemData->_bp);
      // saveBreakpoints();
   }
}

void BreakPointsListPanel::handleBreakPointChanged(bool isToSave) {
   auto breakpointList = _gdbMgr->getBreakpointList();
   _treeWidgetBreakpoints->DeleteRoot();
   _treeWidgetBreakpoints->AddRoot("root");

   auto rootItem = _treeWidgetBreakpoints->GetRootItem();
   int  count    = 0;

   for (auto *bpObj : breakpointList) {
      auto newTreeItem = _treeWidgetBreakpoints->AppendItem(rootItem, wxString::Format("%d", bpObj->m_number) /*, -1, -1, new BreakpointInfoUserData{bpObj, count++}*/);
      _treeWidgetBreakpoints->SetItemText(newTreeItem, 1, wxString::Format("%d", bpObj->_times));
      _treeWidgetBreakpoints->SetItemText(newTreeItem, 2, wxString::Format("%d", bpObj->m_lineNo));
      _treeWidgetBreakpoints->SetItemText(newTreeItem, 3, bpObj->m_funcName);
      _treeWidgetBreakpoints->SetItemText(newTreeItem, 4, bpObj->m_fullname);
      _treeWidgetBreakpoints->SetItemText(newTreeItem, 5, wxString::Format("0x%lx", bpObj->m_addr));
      _treeWidgetBreakpoints->SetItemData(newTreeItem, new BreakpointInfoUserData{bpObj, count++});
      _treeWidgetBreakpoints->SetItemTextColour(newTreeItem, bpObj->_isEnabled ? *wxBLACK : *wxStockGDI::GetColour(wxStockGDI::COLOUR_LIGHTGREY));
   }

   if (isToSave) {
      saveBreakpoints();
   }
}

void BreakPointsListPanel::saveBreakpoints() {
   auto         sessionInfo = _gdbMgr->_sessionInfo;
   wxFileConfig fc{wxEmptyString, wxEmptyString, sessionInfo._sessionFullPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE};
   auto         breakPointList = _gdbMgr->getBreakpointList();

   fc.DeleteGroup("/breakpoints");
   fc.SetPath("/breakpoints");
   fc.Write("count", breakPointList.size());

   for (auto i = 0; i < breakPointList.size(); i++) {
      auto *bpObj  = breakPointList.at(i);
      auto  bpPath = wxString::Format("bp%d", i);
      fc.SetPath("/breakpoints/" + bpPath);
      fc.Write("lineNo", bpObj->m_lineNo);
      fc.Write("file", bpObj->m_fullname);
      fc.Write("enabled", bpObj->_isEnabled);
      fc.Write("temp", bpObj->_isTemp);
   }
   fc.Flush();
}
