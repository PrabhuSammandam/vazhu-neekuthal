#include "MainFrame.h"
#include "ID.h"
#include "jaeTextDocument.h"
#include "ui/BreakPointsPanel.h"
#include "ui/LocalVariablesPanel.h"
#include "ui/StackPanel.h"
#include <vector>
#include <wx/aui/auibook.h>

void MainFrame::onGdbStopped(DebuggerEvent &evt) {
   auto     reason       = (GdbMgr::StopReason)evt._reason;
   wxString path         = evt._fileName;
   int      lineNo       = evt._lineNo;
   wxString functionName = evt._functionName;
   int      stackDepth   = evt._stackDepth;

   if (reason == GdbMgr::EXITED_NORMALLY || reason == GdbMgr::EXITED) {
      wxString title = "Program exited";
      wxString text;
      if (reason == GdbMgr::EXITED_NORMALLY) {
         text = "Program exited normally";
      } else {
         text = "Program exited";
      }
      wxMessageBox(text, title, wxOK);
   }

   updateCurrentLine(path, lineNo);
   _gdbMgr->gdbGetStackFrames(0, 10);
}

void MainFrame::onGdbStateChanged(DebuggerEvent &e) {
   auto state = (GdbMgr::TargetState)e._newState;

   switch (state) {
   case GdbMgr::TARGET_DISCONNECTED:
   case GdbMgr::TARGET_CONNECTED:
   case GdbMgr::TARGET_FINISHED: {
      setMenuEnabled(MENU_DEBUG_ID_NEXT, false);
      setMenuEnabled(MENU_DEBUG_ID_STEP_IN, false);
      setMenuEnabled(MENU_DEBUG_ID_STEP_OUT, false);
      setMenuEnabled(MENU_DEBUG_ID_CONTINUE, false);
      setMenuEnabled(MENU_DEBUG_ID_RUN_TO_LINE, false);
      setMenuEnabled(MENU_DEBUG_ID_INTERRUPT, false);
      setMenuEnabled(MENU_DEBUG_ID_RUN, false);
      _stackListPanel->clearWidget();
      _localVariablesTreePanel->clearWidget();
      for (auto i = 0; i < _aui_notebook->GetPageCount(); i++) {
         auto *page = dynamic_cast<jaeTextDocument *>(_aui_notebook->GetPage(i));
         page->deleteAllMarkers();
      }
   } break;
   case GdbMgr::TARGET_SPECIFIED: {
      setMenuEnabled(MENU_DEBUG_ID_RUN, true);
      setMenuEnabled(MENU_DEBUG_ID_NEXT, false);
      setMenuEnabled(MENU_DEBUG_ID_STEP_IN, false);
      setMenuEnabled(MENU_DEBUG_ID_STEP_OUT, false);
      setMenuEnabled(MENU_DEBUG_ID_CONTINUE, false);
      setMenuEnabled(MENU_DEBUG_ID_RUN_TO_LINE, false);
      setMenuEnabled(MENU_DEBUG_ID_INTERRUPT, false);
   } break;
   case GdbMgr::TARGET_STOPPED: {
      setMenuEnabled(MENU_DEBUG_ID_RUN, false);
      setMenuEnabled(MENU_DEBUG_ID_NEXT, true);
      setMenuEnabled(MENU_DEBUG_ID_STEP_IN, true);
      setMenuEnabled(MENU_DEBUG_ID_STEP_OUT, true);
      setMenuEnabled(MENU_DEBUG_ID_CONTINUE, true);
      setMenuEnabled(MENU_DEBUG_ID_RUN_TO_LINE, true);
      setMenuEnabled(MENU_DEBUG_ID_INTERRUPT, false);
   } break;
   case GdbMgr::TARGET_RUNNING: {
      setMenuEnabled(MENU_DEBUG_ID_RUN, false);
      setMenuEnabled(MENU_DEBUG_ID_NEXT, false);
      setMenuEnabled(MENU_DEBUG_ID_STEP_IN, false);
      setMenuEnabled(MENU_DEBUG_ID_STEP_OUT, false);
      setMenuEnabled(MENU_DEBUG_ID_CONTINUE, false);
      setMenuEnabled(MENU_DEBUG_ID_RUN_TO_LINE, false);
      setMenuEnabled(MENU_DEBUG_ID_INTERRUPT, true);
      _stackListPanel->clearWidget();
      _localVariablesTreePanel->clearWidget();
   } break;
   }
}

void MainFrame::onGdbSignalReceived(DebuggerEvent &e) {
   wxString signalName = e._signalName;
   if (signalName != "SIGINT") {
      //
      wxString msgText;
      msgText << "Program received signal " << signalName;
      wxString title = "Signal received";
      wxMessageBox(msgText, title, wxOK);
   }

   disableCurrentLine();
   _gdbMgr->gdbGetStackFrames(0, 10);
}

void MainFrame::onGdbVariableWatchChanged(DebuggerEvent &e) { _localVariablesTreePanel->handleWatchChanged(e._variableWatch); }

void MainFrame::updateBreakpointsInCodeview() {
   for (auto i = 0; i < _aui_notebook->GetPageCount(); i++) {
      auto *page = dynamic_cast<jaeTextDocument *>(_aui_notebook->GetPage(i));

      std::vector<int> bpList;

      for (auto *bpObj : _gdbMgr->getBreakpointList()) {
         if (page->getFilePath() == bpObj->_fullname) {
            bpList.push_back(bpObj->_line);
         }
      }
      page->setBreakpoints(bpList);
      bpList.clear();
   }
}

void MainFrame::onGdbBreakpointsChanged(DebuggerEvent &e) {
   _breakpointsListPanel->handleBreakPointChanged(e._isBreakpointToSave);
   updateBreakpointsInCodeview();
}

void MainFrame::onGdbStackChanged(DebuggerEvent &evt) {
   m_stackFrameList = std::move(evt._stackFrameList);
   _stackListPanel->setStackFramesList(m_stackFrameList);
}

void MainFrame::onGdbStackFrameChanged(DebuggerEvent &evt) { openFile(evt._frameInfo.m_sourcePath, evt._frameInfo.m_line - 1); }

void MainFrame::onToggleBreakpoint(wxCommandEvent & /*evt*/) {
   int index = _aui_notebook->GetSelection();
   if (index != -1) {
      auto *page = dynamic_cast<jaeTextDocument *>(_aui_notebook->GetPage(index));

      if (page != nullptr) {
         auto line = page->LineFromPosition(page->CurrentPos());

         _gdbMgr->ToggleBreakpoint(page->getFilePath(), (int)line + 1);
      }
   }
}

void MainFrame::onRunToLine(wxCommandEvent & /*evt*/) {
   int index = _aui_notebook->GetSelection();
   if (index != -1) {
      auto *page = dynamic_cast<jaeTextDocument *>(_aui_notebook->GetPage(index));

      if (page != nullptr) {
         auto line = page->LineFromPosition(page->CurrentPos());

         _gdbMgr->gdbJump(page->getFilePath(), (int)line + 1);
      }
   }
}
