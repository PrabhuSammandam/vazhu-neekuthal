#pragma once

#include "GdbMgr.h"
#include "cl_treelistctrl.h"
#include "wx/arrstr.h"
#include "wx/clntdata.h"
#include "wx/panel.h"
#include "wx/string.h"
#include "wx/treebase.h"
#include "wx/treelist.h"
#include <map>

class GdbMgr;

enum DispFormat {
   DISP_NATIVE = 0,
   DISP_DEC,
   DISP_BIN,
   DISP_HEX,
   DISP_CHAR,
};

using DispInfo = struct DispInfo {
   DispFormat _displayFormat = DispFormat::DISP_NATIVE;
   wxString   _lastData      = "";
   bool       _isExpanded    = false;
};

class LocalVariableTreeItemData : public wxTreeItemData {
 public:
   LocalVariableTreeItemData() : wxTreeItemData{} {}
   explicit LocalVariableTreeItemData(const wxString &watchId, bool isPlaceHolder = false) : wxTreeItemData{}, _watchId{watchId}, _isPlaceHolder{isPlaceHolder} {}

   wxString _watchId;
   bool     _isPlaceHolder = false;
};

using DispInfoMap = std::map<wxString, DispInfo>;

class LocalVariablesPanel : public wxPanel {
 public:
   explicit LocalVariablesPanel(wxWindow *parent, MainFrame *mainFrame, wxWindowID id = wxID_ANY);

   auto getTreeListCtrl() -> clTreeListCtrl * { return _treeListCtrl; }
   auto getTreeItemPath(wxTreeItemId treeItem) -> wxString;
   auto getVarWatchObjForItem(wxTreeItemId treeItem) -> VarWatch *;
   auto getVarWatchIdForItem(wxTreeItemId treeItem) -> wxString;
   auto getTreeItemForWatchId(const wxString &watchId) -> wxTreeItemId;
   auto createTreeItemForWatchId(VarWatch *varWatchObj) -> wxTreeItemId;
   auto addItem(VarWatch *varWatchObj) -> bool;
   auto setTreeItemValues(wxTreeItemId treeItem, VarWatch *varWatchObj) -> bool;
   void findOrAddIfNotFoundVariablePath(const wxString &variablePath);
   void onExpand(wxTreeEvent &e);
   void onCollapse(wxTreeEvent &e);
   void onEditEnd(wxTreeEvent &e);
   void handleLocalVariableChanged(wxArrayString &localVarList, const wxString &fileName, const wxString &functionName, int stackDepth);
   void handleWatchChanged(VarWatch *varWatchObj);
   void clearWidget() { _treeListCtrl->DeleteChildren(_treeListCtrl->GetRootItem()); }

 private:
   void createUI();
   void refillVariables();

   clTreeListCtrl *_treeListCtrl    = nullptr;
   GdbMgr         *_gdbMgr          = nullptr;
   MainFrame      *_mainFrame       = nullptr;
   bool            _isExpandAllowed = true;
   int             _stackDepth      = -1;
   wxString        _fileName;
   wxString        _functionName;
   DispInfoMap     _displayInfoMap;
};
