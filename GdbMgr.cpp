#include "GdbMgr.h"
#include "CustomEvents.h"
#include "MainFrame.h"
#include "iostream"
#include "processreaderthread.h"
#include "wx/app.h"
#include "wx/event.h"
#include "wx/gtk/app.h"
#include "wx/msgdlg.h"
#include "wx/process.h"
#include "wx/string.h"
#include "wx/timer.h"
#include "wx/utils.h"
#include "wx/window.h"
#include <algorithm>
#include <ostream>
#include <unordered_map>
#include <wx/regex.h>
#include <wx/sstream.h>
#include <wx/tokenzr.h>
#include <wx/txtstrm.h>

static auto MakeId() -> wxString {
   static unsigned int counter(0);
   wxString            newId;
   newId.Printf(wxT("%08u"), ++counter);
   return newId;
}

GdbMgr::GdbMgr() {
   m_goingDown = false;
   Bind(wxEVT_PROCESS_OUTPUT, &GdbMgr::OnGdbProcessData, this);
   Bind(wxEVT_PROCESS_TERMINATED, &GdbMgr::OnGdbProcessEnd, this);
}

void GdbMgr::OnGdbProcessEnd(AsyncProcessEvent & /*e*/) {
   wxDELETE(m_gdbProcess);

   m_targetState             = GdbMgr::TARGET_STOPPED;
   m_lastTargetState         = GdbMgr::TARGET_STOPPED;
   m_gdbOutputIncompleteLine = "";
   _stackDepth               = 0;
   _currentFunctionName      = "";
   _currentFilePath          = "";
   m_pid                     = 0;

   m_pendingComands.clear();
   m_gdbOutputArr.clear();
   m_localVars.clear();
   m_breakpoints.clear();
   m_threadList.clear();
   m_sourceFiles.clear();
   _cmdHandlers.clear();

   DebuggerEvent debugEvent{wxEVT_DEBUGGER_GDB_PROCESS_ENDED};
   AddPendingEvent(debugEvent);
}

auto GdbMgr::isValidGdbOutput(wxString gdbOutputStr) -> bool {
   auto strLen    = (int)gdbOutputStr.Len();
   char firstChar = gdbOutputStr[0];

   if (firstChar == '(') {
      return true;
   }
   if (firstChar == '^') {
      return true;
   }
   if (firstChar == '*') {
      return true;
   }
   if (firstChar == '+') {
      return true;
   }
   if (firstChar == '=') {
      return true;
   }
   if (firstChar == '&' || firstChar == '~' || firstChar == '@') {
      if (strLen > 2) {
         char secondChar = gdbOutputStr[1];
         char thirdChar  = gdbOutputStr[2];

         if (secondChar == '\\' && thirdChar == '"') {
            return true;
         }
      }
      return false;
   }

   if (strLen > 9 && gdbOutputStr[8] == '^') {
      auto startResult = gdbOutputStr.SubString(9, 50);

      if (startResult.StartsWith("done") || startResult.StartsWith("running") || startResult.StartsWith("connected") || startResult.StartsWith("error") || startResult.StartsWith("exit")) {
         return true;
      }
   }
   return false;
}

void GdbMgr::OnGdbProcessData(AsyncProcessEvent &e) {
   if ((m_gdbProcess == nullptr) || !m_gdbProcess->IsAlive()) {
      return;
   }
   const wxString &bufferRead = e.GetOutput();
   //   std::cout << bufferRead << std::endl;
   wxArrayString lines = wxStringTokenize(bufferRead, "\n", wxTOKEN_STRTOK);

   if (lines.IsEmpty()) {
      return;
   }
   // Prepend the partially saved line from previous iteration to the first line of this iteration
   if (!m_gdbOutputIncompleteLine.empty()) {
      lines.Item(0).Prepend(m_gdbOutputIncompleteLine);
      m_gdbOutputIncompleteLine.Clear();
   }
   // If the last line is in-complete, remove it from the array and keep it for next iteration
   if (!bufferRead.EndsWith(wxT("\n"))) {
      m_gdbOutputIncompleteLine = lines.Last();
      lines.RemoveAt(lines.GetCount() - 1);
   }
   for (const auto &line : lines) {
      if (line.IsEmpty() == false) {
         m_gdbOutputArr.Add(line);
         if (line.Contains("done,stack=[frame") || line.Contains("^done,threads=[")) {
            continue;
         }
         std::cout << line << std::endl;
      }
   }
   for (const auto &row : m_gdbOutputArr) {
      if (isValidGdbOutput(row)) {
         auto *resp = GdbParser(row).tokenize().parseOutput();

         if (resp->_cmdIdAvailable && _cmdHandlers.find(resp->_cmdId) != _cmdHandlers.end()) {
            auto *handler = removeCmdHandler(resp->_cmdId);

            if (handler != nullptr) {
               handler->processResponse(resp);
               delete resp;
               delete handler;
            }
         } else {
            dispatchResponse(resp);
         }
      } else {
         onTargetStreamOutput(row);
      }
   }
   m_gdbOutputArr.clear();
}

void GdbMgr::quit() {
   auto pid = m_gdbProcess->GetPid();
   wxKill(pid, wxSIGKILL, nullptr, wxKILL_CHILDREN);
   _sessionInfo.clear();
}

auto GdbMgr::startGdbProcess(GdbSessionInfo &sessionInfo) {
   wxString gdbCommand = sessionInfo._gdbExePath + sessionInfo._gdbArgs + " --silent --nh";
   m_goingDown         = false;
   m_gdbProcess        = ::CreateAsyncProcess(this, gdbCommand, IProcessCreateDefault | IProcessStderrEvent, sessionInfo._exeWorkingDir);

   m_gdbProcess->SetHardKill(true);

   writeCommand("-enable-pretty-printing");
   writeCommand("-gdb-set mi-async on");
}

auto GdbMgr::isValidSession() const -> bool { return _sessionInfo._isValid; }

auto GdbMgr::initExeSession(GdbSessionInfo &sessionInfo) -> int {
   _sessionInfo._isValid = false;

   startGdbProcess(sessionInfo);
   writeCommand("-file-exec-and-symbols " + sessionInfo._exePath);

   if (!sessionInfo._exeArgs.IsEmpty()) {
      writeCommand("-exec-arguments " + sessionInfo._exeArgs);
   }

   if (!sessionInfo._exeWorkingDir.IsEmpty()) {
      writeCommand("-environment-cd " + sessionInfo._exeWorkingDir);
   }

   gdbGetFiles();

   _sessionInfo          = sessionInfo;
   _sessionInfo._isValid = true;

   loadBreakpoints();

   return 0;
}

auto GdbMgr::initAttachPidSession(GdbSessionInfo &sessionInfo) -> int { return -1; }

auto GdbMgr::initCoredumpSession(GdbSessionInfo &sessionInfo) -> int {
   _sessionInfo._isValid = false;

   startGdbProcess(sessionInfo);
   writeCommand("-file-exec-and-symbols " + sessionInfo._exePath);
   writeCommand("-target-select core " + sessionInfo._coredumpPath);

   gdbGetFiles();

   _sessionInfo          = sessionInfo;
   _sessionInfo._isValid = true;

   writeCommand("-stack-list-variables --no-values");
   gdbGetStackFrames();

   m_targetState = GdbMgr::TARGET_RUNNING;
   auto e        = DebuggerEvent{wxEVT_DEBUGGER_GDB_STATE_CHANGED};
   e._newState   = GdbMgr::TARGET_FINISHED;
   ProcessEvent(e);

   return 0;
}

auto GdbMgr::initRemoteSession(GdbSessionInfo &sessionInfo) -> int {
   _sessionInfo._isValid = false;

   startGdbProcess(sessionInfo);
   writeCommand("-file-exec-and-symbols " + sessionInfo._exePath);

   if (!sessionInfo._exeArgs.IsEmpty()) {
      writeCommand("-exec-arguments " + sessionInfo._exeArgs);
   }

   if (!sessionInfo._exeWorkingDir.IsEmpty()) {
      writeCommand("-environment-cd " + sessionInfo._exeWorkingDir);
   }

   writeCommand("set sysroot /auto/spvss-evo-bin1/Published/code/compilers/stbgcc-6.3-1.8/aarch64-unknown-linux-gnueabi/sys-root");
   writeCommand("set solib-search-path "
                "/mnt/nfs_share/NDS/lib:/mnt/nfs_share/NDS/libs:/home/psammandam/Downloads/BEIN/releases/20220107_ES3_CDI_FORMAL_Driver_v1.4/03.Modules/Soft/debug/LIB/:/home/psammandam/Downloads/"
                "BEIN/releases/20220107_ES3_CDI_FORMAL_Driver_v1.4/05.Libraries/Soft/debug/lib/:/home/psammandam/Downloads/BEIN/releases/20220107_ES3_CDI_FORMAL_Driver_v1.4/05.Libraries/Soft/debug/"
                "xwifi/lib/modules/:/home/psammandam/Downloads/BEIN/releases/20220107_ES3_CDI_FORMAL_Driver_v1.4/05.Libraries/Soft/debug/xwifi/lib/");
   writeCommand(wxString::Format("-target-select extended-remote %s:%d", sessionInfo._remoteHost, (int)sessionInfo._remotePort));
   writeCommand("set scheduler-locking step ");

   // gdbGetFiles();

   writeCommand("-info-os processes", new DebugCmdProcessPids{this});
   _sessionInfo          = sessionInfo;
   _sessionInfo._isValid = true;
   m_isRemote            = true;

   // loadBreakpoints();

   return 0;
}

void GdbMgr::dispatchResponse(GdbResponse *resp) {
   if (resp == nullptr) {
      return;
   }
   switch (resp->getType()) {
   case GdbResponse::EXEC_ASYNC_OUTPUT:
      onExecAsyncOut(resp->tree, resp->reason);
      break;
   case GdbResponse::STATUS_ASYNC_OUTPUT:
      onStatusAsyncOut(resp->tree, resp->reason);
      break;
   case GdbResponse::NOTIFY_ASYNC_OUTPUT:
      onNotifyAsyncOut(resp->tree, resp->reason);
      break;
   case GdbResponse::LOG_STREAM_OUTPUT:
      onLogStreamOutput(resp->getString());
      break;
   case GdbResponse::TARGET_STREAM_OUTPUT:
      onTargetStreamOutput(resp->getString());
      break;
   case GdbResponse::CONSOLE_STREAM_OUTPUT:
      onConsoleStreamOutput(resp->getString());
      break;
   case GdbResponse::RESULT:
      onResult(resp->tree);
      break;
   default:
      break;
   }
   delete resp;
}

void GdbMgr::writeCommand(const wxString &command) { writeCommand(command, nullptr); }

void GdbMgr::writeCommand(const wxString &command, DebugCmdHandler *handler) {
   wxString cmd;
   wxString id = MakeId();
   cmd << id << command;

   m_gdbProcess->Write(cmd);

   if (handler != nullptr) {
      addCmdHandler(id, handler);
   }
   std::cout << cmd << std::endl;
}

auto GdbMgr::parseReasonString(const wxString &reasonString) -> GdbMgr::StopReason {
   if (reasonString == "breakpoint-hit") {
      return GdbMgr::BREAKPOINT_HIT;
   }
   if (reasonString == "end-stepping-range") {
      return GdbMgr::END_STEPPING_RANGE;
   }
   if (reasonString == "signal-received" || reasonString == "exited-signalled") {
      return GdbMgr::SIGNAL_RECEIVED;
   }
   if (reasonString == "exited-normally") {
      return GdbMgr::EXITED_NORMALLY;
   }
   if (reasonString == "function-finished") {
      return GdbMgr::FUNCTION_FINISHED;
   }
   if (reasonString == "exited") {
      return GdbMgr::EXITED;
   }

   return GdbMgr::UNKNOWN;
}

void GdbMgr::onStatusAsyncOut(Tree &tree, GDB_ASYNC_CLASS ac) {}

void GdbMgr::onNotifyAsyncOut(Tree &tree, GDB_ASYNC_CLASS ac) {
   if (ac == GDB_ASYNC_CLASS::BREAKPOINT_DELETED) {
      int id = tree.getInt("id");
      processBreakpointDeleted(id);
   } else if (ac == GDB_ASYNC_CLASS::BREAKPOINT_MODIFIED) {
      for (int i = 0; i < tree.getRootChildCount(); i++) {
         TreeNode *rootNode = tree.getChildAt(i);
         wxString  rootName = rootNode->getName();

         if (rootName == "bkpt") {
            processBreakpointChanged(tree);
         }
      }
   } else if (ac == GDB_ASYNC_CLASS::THREAD_CREATED) {
      if (checkGdbRunningOrStarting() == false) {
         writeCommand("-thread-info", new DebugCmdGetThreadList{this});
      }
   } else if (ac == GDB_ASYNC_CLASS::LIBRARY_LOADED) {
      m_scanSources = true;
   } else if (ac == GDB_ASYNC_CLASS::THREAD_GROUP_STARTED) {
      m_pid = tree.getInt("pid");
   }
}

void GdbMgr::onExecAsyncOut(Tree &tree, GDB_ASYNC_CLASS ac) {
   // The program has stopped
   if (ac == GDB_ASYNC_CLASS::STOPPED) {
      m_targetState = GdbMgr::TARGET_STOPPED;

      if (m_pid == 0) {
         // CHECK writeCommand("-list-thread-groups"); // get the debuggee pid
      }

      // Get the reason
      wxString           reasonString = tree.getString("reason");
      GdbMgr::StopReason reason       = reasonString.IsEmpty() ? GdbMgr::UNKNOWN : parseReasonString(reasonString);

      if (reason == GdbMgr::BREAKPOINT_HIT || reason == GdbMgr::END_STEPPING_RANGE) {
         _currentFunctionName = tree.getString("frame/func");
         _currentFilePath     = tree.getString("frame/fullname");
      }

      if (reason == GdbMgr::EXITED_NORMALLY || reason == GdbMgr::EXITED) {
         m_targetState = GdbMgr::TARGET_FINISHED;
      }

      wxString fileName = tree.getString("frame/fullname");
      int      lineNo   = tree.getInt("frame/line");

      if (reason == GdbMgr::SIGNAL_RECEIVED) {
         wxString signalName = tree.getString("signal-name");
         if (signalName == "SIGTRAP" && m_isRemote) {
            auto e = DebuggerEvent::makeGdbStoppedEvent(reason, fileName, _currentFunctionName, _stackDepth, lineNo);
            ProcessEvent(e);
         } else {
            if (signalName == "SIGSEGV") {
               m_targetState = GdbMgr::TARGET_FINISHED;
            }
            DebuggerEvent e{wxEVT_DEBUGGER_GDB_SIGNALED};
            e._signalName = signalName;
            ProcessEvent(e);
         }
      } else {
         if (m_targetState != GdbMgr::TARGET_FINISHED) {
            // writeCommand("-thread-info", new DebugCmdGetThreadList{this});
            // writeCommand("-var-update --all-values *"); // this is for custom added watch
            writeCommand("-stack-info-depth");
            writeCommand("-stack-list-variables --no-values");
         }
         auto e = DebuggerEvent::makeGdbStoppedEvent(reason, fileName, _currentFunctionName, _stackDepth, lineNo);
         ProcessEvent(e);
      }

      m_currentFrameIdx = tree.getInt("frame/level");

      auto e = DebuggerEvent::makeFrameIdChangedEvent((int)m_currentFrameIdx);
      ProcessEvent(e);

   } else if (ac == GDB_ASYNC_CLASS::RUNNING) {
      m_targetState = GdbMgr::TARGET_RUNNING;
   }

   // Get the current thread
   wxString threadIdStr = tree.getString("thread-id");
   if (threadIdStr.IsEmpty() == false) {
      long threadId = 0;
      threadIdStr.ToLong(&threadId);

      auto e = DebuggerEvent::makeThreadIdChangedEvent((int)threadId);
      ProcessEvent(e);
   }

   if (m_lastTargetState != m_targetState) {
      auto e      = DebuggerEvent{wxEVT_DEBUGGER_GDB_STATE_CHANGED};
      e._newState = m_targetState;

      ProcessEvent(e);
      m_lastTargetState = m_targetState;
   }
}

void GdbMgr::onResult(Tree &tree) {
   for (int treeChildIdx = 0; treeChildIdx < tree.getRootChildCount(); treeChildIdx++) {
      TreeNode *rootNode = tree.getChildAt(treeChildIdx);
      wxString  rootName = rootNode->getName();

      if (rootName == "changelist") {
         for (int j = 0; j < rootNode->getChildCount(); j++) {
            TreeNode *child       = rootNode->getChild(j);
            wxString  watchId     = child->getChildDataString("name");
            bool      typeChanged = child->getChildDataString("type_changed") == "true";
            VarWatch *varWatchObj = getVarWatchInfo(watchId);

            // If the type has changed then all of the children must be removed.
            if (varWatchObj != nullptr && typeChanged) {
               wxString                varName    = varWatchObj->getName();
               std::vector<VarWatch *> removeList = getWatchChildren(*varWatchObj);

               for (auto &cidx : removeList) {
                  gdbRemoveVarWatch(cidx->getWatchId());
               }

               varWatchObj->setValue("");
               varWatchObj->setVarType(child->getChildDataString("new_type"));
               varWatchObj->setHasChildren(child->getChildDataInt("new_num_children") > 0);

               auto e = DebuggerEvent::makeVariableWatchChangedEvent(varWatchObj);
               ProcessEvent(e);
            }
            // value changed?
            else if (varWatchObj != nullptr) {
               auto     newValue   = child->getChildDataString("value");
               wxString inScopeStr = child->getChildDataString("in_scope");

               varWatchObj->setValue(newValue);
               varWatchObj->setInScope((inScopeStr == "true" || inScopeStr.IsEmpty()));

               if (varWatchObj->getValue() == "{...}" && varWatchObj->hasChildren() == false) {
                  varWatchObj->setHasChildren(true);
               }
               auto e = DebuggerEvent::makeVariableWatchChangedEvent(varWatchObj);
               ProcessEvent(e);
            }
         }
      } else if (rootName == "current-thread-id") {
         // Get the current thread
         int threadId = rootNode->getDataInt(-1);
         if (threadId != -1) {
            auto e = DebuggerEvent::makeThreadIdChangedEvent((int)threadId);
            ProcessEvent(e);
         }
      } else if (rootName == "depth") {
         _stackDepth = rootNode->getDataInt(-1);
      } else if (rootName == "frame") {
         m_currentFrameIdx = rootNode->getChildDataInt("level");

         auto e = DebuggerEvent::makeGdbStoppedEvent(GdbMgr::UNKNOWN, rootNode->getChildDataString("fullname"), _currentFunctionName, _stackDepth, rootNode->getChildDataInt("line"));
         ProcessEvent(e);

         DebuggerEvent ee{wxEVT_DEBUGGER_GDB_FRAME_VARIABLE_RESET};
         ProcessEvent(ee);

         TreeNode *argsNode = rootNode->findChild("args");

         if (argsNode != nullptr) {
            for (int i = 0; i < argsNode->getChildCount(); i++) {
               TreeNode *child = argsNode->getChild(i);

               DebuggerEvent e{wxEVT_DEBUGGER_GDB_FRAME_VARIABLE_CHANGED};
               e._variableName  = child->getChildDataString("name");
               e._variableValue = child->getChildDataString("value");
               ProcessEvent(e);
            }
         }
      }
      // Local variables?
      else if (rootName == "variables") {
         wxArrayString localVariableList;

         for (int j = 0; j < rootNode->getChildCount(); j++) {
            TreeNode *child   = rootNode->getChild(j);
            wxString  varName = child->getChildDataString("name");

            localVariableList.push_back(varName);
         }

         DebuggerEvent e{wxEVT_DEBUGGER_GDB_LOCAL_VARIABLE_CHANGED, localVariableList, _currentFilePath, _currentFunctionName, _stackDepth};
         ProcessEvent(e);
      } else if (rootName == "msg") {
         wxString message = rootNode->getData();
         auto     e       = DebuggerEvent::makeMessageEvent(message);
         ProcessEvent(e);
      } else if (rootName == "groups") {
         if (m_pid == 0 && rootNode->getChildCount() > 0) {
            TreeNode *firstChild = rootNode->getChild(0);
            m_pid                = firstChild->getChildDataInt("pid", 0);
         }
      }
   }
}

void GdbMgr::onConsoleStreamOutput(wxString str) {
   str.Replace("\r", "");
   wxStringTokenizer tokenizer{str, '\n'};

   while (tokenizer.HasMoreTokens()) {
      wxString text = tokenizer.GetNextToken();
      if (text.IsEmpty() /* && i + 1 == list.size() */) {
         continue;
      }

      auto e = DebuggerEvent::makeConsoleLogEvent(text);
      ProcessEvent(e);
   }
}

void GdbMgr::onTargetStreamOutput(wxString str) { std::cout << str << std::endl; }

void GdbMgr::onLogStreamOutput(wxString str) {}

void GdbMgr::processBreakpointDeleted(int id) {
   auto found_itr = std::find_if(m_breakpoints.begin(), m_breakpoints.end(), [id](BreakPoint *bp) { return bp->m_number == id; });

   if (found_itr != m_breakpoints.end()) {
      auto *bp = *found_itr;

      m_breakpoints.erase(found_itr);
      delete bp;

      DebuggerEvent e{wxEVT_DEBUGGER_GDB_BREAKPOINT_CHANGED};
      ProcessEvent(e);
   }
}

void GdbMgr::processBreakpointChanged(Tree &tree, bool isToSave) {
   TreeNode *rootNode = tree.findChild("bkpt");

   if (rootNode == nullptr) {
      return;
   }

   int         breakpointNo  = rootNode->getChildDataInt("number");
   BreakPoint *breakpointObj = nullptr;
   auto        found_itr     = std::find_if(m_breakpoints.begin(), m_breakpoints.end(), [breakpointNo](BreakPoint *bp) { return bp->m_number == breakpointNo; });

   if (found_itr == m_breakpoints.end()) {
      breakpointObj = new BreakPoint(breakpointNo);
      m_breakpoints.push_back(breakpointObj);
   } else {
      breakpointObj = *found_itr;
   }

   breakpointObj->m_lineNo   = rootNode->getChildDataInt("line");
   breakpointObj->m_fullname = rootNode->getChildDataString("fullname");

   // We did not receive 'fullname' from gdb.
   // Lets try original-location instead...
   if (breakpointObj->m_fullname.IsEmpty()) {
      wxString orgLoc = rootNode->getChildDataString("original-location");
      int      divPos = orgLoc.Find(':', true);
      if (divPos != -1) {
         breakpointObj->m_fullname = orgLoc.Left(divPos);
      }
   }
   breakpointObj->m_funcName = rootNode->getChildDataString("func");
   breakpointObj->m_addr     = rootNode->getChildDataLongLong("addr");
   breakpointObj->_times     = rootNode->getChildDataInt("times");

   if (rootNode->findChild("enabled") != nullptr) {
      auto enabled              = rootNode->getChildDataString("enabled");
      breakpointObj->_isEnabled = enabled == "y";
   }

   if (rootNode->findChild("disp") != nullptr) {
      auto disp              = rootNode->getChildDataString("disp");
      breakpointObj->_isTemp = disp == "del";
   }

   DebuggerEvent e{wxEVT_DEBUGGER_GDB_BREAKPOINT_CHANGED};
   e._isBreakpointToSave = false;
   ProcessEvent(e);
}

auto GdbMgr::gdbGetFiles() -> bool {
   writeCommand(R"(-file-list-exec-source-files)", new DebugCmdHandlerGetFiles{this});
   return false;
}

auto GdbMgr::getThreadList() -> std::vector<ThreadInfo> {
   std::vector<ThreadInfo> list;
   for (const auto &it : m_threadList) {
      list.push_back(it.second);
   }
   return list;
}

void GdbMgr::loadBreakpoints() {
   wxFileConfig fc{wxEmptyString, wxEmptyString, _sessionInfo._sessionFullPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE};

   fc.SetPath("/breakpoints");
   auto count = fc.ReadLong("count", 0);

   for (auto i = 0; i < count; i++) {
      auto bpPath = wxString::Format("bp%d", i);
      fc.SetPath("/breakpoints/" + bpPath);
      auto lineNo           = fc.Read("lineNo");
      auto fileName         = fc.Read("file");
      auto isTemp           = fc.ReadBool("temp", false);
      auto isEnabled        = fc.ReadBool("enabled", false);
      auto lineSpecLocation = fileName + ":" + lineNo;

      writeCommand(wxString::Format("-break-insert %s %s %s", isTemp ? "-t" : "", isEnabled ? "" : "-d", lineSpecLocation), new DebugCmdBreakpoint{this, false});
   }
   fc.Flush();
}

auto GdbMgr::getBreakpointList() -> std::vector<BreakPoint *> & { return m_breakpoints; }

void GdbMgr::removeBreakpoint(BreakPoint *bpObj) {
   auto fountItr = std::find_if(m_breakpoints.begin(), m_breakpoints.end(), [bpObj](auto *t) { return bpObj == t; });
   if (fountItr != m_breakpoints.end()) {
      m_breakpoints.erase(fountItr);
      delete *fountItr;
   }
}

auto GdbMgr::checkGdbRunningOrStarting() -> bool {
   if (m_targetState == GdbMgr::TARGET_STARTING || m_targetState == GdbMgr::TARGET_RUNNING) {
      auto e = DebuggerEvent::makeMessageEvent("Program is currently running");
      ProcessEvent(e);
      return true;
   }
   return false;
}

void GdbMgr::ensureStopped() {
   if (m_targetState == GdbMgr::TARGET_RUNNING) {
      stop();
   }
}

void GdbMgr::stop() {
   if (!_sessionInfo._isValid) {
      return;
   }
   if (m_targetState != GdbMgr::TARGET_RUNNING) {
      auto e = DebuggerEvent::makeMessageEvent("Program is not running");
      ProcessEvent(e);
      return;
   }
   if (m_isRemote) {
      // Send 'kill' to interrupt gdbserver
      if (m_gdbProcess->GetPid() != 0) {
         wxProcess::Kill(static_cast<int>(m_gdbProcess->GetPid()), wxSIGINT, wxKILL_NOCHILDREN);
      }

      writeCommand("-exec-interrupt --all");
      writeCommand("-exec-step-instruction");
   } else {
      if (m_pid != 0) {
         wxProcess::Kill(m_pid, wxSIGINT);
      }
   }
}

auto GdbMgr::gdbJump(const wxString &fileName, int lineNo) -> int {
   if (m_targetState != GdbMgr::TARGET_STOPPED) {
      auto e = DebuggerEvent::makeMessageEvent("Program is not stopped");
      ProcessEvent(e);
      return -3;
   }

   writeCommand(wxString::Format("-break-insert -t %s:%d", fileName, lineNo), new DebugCmdBreakpoint{this});
   writeCommand(wxString::Format("-exec-jump %s:%d", fileName, lineNo));

   return 0;
}

void GdbMgr::gdbStepIn() {
   if (!_sessionInfo._isValid) {
      return;
   }
   if (m_targetState != GdbMgr::TARGET_STOPPED) {
      auto e = DebuggerEvent::makeMessageEvent("Program is not stopped");
      ProcessEvent(e);
      return;
   }

   writeCommand("-exec-step");
   writeCommand("-var-update --all-values *");
}

void GdbMgr::gdbStepOut() {
   if (!_sessionInfo._isValid) {
      return;
   }
   if (m_targetState != GdbMgr::TARGET_STOPPED) {
      auto e = DebuggerEvent::makeMessageEvent("Program is not stopped");
      ProcessEvent(e);
      return;
   }

   writeCommand("-exec-finish");
   writeCommand("-var-update --all-values *");
}

void GdbMgr::gdbNext() {
   if (!_sessionInfo._isValid) {
      return;
   }
   if (m_targetState != GdbMgr::TARGET_STOPPED) {
      auto e = DebuggerEvent::makeMessageEvent("Program is not stopped");
      ProcessEvent(e);
      return;
   }

   writeCommand("-exec-next");
}

void GdbMgr::gdbContinue() {
   if (!_sessionInfo._isValid) {
      return;
   }
   if (m_targetState != GdbMgr::TARGET_STOPPED) {
      auto e = DebuggerEvent::makeMessageEvent("Program is not stopped");
      ProcessEvent(e);
      return;
   }

   writeCommand("-exec-continue");
}

void GdbMgr::gdbRun() {
   if (!_sessionInfo._isValid) {
      return;
   }
   if (checkGdbRunningOrStarting()) {
      return;
   }
   m_pid         = 0;
   m_targetState = GdbMgr::TARGET_STARTING;

   writeCommand(wxString::Format("-exec-run %s", _sessionInfo._isBreakAtMain ? "--start" : ""));
}

auto GdbMgr::gdbSetBreakpointAtFunc(const wxString &func) -> int {
   ensureStopped();
   writeCommand(wxString::Format("-break-insert -f %s", func), new DebugCmdBreakpoint{this});

   return 0;
}

auto GdbMgr::gdbSetBreakpoint(const wxString &filename, int lineNo) -> int {
   assert(!filename.IsEmpty());

   if (filename.IsEmpty()) {
      return -1;
   }

   ensureStopped();
   writeCommand(wxString::Format("-break-insert %s:%d", filename, lineNo), new DebugCmdBreakpoint{this});
   return 0;
}

auto GdbMgr::gdbToggleBreakpoint(const wxString &filename, int lineNo) -> int {
   assert(!filename.IsEmpty());

   if (filename.IsEmpty()) {
      return -1;
   }

   ensureStopped();

   for (auto *bp : m_breakpoints) {
      if (bp->m_fullname == filename && bp->m_lineNo == lineNo) {
         gdbRemoveBreakpoint(bp);
         return 0;
      }
   }
   gdbSetBreakpoint(filename, lineNo);
   return 0;
}

auto GdbMgr::gdbSetBreakpoint(const wxString &lineSpecLocation, bool isTemp, bool isDisabled) -> int {
   //   ensureStopped();

   // -break-insert main
   //^done,bkpt={number="1",addr="0x0001072c",file="recursive2.c",fullname="/home/foo/recursive2.c,line="4",thread-groups=["i1"],times="0"}
   writeCommand(wxString::Format("-break-insert %s %s %s", isTemp ? "-t" : "", isDisabled ? "-d" : "", lineSpecLocation), new DebugCmdBreakpoint{this});

   return 0;
}

void GdbMgr::gdbRemoveAllBreakpoints() {
   ensureStopped();

   // Get id for all breakpoints
   std::vector<int> idList;

   for (auto *bkpt : m_breakpoints) {
      idList.push_back(bkpt->m_number);
      delete bkpt;
   }
   m_breakpoints.clear();

   for (int id : idList) {
      writeCommand(wxString::Format("-break-delete %d", id));
   }

   DebuggerEvent e{wxEVT_DEBUGGER_GDB_BREAKPOINT_CHANGED};
   ProcessEvent(e);
}

void GdbMgr::gdbRemoveBreakpoint(BreakPoint *bkpt) {
   ensureStopped();
   writeCommand(wxString::Format("-break-delete %d", bkpt->m_number));

   removeBreakpoint(bkpt);

   DebuggerEvent e{wxEVT_DEBUGGER_GDB_BREAKPOINT_CHANGED};
   ProcessEvent(e);
}

void GdbMgr::gdbEnableBreakpoint(std::vector<BreakPoint *> &bkpt) {
   ensureStopped();
   wxString bpStr = "";

   for (auto *bp : bkpt) {
      bpStr << bp->m_number << " ";
      bp->_isEnabled = true;
   }
   writeCommand(wxString::Format("-break-enable %s", bpStr));

   DebuggerEvent e{wxEVT_DEBUGGER_GDB_BREAKPOINT_CHANGED};
   ProcessEvent(e);
}

void GdbMgr::gdbDisableBreakpoint(std::vector<BreakPoint *> &bkpt) {
   ensureStopped();
   wxString bpStr = "";

   for (auto *bp : bkpt) {
      bpStr << bp->m_number << " ";
      bp->_isEnabled = false;
   }
   writeCommand(wxString::Format("-break-disable %s", bpStr));

   DebuggerEvent e{wxEVT_DEBUGGER_GDB_BREAKPOINT_CHANGED};
   ProcessEvent(e);
}

void GdbMgr::gdbEnableAllBreakpoints() { gdbEnableBreakpoint(m_breakpoints); }

void GdbMgr::gdbDisableAllBreakpoints() { gdbDisableBreakpoint(m_breakpoints); }

void GdbMgr::gdbGetThreadList() {
   if (checkGdbRunningOrStarting() == false) {
      writeCommand("-thread-info", new DebugCmdGetThreadList{this});
   }
}

void GdbMgr::gdbGetStackFrames() {
   //   return;
   writeCommand("-stack-list-frames", new DebugCmdHandlerStackList{this});
}

auto GdbMgr::gdbAddVariableWatch(const wxString &variableName) -> bool {
   if (variableName.IsEmpty()) {
      return false;
   }

   wxString watchId = wxString::Format("w%d", m_varWatchLastId++);
   writeCommand(wxString::Format("--var-create %s @ %s", watchId, variableName));

   return true;
}

void GdbMgr::gdbSelectThread(int threadId) {
   if (m_selectedThreadId != threadId) {
      writeCommand(wxString::Format("-thread-select %d", threadId), new DebugCmdSelectThread{this});
   }
}

void GdbMgr::gdbSelectFrame(int selectedFrameIdx) {
   Tree resultData;

   if (checkGdbRunningOrStarting() == true) {
      return;
   }

   if (m_currentFrameIdx != selectedFrameIdx) {
      writeCommand(wxString::Format("-stack-select-frame %d", selectedFrameIdx));
      writeCommand("-stack-info-frame");
      writeCommand("-stack-list-variables --no-values");
   }
}

auto GdbMgr::getVarWatchInfo(const wxString &watchId) -> VarWatch * {
   auto it = std::find_if(m_watchList.begin(), m_watchList.end(), [watchId](VarWatch *vw) { return vw->getWatchId() == watchId; });
   return (it == m_watchList.end()) ? nullptr : *it;
}

void GdbMgr::gdbRemoveVarWatch(const wxString &watchId) {
   ensureStopped();

   // Remove from the list
   auto it = std::find_if(m_watchList.begin(), m_watchList.end(), [watchId](VarWatch *vw) { return vw->getWatchId() == watchId; });
   if (it != m_watchList.end()) {
      auto *watch = *it;
      m_watchList.erase(it);
      delete watch;
   }

   // Remove children first
   std::vector<wxString> removeList;

   for (auto *childWatch : m_watchList) {
      if (childWatch->getParentWatchId() == watchId) {
         removeList.push_back(childWatch->getWatchId());
      }
   }

   for (auto &i : removeList) {
      gdbRemoveVarWatch(i);
   }

   writeCommand(wxString::Format("-var-delete %s", watchId));
}

auto GdbMgr::gdbChangeWatchVariable(const wxString &watchId, const wxString &newValue) -> int {
   if (checkGdbRunningOrStarting()) {
      return 1;
   }
   writeCommand(wxString::Format("-var-assign %s %s", watchId, newValue));
   return 0;
}

auto GdbMgr::getWatchChildren(VarWatch &parentWatch) -> std::vector<VarWatch *> {
   std::vector<VarWatch *> list;

   for (auto *loopWatch : m_watchList) {
      if (parentWatch.getWatchId() == loopWatch->getParentWatchId()) {
         list.push_back(loopWatch);
      }
   }
   return list;
}

void GdbMgr::addCmdHandler(const wxString &id, DebugCmdHandler *handler) { _cmdHandlers[id] = handler; }

auto GdbMgr::removeCmdHandler(const wxString &id) -> DebugCmdHandler * {
   DebugCmdHandler *handler = nullptr;
   auto             it      = _cmdHandlers.find(id);

   if (it != _cmdHandlers.end()) {
      handler = it->second;
      _cmdHandlers.erase(id);
   }
   return handler;
}

void GdbMgr::clearCmdHandlers() {
   auto iter = _cmdHandlers.begin();

   while (iter != _cmdHandlers.end()) {
      delete iter->second;
      iter++;
   }
   _cmdHandlers.clear();
}

auto DebugCmdProcessPids::processResponse(GdbResponse *response) -> bool {
   auto &tree = response->tree;
   for (int treeChildIdx = 0; treeChildIdx < tree.getRootChildCount(); treeChildIdx++) {
      TreeNode *rootNode = tree.getChildAt(treeChildIdx);
      wxString  rootName = rootNode->getName();
      bool      found    = false;

      if (rootName == "OSDataTable") {
         auto *bodyRootNode = rootNode->findChild("body");

         for (int i = 0; i < bodyRootNode->getChildCount(); i++) {
            auto *itemNode    = bodyRootNode->getChild(i);
            auto  processName = itemNode->getChildDataString("col2");

            //            std::cout << "process name" << processName << std::endl;

            if (processName.Contains(_processName)) {
               auto pid = itemNode->getChildDataInt("col0");
               std::cout << "found the pid" << pid << std::endl;
               _gdbMgr->writeCommand(wxString::Format("-target-attach %d", (int)pid));
               found = true;
               _gdbMgr->loadBreakpoints();
               break;
            }
         }
         if (!found) {
            wxMessageBox("Failed to find the process " + _processName, "Attach Process", wxOK);
         }
      }
   }
   return true;
}

auto DebugCmdBreakpoint::processResponse(GdbResponse *response) -> bool {
   auto &tree = response->tree;
   for (int treeChildIdx = 0; treeChildIdx < tree.getRootChildCount(); treeChildIdx++) {
      TreeNode *rootNode = tree.getChildAt(treeChildIdx);
      wxString  rootName = rootNode->getName();

      if (rootName == "bkpt") {
         _gdbMgr->processBreakpointChanged(tree, _isToSave);
      }
   }
   return true;
}

auto DebugCmdGetThreadList::processResponse(GdbResponse *response) -> bool {
   auto &tree = response->tree;
   for (int treeChildIdx = 0; treeChildIdx < tree.getRootChildCount(); treeChildIdx++) {
      TreeNode *rootNode = tree.getChildAt(treeChildIdx);
      wxString  rootName = rootNode->getName();

      if (rootName == "threads") {
         _gdbMgr->m_threadList.clear();

         // Parse the result
         for (int cIdx = 0; cIdx < rootNode->getChildCount(); cIdx++) {
            TreeNode *child    = rootNode->getChild(cIdx);
            wxString  threadId = child->getChildDataString("id");
            wxString  targetId = child->getChildDataString("target-id");
            wxString  funcName = child->getChildDataString("frame/func");
            wxString  lineNo   = child->getChildDataString("frame/line");
            wxString  details  = child->getChildDataString("details");

            if (details.IsEmpty()) {
               if (!funcName.IsEmpty()) {
                  details = "Executing " + funcName + "()";

                  if (!lineNo.IsEmpty()) {
                     details += " @ L" + lineNo;
                  }
               }
            }

            long tempLong = 0;
            threadId.ToLong(&tempLong);

            ThreadInfo tinfo;
            tinfo.m_id                        = static_cast<int>(tempLong);
            tinfo.m_name                      = targetId;
            tinfo.m_details                   = details;
            tinfo.m_func                      = funcName;
            _gdbMgr->m_threadList[tinfo.m_id] = tinfo;
         }

         DebuggerEvent e{wxEVT_DEBUGGER_GDB_THREAD_LIST_CHANGED};
         _gdbMgr->ProcessEvent(e);
      }
   }
   return true;
}

auto DebugCmdHandlerStackList::processResponse(GdbResponse *response) -> bool {
   auto &tree = response->tree;

   for (int treeChildIdx = 0; treeChildIdx < tree.getRootChildCount(); treeChildIdx++) {
      TreeNode *rootNode = tree.getChildAt(treeChildIdx);
      wxString  rootName = rootNode->getName();

      if (rootName == "stack") {
         std::vector<StackFrameEntry> stackFrameList;
         for (int j = 0; j < rootNode->getChildCount(); j++) {
            const TreeNode *child = rootNode->getChild(j);

            StackFrameEntry entry;
            entry.m_functionName = child->getChildDataString("func");
            entry.m_line         = child->getChildDataInt("line");
            entry.m_sourcePath   = child->getChildDataString("fullname");
            entry._level         = child->getChildDataInt("level");

            stackFrameList.push_back(entry);
         }

         DebuggerEvent ee{wxEVT_DEBUGGER_GDB_STACK_CHANGED};
         ee._stackFrameList = stackFrameList;
         _gdbMgr->ProcessEvent(ee);

         auto e = DebuggerEvent::makeFrameIdChangedEvent(_gdbMgr->m_currentFrameIdx);
         _gdbMgr->ProcessEvent(e);
      }
   }
   return true;
}

auto DebugCmdHandlerGetFiles::processResponse(GdbResponse *response) -> bool {
   Tree &                             resultData = response->tree;
   std::unordered_map<wxString, bool> fileLookup;

   // Clear the old list
   for (auto *sourceFile : _gdbMgr->m_sourceFiles) {
      fileLookup[sourceFile->m_fullName] = false;
      delete sourceFile;
   }
   _gdbMgr->m_sourceFiles.clear();

   // Create the new list
   for (int k = 0; k < resultData.getRootChildCount(); k++) {
      TreeNode *rootNode = resultData.getChildAt(k);
      wxString  rootName = rootNode->getName();

      if (rootName == "files") {
         for (int j = 0; j < rootNode->getChildCount(); j++) {
            TreeNode *childNode = rootNode->getChild(j);
            wxString  name      = childNode->getChildDataString("file");
            wxString  fullname  = childNode->getChildDataString("fullname");

            if (fullname.IsEmpty()) {
               continue;
            }

            SourceFile *sourceFile = nullptr;

            if (!name.Contains("<built-in>") && name[0] != '/') {
               // Already added this file?
               bool alreadyAdded = false;

               if (fileLookup.find(fullname) != fileLookup.end()) {
                  if (fileLookup[fullname] == true) {
                     alreadyAdded = true;
                  }
               } else {
                  // modified = true;
               }

               if (!alreadyAdded) {
                  fileLookup[fullname] = true;

                  sourceFile = new SourceFile;

                  sourceFile->m_name     = name;
                  sourceFile->m_fullName = fullname;

                  _gdbMgr->m_sourceFiles.push_back(sourceFile);
               }
            }
         }
      }
   }
   return true;
}
