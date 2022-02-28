#pragma once

#include "GdbModels.h"
#include "cl_treelistctrl.h"
#include "wx/button.h"
#include "wx/clntdata.h"
#include "wx/event.h"
#include "wx/listctrl.h"
#include "wx/panel.h"
#include "wx/treebase.h"

class GdbMgr;
class MainFrame;

class BreakpointInfoUserData : public wxTreeItemData {
 public:
   BreakpointInfoUserData() : wxTreeItemData{} {}
   explicit BreakpointInfoUserData(BreakPoint *bp, int index) : wxTreeItemData{}, _bp{bp}, _index{index} {}

   BreakPoint *_bp    = nullptr;
   int         _index = -1;
};

class BreakPointsListPanel : public wxPanel {
 public:
   explicit BreakPointsListPanel(wxWindow *parent, MainFrame *mainFrame, wxWindowID id = wxID_ANY);
   auto getListCtrl() -> clTreeListCtrl * { return _treeWidgetBreakpoints; }
   void handleBreakPointChanged(bool isToSave = true);
   void clearWidget();

 private:
   void createUI();
   void onBreakpointItemActivate(wxTreeEvent &e);
   void onTreeItemMenu(wxTreeEvent &event);
   void onNewBreakpoint(wxCommandEvent &event);
   void onEnableBreakpoint(wxCommandEvent &event);
   void onDisableBreakpoint(wxCommandEvent &event);
   void onEnableAllBreakpoint(wxCommandEvent &event);
   void onDisableAllBreakpoint(wxCommandEvent &event);
   void onRemoveAllBreakpoint(wxCommandEvent &event);
   void onRemoveBreakpoint(wxCommandEvent &event);
   void saveBreakpoints();

   clTreeListCtrl *_treeWidgetBreakpoints = nullptr;
   MainFrame *     _mainFrame             = nullptr;
   GdbMgr *        _gdbMgr                = nullptr;
};
