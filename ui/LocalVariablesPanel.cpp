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
#include "wx/treelist.h"
#include <iostream>
#include <ostream>

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
   auto              tempRootItem    = _treeListCtrl->GetRootItem();
   wxTreeItemId      loopItem;
   wxTreeItemIdValue t = nullptr;

   for (const auto &loopWatchId : watchidPartList) {
      bool found = false;

      for (loopItem = _treeListCtrl->GetFirstChild(tempRootItem, t); loopItem.IsOk(); loopItem = _treeListCtrl->GetNextSibling(loopItem)) {
         if (getVarWatchIdForItem(loopItem) == loopWatchId) {
            found = true;
            break;
         }
      }
      if (!found) {
         return wxTreeItemId{};
      }
   }
   return loopItem;
}

auto LocalVariablesPanel::getVarWatchObjForItem(wxTreeItemId treeItem) -> VarWatch * {
   VarWatch *varWatchObj = nullptr;
   auto *    itemData    = dynamic_cast<LocalVariableTreeItemData *>(_treeListCtrl->GetItemData(treeItem));

   if (itemData != nullptr) {
      varWatchObj = _gdbMgr->getVarWatchInfo(itemData->_watchId);
   }
   return varWatchObj;
}

auto LocalVariablesPanel::getVarWatchIdForItem(wxTreeItemId treeItem) -> wxString {
   auto *varWatchObj = getVarWatchObjForItem(treeItem);
   return (varWatchObj != nullptr) ? varWatchObj->getWatchId() : "";
}

void LocalVariablesPanel::onExpand(wxTreeEvent &e) {
   auto expandedItem = e.GetItem();

   if (expandedItem.IsOk() == false) {
      return;
   }

   auto variablePath = getTreeItemPath(expandedItem);

   if (_displayInfoMap.find(variablePath) != _displayInfoMap.end()) {
      _displayInfoMap[variablePath]._isExpanded = true;
   }
   auto watchId = getVarWatchIdForItem(expandedItem);
   _gdbMgr->writeCommand(wxString::Format("-var-list-children --all-values %s", watchId), new DebugCmdExpandVariableWatch{_gdbMgr, watchId, this});
}

void LocalVariablesPanel::onCollapse(wxTreeEvent &e) {
   auto variablePath = getTreeItemPath(e.GetItem());

   if (_displayInfoMap.find(variablePath) != _displayInfoMap.end()) {
      _displayInfoMap[variablePath]._isExpanded = false;
   }
   auto *varWatchObj = getVarWatchObjForItem(e.GetItem());
   if (varWatchObj != nullptr) {
      _gdbMgr->gdbRemoveVarWatch(varWatchObj->getWatchId());
   }
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

void LocalVariablesPanel::addNewWatch(const wxString &variableName) {
   if (variableName.IsEmpty() == false) {
      wxString watchId = wxString::Format("w%d", _gdbMgr->getNextWatchId());
      _gdbMgr->writeCommand(wxString::Format("-var-create %s @ %s", watchId, variableName), new DebugCmdAddVariableWatch{_gdbMgr, watchId, variableName, this});
   }
}

void LocalVariablesPanel::clear() {
   wxTreeItemIdValue t = nullptr;

   for (auto item = _treeListCtrl->GetFirstChild(_treeListCtrl->GetRootItem(), t); item.IsOk(); item = _treeListCtrl->GetNextSibling(item)) {
      _gdbMgr->gdbRemoveVarWatch(getVarWatchIdForItem(item));
   }
   _treeListCtrl->DeleteRoot();
   _treeListCtrl->AddRoot("");
}

void LocalVariablesPanel::handleLocalVariableChanged(wxArrayString &localVarList, const wxString &fileName, const wxString &functionName, int stackDepth) {
   clear();

   if (!(_fileName == fileName && _functionName == functionName && _stackDepth == stackDepth)) {
      _fileName     = fileName;
      _functionName = functionName;
      _stackDepth   = stackDepth;
      //      std::cout << "*********************** clearing the stack cache ************************************" << std::endl;
   }

   for (const auto &variableName : localVarList) {
      addNewWatch(variableName);
   }
}

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

void LocalVariablesPanel::CMD_handleVariableWatchAdd(GdbResponse *resp, DebugCmdHandler *debugCmdAddVariableWatch) {
   auto *debugCmdObj = dynamic_cast<DebugCmdAddVariableWatch *>(debugCmdAddVariableWatch);

   auto     rootTreeItem     = _treeListCtrl->GetRootItem();
   wxString watchId          = debugCmdObj->_watchId;
   wxString variableName     = debugCmdObj->_variableName;
   wxString variableValue    = resp->tree.getString("value");
   wxString variableType     = resp->tree.getString("type");
   int      numChild         = resp->tree.getInt("numchild", 0);
   int      isDynamic        = resp->tree.getInt("dynamic", 0);
   auto *   newVariableWatch = new VarWatch{watchId, variableName, variableValue, variableType, numChild > 0};

   _gdbMgr->m_watchList.push_back(newVariableWatch);

   auto newVarTreeItem = _treeListCtrl->AppendItem(rootTreeItem, variableName, wxTreeListCtrl::NO_IMAGE, wxTreeListCtrl::NO_IMAGE, new LocalVariableTreeItemData{watchId});
   //   auto value          = newVariableWatch->getValue();
   auto variablePath = getTreeItemPath(newVarTreeItem);

   findOrAddIfNotFoundVariablePath(variablePath);

   // std::cout << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& isDynamic " << isDynamic << std::endl;

   if (isDynamic != 0 || numChild > 0) {
      _treeListCtrl->SetItemHasChildren(newVarTreeItem, true);
   }

   _treeListCtrl->SetItemText(newVarTreeItem, 1, variableValue);
   _treeListCtrl->SetItemText(newVarTreeItem, 2, variableType);

   _treeListCtrl->SetItemTextColour(newVarTreeItem, (_displayInfoMap[variablePath]._lastData != variableValue) ? *wxRED : *wxBLACK);
   _displayInfoMap[variablePath]._lastData = variableValue;

   if (_displayInfoMap[variablePath]._isExpanded) {
      _isExpandAllowed = false;
      _treeListCtrl->Expand(newVarTreeItem);
      _isExpandAllowed = true;
   }
}

void LocalVariablesPanel::CMD_handleVariableWatchChildAdded(VarWatch *varWatchObj) {
   wxTreeItemIdValue t           = nullptr;
   auto              rootItem    = _treeListCtrl->GetRootItem();
   auto              name        = varWatchObj->getName();
   auto              varType     = varWatchObj->getVarType();
   auto              hasChildren = varWatchObj->hasChildren();
   //   auto              inScope     = varWatchObj->inScope();
   auto     tokens = wxStringTokenize(varWatchObj->getWatchId(), ".");
   wxString thisWatchId;

   for (auto partIndex = 0; partIndex < (int)tokens.size(); partIndex++) {
      if (!thisWatchId.empty()) {
         thisWatchId += ".";
      }
      thisWatchId += tokens[partIndex];

      wxTreeItemId foundItem;
      bool         found = false;
      for (auto item = _treeListCtrl->GetFirstChild(rootItem, t); item.IsOk(); item = _treeListCtrl->GetNextSibling(item)) {
         auto *itemData = dynamic_cast<LocalVariableTreeItemData *>(_treeListCtrl->GetItemData(item));

         if (itemData != nullptr) {
            if (itemData->_watchId == thisWatchId) {
               foundItem = item;
               found     = true;
               break;
            }
         }
      }

      wxTreeItemId tempTreeItem;

      if (!found) {
         tempTreeItem = _treeListCtrl->AppendItem(rootItem, name);
         _treeListCtrl->SetItemText(tempTreeItem, 1, "");
         _treeListCtrl->SetItemText(tempTreeItem, 2, varType);
         _treeListCtrl->SetItemData(tempTreeItem, new LocalVariableTreeItemData{thisWatchId});

         //       std::cout << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& isDynamic " << varWatchObj->isDynamic() << std::endl;
         if (hasChildren || varWatchObj->isDynamic()) {
            _treeListCtrl->SetItemHasChildren(tempTreeItem, true);
         }

         rootItem = tempTreeItem;
      } else {
         tempTreeItem = foundItem;
         rootItem     = foundItem;
      }

      if ((partIndex + 1) == (int)tokens.size()) {
         auto variablePath = getTreeItemPath(tempTreeItem);
         findOrAddIfNotFoundVariablePath(variablePath);

         auto valueString = varWatchObj->getOriginalVariabeValue(); // varWatchObj->getValue();

         _treeListCtrl->SetItemText(tempTreeItem, 1, valueString);
         _treeListCtrl->SetItemTextColour(tempTreeItem, (_displayInfoMap[variablePath]._lastData != valueString) ? *wxRED : *wxBLACK);
         _displayInfoMap[variablePath]._lastData = valueString;

         if (_displayInfoMap[variablePath]._isExpanded && hasChildren) {
            _treeListCtrl->Expand(tempTreeItem);
         }
      }
   }
   _treeListCtrl->Refresh(true);
}

auto LocalVariablesPanel::getDisplayString(const wxString &watchId) -> wxString {
   VarWatch *variableWatchObj = (watchId.IsEmpty()) ? nullptr : _gdbMgr->getVarWatchInfo(watchId);
   wxString  displayValue     = (variableWatchObj != nullptr) ? variableWatchObj->getValue() : "";
   return displayValue;
}

void LocalVariablesPanel::findOrAddIfNotFoundVariablePath(const wxString &variablePath) {
   if (_displayInfoMap.find(variablePath) == _displayInfoMap.end()) {
      DispInfo newDispInfo;
      _displayInfoMap[variablePath] = newDispInfo;
   }
}

/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/
DebugCmdAddVariableWatch::DebugCmdAddVariableWatch(GdbMgr *gdbMgr, const wxString &watchId, const wxString &variableName, LocalVariablesPanel *localVariablesPanel)
    : DebugCmdHandler{gdbMgr}, _localVariablesPanel{localVariablesPanel}, _watchId{watchId}, _variableName{variableName} {}

auto DebugCmdAddVariableWatch::processResponse(GdbResponse *response) -> bool {
   _localVariablesPanel->CMD_handleVariableWatchAdd(response, this);
   return true;
}

/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/

DebugCmdExpandVariableWatch::DebugCmdExpandVariableWatch(GdbMgr *gdbMgr, const wxString &watchId, LocalVariablesPanel *localVariablesPanel)
    : DebugCmdHandler{gdbMgr}, _localVariablesPanel{localVariablesPanel}, _watchId{watchId} {}

auto DebugCmdExpandVariableWatch::processResponse(GdbResponse *response) -> bool {

   if (_gdbMgr->getVarWatchInfo(_watchId) == nullptr) {
      return true;
   }

   auto &    tree = response->tree;
   TreeNode *root = tree.findChild("children");

   if (root != nullptr) {
      for (int i = 0; i < root->getChildCount(); i++) {
         TreeNode *child        = root->getChild(i);
         wxString  childWatchId = child->getChildDataString("name");
         VarWatch *varWatchObj  = _gdbMgr->getVarWatchInfo(childWatchId);

         if (varWatchObj == nullptr) {
            wxString childExp   = child->getChildDataString("exp");
            wxString childValue = child->getChildDataString("value");
            wxString childType  = child->getChildDataString("type");
            int      numChild   = child->getChildDataInt("numchild", 0);
            int      isDynamic  = child->getChildDataInt("dynamic", 0);

            varWatchObj = new VarWatch(childWatchId, childExp, childValue, childType, numChild > 0);
            varWatchObj->setInScope(true);
            varWatchObj->setParentWatchId(_watchId);
            varWatchObj->setIsDynamic(isDynamic != 0);
            _gdbMgr->m_watchList.push_back(varWatchObj);
         }
         _localVariablesPanel->CMD_handleVariableWatchChildAdded(varWatchObj);
      }
   }
   return true;
}
