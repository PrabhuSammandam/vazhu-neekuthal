#pragma once

#include "GdbParser.h"
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
   void addNewWatch(const wxString &variableName);
   auto getTreeItemPath(wxTreeItemId treeItem) -> wxString;
   auto getVarWatchObjForItem(wxTreeItemId treeItem) -> VarWatch *;
   auto getVarWatchIdForItem(wxTreeItemId treeItem) -> wxString;
   auto getTreeItemForWatchId(const wxString &watchId) -> wxTreeItemId;
   auto getDisplayString(const wxString &watchId) -> wxString;
   void findOrAddIfNotFoundVariablePath(const wxString &variablePath);
   void onExpand(wxTreeEvent &e);
   void onCollapse(wxTreeEvent &e);
   void onEditEnd(wxTreeEvent &e);
   void handleLocalVariableChanged(wxArrayString &localVarList, const wxString &fileName, const wxString &functionName, int stackDepth);
   void handleWatchChanged(VarWatch *varWatchObj);
   void CMD_handleVariableWatchChildAdded(VarWatch *varWatch);
   void CMD_handleVariableWatchAdd(GdbResponse *resp, DebugCmdHandler *debugCmdAddVariableWatch);
   void clear();

 private:
   void createUI();

   clTreeListCtrl *_treeListCtrl    = nullptr;
   GdbMgr *        _gdbMgr          = nullptr;
   MainFrame *     _mainFrame       = nullptr;
   bool            _isExpandAllowed = true;
   int             _stackDepth      = -1;
   wxString        _fileName;
   wxString        _functionName;
   DispInfoMap     _displayInfoMap;
   friend class DebugCmdAddVariableWatch;
};

class DebugCmdAddVariableWatch : public DebugCmdHandler {
 public:
   DebugCmdAddVariableWatch(GdbMgr *gdbMgr, const wxString &watchId, const wxString &variableName, LocalVariablesPanel *localVariablesPanel);
   ~DebugCmdAddVariableWatch() override                       = default;
   DebugCmdAddVariableWatch(DebugCmdAddVariableWatch &other)  = delete;
   DebugCmdAddVariableWatch(DebugCmdAddVariableWatch &&other) = delete;
   auto operator=(DebugCmdAddVariableWatch &&other) -> DebugCmdAddVariableWatch & = delete;
   auto operator=(DebugCmdAddVariableWatch &other) -> DebugCmdAddVariableWatch & = delete;

   auto processResponse(GdbResponse *response) -> bool override;

 private:
   LocalVariablesPanel *_localVariablesPanel = nullptr;
   wxString             _watchId;
   wxString             _variableName;
   friend class LocalVariablesPanel;
};

class DebugCmdExpandVariableWatch : public DebugCmdHandler {
 public:
   DebugCmdExpandVariableWatch(GdbMgr *gdbMgr, const wxString &watchId, LocalVariablesPanel *localVariablesPanel);
   ~DebugCmdExpandVariableWatch() override                          = default;
   DebugCmdExpandVariableWatch(DebugCmdExpandVariableWatch &other)  = delete;
   DebugCmdExpandVariableWatch(DebugCmdExpandVariableWatch &&other) = delete;
   auto operator=(DebugCmdExpandVariableWatch &&other) -> DebugCmdExpandVariableWatch & = delete;
   auto operator=(DebugCmdExpandVariableWatch &other) -> DebugCmdExpandVariableWatch & = delete;

   auto processResponse(GdbResponse *response) -> bool override;

 private:
   LocalVariablesPanel *_localVariablesPanel = nullptr;
   wxString             _watchId;
   friend class LocalVariablesPanel;
};
