#include "MainFrame.h"
#include "CustomEvents.h"
#include "GdbMgr.h"
#include "ID.h"
#include "jaeCppTextDocument.h"
#include "ui/BreakPointsPanel.h"
#include "ui/LocalVariablesPanel.h"
#include "ui/SessionPanel.h"
#include "ui/StackPanel.h"
#include "wx/accel.h"
#include "wx/filename.h"
#include "wx/string.h"
#include <iostream>
#include <ostream>
#include <wx/aui/aui.h>
#include <wx/aui/auibook.h>
#include <wx/dir.h>
#include <wx/event.h>
#include <wx/fileconf.h>
#include <wx/gtk/frame.h>
#include <wx/gtk/menu.h>
#include <wx/gtk/window.h>
#include <wx/msgdlg.h>
#include <wx/stc/stc.h>
#include <wx/stdpaths.h>
#include "ui/SourceFilesPanel.h"
#include "ui/GoToLineDialog.h"
#include "wx/fdrepdlg.h"

#define MAX_FILES_OPENED 50

MainFrame::MainFrame(CmdLineOptions &cmdLineOption)
    : wxFrame{nullptr, JA_ID_FRAME_MAIN_WINDOW, "Vazhu-Neekuthal"}, _cmdLineOption{cmdLineOption}, _aui_mgr(new wxAuiManager{this}), _aui_notebook(new wxAuiNotebook{this}),
      _bottomLeftNotebook(new wxAuiNotebook{this}), _bottomRightNotebook(new wxAuiNotebook{this}), _gdbMgr(new GdbMgr{}),
      _localVariablesTreePanel(new LocalVariablesPanel{this, this, PANEL_LOCAL_VARIABLES}) {

   _sourceFilesPanel = new SourceFilesPanel{this};
   _gdbMgr->SetNextHandler(this);

   auto codePI        = wxAuiPaneInfo().Name("notebook").CenterPane().PaneBorder(false).Maximize();
   auto localVarPI    = wxAuiPaneInfo().Name("LocalVariables").Right().Position(0).PaneBorder(false).MinSize(500, 1200).Caption("Local Variables").Hide();
   auto sourcePanelPI = wxAuiPaneInfo().Name("SourceFiles").Left().Position(0).PaneBorder(false).MinSize(300, 1200).Caption("Source Files").Hide();
   auto paneInfo      = wxAuiPaneInfo().PaneBorder(false).Bottom().Position(0).MinSize(100, 300).Layer(0).Hide();

   _aui_mgr->AddPane(_aui_notebook, codePI);
   _aui_mgr->AddPane(_localVariablesTreePanel, localVarPI);
   _aui_mgr->AddPane(_sourceFilesPanel, sourcePanelPI);
   _aui_mgr->AddPane(_bottomLeftNotebook, paneInfo.Name("bottomLeft"));
   _aui_mgr->AddPane(_bottomRightNotebook, paneInfo.Name("bottomRight").Position(1));

   createMenuBar();
   createToolBar();
   createStatusBar();
   createBottomPanels();

   displayPanel(MENU_ITEM_VIEW_SESSIONS, true);

   _aui_mgr->Update();

   auto               noOfEntries = 8;
   wxAcceleratorEntry entries[noOfEntries];
   entries[0].Set(wxACCEL_NORMAL, WXK_F5, MENU_DEBUG_ID_CONTINUE);
   entries[1].Set(wxACCEL_NORMAL, WXK_F6, MENU_DEBUG_ID_NEXT);
   entries[2].Set(wxACCEL_NORMAL, WXK_F7, MENU_DEBUG_ID_STEP_IN);
   entries[3].Set(wxACCEL_NORMAL, WXK_F8, MENU_DEBUG_ID_STEP_OUT);
   entries[4].Set(wxACCEL_ALT, (int)'0', MENU_ITEM_GOTO_SESSIONS);
   entries[5].Set(wxACCEL_ALT, (int)'1', MENU_ITEM_GOTO_BREAKPOINTS);
   entries[6].Set(wxACCEL_ALT, (int)'l', MENU_ITEM_VIEW_SOURCE_SEARCH_FOCUS);
   entries[7].Set(wxACCEL_CTRL, (int)'g', MENU_ITEM_EDIT_GOTO_LINE);
   //   entries[7].Set(wxACCEL_NORMAL, WXK_F2, MENU_ITEM_VIEW_SOURCE_FILES);

   // entries[2].Set(wxACCEL_SHIFT, (int)'A', ID_ABOUT);
   // entries[3].Set(wxACCEL_NORMAL, WXK_DELETE, wxID_CUT);
   wxAcceleratorTable accel(noOfEntries, entries);
   this->SetAcceleratorTable(accel);

   Bind(wxEVT_DEBUGGER_GDB_PROCESS_ENDED, &MainFrame::onDebuggerStopped, this);
   Bind(wxEVT_DEBUGGER_GDB_LOCAL_VARIABLE_CHANGED, &MainFrame::onGdbLocalVariableChanged, this);
   Bind(wxEVT_DEBUGGER_GDB_LOCAL_VARIABLE_ADDED, &MainFrame::onGdbLocalVariableAdded, this);
   Bind(wxEVT_DEBUGGER_GDB_STATE_CHANGED, &MainFrame::onGdbStateChanged, this);
   Bind(wxEVT_DEBUGGER_GDB_SIGNALED, &MainFrame::onGdbSignalReceived, this);
   Bind(wxEVT_DEBUGGER_GDB_STOPPED, &MainFrame::onGdbStopped, this);
   Bind(wxEVT_DEBUGGER_GDB_VARIABLE_WATCH_CHANGED, &MainFrame::onGdbVariableWatchChanged, this);
   Bind(wxEVT_DEBUGGER_GDB_BREAKPOINT_CHANGED, &MainFrame::onGdbBreakpointsChanged, this);
   Bind(wxEVT_DEBUGGER_GDB_STACK_CHANGED, &MainFrame::onGdbStackChanged, this);
   Bind(wxEVT_DEBUGGER_GDB_FRAME_ID_CHANGED, &MainFrame::onGdbStackFrameChanged, this);
   Bind(wxEVT_MENU, &MainFrame::onToggleBreakpoint, this, CODE_VIEW_CONTEXT_MENU_TOGGLE_BREAKPOINT);
   Bind(wxEVT_MENU, &MainFrame::onRunToLine, this, CODE_VIEW_CONTEXT_MENU_RUN_TO_LINE);
   _sourceFilesPanel->Bind(wxEVT_SOURCE_FILE_OPEN, [this](auto &event) { openFile(event.GetString()); });
   Bind(wxEVT_MENU, &MainFrame::onSourceFilesSearchFocus, this, MENU_ITEM_VIEW_SOURCE_SEARCH_FOCUS);

   _sessionPanel->loadSessions();

   if (!_cmdLineOption._session.empty()) {
      GdbSessionInfo *sessionInfo = nullptr;

      for (auto *session : _sessionPanel->getSessionInfoList()) {
         if (session->_sessionName == _cmdLineOption._session) {
            sessionInfo = session;
            break;
         }

         if (!session->_exePath.empty() && wxFileExists(session->_exePath) && wxFileExists(_cmdLineOption._session)) {
            wxFileName fileName{session->_exePath};
            wxFileName newSessionFileName{_cmdLineOption._session};

            //         std::cout << fileName.GetName() << "==" << newSessionFileName.GetName() << std::endl;
            if (fileName.GetName() == newSessionFileName.GetName()) {
               sessionInfo = session;
               break;
            }
         }
      }

      if (sessionInfo != nullptr) {
         runSession(sessionInfo);
      }
   }
}

void MainFrame::createMenuBar() {
   auto *menuBar = new wxMenuBar{};
   SetMenuBar(menuBar);

   createSessionMenu(menuBar);
   createEditMenu(menuBar);
   createControlMenu(menuBar);
   createViewMenu(menuBar);
   createGoToMenu(menuBar);
   createHelpMenu(menuBar);
}

void MainFrame::createToolBar() {}

void MainFrame::createStatusBar() {}

void MainFrame::createBottomPanels() {
   _sessionPanel         = new SessionPanel{_bottomLeftNotebook, this, PANEL_SESSIONS};
   _breakpointsListPanel = new BreakPointsListPanel{_bottomLeftNotebook, this, PANEL_BREAKPOINTS};
   _stackListPanel       = new StackListPanel{_bottomRightNotebook, this, PANEL_STACK};
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
void MainFrame::createGoToMenu(wxMenuBar *menuBar) {
   auto *goToMenu = new wxMenu{};
   menuBar->Append(goToMenu, _("&Go To"));

   goToMenu->Append(MENU_ITEM_GOTO_BREAKPOINTS, _("&Breakpoints"));
   goToMenu->Append(MENU_ITEM_GOTO_SESSIONS, _("&Sessions"));
   goToMenu->Append(MENU_ITEM_GOTO_LOCAL_VARIABLES, _("&Local Variables"));

   Bind(wxEVT_MENU, &MainFrame::onGoToBreakpointsPanel, this, MENU_ITEM_GOTO_BREAKPOINTS);
   Bind(wxEVT_MENU, &MainFrame::onGoToSessionsPanel, this, MENU_ITEM_GOTO_SESSIONS);
   Bind(wxEVT_MENU, &MainFrame::onGoToLocalsPanel, this, MENU_ITEM_GOTO_LOCAL_VARIABLES);
}

void MainFrame::onGoToBreakpointsPanel(wxCommandEvent &evt) {
   auto *win = _bottomLeftNotebook->FindWindow(PANEL_BREAKPOINTS);
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

void MainFrame::onGoToSessionsPanel(wxCommandEvent &evt) {
   auto *win = _bottomLeftNotebook->FindWindow(PANEL_SESSIONS);
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

void MainFrame::onGoToLocalsPanel(wxCommandEvent & /*evt*/) { _localVariablesTreePanel->SetFocus(); }

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

void MainFrame::createViewMenu(wxMenuBar *menuBar) {
   auto *viewMenu = new wxMenu{};
   menuBar->Append(viewMenu, _("&View"));

   viewMenu->AppendCheckItem(MENU_ITEM_VIEW_BREAKPOINTS, _("&Breakpoints\tCtrl-b"));
   viewMenu->AppendCheckItem(MENU_ITEM_VIEW_SESSIONS, _("&Sessions\tCtrl-e"));
   viewMenu->AppendCheckItem(MENU_ITEM_VIEW_STACKS, _("&Stack\tCtrl-k"));
   viewMenu->AppendCheckItem(MENU_ITEM_VIEW_LOCAL_VARIABLES, _("&Local Variables\tCtrl-l"));

   auto *menuItem = viewMenu->AppendCheckItem(MENU_ITEM_VIEW_SOURCE_FILES, _("&Source Files"));
   menuItem->SetAccel(new wxAcceleratorEntry(wxACCEL_CTRL, 's'));

   Bind(wxEVT_MENU, &MainFrame::onViewBreakpointsPanel, this, MENU_ITEM_VIEW_BREAKPOINTS);
   Bind(wxEVT_MENU, &MainFrame::onViewSessionsPanel, this, MENU_ITEM_VIEW_SESSIONS);
   Bind(wxEVT_MENU, &MainFrame::onViewStackPanel, this, MENU_ITEM_VIEW_STACKS);
   Bind(wxEVT_MENU, &MainFrame::onViewLocalsPanel, this, MENU_ITEM_VIEW_LOCAL_VARIABLES);
   Bind(wxEVT_MENU, &MainFrame::onViewSourcePanel, this, MENU_ITEM_VIEW_SOURCE_FILES);
}

void MainFrame::onViewBreakpointsPanel(wxCommandEvent &evt) {
   if (evt.IsChecked()) {
      auto *page = findPage(_bottomLeftNotebook, PANEL_BREAKPOINTS);
      if (page != nullptr) {
         page->SetFocus();
      } else {
         _bottomLeftNotebook->AddPage(_breakpointsListPanel, "Breakpoints");
      }
      _aui_mgr->GetPane(_bottomLeftNotebook).Show();
   } else {
      removePage(_bottomLeftNotebook, PANEL_BREAKPOINTS);
      if (_bottomLeftNotebook->GetPageCount() == 0) {
         _aui_mgr->GetPane(_bottomLeftNotebook).Hide();
      }
   }
   _aui_mgr->Update();
}

void MainFrame::onViewSessionsPanel(wxCommandEvent &evt) {
   if (evt.IsChecked()) {
      auto *page = findPage(_bottomLeftNotebook, PANEL_SESSIONS);
      if (page != nullptr) {
         page->SetFocus();
      } else {
         _bottomLeftNotebook->AddPage(_sessionPanel, "Sessions");
      }
      _aui_mgr->GetPane(_bottomLeftNotebook).Show();
   } else {
      removePage(_bottomLeftNotebook, PANEL_SESSIONS);
      if (_bottomLeftNotebook->GetPageCount() == 0) {
         _aui_mgr->GetPane(_bottomLeftNotebook).Hide();
      }
   }
   _aui_mgr->Update();
}

void MainFrame::onViewStackPanel(wxCommandEvent &evt) {
   if (evt.IsChecked()) {
      auto *page = findPage(_bottomRightNotebook, PANEL_BREAKPOINTS);
      if (page != nullptr) {
         page->SetFocus();
      } else {
         _bottomRightNotebook->AddPage(_stackListPanel, "Stack");
      }
      _aui_mgr->GetPane(_bottomRightNotebook).Show();
   } else {
      removePage(_bottomRightNotebook, PANEL_STACK);
      if (_bottomRightNotebook->GetPageCount() == 0) {
         _aui_mgr->GetPane(_bottomRightNotebook).Hide();
      }
   }
   _aui_mgr->Update();
}

void MainFrame::onViewLocalsPanel(wxCommandEvent &evt) {
   if (evt.IsChecked()) {
      _aui_mgr->GetPane(_localVariablesTreePanel).Show();
   } else {
      _aui_mgr->GetPane(_localVariablesTreePanel).Hide();
   }
   _aui_mgr->Update();
}

void MainFrame::onViewSourcePanel(wxCommandEvent &evt) {
   std::cout << "view source panel" << std::endl;
   if (evt.IsChecked()) {
      _aui_mgr->GetPane(_sourceFilesPanel).Show();
   } else {
      _aui_mgr->GetPane(_sourceFilesPanel).Hide();
   }
   _aui_mgr->Update();
}

void MainFrame::removePage(wxAuiNotebook *noteBook, int pageId) {
   auto *win = noteBook->FindWindow(pageId);
   if (win != nullptr) {
      auto pageCount = noteBook->GetPageCount();
      for (auto i = 0; i < pageCount; i++) {
         if (win == noteBook->GetPage(i)) {
            noteBook->RemovePage(i);
            break;
         }
      }
   }
}

auto MainFrame::findPage(wxAuiNotebook *noteBook, int pageId) -> wxWindow * {
   auto *win = noteBook->FindWindow(pageId);
   if (win != nullptr) {
      auto pageCount = noteBook->GetPageCount();
      for (auto i = 0; i < pageCount; i++) {
         if (win == noteBook->GetPage(i)) {
            noteBook->RemovePage(i);
            return win;
         }
      }
   }
   return nullptr;
}

void MainFrame::relayoutBottomPanes() {
   _aui_mgr->DetachPane(_aui_notebook);
   _aui_mgr->DetachPane(_bottomRightNotebook);
   _aui_mgr->DetachPane(_bottomLeftNotebook);
   _aui_mgr->DetachPane(_localVariablesTreePanel);
   _aui_mgr->DetachPane(_sourceFilesPanel);
   _aui_mgr->Update();

   _aui_mgr->AddPane(_aui_notebook, wxAuiPaneInfo().Name("notebook").CenterPane().PaneBorder(false).Maximize());
   _aui_mgr->AddPane(_localVariablesTreePanel, wxAuiPaneInfo().Name("LocalVariables").Right().Position(0).PaneBorder(false).MinSize(500, 1200).Caption("Local Variables"));
   _aui_mgr->AddPane(_sourceFilesPanel, wxAuiPaneInfo().Name("SourceFiles").Left().Position(0).PaneBorder(false).MinSize(300, 1200).Caption("Source Files"));

   auto paneInfo = wxAuiPaneInfo().PaneBorder(false).Bottom().Position(0).MinSize(100, 300).Layer(0);

   if (_bottomLeftNotebook->GetPageCount() > 0) {
      _aui_mgr->AddPane(_bottomLeftNotebook, paneInfo.Name("bottomLeft"));
   }
   if (_bottomRightNotebook->GetPageCount() > 0) {
      _aui_mgr->AddPane(_bottomRightNotebook, paneInfo.Name("bottomRight").Position(1));
   }
   _aui_mgr->Update();
}

void MainFrame::displayPanel(int panelId, bool display) {
   wxCommandEvent evt{wxEVT_MENU, panelId};
   evt.SetInt(display ? 1 : 0);
   GetEventHandler()->ProcessEvent(evt);
   GetMenuBar()->Check(panelId, display);
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

void MainFrame::onSourceFilesSearchFocus(wxCommandEvent &e) {
   // std::cout << "search focus" << std::endl;
   _sourceFilesPanel->setSearchFocus();
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

void MainFrame::createControlMenu(wxMenuBar *menuBar) {
   auto *editMenu = new wxMenu{};
   menuBar->Append(editMenu, _("&Control"));

   auto *menuItem = editMenu->Append(MENU_DEBUG_ID_RUN, _("&Run\tCtrl-r"));
   editMenu->AppendSeparator();

   menuItem = editMenu->Append(MENU_DEBUG_ID_STEP_IN, _("Step &In"));
   menuItem->SetAccel(new wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F7));

   menuItem = editMenu->Append(MENU_DEBUG_ID_STEP_OUT, _("Step &Out"));
   menuItem->SetAccel(new wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F8));

   menuItem = editMenu->Append(MENU_DEBUG_ID_NEXT, _("&Next"));
   menuItem->SetAccel(new wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F6));

   menuItem = editMenu->Append(MENU_DEBUG_ID_CONTINUE, _("&Continue"));
   menuItem->SetAccel(new wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F5));

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
   _breakpointsListPanel->clearWidget();
   _stackListPanel->clearWidget();
   _localVariablesTreePanel->clearWidget();
   _aui_notebook->DeleteAllPages();
}

void MainFrame::onGdbLocalVariableChanged(DebuggerEvent &evt) { _localVariablesTreePanel->handleLocalVariableChanged(evt._variablesList, evt._fileName, evt._functionName, evt._stackDepth); }

void MainFrame::onGdbLocalVariableAdded(DebuggerEvent &evt) { _localVariablesTreePanel->addItem(evt._variableWatch); }

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
void MainFrame::onHelpSettings(wxCommandEvent &evt) {}

void MainFrame::onHelpAbout(wxCommandEvent &evt) {}
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
      codeView->SetSize(200, 200);

      wxFileName w(fileName);
      w.Normalize();

      auto fname = w.GetFullPath();
      codeView->LoadFile(fname);
      codeView->ClearSelections();

      // Add the new codeview tab
      _aui_notebook->AddPage(codeView, w.GetFullName());
      auto count = _aui_notebook->GetPageCount();
      _aui_notebook->SetSelection(count - 1);
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

void MainFrame::setMenuEnabled(int menuId, bool isEnable) {
   wxMenu *tempMenu     = nullptr;
   auto   *tempMenuItem = GetMenuBar()->FindItem(menuId, &tempMenu);
   if (tempMenuItem != nullptr) {
      tempMenuItem->Enable(isEnable);
   }
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
void MainFrame::createEditMenu(wxMenuBar *menuBar) {
   auto *editMenu = new wxMenu{};
   menuBar->Append(editMenu, _("&Edit"));

   editMenu->AppendCheckItem(MENU_ITEM_EDIT_FIND, _("&Find..."));
   editMenu->Append(MENU_ITEM_EDIT_GOTO_LINE, _("&Go To Line..."));

   Bind(wxEVT_MENU, &MainFrame::onGoToLine, this, MENU_ITEM_EDIT_GOTO_LINE);
   Bind(wxEVT_MENU, &MainFrame::onFind, this, MENU_ITEM_EDIT_FIND);

   Bind(wxEVT_FIND, &MainFrame::onFindDialog, this, wxID_ANY);
   Bind(wxEVT_FIND_NEXT, &MainFrame::onFindDialog, this, wxID_ANY);
   Bind(wxEVT_FIND_CLOSE, &MainFrame::onFindDialog, this, wxID_ANY);
}

void MainFrame::onGoToLine(wxCommandEvent & /*event*/) {
   if (!_gdbMgr->isValidSession()) {
      return;
   }

   GoToLineDialog dlg{this};

   if (dlg.ShowModal() == wxID_OK) {
      gotoLine(dlg.getLine());
   }
}

void MainFrame::onFind(wxCommandEvent & /*event*/) {
   if (!_gdbMgr->isValidSession()) {
      return;
   }

   if (m_dlgFind != nullptr) {
      m_dlgFind->Destroy();
      m_dlgFind = nullptr;
   }

   m_dlgFind = new wxFindReplaceDialog(this, &_find_data, "Find dialog",
                                       // just for testing
                                       wxFR_NOWHOLEWORD);

   m_dlgFind->Show(true);
}

void MainFrame::onFindDialog(wxFindDialogEvent &event) {
#if 1
   wxEventType type = event.GetEventType();
   // clang-format off
    if ( type == wxEVT_FIND || type == wxEVT_FIND_NEXT )
    {
        wxLogMessage("Find %s'%s' (flags: %s)",
                     type == wxEVT_FIND_NEXT ? "next " : "",
                     event.GetFindString(),
                     event.GetFlags());
    }
    else if ( type == wxEVT_FIND_REPLACE ||
                type == wxEVT_FIND_REPLACE_ALL )
    {
        #if 0
        wxLogMessage("Replace %s'%s' with '%s' (flags: %s)",
                     type == wxEVT_FIND_REPLACE_ALL ? "all " : "",
                     event.GetFindString(),
                     event.GetReplaceString(),
                     DecodeFindDialogEventFlags(event.GetFlags()));
        #endif
    }
    else if ( type == wxEVT_FIND_CLOSE )
    {
        wxFindReplaceDialog *dlg = event.GetDialog();

        int idMenu;
        wxString txt;
        if ( dlg == m_dlgFind )
        {
            txt = "Find";
            idMenu = MENU_ITEM_EDIT_FIND;
            m_dlgFind = nullptr;
        }
        #if 0
        else if ( dlg == m_dlgReplace )
        {
            txt = "Replace";
            idMenu = DIALOGS_REPLACE;
            m_dlgReplace = NULL;
        }
        #endif
        else
        {
            txt = "Unknown";
            idMenu = wxID_ANY;

            wxFAIL_MSG( "unexpected event" );
        }

        wxLogMessage("%s dialog is being closed.", txt);

        if ( idMenu != wxID_ANY )
        {
            GetMenuBar()->Check(idMenu, false);
        }

        dlg->Destroy();
    }
    else
    {
        wxLogError("Unknown find dialog event!");
    }
   // clang-format on

#endif
}
