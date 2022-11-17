#include "GdbMgr.h"
#include "CustomEvents.h"
#include "GdbMiProcess.h"
#include "GdbMiResp.h"
#include "GdbModels.h"
#include "wx/event.h"
#include "wx/string.h"
#include <algorithm>
#include <array>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <ostream>
#include <pthread.h>
#include <string>
#include <sys/wait.h>
#include <unordered_map>
#include <vector>
#include <wx/regex.h>
#include <wx/sstream.h>
#include <wx/tokenzr.h>
#include <wx/txtstrm.h>
#include <wx/utils.h>

//*stopped,reason="breakpoint-hit",disp="keep",bkptno="1",frame={addr="0x00005555555551d5",func="main",args=[],file="main.cpp",fullname="/home/psammandam/disk/projects/wxwidgets/vazhu-neekuthal/test/main.cpp",line="4",arch="i386:x86-64"},thread-id="1",stopped-threads="all",core="0"

//*stopped,reason="end-stepping-range",frame={addr="0x00005555555551e3",func="main",args=[],file="main.cpp",fullname="/home/psammandam/disk/projects/wxwidgets/vazhu-neekuthal/test/main.cpp",line="7",arch="i386:x86-64"},thread-id="1",stopped-threads="all",core="0"

//*stopped,reason="exited-normally"

//*stopped,reason="signal-received",signal-name="SIGINT",signal-meaning="Interrupt",frame={addr="0x00007ffff6c0522f",func="__GI___poll",args=[{name="fds",value="0x55555692a890"},{name="nfds",value="3"},{name="timeout",value="12984"}],file="../sysdeps/unix/sysv/linux/poll.c",fullname="./io/../sysdeps/unix/sysv/linux/poll.c",line="29",arch="i386:x86-64"},thread-id="1",stopped-threads="all",core="3"

static auto fillGdbStoppedFrameInfo(GdbMiResult *result, GdbStoppedInfoBase &bpInfo) -> bool;
static auto fillGdbBreakpointHitInfo(GdbMiAsyncRecord *asyncRecord, GdbBreakpointHitInfo &bpInfo) -> bool;
static auto fillGdbEndSteppingRangeInfo(GdbMiAsyncRecord *asyncRecord, GdbEndSteppingRangeInfo &bpInfo) -> bool;
static auto fillGdbSignalReceivedInfo(GdbMiAsyncRecord *asyncRecord, GdbSignalReceivedInfo &bpInfo) -> bool;
static auto fillGdbFunctionFinishedInfo(GdbMiAsyncRecord *asyncRecord, GdbStoppedInfoBase &bpInfo) -> bool;

static auto MakeId() -> wxString {
   static unsigned int counter(0);
   wxString            newId;
   newId.Printf(wxT("%08u"), ++counter);
   return newId;
}

GdbMgr::GdbMgr() : _mi(new GdbMiProcess{}) {
   Bind(wxEVT_PROCESS_OUTPUT, &GdbMgr::OnGdbProcessData, this);
   Bind(wxEVT_PROCESS_TERMINATED, &GdbMgr::OnGdbProcessEnd, this);

   _mi->_imiprocess = this;
}

void GdbMgr::OnGdbProcessEnd(AsyncProcessEvent & /*e*/) {
   _targetState         = GdbMgr::TARGET_STOPPED;
   _stackDepth          = 0;
   _currentFunctionName = "";
   _currentFilePath     = "";

   _breakpointsList.clear();
   _threadList.clear();

   DebuggerEvent debugEvent{wxEVT_DEBUGGER_GDB_PROCESS_ENDED};
   AddPendingEvent(debugEvent);
}

void GdbMgr::OnGdbProcessData(AsyncProcessEvent &evt) {
   if (evt.asyncRecord() != nullptr) {
      handleAsyncRecord(evt.asyncRecord());
      delete evt.asyncRecord();
   }
}

void GdbMgr::terminateSession() {
   _mi->terminate();
   _sessionInfo.clear();
   _targetState = TargetState::TARGET_DISCONNECTED;
   notifyStateChanged(_targetState);
}

auto GdbMgr::startGdbProcess(GdbSessionInfo &sessionInfo) {
   std::vector<std::string> args{};
   _mi->startGdbProcess(sessionInfo._gdbExePath.ToStdString(), args);

   gdbInferiorTtySet(_mi->getInferiorTtyName());
   gdbEnablePrettyPrinting();
   gdbSet("mi-async on");

   _targetState = TargetState::TARGET_CONNECTED;
   notifyStateChanged(_targetState);
}

auto GdbMgr::isValidSession() const -> bool { return _sessionInfo._isValid; }

auto GdbMgr::initExeSession(GdbSessionInfo &sessionInfo) -> int {
   _sessionInfo._isValid = false;

   startGdbProcess(sessionInfo);

   gdbFileExecAndSymbols(sessionInfo._exePath.ToStdString());

   if (!sessionInfo._exeArgs.IsEmpty()) {
      gdbExecArguments(sessionInfo._exeArgs.ToStdString());
   }

   if (!sessionInfo._exeWorkingDir.IsEmpty()) {
      gdbEnvironmentCd(sessionInfo._exeWorkingDir.ToStdString());
   }

   _sessionInfo          = sessionInfo;
   _sessionInfo._isValid = true;

   loadBreakpoints();

   _targetState = TargetState::TARGET_SPECIFIED;
   notifyStateChanged(_targetState);

   return 0;
}

auto GdbMgr::initAttachPidSession(GdbSessionInfo & /*sessionInfo*/) -> int { return -1; }

auto GdbMgr::initCoredumpSession(GdbSessionInfo &sessionInfo) -> int {
   _sessionInfo._isValid = false;

   startGdbProcess(sessionInfo);
   gdbFileExecAndSymbols(sessionInfo._exePath.ToStdString());
   gdbTargetSelect("core", std::vector<std::string>{sessionInfo._coredumpPath.ToStdString()});

   // gdbGetFiles();

   _sessionInfo          = sessionInfo;
   _sessionInfo._isValid = true;

   updateLocalVariables();
   gdbGetStackFrames();

   _targetState = GdbMgr::TARGET_FINISHED;
   notifyStateChanged(_targetState);

   return 0;
}

auto GdbMgr::initRemoteSession(GdbSessionInfo &sessionInfo) -> int {
   _sessionInfo._isValid = false;

   startGdbProcess(sessionInfo);
   gdbFileExecAndSymbols(sessionInfo._exePath.ToStdString());

   if (!sessionInfo._exeArgs.IsEmpty()) {
      gdbExecArguments(sessionInfo._exeArgs.ToStdString());
   }

   if (!sessionInfo._exeWorkingDir.IsEmpty()) {
      gdbEnvironmentCd(sessionInfo._exeWorkingDir.ToStdString());
   }

   gdbTargetSelect("extended-remote", std::vector<std::string>{wxString::Format("%s:%d", sessionInfo._remoteHost, (int)sessionInfo._remotePort).ToStdString()});

   // gdbGetFiles();

   OsInfoProcessesList_t processList;
   gdbInfoOsProcesses(processList);

   for (const auto &process : processList) {
      std::cout << " process name = " << process._processName << ", process pid=" << process._processPid << std::endl;

      if (!sessionInfo._remoteProcess.empty() && process._processName.find(sessionInfo._remoteProcess) != std::string::npos) {
         gdbTargetAttach(process._processPid);
         break;
      }
   }

   _sessionInfo          = sessionInfo;
   _sessionInfo._isValid = true;
   _isRemote             = true;

   loadBreakpoints();

   return 0;
}

void GdbMgr::writeCommandBlock(const std::string &command) {
   printf(">> %s\n", command.c_str());
   fflush(stdout);
   wxBusyCursor wait;
   _mi->writeMiData(command);

   std::unique_lock<std::mutex> mlock(_gdbCmdMutex);
   _gdbCmdCondVar.wait(mlock, [this] { return _gdbMiOutput != nullptr; });
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

auto GdbMgr::gdbGetFiles(SourceFileList_t &sourceFilesList) -> bool {
   gdbFileListExecSourceFiles("", sourceFilesList);
   return false;
}

auto GdbMgr::getThreadList() -> std::vector<ThreadInfo> {
   std::vector<ThreadInfo> list;
   for (const auto &it : _threadList) {
      list.push_back(it.second);
   }
   return list;
}

void GdbMgr::loadBreakpoints() {
   wxFileConfig fileConfig{wxEmptyString, wxEmptyString, _sessionInfo._sessionFullPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE};

   fileConfig.SetPath("/breakpoints");
   auto count = fileConfig.ReadLong("count", 0);

   for (auto i = 0; i < count; i++) {
      auto bpPath = wxString::Format("bp%d", i);
      fileConfig.SetPath("/breakpoints/" + bpPath);
      auto lineNo           = fileConfig.Read("lineNo");
      auto fileName         = fileConfig.Read("file");
      auto isTemp           = fileConfig.ReadBool("temp", false);
      auto isEnabled        = fileConfig.ReadBool("enabled", false);
      auto lineSpecLocation = fileName + ":" + lineNo;

      gdbBreakInsert(lineSpecLocation.ToStdString(), isTemp, false, false, !isEnabled);
   }
   fileConfig.Flush();
}

void GdbMgr::handleStopped(GdbMgr::StopReason reasonEnum, GdbStoppedInfoBase &stoppedInfo) {
   _prevFunctionName    = _currentFunctionName;
   _prevFilePath        = _currentFilePath;
   _currentFunctionName = stoppedInfo.frame.func;
   _currentFilePath     = stoppedInfo.frame.fullname;
   _currentLineNo       = stoppedInfo.frame.line;

   auto evt = DebuggerEvent::makeGdbStoppedEvent(reasonEnum, _currentFilePath, _currentFunctionName, _stackDepth, _currentLineNo);
   ProcessEvent(evt);
   // gdbStackInfoDepth();
   updateLocalVariables();
}

void GdbMgr::gdbStop() {
   if (_targetState != GdbMgr::TARGET_RUNNING) {
      auto evt = DebuggerEvent::makeMessageEvent("Program is not running");
      ProcessEvent(evt);
      return;
   }
   /* below will not work when the -gdb-set mi-async on */
   // wxProcess::Kill(_mi->_gdbProcessPid, wxSIGINT);

   /* below will work only when the -gdb-set mi-async on */
   gdbExecInterrupt();
}

auto GdbMgr::gdbJump(const wxString &fileName, int lineNo) -> int {
   if (_targetState != GdbMgr::TARGET_STOPPED) {
      auto evt = DebuggerEvent::makeMessageEvent("Program is not stopped");
      ProcessEvent(evt);
      return -3;
   }
   gdbBreakInsert(wxString::Format("%s:%d", fileName, lineNo).ToStdString(), true);
   gdbExecJump(wxString::Format("%s:%d", fileName, lineNo).ToStdString());

   return 0;
}

void GdbMgr::gdbStepIn() {
   if (_targetState != GdbMgr::TARGET_STOPPED) {
      auto evt = DebuggerEvent::makeMessageEvent("Program is not stopped");
      ProcessEvent(evt);
      return;
   }
   gdbExecStep();
}

void GdbMgr::gdbStepOut() {
   if (_targetState != GdbMgr::TARGET_STOPPED) {
      auto evt = DebuggerEvent::makeMessageEvent("Program is not stopped");
      ProcessEvent(evt);
      return;
   }
   gdbExecFinish();
}

void GdbMgr::gdbNext() {
   if (_targetState != GdbMgr::TARGET_STOPPED) {
      auto evt = DebuggerEvent::makeMessageEvent("Program is not stopped");
      ProcessEvent(evt);
      return;
   }
   gdbExecNext();
}

void GdbMgr::gdbRunOrContinue() {
   if (_targetState == GdbMgr::TARGET_SPECIFIED) {
      gdbExecRun();
   } else {
      gdbExecContinue();
   }
}

void GdbMgr::gdbContinue() {
   if (_targetState != GdbMgr::TARGET_STOPPED) {
      auto evt = DebuggerEvent::makeMessageEvent("Program is not stopped");
      ProcessEvent(evt);
      return;
   }
   gdbExecContinue();
}

void GdbMgr::gdbRun() {
   if (_targetState != TargetState::TARGET_SPECIFIED) {
      auto evt = DebuggerEvent::makeMessageEvent("Target not specified");
      ProcessEvent(evt);
      return;
   }
   gdbExecRun(_sessionInfo._isBreakAtMain);

   _targetState = GdbMgr::TARGET_RUNNING;
   notifyStateChanged(_targetState);
}

/******************************************************************************************************/

auto GdbMgr::SetBreakpoint(const wxString &func) -> int {
   if (_targetState != TargetState::TARGET_STOPPED && _targetState != TargetState::TARGET_SPECIFIED) {
      return 0;
   }
   gdbBreakInsert(func.ToStdString(), false, false, true);
   UpdateBreakpointList();
   return 1;
}

auto GdbMgr::SetBreakpoint(const wxString &filename, int lineNo) -> int {
   if (_targetState != TargetState::TARGET_STOPPED && _targetState != TargetState::TARGET_SPECIFIED) {
      return 0;
   }
   if (filename.IsEmpty()) {
      return 0;
   }
   gdbBreakInsert(wxString::Format("%s:%d", filename, lineNo).ToStdString());
   UpdateBreakpointList();
   return 1;
}

auto GdbMgr::ToggleBreakpoint(const wxString &filename, int lineNo) -> int {
   if (_targetState != TargetState::TARGET_STOPPED && _targetState != TargetState::TARGET_SPECIFIED) {
      return 0;
   }
   if (filename.IsEmpty()) {
      return 0;
   }
   for (auto *breakPoint : _breakpointsList) {
      if (breakPoint->_fullname == filename && breakPoint->_line == lineNo) {
         gdbBreakDelete(std::vector<std::string>{breakPoint->_no});
         UpdateBreakpointList();
         return 1;
      }
   }
   SetBreakpoint(filename, lineNo);
   UpdateBreakpointList();
   return 1;
}

auto GdbMgr::SetBreakpoint(const wxString &lineSpecLocation, bool isTemp, bool isDisabled) -> int {
   if (_targetState != TargetState::TARGET_STOPPED && _targetState != TargetState::TARGET_SPECIFIED) {
      return 0;
   }
   gdbBreakInsert(lineSpecLocation.ToStdString(), isTemp, false, false, isDisabled);
   UpdateBreakpointList();
   return 1;
}

void GdbMgr::RemoveBreakpoint(const std::string &no) {
   if (_targetState != TargetState::TARGET_STOPPED && _targetState != TargetState::TARGET_SPECIFIED) {
      return;
   }
   gdbBreakDelete(std::vector<std::string>{no});
   UpdateBreakpointList();
}

void GdbMgr::EnableBreakpoint(std::vector<std::string> &bkpt) {
   if (_targetState != TargetState::TARGET_STOPPED && _targetState != TargetState::TARGET_SPECIFIED) {
      return;
   }
   gdbBreakEnable(bkpt);
   UpdateBreakpointList();
}

void GdbMgr::DisableBreakpoint(std::vector<std::string> &bkpt) {
   if (_targetState != TargetState::TARGET_STOPPED && _targetState != TargetState::TARGET_SPECIFIED) {
      return;
   }
   gdbBreakDisable(bkpt);
   UpdateBreakpointList();
}

void GdbMgr::EnableAllBreakpoints() {
   std::vector<std::string> bp;

   for (auto *i : _breakpointsList) {
      bp.push_back(i->_no);
   }
   gdbBreakEnable(bp);
   UpdateBreakpointList();
}

void GdbMgr::DisableAllBreakpoints() {
   std::vector<std::string> tempBreakpointList;

   for (auto *breakPoint : _breakpointsList) {
      tempBreakpointList.push_back(breakPoint->_no);
   }
   gdbBreakDisable(tempBreakpointList);
   UpdateBreakpointList();
}

void GdbMgr::UpdateBreakpointList() {
   std::for_each(_breakpointsList.begin(), _breakpointsList.end(), [](auto *bkpt) { delete bkpt; });
   _breakpointsList.clear();

   gdbBreakList(_breakpointsList);

   DebuggerEvent evt{wxEVT_DEBUGGER_GDB_BREAKPOINT_CHANGED};
   ProcessEvent(evt);
}

auto GdbMgr::getBreakpoint(const std::string &no) -> BreakPoint * {
   for (auto *bkpt : _breakpointsList) {
      if (bkpt->_no == no) {
         return bkpt;
      }
   }
   return nullptr;
}

void GdbMgr::updateLocalVariables() {

   for (auto *variable : _localVariableList) {
      gdbVarDelete(variable->getWatchId().ToStdString());
      delete variable;
   }
   _localVariableList.clear();

#if 0
   auto varList = gdbStackListVariables();

   for (auto &var : varList) {
      gdbVarCreate(var);
   }
#else
   VariableWatchList_t varWatchList;

   gdbStackListVariables(varWatchList);

   for (auto *var : varWatchList) {
      if (var->getVarType() == "wxString") {
         auto watchId = var->getName() + ".m_impl._M_dataplus._M_p";
         gdbVarCreate(watchId.ToStdString());
      } else {
         gdbVarCreate(var->getName().ToStdString());
      }
      delete var;
   }
#endif

   if (!_localVariableList.empty()) {
      wxArrayString localVariableList;

      DebuggerEvent evt{wxEVT_DEBUGGER_GDB_LOCAL_VARIABLE_CHANGED, localVariableList, _currentFilePath, _currentFunctionName, _stackDepth};
      ProcessEvent(evt);
   }
}

/******************************************************************************************************/

void GdbMgr::gdbSelectFrame(int selectedFrameIdx) {
   if (_currentFrameIdx != selectedFrameIdx) {
      gdbStackSelectFrame(selectedFrameIdx);
      struct StackFrameEntry frameInfo;
      gdbStackInfoFrame(frameInfo);
      updateLocalVariables();

      DebuggerEvent evt;
      evt.SetEventType(wxEVT_DEBUGGER_GDB_FRAME_ID_CHANGED);
      evt._frameId   = selectedFrameIdx;
      evt._frameInfo = frameInfo;

      ProcessEvent(evt);
   }
}

auto GdbMgr::getVarWatchInfo(const wxString &watchId) -> VarWatch * {
   auto it = std::find_if(_localVariableList.begin(), _localVariableList.end(), [watchId](VarWatch *vw) { return vw->getWatchId() == watchId; });
   return (it == _localVariableList.end()) ? nullptr : *it;
}

void GdbMgr::gdbRemoveVarWatch(const wxString &watchId, bool removeOblyChildren) {
   if (!removeOblyChildren) {
      // Remove from the list
      auto it = std::find_if(_localVariableList.begin(), _localVariableList.end(), [watchId](VarWatch *vw) { return vw->getWatchId() == watchId; });
      if (it != _localVariableList.end()) {
         auto *watch = *it;
         _localVariableList.erase(it);
         delete watch;
      }
   }

   // Remove children first
   std::vector<wxString> removeList;

   for (auto *childWatch : _localVariableList) {
      if (childWatch->getParentWatchId() == watchId) {
         removeList.push_back(childWatch->getWatchId());
      }
   }

   for (auto &i : removeList) {
      gdbRemoveVarWatch(i);
   }

   gdbVarDelete(watchId.ToStdString(), removeOblyChildren);
}

auto GdbMgr::gdbChangeWatchVariable(const wxString &watchId, const wxString &newValue) -> int {
   if (_targetState != TargetState::TARGET_STOPPED && _targetState != TargetState::TARGET_SPECIFIED) {
      return 0;
   }
   gdbVarAssign(watchId.ToStdString(), newValue.ToStdString());
   return 1;
}

auto GdbMgr::getWatchChildren(VarWatch &parentWatch) -> std::vector<VarWatch *> {
   std::vector<VarWatch *> list;

   for (auto *loopWatch : _localVariableList) {
      if (parentWatch.getWatchId() == loopWatch->getParentWatchId()) {
         list.push_back(loopWatch);
      }
   }
   return list;
}

void GdbMgr::handleInferiorData(std::string &data) {}

void GdbMgr::handleTerminated() { this->AddPendingEvent(AsyncProcessEvent{wxEVT_PROCESS_TERMINATED}); }

auto GdbMgr::doneResultCommand(const std::string &command) -> bool { return simpleResultCommand(command, GdbMiResultRecord::RESULT_CLASS_DONE); }

auto GdbMgr::simpleResultCommand(const std::string &command, int resultClass) -> bool {
   writeCommandBlock(command);

   auto returnCode = _gdbMiOutput != nullptr && _gdbMiOutput->_resultClass == resultClass;

   if ((_gdbMiOutput != nullptr) && _gdbMiOutput->_resultClass != GdbMiResultRecord::RESULT_CLASS_DONE) {
      std::cout << "command " << command << " failed" << std::endl;
   }
   deleteResultRecord();
   return returnCode;
}

void GdbMgr::gdbGetStackFrames(int low_frame, int high_frame) {
   if (_targetState != TargetState::TARGET_STOPPED && _targetState != TargetState::TARGET_SPECIFIED) {
      return;
   }
   DebuggerEvent evt{wxEVT_DEBUGGER_GDB_STACK_CHANGED};
   gdbStackListFrames(low_frame, high_frame, evt._stackFrameList);

   ProcessEvent(evt);
}

/***************************************************************************************************/
/***************************************************************************************************/
/***************************************************************************************************/

void GdbMgr::deleteResultRecord() {
   delete _gdbMiOutput;
   _gdbMiOutput = nullptr;
}

void GdbMgr::notifyStateChanged(int newState) {
   auto evt      = DebuggerEvent{wxEVT_DEBUGGER_GDB_STATE_CHANGED};
   evt._newState = newState;
   ProcessEvent(evt);
}

void GdbMgr::handleMiOutput(GdbMiOutput *gdbMiOutput) {
   if (gdbMiOutput->isOobRecord()) {
      if (dynamic_cast<GdbMiOutOfBandRecord *>(gdbMiOutput)->isAsyncRecord()) {
         AsyncProcessEvent e(wxEVT_PROCESS_OUTPUT);
         e.asyncRecord(dynamic_cast<GdbMiAsyncRecord *>(gdbMiOutput));
         this->AddPendingEvent(e);
      }
   } else if (gdbMiOutput->isResultRecord()) {
      std::lock_guard<std::mutex> guard(_gdbCmdMutex);
      _gdbMiOutput = dynamic_cast<GdbMiResultRecord *>(gdbMiOutput);
      // printf("<< %p\n", _gdbMiOutput);
      // fflush(stdout);

      _gdbCmdCondVar.notify_one();
   }
}

void GdbMgr::handleAsyncRecord(GdbMiAsyncRecord *asyncRecord) {
   if (asyncRecord == nullptr) {
      return;
   }
   switch (asyncRecord->asyncClass()) {

   case GdbMiAsyncRecord::ASYNC_CLASS_RUNNING:
      _targetState = GdbMgr::TARGET_RUNNING;
      break;

   case GdbMiAsyncRecord::ASYNC_CLASS_STOPPED: {
      if ((asyncRecord != nullptr) && asyncRecord->getChildrens().empty()) {
         return;
      }

      std::string        reason     = asyncRecord->getChildrens()[0]->_const;
      GdbMgr::StopReason reasonEnum = reason.empty() ? GdbMgr::UNKNOWN : parseReasonString(reason);
      _targetState                  = GdbMgr::TARGET_STOPPED;

      switch (reasonEnum) {
      case GdbMgr::StopReason::BREAKPOINT_HIT: {
         GdbBreakpointHitInfo breakpointHitInfo;

         fillGdbBreakpointHitInfo(asyncRecord, breakpointHitInfo);
         handleStopped(reasonEnum, breakpointHitInfo);
      } break;
      case GdbMgr::StopReason::END_STEPPING_RANGE: {
         GdbEndSteppingRangeInfo endSteppingRangeInfo;

         fillGdbEndSteppingRangeInfo(asyncRecord, endSteppingRangeInfo);
         handleStopped(reasonEnum, endSteppingRangeInfo);
      } break;
      case GdbMgr::StopReason::FUNCTION_FINISHED: {
         GdbStoppedInfoBase stoppedInfo;

         fillGdbFunctionFinishedInfo(asyncRecord, stoppedInfo);
         handleStopped(reasonEnum, stoppedInfo);
      } break;
      case GdbMgr::StopReason::EXITED_NORMALLY:
      case GdbMgr::StopReason::EXITED: {
         _targetState = GdbMgr::TARGET_FINISHED;
      } break;
      case GdbMgr::StopReason::SIGNAL_RECEIVED: {
         GdbSignalReceivedInfo signalReceivedInfo;

         fillGdbSignalReceivedInfo(asyncRecord, signalReceivedInfo);

         signalReceivedInfo.print();
         //         if (signalReceivedInfo.signal_name == "SIGINT") {
         DebuggerEvent evt{wxEVT_DEBUGGER_GDB_SIGNALED};
         evt._signalName = signalReceivedInfo.signal_name;
         ProcessEvent(evt);
         //         }
      } break;
      case UNKNOWN: {
         /* when debugging with the remote target attach, we are getting this stopped without reason  */
         /* *stopped,frame={addr="0x0000007fae914364",func="nanosleep",args=[],file="../sysdeps/unix/syscall-template.S",fullname="/home/vagrant/build/.build/src/glibc-2.24/posix/../sysdeps/unix/syscall-template.S",line="86"},thread-id="1",stopped-threads="all",core="2"*/
         GdbStoppedInfoBase bpInfo;

         fillGdbFunctionFinishedInfo(asyncRecord, bpInfo);
         handleStopped(reasonEnum, bpInfo);
      } break;
      }
   } break;

   case GdbMiAsyncRecord::ASYNC_CLASS_BREAKPOINT_CREATED:
   case GdbMiAsyncRecord::ASYNC_CLASS_BREAKPOINT_MODIFIED:
   case GdbMiAsyncRecord::ASYNC_CLASS_BREAKPOINT_DELETED: {
      //=breakpoint-created,bkpt={number="2",type="breakpoint",disp="del",enabled="y",addr="0x00000000000011d5",func="main()",file="main.cpp",fullname="/home/psammandam/disk/projects/wxwidgets/vazhu-neekuthal/test/main.cpp",line="4",thread-groups=["i1"],times="0",original-location="-qualified
      // main"}
      // =breakpoint-modified,bkpt={number="2",type="breakpoint",disp="del",enabled="y",addr="0x00005555555551d5",func="main()",file="main.cpp",fullname="/home/psammandam/disk/projects/wxwidgets/vazhu-neekuthal/test/main.cpp",line="4",thread-groups=["i1"],times="1",original-location="-qualified
      // main"}
      UpdateBreakpointList();
   } break;

   default:
      break;
   }
   notifyStateChanged(_targetState);
}

/***************************************************************************************************/
/***************************************************************************************************/
/***************************************************************************************************/
/* STATIC FUNCTIONS */
auto fillGdbStoppedFrameInfo(GdbMiResult *result, GdbStoppedInfoBase &bpInfo) -> bool {
   if ((result != nullptr) && result->isTuple()) {
      bpInfo.frame.func     = result->asStr("func");
      bpInfo.frame.fullname = result->asStr("fullname");
      bpInfo.frame.line     = result->asInt("line");
      bpInfo.frame.file     = result->asStr("file");
      bpInfo.frame.addr     = result->asLongLong("addr");
   }
   return true;
}

/*
*stopped,reason="breakpoint-hit",disp="del",bkptno="1",frame={addr="0x00005555555551d5",func="main",args=[],file="main.cpp",fullname="/home/psammandam/disk/projects/wxwidgets/vazhu-neekuthal/test/main.cpp",line="4",arch="i386:x86-64"},thread-id="1",stopped-threads="all",core="2"
=breakpoint-deleted,id="1"
*/
auto fillGdbBreakpointHitInfo(GdbMiAsyncRecord *asyncRecord, GdbBreakpointHitInfo &bpInfo) -> bool {
   bpInfo.disposition = asyncRecord->asStr("disp");
   bpInfo.no          = asyncRecord->asInt("bkptno");
   bpInfo.thread_id   = asyncRecord->asInt("thread-id");
   fillGdbStoppedFrameInfo(asyncRecord->findResult("frame"), bpInfo);
   return true;
}

auto fillGdbEndSteppingRangeInfo(GdbMiAsyncRecord *asyncRecord, GdbEndSteppingRangeInfo &bpInfo) -> bool {
   bpInfo.thread_id = asyncRecord->asInt("thread-id");
   fillGdbStoppedFrameInfo(asyncRecord->findResult("frame"), bpInfo);
   return true;
}

auto fillGdbFunctionFinishedInfo(GdbMiAsyncRecord *asyncRecord, GdbStoppedInfoBase &bpInfo) -> bool {
   bpInfo.thread_id = asyncRecord->asInt("thread-id");
   fillGdbStoppedFrameInfo(asyncRecord->findResult("frame"), bpInfo);
   return true;
}

auto fillGdbSignalReceivedInfo(GdbMiAsyncRecord *asyncRecord, GdbSignalReceivedInfo &bpInfo) -> bool {
   bpInfo.signal_name    = asyncRecord->asStr("signal-name");
   bpInfo.signal_meaning = asyncRecord->asStr("signal-meaning");
   bpInfo.thread_id      = asyncRecord->asInt("thread-id");
   fillGdbStoppedFrameInfo(asyncRecord->findResult("frame"), bpInfo);
   return true;
}
