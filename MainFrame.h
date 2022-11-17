#pragma once

#include "wx/fdrepdlg.h"
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "GdbMgr.h"
#include <wx/aui/framemanager.h>
#include <wx/event.h>
#include <wx/frame.h>

class jaeCppTextDocument;
class BreakPointsListPanel;
class StackListPanel;
class LocalVariablesPanel;
class SessionPanel;
class GdbSessionInfo;
class wxAuiNotebook;
class SourceFilesPanel;

class CmdLineOptions {
 public:
   wxString _session;
};

class MainFrame : public wxFrame {
 public:
   MainFrame(CmdLineOptions &cmdLineOption);

   auto getGdbMgr() -> GdbMgr * { return _gdbMgr; }
   auto openFile(const wxString &fileName) -> jaeCppTextDocument *;
   auto openFile(const wxString &fileName, int lineNo) -> jaeCppTextDocument *;
   void openSession(GdbSessionInfo *sessionInfoObj);
   void runSession(GdbSessionInfo *sessionInfoObj);
   void closeSession(GdbSessionInfo *sessionInfoObj);
   void editSession(GdbSessionInfo *sessionInfoObj);
   void deleteSession(GdbSessionInfo *sessionInfoObj);
   void saveSession(GdbSessionInfo &sessionInfo, bool isEdited);
   void setMenuEnabled(int id, bool isEnable);
   void updateCurrentLine(wxString filename, int lineno);
   void gotoLine(int lineno);
   void disableCurrentLine();

 private:
   void createMenuBar();
   void createToolBar();
   void createStatusBar();
   void createSessionMenu(wxMenuBar *menuBar);
   void createEditMenu(wxMenuBar *menuBar);
   void createControlMenu(wxMenuBar *menuBar);
   void createViewMenu(wxMenuBar *menuBar);
   void createGoToMenu(wxMenuBar *menuBar);
   void createHelpMenu(wxMenuBar *menuBar);

   void createBottomPanels();

   void updateBreakpointsInCodeview();

   /* {{{{{{{{{{{{{{{{{{{{ session menu handling {{{{{{{{{{{{{{{{{{{{ */
   void onSessionNewExec(wxCommandEvent &e);
   void onSessionNewLocalAtach(wxCommandEvent &e);
   void onSessionNewCoredump(wxCommandEvent &e);
   void onSessionNewRemoteAttach(wxCommandEvent &e);
   void onSessionOpen(wxCommandEvent &e);
   void onSessionClose(wxCommandEvent &e);
   /* }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}*/

   /* {{{{{{{{{{{{{{{{{{{{ edit menu handling {{{{{{{{{{{{{{{{{{{{ */
   void onGoToLine(wxCommandEvent &event);
   void onFind(wxCommandEvent &event);
   void onFindDialog(wxFindDialogEvent &event);
   /* }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}*/

   /* {{{{{{{{{{{{{{{{{{{{ control menu handling {{{{{{{{{{{{{{{{{{{{ */
   void onControlRun(wxCommandEvent & /*e*/) { _gdbMgr->gdbRun(); }
   void onControlNext(wxCommandEvent & /*e*/) { _gdbMgr->gdbNext(); }
   void onControlStepIn(wxCommandEvent & /*e*/) { _gdbMgr->gdbStepIn(); }
   void onControlStepOut(wxCommandEvent & /*e*/) { _gdbMgr->gdbStepOut(); }
   void onControlContinue(wxCommandEvent &e);
   void onControlRunToLine(wxCommandEvent &e);
   void onControlInterrupt(wxCommandEvent & /*e*/) { _gdbMgr->gdbStop(); }
   /* }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}*/

   /* {{{{{{{{{{{{{{{{{{{{ view menu handling {{{{{{{{{{{{{{{{{{{{ */
   void onViewBreakpointsPanel(wxCommandEvent &evt);
   void onViewSessionsPanel(wxCommandEvent &evt);
   void onViewStackPanel(wxCommandEvent &evt);
   void onViewLocalsPanel(wxCommandEvent &evt);
   void onViewSourcePanel(wxCommandEvent &evt);
   /* }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}*/

   /* {{{{{{{{{{{{{{{{{{{{ go to menu handling {{{{{{{{{{{{{{{{{{{{ */
   void onSourceFilesSearchFocus(wxCommandEvent &evt);
   void onGoToBreakpointsPanel(wxCommandEvent &evt);
   void onGoToSessionsPanel(wxCommandEvent &evt);
   void onGoToLocalsPanel(wxCommandEvent &evt);
   /* }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}*/

   /* {{{{{{{{{{{{{{{{{{{{ help menu handling {{{{{{{{{{{{{{{{{{{{ */
   void onHelpSettings(wxCommandEvent &e);
   void onHelpAbout(wxCommandEvent &e);
   /* }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}*/

   /* {{{{{{{{{{{{{{{{{{{{ debugger events {{{{{{{{{{{{{{{{{{{{ */
   void onDebuggerStopped(DebuggerEvent &e);
   void onGdbLocalVariableChanged(DebuggerEvent &e);
   void onGdbLocalVariableAdded(DebuggerEvent &e);
   void onGdbStateChanged(DebuggerEvent &e);
   void onGdbSignalReceived(DebuggerEvent &e);
   void onGdbStopped(DebuggerEvent &e);
   void onGdbVariableWatchChanged(DebuggerEvent &e);
   void onGdbBreakpointsChanged(DebuggerEvent &e);
   void onGdbStackChanged(DebuggerEvent &e);
   void onGdbStackFrameChanged(DebuggerEvent &e);

   /* }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}*/

   void onToggleBreakpoint(wxCommandEvent &evt);
   void onRunToLine(wxCommandEvent &evt);
   void removePage(wxAuiNotebook *noteBook, int pageId);
   auto findPage(wxAuiNotebook *noteBook, int pageId) -> wxWindow *;
   void relayoutBottomPanes();
   void displayPanel(int panelId, bool display);

   CmdLineOptions               _cmdLineOption;
   wxAuiManager                *_aui_mgr                 = nullptr;
   wxAuiNotebook               *_aui_notebook            = nullptr;
   wxAuiNotebook               *_bottomLeftNotebook      = nullptr;
   wxAuiNotebook               *_bottomRightNotebook     = nullptr;
   GdbMgr                      *_gdbMgr                  = nullptr;
   BreakPointsListPanel        *_breakpointsListPanel    = nullptr;
   StackListPanel              *_stackListPanel          = nullptr;
   LocalVariablesPanel         *_localVariablesTreePanel = nullptr;
   SessionPanel                *_sessionPanel            = nullptr;
   SourceFilesPanel            *_sourceFilesPanel        = nullptr;
   wxString                     m_currentFile;
   int                          m_currentLine = 0;
   std::vector<StackFrameEntry> m_stackFrameList;
   wxFindReplaceData            _find_data;
   wxFindReplaceDialog         *m_dlgFind = nullptr;
};
