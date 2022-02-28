#pragma once

#include "wx/utils.h"
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

class MainFrame : public wxFrame {
 public:
   MainFrame();

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
   void createControlMenu(wxMenuBar *menuBar);
   void createViewMenu(wxMenuBar *menuBar);
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

   /* {{{{{{{{{{{{{{{{{{{{ control menu handling {{{{{{{{{{{{{{{{{{{{ */
   void onControlRun(wxCommandEvent & /*e*/) { _gdbMgr->gdbRun(); }
   void onControlNext(wxCommandEvent & /*e*/) { _gdbMgr->gdbNext(); }
   void onControlStepIn(wxCommandEvent & /*e*/) { _gdbMgr->gdbStepIn(); }
   void onControlStepOut(wxCommandEvent & /*e*/) { _gdbMgr->gdbStepOut(); }
   void onControlContinue(wxCommandEvent &e);
   void onControlRunToLine(wxCommandEvent &e);
   void onControlInterrupt(wxCommandEvent & /*e*/) { _gdbMgr->stop(); }
   /* }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}*/

   /* {{{{{{{{{{{{{{{{{{{{ view menu handling {{{{{{{{{{{{{{{{{{{{ */
   void onViewNotebookBottomLeft(wxCommandEvent &e);
   /* }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}*/

   /* {{{{{{{{{{{{{{{{{{{{ help menu handling {{{{{{{{{{{{{{{{{{{{ */
   void onHelpSettings(wxCommandEvent &e);
   void onHelpAbout(wxCommandEvent &e);
   /* }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}*/

   /* {{{{{{{{{{{{{{{{{{{{ debugger events {{{{{{{{{{{{{{{{{{{{ */
   void onDebuggerStopped(DebuggerEvent &e);
   void onGdbLocalVariableChanged(DebuggerEvent &e);
   void onGdbStateChanged(DebuggerEvent &e);
   void onGdbSignalReceived(DebuggerEvent &e);
   void onGdbStopped(DebuggerEvent &e);
   void onGdbVariableWatchChanged(DebuggerEvent &e);
   void onGdbBreakpointsChanged(DebuggerEvent &e);
   void onGdbStackFrameChanged(DebuggerEvent &e);
   /* }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}*/

   void onToggleBreakpoint(wxCommandEvent &evt);
   void onRunToLine(wxCommandEvent &evt);

   wxAuiManager *               _aui_mgr                 = nullptr;
   wxAuiNotebook *              _aui_notebook            = nullptr;
   wxAuiNotebook *              _bottomLeftNotebook      = nullptr;
   wxAuiNotebook *              _bottomRightNotebook     = nullptr;
   GdbMgr *                     _gdbMgr                  = nullptr;
   BreakPointsListPanel *       _breakpointsListPanel    = nullptr;
   StackListPanel *             _stackListPanel          = nullptr;
   LocalVariablesPanel *        _localVariablesTreePanel = nullptr;
   SessionPanel *               _sessionPanel            = nullptr;
   wxString                     m_currentFile;
   int                          m_currentLine = 0;
   std::vector<StackFrameEntry> m_stackFrameList;
};
