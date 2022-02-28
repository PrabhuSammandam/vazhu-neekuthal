#pragma once

#include "wx/panel.h"
#include "wx/listctrl.h"

class GdbMgr;
class MainFrame;

class StackListPanel : public wxPanel {
 public:
   explicit StackListPanel(wxWindow *parent, MainFrame *mainFrame, wxWindowID id = wxID_ANY);
   auto getListCtrl() -> wxListCtrl * { return _listCtrlStackList; }
   void clearWidget();

 private:
   void        createUI();
   wxListCtrl *_listCtrlStackList = nullptr;
   GdbMgr *    _gdbMgr            = nullptr;
   MainFrame * _mainFrame         = nullptr;
};
