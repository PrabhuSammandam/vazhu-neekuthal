#include "MainFrame.h"
#include "CustomEvents.h"
#include "GdbMgr.h"
#include "GdbSessionInfo.h"
#include "ID.h"
#include "jaeCppTextDocument.h"
#include "ui/BreakPointsPanel.h"
#include "ui/ExecSessionDialog.h"
#include "ui/LocalVariablesPanel.h"
#include "ui/RemoteAttachSessionDialog.h"
#include "ui/SessionPanel.h"
#include "ui/StackPanel.h"
#include "wx/accel.h"
#include "wx/app.h"
#include "wx/datetime.h"
#include "wx/file.h"
#include "wx/filefn.h"
#include "wx/filename.h"
#include "wx/process.h"
#include "wx/stdpaths.h"
#include "wx/string.h"
#include "wx/utils.h"
#include "wx/wfstream.h"
#include <iostream>
#include <ostream>
#include <wx/aui/aui.h>
#include <wx/aui/auibook.h>
#include <wx/aui/auibook.h>
#include <wx/aui/framemanager.h>
#include <wx/dir.h>
#include <wx/event.h>
#include <wx/fileconf.h>
#include <wx/gtk/frame.h>
#include <wx/gtk/menu.h>
#include <wx/gtk/window.h>
#include <wx/msgdlg.h>
#include <wx/stc/stc.h>
#include <wx/stdpaths.h>

#define MAX_FILES_OPENED 50

MainFrame::MainFrame() : wxFrame{nullptr, JA_ID_FRAME_MAIN_WINDOW, "Vazhu-Neekuthal"} {
   _gdbMgr                  = new GdbMgr{};
   _aui_mgr                 = new wxAuiManager{this};
   _aui_notebook            = new wxAuiNotebook{this};
   _bottomLeftNotebook      = new wxAuiNotebook{this};
   _bottomRightNotebook     = new wxAuiNotebook{this};
   _localVariablesTreePanel = new LocalVariablesPanel{this, this};

   _gdbMgr->SetNextHandler(this);

   _aui_mgr->AddPane(_aui_notebook, wxAuiPaneInfo().Name("notebook").CenterPane().PaneBorder(false));
   _aui_mgr->AddPane(_localVariablesTreePanel, wxAuiPaneInfo().Name("LocalVariables").Right().Position(0).PaneBorder(false).MinSize(500, 1200).Caption("Local Variables").LeftDockable(true).Layer(1));

   createMenuBar();
   createToolBar();
   createStatusBar();
   createBottomPanels();

   _aui_mgr->Update();

   auto               noOfEntries = 6;
   wxAcceleratorEntry entries[noOfEntries];
   entries[0].Set(wxACCEL_NORMAL, WXK_F5, MENU_DEBUG_ID_CONTINUE);
   entries[1].Set(wxACCEL_NORMAL, WXK_F6, MENU_DEBUG_ID_NEXT);
   entries[2].Set(wxACCEL_NORMAL, WXK_F7, MENU_DEBUG_ID_STEP_IN);
   entries[3].Set(wxACCEL_NORMAL, WXK_F8, MENU_DEBUG_ID_STEP_OUT);
   entries[4].Set(wxACCEL_ALT, (int)'0', MENU_ITEM_VIEW_SESSIONS);
   entries[5].Set(wxACCEL_ALT, (int)'1', MENU_ITEM_VIEW_BREAKPOINTS);
   // entries[2].Set(wxACCEL_SHIFT, (int)'A', ID_ABOUT);
   // entries[3].Set(wxACCEL_NORMAL, WXK_DELETE, wxID_CUT);
   wxAcceleratorTable accel(noOfEntries, entries);
   this->SetAcceleratorTable(accel);

   Bind(wxEVT_DEBUGGER_GDB_PROCESS_ENDED, &MainFrame::onDebuggerStopped, this);
   Bind(wxEVT_DEBUGGER_GDB_LOCAL_VARIABLE_CHANGED, &MainFrame::onGdbLocalVariableChanged, this);
   Bind(wxEVT_DEBUGGER_GDB_STATE_CHANGED, &MainFrame::onGdbStateChanged, this);
   Bind(wxEVT_DEBUGGER_GDB_SIGNALED, &MainFrame::onGdbSignalReceived, this);
   Bind(wxEVT_DEBUGGER_GDB_STOPPED, &MainFrame::onGdbStopped, this);
   Bind(wxEVT_DEBUGGER_GDB_VARIABLE_WATCH_CHANGED, &MainFrame::onGdbVariableWatchChanged, this);
   Bind(wxEVT_DEBUGGER_GDB_BREAKPOINT_CHANGED, &MainFrame::onGdbBreakpointsChanged, this);
   Bind(wxEVT_DEBUGGER_GDB_STACK_CHANGED, &MainFrame::onGdbStackFrameChanged, this);
   Bind(wxEVT_MENU, &MainFrame::onToggleBreakpoint, this, CODE_VIEW_CONTEXT_MENU_TOGGLE_BREAKPOINT);
   Bind(wxEVT_MENU, &MainFrame::onRunToLine, this, CODE_VIEW_CONTEXT_MENU_RUN_TO_LINE);

   _sessionPanel->loadSessions();
}

void MainFrame::createMenuBar() {
   auto *menuBar = new wxMenuBar{};
   SetMenuBar(menuBar);

   createSessionMenu(menuBar);
   createControlMenu(menuBar);
   createViewMenu(menuBar);
   createHelpMenu(menuBar);
}

void MainFrame::createToolBar() {}

void MainFrame::createStatusBar() {}

void MainFrame::createBottomPanels() {
   _sessionPanel = new SessionPanel{_bottomLeftNotebook, this, PANEL_SESSIONS};
   _bottomLeftNotebook->AddPage(_sessionPanel, "Sessions");

   _breakpointsListPanel = new BreakPointsListPanel{_bottomLeftNotebook, this, PANEL_BREAKPOINTS};
   _bottomLeftNotebook->AddPage(_breakpointsListPanel, "Breakpoints");

   _stackListPanel = new StackListPanel{_bottomRightNotebook, this};
   _bottomRightNotebook->AddPage(_stackListPanel, "Stack");

   auto paneInfo = wxAuiPaneInfo().PaneBorder(false).Bottom().Position(0).MinSize(100, 300).Layer(0);

   _aui_mgr->AddPane(_bottomLeftNotebook, paneInfo.Name("bottomLeft"));
   _aui_mgr->AddPane(_bottomRightNotebook, paneInfo.Name("bottomRight").Position(1));
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

void MainFrame::createViewMenu(wxMenuBar *menuBar) {
   auto *viewMenu = new wxMenu{};
   menuBar->Append(viewMenu, _("&View"));

   viewMenu->Append(MENU_ITEM_VIEW_BREAKPOINTS, _("&Breakpoints"));
   viewMenu->Append(MENU_ITEM_VIEW_SESSIONS, _("&Sessions"));

   Bind(wxEVT_MENU, &MainFrame::onViewNotebookBottomLeft, this, MENU_ITEM_VIEW_BREAKPOINTS);
   Bind(wxEVT_MENU, &MainFrame::onViewNotebookBottomLeft, this, MENU_ITEM_VIEW_SESSIONS);
}

void MainFrame::onViewNotebookBottomLeft(wxCommandEvent &e) {
   int panelId = 0;
   if (e.GetId() == MENU_ITEM_VIEW_BREAKPOINTS) {
      panelId = PANEL_BREAKPOINTS;
   } else if (e.GetId() == MENU_ITEM_VIEW_SESSIONS) {
      panelId = PANEL_SESSIONS;
   } else {
      return;
   }

   auto *win = _bottomLeftNotebook->FindWindow(panelId);
   if (win != nullptr) {
      auto pageCount = _bottomLeftNotebook->GetPageCount();
      for (auto i = 0; i < pageCount; i++) {
         if (win == _bottomLeftNotebook->GetPage(i)) {
            _bottomLeftNotebook->SetSelection(i);
            win->SetFocus();
            break;
         }
      }
   }
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

void MainFrame::createControlMenu(wxMenuBar *menuBar) {
   auto *editMenu = new wxMenu{};
   menuBar->Append(editMenu, _("&Control"));

   editMenu->Append(MENU_DEBUG_ID_RUN, _("&Run"));
   editMenu->AppendSeparator();
   editMenu->Append(MENU_DEBUG_ID_STEP_IN, _("Step &In"));
   editMenu->Append(MENU_DEBUG_ID_STEP_OUT, _("Step &Out"));
   editMenu->Append(MENU_DEBUG_ID_NEXT, _("&Next"));
   editMenu->Append(MENU_DEBUG_ID_CONTINUE, _("&Continue"));
   editMenu->Append(MENU_DEBUG_ID_RUN_TO_LINE, _("Run To &Line"));
   editMenu->Append(MENU_DEBUG_ID_INTERRUPT, _("In&terrupt"));

   Bind(wxEVT_MENU, &MainFrame::onControlRun, this, MENU_DEBUG_ID_RUN);
   Bind(wxEVT_MENU, &MainFrame::onControlNext, this, MENU_DEBUG_ID_NEXT);
   Bind(wxEVT_MENU, &MainFrame::onControlStepIn, this, MENU_DEBUG_ID_STEP_IN);
   Bind(wxEVT_MENU, &MainFrame::onControlStepOut, this, MENU_DEBUG_ID_STEP_OUT);
   Bind(wxEVT_MENU, &MainFrame::onControlContinue, this, MENU_DEBUG_ID_CONTINUE);
   Bind(wxEVT_MENU, &MainFrame::onControlRunToLine, this, MENU_DEBUG_ID_RUN_TO_LINE);
   Bind(wxEVT_MENU, &MainFrame::onControlInterrupt, this, MENU_DEBUG_ID_INTERRUPT);

   setMenuEnabled(MENU_DEBUG_ID_RUN, false);
   setMenuEnabled(MENU_DEBUG_ID_STEP_IN, false);
   setMenuEnabled(MENU_DEBUG_ID_STEP_OUT, false);
   setMenuEnabled(MENU_DEBUG_ID_NEXT, false);
   setMenuEnabled(MENU_DEBUG_ID_CONTINUE, false);
   setMenuEnabled(MENU_DEBUG_ID_RUN_TO_LINE, false);
   setMenuEnabled(MENU_DEBUG_ID_INTERRUPT, false);
}

void MainFrame::onControlRunToLine(wxCommandEvent &e) {}
void MainFrame::onControlContinue(wxCommandEvent & /*e*/) {
   _gdbMgr->gdbContinue();
   disableCurrentLine();
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

void MainFrame::createHelpMenu(wxMenuBar *menuBar) {
   auto *helpMenu = new wxMenu{};

   helpMenu->Append(MENU_ITEM_HELP_SETTINGS, _("&Settings"));
   helpMenu->Append(MENU_ITEM_HELP_ABOUT, _("&About"));

   Bind(wxEVT_MENU, &MainFrame::onHelpSettings, this, MENU_ITEM_HELP_SETTINGS);
   Bind(wxEVT_MENU, &MainFrame::onHelpAbout, this, MENU_ITEM_HELP_ABOUT);

   menuBar->Append(helpMenu, "&Help");
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

void MainFrame::onDebuggerStopped(DebuggerEvent & /*e*/) {
   _breakpointsListPanel->getListCtrl()->DeleteChildren(_breakpointsListPanel->getListCtrl()->GetRootItem());
   _stackListPanel->getListCtrl()->DeleteAllItems();
   _localVariablesTreePanel->getTreeListCtrl()->DeleteChildren(_localVariablesTreePanel->getTreeListCtrl()->GetRootItem());

   _aui_notebook->DeleteAllPages();
}

void MainFrame::onGdbLocalVariableChanged(DebuggerEvent &e) { _localVariablesTreePanel->handleLocalVariableChanged(e._variablesList, e._fileName, e._functionName, e._stackDepth); }

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
void MainFrame::onHelpSettings(wxCommandEvent &e) {}

void MainFrame::onHelpAbout(wxCommandEvent &e) {}
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

auto MainFrame::openFile(const wxString &fileName, int lineNo) -> jaeCppTextDocument * {
   auto *codeView = openFile(fileName);

   if (codeView != nullptr) {
      codeView->gotoLineAndEnsureVisible(lineNo);
   }
   return codeView;
}

auto MainFrame::openFile(const wxString &fileName) -> jaeCppTextDocument * {
   if (fileName.IsEmpty()) {
      return nullptr;
   }

   // Already open?
   int foundCodeViewTabIdx = -1;
   for (int tabIdx = 0; tabIdx < (int)_aui_notebook->GetPageCount(); tabIdx++) {
      auto *tab = dynamic_cast<jaeCppTextDocument *>(_aui_notebook->GetPage(tabIdx));
      if (tab->getFilePath() == fileName) {
         foundCodeViewTabIdx = tabIdx;
      }
   }

   jaeCppTextDocument *codeView = nullptr;

   if (foundCodeViewTabIdx != -1) {
      _aui_notebook->SetSelection(foundCodeViewTabIdx);
      codeView = dynamic_cast<jaeCppTextDocument *>(_aui_notebook->GetPage(foundCodeViewTabIdx));
   } else {
      if (_aui_notebook->GetPageCount() > MAX_FILES_OPENED) {
         // Find the oldest tab
         int  oldestTabIdx  = -1;
         auto tnow          = wxGetLocalTime();
         auto oldestTabTime = tnow;

         for (int tabIdx = 0; tabIdx < (int)_aui_notebook->GetPageCount(); tabIdx++) {
            auto *testTab = dynamic_cast<jaeCppTextDocument *>(_aui_notebook->GetPage(foundCodeViewTabIdx));
            if (oldestTabIdx == -1 || testTab->getLastAccessTime() < oldestTabTime) {
               oldestTabTime = testTab->getLastAccessTime();
               oldestTabIdx  = tabIdx;
            }
         }

         // Close the oldest tab
         auto *oldestestTab = dynamic_cast<jaeCppTextDocument *>(_aui_notebook->GetPage(oldestTabIdx));
         _aui_notebook->RemovePage(oldestTabIdx);
         delete oldestestTab;
      }
      // Create the tab
      codeView = new jaeCppTextDocument(this);

      wxFileName w(fileName);
      w.Normalize();

      auto fname = w.GetFullPath();
      codeView->LoadFile(fname);
      codeView->ClearSelections();

      // Add the new codeview tab
      _aui_notebook->AddPage(codeView, w.GetFullName());
      _aui_notebook->SetSelection(_aui_notebook->GetPageCount() - 1);
   }

   _breakpointsListPanel->handleBreakPointChanged(false);
   updateBreakpointsInCodeview();

   return codeView;
}

void MainFrame::updateCurrentLine(wxString filename, int lineno) {
   jaeCppTextDocument *currentCodeViewTab = nullptr;

   m_currentFile = filename;
   m_currentLine = lineno;

   if (!filename.IsEmpty()) {
      currentCodeViewTab = openFile(filename);
   }

   // Update the current line view
   if (currentCodeViewTab != nullptr) {
      gotoLine(m_currentLine);
   } else {
      disableCurrentLine();
   }
}

void MainFrame::disableCurrentLine() {
   for (int tabIdx = 0; tabIdx < (int)_aui_notebook->GetPageCount(); tabIdx++) {
      auto *codeViewTab = dynamic_cast<jaeCppTextDocument *>(_aui_notebook->GetPage(tabIdx));

      codeViewTab->disableLine();
   }
}

void MainFrame::gotoLine(int lineno) {
   for (int tabIdx = 0; tabIdx < (int)_aui_notebook->GetPageCount(); tabIdx++) {
      auto *codeViewTab = dynamic_cast<jaeCppTextDocument *>(_aui_notebook->GetPage(tabIdx));

      if (codeViewTab->getFilePath() == m_currentFile) {
         codeViewTab->gotoLineAndEnsureVisible(lineno - 1);
      }
   }
}

void MainFrame::setMenuEnabled(int id, bool isEnable) {
   wxMenu *tempMenu     = nullptr;
   auto *  tempMenuItem = GetMenuBar()->FindItem(id, &tempMenu);
   if (tempMenuItem != nullptr) {
      tempMenuItem->Enable(isEnable);
   }
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
