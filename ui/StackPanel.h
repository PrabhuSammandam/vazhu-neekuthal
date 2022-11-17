#pragma once

#include <wx/listctrl.h>
#include <wx/panel.h>
#include "GdbModels.h"

class GdbMgr;
class MainFrame;

class StackListPanel : public wxPanel {
 public:
   explicit StackListPanel(wxWindow *parent, MainFrame *mainFrame, wxWindowID id = wxID_ANY);
   auto getListCtrl() -> wxListCtrl * { return _listCtrlStackList; }
   void clearWidget();
   void setStackFramesList(StackFramesList_t &framesList);

 private:
   void        createUI();
   wxListCtrl *_listCtrlStackList = nullptr;
   GdbMgr     *_gdbMgr            = nullptr;
   MainFrame  *_mainFrame         = nullptr;
};
