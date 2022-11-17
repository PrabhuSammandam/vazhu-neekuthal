#include "GdbModels.h"
#include "GdbMgr.h"
#include "ID.h"
#include "MainFrame.h"
#include "cl_treelistctrl.h"
#include "ui/LocalVariablesPanel.h"
#include "wx/gdicmn.h"
#include "wx/gtk/colour.h"
#include "wx/gtk/font.h"
#include "wx/sizer.h"
#include "wx/string.h"
#include "wx/tokenzr.h"
#include "wx/treebase.h"
#include <iostream>
#include <ostream>
#include <string>

LocalVariablesPanel::LocalVariablesPanel(wxWindow *parent, MainFrame *mainFrame, wxWindowID id) : wxPanel{parent, id}, _mainFrame{mainFrame} {
   if (_mainFrame != nullptr) {
      _gdbMgr = _mainFrame->getGdbMgr();
   }
   createUI();
}

void LocalVariablesPanel::createUI() {
   auto *boxSizer = new wxBoxSizer{wxVERTICAL};
   int   flags    = wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_TWIST_BUTTONS | wxTR_HIDE_ROOT;

   flags |= wxTR_COLUMN_LINES | wxTR_ROW_LINES | wxTR_FULL_ROW_HIGHLIGHT | wxTR_EDIT_LABELS;

   _treeListCtrl = new clTreeListCtrl{this, TREE_LIST_LOCAL_VARIABLES, wxDefaultPosition, wxDefaultSize, flags};
   _treeListCtrl->SetFont(wxFont{"Noto Sans Mono"});

   boxSizer->Add(_treeListCtrl, wxSizerFlags(2).Expand().Border());

   _treeListCtrl->AddColumn("Name", 200);
   _treeListCtrl->AddColumn("Value", 200);
   _treeListCtrl->AddColumn("Type", 400);

   _treeListCtrl->SetColumnEditable(1, true);

   _treeListCtrl->AddRoot("root");

   this->SetAutoLayout(true);
   this->SetSizer(boxSizer);

   Bind(wxEVT_COMMAND_TREE_ITEM_EXPANDED, &LocalVariablesPanel::onExpand, this, TREE_LIST_LOCAL_VARIABLES);
   Bind(wxEVT_COMMAND_TREE_ITEM_COLLAPSED, &LocalVariablesPanel::onCollapse, this, TREE_LIST_LOCAL_VARIABLES);
   Bind(wxEVT_COMMAND_TREE_END_LABEL_EDIT, &LocalVariablesPanel::onEditEnd, this, TREE_LIST_LOCAL_VARIABLES);
}

auto LocalVariablesPanel::getTreeItemPath(wxTreeItemId treeItem) -> wxString {
   auto parentItem = _treeListCtrl->GetItemParent(treeItem);

   if (parentItem.IsOk()) {
      return getTreeItemPath(parentItem) + "/" + _treeListCtrl->GetItemText(treeItem, 0);
   }
   return _treeListCtrl->GetItemText(treeItem, 0);
}

auto LocalVariablesPanel::getTreeItemForWatchId(const wxString &watchId) -> wxTreeItemId {
   if (watchId.IsEmpty()) {
      return wxTreeItemId{};
   }

   auto              watchidPartList = wxStringTokenize(watchId, ".");
   wxTreeItemIdValue t               = nullptr;
   wxTreeItemId      loopItem        = _treeListCtrl->GetFirstChild(_treeListCtrl->GetRootItem(), t);
   wxTreeItemId      prevItem;
   wxString          loopWatchId;

   if (!loopItem.IsOk()) {
      return wxTreeItemId{};
   }

   for (int i = 0; i < watchidPartList.size(); i++) {
      loopWatchId += (((i == 0) ? "" : ".") + watchidPartList[i]);
      bool found = false;

      for (; loopItem.IsOk(); loopItem = _treeListCtrl->GetNextSibling(loopItem)) {
         auto varWatchId = getVarWatchIdForItem(loopItem);

         if (varWatchId == loopWatchId) {
            found = true;

            if ((i + 1) == watchidPartList.size()) {
               return loopItem;
            }

            loopItem = _treeListCtrl->GetFirstChild(loopItem, t);
            break;
         }
      }
      if (!found) {
         return wxTreeItemId{};
      }
   }
   return loopItem;
}

auto LocalVariablesPanel::createTreeItemForWatchId(VarWatch *varWatchObj) -> wxTreeItemId {
   auto         rootItem        = _treeListCtrl->GetRootItem();
   auto         watchidPartList = wxStringTokenize(varWatchObj->getWatchId(), ".");
   wxString     loopWatchId;
   wxTreeItemId tempTreeItem;

   for (int i = 0; i < watchidPartList.size(); i++) {
      loopWatchId += (((i == 0) ? "" : ".") + watchidPartList[i]);
      wxTreeItemId foundItem = getTreeItemForWatchId(loopWatchId);

      if (!foundItem.IsOk()) {
         tempTreeItem = _treeListCtrl->AppendItem(rootItem, varWatchObj->getName());
         _treeListCtrl->SetItemText(tempTreeItem, 1, "");
         _treeListCtrl->SetItemText(tempTreeItem, 2, varWatchObj->getVarType());
         _treeListCtrl->SetItemData(tempTreeItem, new LocalVariableTreeItemData{loopWatchId});
         _treeListCtrl->SetItemHasChildren(tempTreeItem, (varWatchObj->hasChildren()));

         rootItem = tempTreeItem;
      } else {
         tempTreeItem = foundItem;
         rootItem     = foundItem;
      }
   }
   return tempTreeItem;
}

auto LocalVariablesPanel::addItem(VarWatch *varWatchObj) -> bool {
   if (!getTreeItemForWatchId(varWatchObj->getWatchId()).IsOk()) {
      auto tempTreeItem = createTreeItemForWatchId(varWatchObj);
      setTreeItemValues(tempTreeItem, varWatchObj);
   }
   return true;
}

auto LocalVariablesPanel::getVarWatchObjForItem(wxTreeItemId treeItem) -> VarWatch * {
   auto *itemData = dynamic_cast<LocalVariableTreeItemData *>(_treeListCtrl->GetItemData(treeItem));

   if (itemData != nullptr) {
      return _gdbMgr->getVarWatchInfo(itemData->_watchId);
   }
   return nullptr;
}

auto LocalVariablesPanel::getVarWatchIdForItem(wxTreeItemId treeItem) -> wxString {
   auto *itemData = dynamic_cast<LocalVariableTreeItemData *>(_treeListCtrl->GetItemData(treeItem));

   if (itemData != nullptr) {
      return itemData->_watchId;
   }
   return "";
}

void LocalVariablesPanel::onExpand(wxTreeEvent &e) {
   auto expandedItem = e.GetItem();

   if (!expandedItem.IsOk()) {
      return;
   }

   auto variablePath = getTreeItemPath(expandedItem);

   if (_displayInfoMap.find(variablePath) != _displayInfoMap.end()) {
      _displayInfoMap[variablePath]._isExpanded = true;
   }
   auto                watchId = getVarWatchIdForItem(expandedItem);
   VariableWatchList_t varList;
   _gdbMgr->gdbVarListChildren(watchId.ToStdString(), varList);

   for (auto *lv : varList) {
#if 1
      if (lv->getName() == "private" || lv->getName() == "protected" || lv->getName() == "public") {
         VariableWatchList_t contextVarList;

         _gdbMgr->gdbVarListChildren(lv->getWatchId().ToStdString(), contextVarList);

         for (auto *contextLocalVar : contextVarList) {
            auto newItem = _treeListCtrl->AppendItem(expandedItem, contextLocalVar->getName());
            _treeListCtrl->SetItemData(newItem, new LocalVariableTreeItemData{contextLocalVar->getWatchId()});

            setTreeItemValues(newItem, contextLocalVar);
         }
      } else {
#endif
         auto newItem = _treeListCtrl->AppendItem(expandedItem, lv->getName());
         _treeListCtrl->SetItemData(newItem, new LocalVariableTreeItemData{lv->getWatchId()});

         setTreeItemValues(newItem, lv);
      }
   }
}

void LocalVariablesPanel::onCollapse(wxTreeEvent &e) {
   auto variablePath = getTreeItemPath(e.GetItem());

   if (_displayInfoMap.find(variablePath) != _displayInfoMap.end()) {
      _displayInfoMap[variablePath]._isExpanded = false;
   }
   auto *varWatchObj = getVarWatchObjForItem(e.GetItem());
   if (varWatchObj != nullptr) {
      _gdbMgr->gdbRemoveVarWatch(varWatchObj->getWatchId(), true);
   }
   refillVariables();
}

void LocalVariablesPanel::onEditEnd(wxTreeEvent &e) {
   auto item = e.GetItem();

   if (!item.IsOk()) {
      return;
   }

   auto *varWatchObj = getVarWatchObjForItem(item);

   if (varWatchObj != nullptr) {
      auto oldValue     = varWatchObj->getValue();
      auto variablePath = getTreeItemPath(item);

      const auto &newValue = e.GetLabel();

      if (newValue != oldValue) {
         _gdbMgr->gdbChangeWatchVariable(varWatchObj->getWatchId(), newValue);
      }
   }
}

auto LocalVariablesPanel::setTreeItemValues(wxTreeItemId treeItem, VarWatch *varWatchObj) -> bool {
   auto variablePath = getTreeItemPath(treeItem);
   findOrAddIfNotFoundVariablePath(variablePath);

   _treeListCtrl->SetItemHasChildren(treeItem, varWatchObj->hasChildren() || varWatchObj->isDynamic());
   _treeListCtrl->SetItemText(treeItem, 1, varWatchObj->getValue());
   _treeListCtrl->SetItemText(treeItem, 2, varWatchObj->getVarType());
   _treeListCtrl->SetItemTextColour(treeItem, (_displayInfoMap[variablePath]._lastData != varWatchObj->getValue()) ? *wxRED : *wxBLACK);
   _displayInfoMap[variablePath]._lastData = varWatchObj->getValue();

   if (_displayInfoMap[variablePath]._isExpanded) {
      _treeListCtrl->Expand(treeItem);
   }
   return true;
}

void LocalVariablesPanel::handleLocalVariableChanged(wxArrayString & /*unused*/, const wxString & /*unused*/, const wxString & /*unused*/, int /*unused*/) { refillVariables(); }

void LocalVariablesPanel::handleWatchChanged(VarWatch *varWatchObj) {
   auto changedWatchId = varWatchObj->getWatchId();
   auto treeItem       = getTreeItemForWatchId(varWatchObj->getWatchId());

   if (treeItem.IsOk()) {
      if (varWatchObj->inScope()) {
         auto newValue = varWatchObj->getValue();
         _treeListCtrl->SetItemText(treeItem, 1, newValue);
      } else {
         _treeListCtrl->Delete(treeItem);
         _gdbMgr->gdbRemoveVarWatch(varWatchObj->getWatchId());
      }
   }
}

void LocalVariablesPanel::findOrAddIfNotFoundVariablePath(const wxString &variablePath) {
   if (_displayInfoMap.find(variablePath) == _displayInfoMap.end()) {
      DispInfo newDispInfo;
      _displayInfoMap[variablePath] = newDispInfo;
   }
}

void LocalVariablesPanel::refillVariables() {
   _treeListCtrl->DeleteRoot();
   _treeListCtrl->AddRoot("");

   for (auto *lv : _gdbMgr->getLocalVariableList()) {
      auto newItem = _treeListCtrl->AppendItem(_treeListCtrl->GetRootItem(), lv->getName());
      _treeListCtrl->SetItemData(newItem, new LocalVariableTreeItemData{lv->getWatchId()});

      setTreeItemValues(newItem, lv);
   }
}
