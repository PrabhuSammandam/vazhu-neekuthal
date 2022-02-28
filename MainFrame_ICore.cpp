#include "MainFrame.h"
#include "ID.h"
#include "jaeTextDocument.h"
#include "ui/BreakPointsPanel.h"
#include "ui/LocalVariablesPanel.h"
#include "ui/StackPanel.h"
#include <vector>
#include <wx/aui/auibook.h>

void MainFrame::onGdbStopped(DebuggerEvent &e) {
   auto     reason       = (GdbMgr::StopReason)e._reason;
   wxString path         = e._fileName;
   int      lineNo       = e._lineNo;
   wxString functionName = e._functionName;
   int      stackDepth   = e._stackDepth;

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

   //   std::cout << "onStopped file path" << path << std::endl;
   updateCurrentLine(path, lineNo);

   _gdbMgr->gdbGetStackFrames();
}

void MainFrame::onGdbStateChanged(DebuggerEvent &e) {
   auto state     = (GdbMgr::TargetState)e._newState;
   bool isRunning = !(state == GdbMgr::TARGET_STOPPED || state == GdbMgr::TARGET_FINISHED);
   bool isStopped = state == GdbMgr::TARGET_STOPPED;

   if (state == GdbMgr::TARGET_STARTING || state == GdbMgr::TARGET_RUNNING) {
      _stackListPanel->clearWidget();
   }

   if (state == GdbMgr::TARGET_STARTING || state == GdbMgr::TARGET_RUNNING) {
      _localVariablesTreePanel->Enable(false);
   } else {
      _localVariablesTreePanel->Enable(true);
   }

   setMenuEnabled(MENU_DEBUG_ID_NEXT, isStopped);
   setMenuEnabled(MENU_DEBUG_ID_STEP_IN, isStopped);
   setMenuEnabled(MENU_DEBUG_ID_STEP_OUT, isStopped);
   setMenuEnabled(MENU_DEBUG_ID_CONTINUE, isStopped);
   setMenuEnabled(MENU_DEBUG_ID_RUN_TO_LINE, isStopped);
   setMenuEnabled(MENU_DEBUG_ID_INTERRUPT, isRunning);
   setMenuEnabled(MENU_DEBUG_ID_RUN, !isRunning);
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
   _gdbMgr->gdbGetStackFrames();
}

void MainFrame::onGdbVariableWatchChanged(DebuggerEvent &e) { _localVariablesTreePanel->handleWatchChanged(e._variableWatch); }

void MainFrame::updateBreakpointsInCodeview() {
   for (auto i = 0; i < _aui_notebook->GetPageCount(); i++) {
      auto *page = dynamic_cast<jaeTextDocument *>(_aui_notebook->GetPage(i));

      std::vector<int> bpList;

      for (auto *bpObj : _gdbMgr->getBreakpointList()) {
         if (page->getFilePath() == bpObj->m_fullname) {
            bpList.push_back(bpObj->m_lineNo);
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

void MainFrame::onGdbStackFrameChanged(DebuggerEvent &e) {
   m_stackFrameList = e._stackFrameList;

   auto *listCtrl = _stackListPanel->getListCtrl();

   listCtrl->DeleteAllItems();

   for (auto i = 0; i < (int)m_stackFrameList.size(); i++) {
      auto item = m_stackFrameList.at(i);

      auto row = listCtrl->InsertItem(i, wxString::Format("%d", item._level));
      listCtrl->SetItem(row, 1, wxString::Format("%d", item.m_line));
      listCtrl->SetItem(row, 2, item.m_functionName);
      listCtrl->SetItem(row, 3, item.m_sourcePath);
      listCtrl->SetItemData(row, item._level);
   }
}

void MainFrame::onToggleBreakpoint(wxCommandEvent &evt) {
   int index = _aui_notebook->GetSelection();
   if (index != -1) {
      auto *page = dynamic_cast<jaeTextDocument *>(_aui_notebook->GetPage(index));

      if (page != nullptr) {
         auto line = page->LineFromPosition(page->CurrentPos());

         _gdbMgr->gdbToggleBreakpoint(page->getFilePath(), (int)line + 1);
      }
   }
}

void MainFrame::onRunToLine(wxCommandEvent &evt) {
   int index = _aui_notebook->GetSelection();
   if (index != -1) {
      auto *page = dynamic_cast<jaeTextDocument *>(_aui_notebook->GetPage(index));

      if (page != nullptr) {
         auto line = page->LineFromPosition(page->CurrentPos());

         _gdbMgr->gdbJump(page->getFilePath(), (int)line + 1);
      }
   }
}
