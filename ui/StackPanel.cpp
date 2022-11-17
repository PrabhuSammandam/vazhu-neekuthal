#include "wx/listbase.h"
#include "GdbMgr.h"
#include "ID.h"
#include "MainFrame.h"
#include "StackPanel.h"
#include "wx/sizer.h"

StackListPanel::StackListPanel(wxWindow *parent, MainFrame *mainFrame, wxWindowID id) : wxPanel{parent, id}, _mainFrame{mainFrame} {
   _gdbMgr = (_mainFrame) != nullptr ? _mainFrame->getGdbMgr() : nullptr;
   createUI();
}

void StackListPanel::createUI() {
   auto *boxSizer = new wxBoxSizer{wxVERTICAL};

   _listCtrlStackList = new wxListCtrl{this, LIST_STACK_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES};
   _listCtrlStackList->SetFont(wxFont{"Noto Sans Mono"});

   boxSizer->Add(_listCtrlStackList, wxSizerFlags(2).Expand().Border());

   _listCtrlStackList->InsertColumn(0, "Level", wxLIST_FORMAT_LEFT, 40);
   _listCtrlStackList->InsertColumn(1, "Line", wxLIST_FORMAT_LEFT, 60);
   _listCtrlStackList->InsertColumn(2, "Function", wxLIST_FORMAT_LEFT, 300);
   _listCtrlStackList->InsertColumn(3, "File", wxLIST_FORMAT_LEFT, 600);

   this->SetAutoLayout(true);
   this->SetSizer(boxSizer);

   Bind(
       wxEVT_LIST_ITEM_SELECTED,
       [this](wxListEvent &e) {
          const auto &item     = e.GetItem();
          auto        itemData = item.GetData();
          _gdbMgr->gdbSelectFrame((int)itemData);
          //          std::cout << "Stack level " << (long)itemData << std::endl;
       },
       LIST_STACK_LIST);
}

void StackListPanel::clearWidget() { _listCtrlStackList->DeleteAllItems(); }

void StackListPanel::setStackFramesList(StackFramesList_t &framesList) {
   _listCtrlStackList->DeleteAllItems();

   for (const auto &item : framesList) {
      auto row = _listCtrlStackList->InsertItem(_listCtrlStackList->GetItemCount(), wxString::Format("%d", item._level));
      _listCtrlStackList->SetItem(row, 1, wxString::Format("%d", item.m_line));
      _listCtrlStackList->SetItem(row, 2, item.m_functionName);
      _listCtrlStackList->SetItem(row, 3, item.m_sourcePath);
      _listCtrlStackList->SetItemData(row, item._level);
   }
}
