#pragma once

#include "cl_treelistctrl.h"
#include "wx/event.h"
#include "wx/treebase.h"
#include <string>
#include <utility>

class GdbMgr;
class MainFrame;

class BreakpointInfoUserData : public wxTreeItemData {
 public:
   BreakpointInfoUserData() = default;
   explicit BreakpointInfoUserData(std::string no, bool isMultiLeaf) : _no{std::move(no)}, _isMultiLeaf{isMultiLeaf} {}
   std::string _no;
   bool        _isMultiLeaf{};
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
   MainFrame      *_mainFrame             = nullptr;
   GdbMgr         *_gdbMgr                = nullptr;
};
