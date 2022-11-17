#pragma once

#include <condition_variable>
#include <mutex>
#include "CustomEvents.h"
#include "GdbMiProcess.h"
#include "GdbMiResp.h"
#include "GdbModels.h"
#include "GdbSessionInfo.h"
#include "wx/event.h"
#include "wx/string.h"
#include <iostream>
#include <map>
#include <string>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>
#include <wx/arrstr.h>

class MainFrame;

class GdbMgr : public wxEvtHandler, public IGdbMiProcess {
 public:
   enum TargetState { TARGET_DISCONNECTED, TARGET_CONNECTED, TARGET_SPECIFIED, TARGET_STARTING, TARGET_RUNNING, TARGET_STOPPED, TARGET_FINISHED };
   enum StopReason { UNKNOWN, END_STEPPING_RANGE, BREAKPOINT_HIT, SIGNAL_RECEIVED, EXITED_NORMALLY, FUNCTION_FINISHED, EXITED };

   explicit GdbMgr();

   /*****************************************************************************************************/
   auto isValidSession() const -> bool;
   auto initExeSession(GdbSessionInfo &sessionInfo) -> int;
   auto initAttachPidSession(GdbSessionInfo &sessionInfo) -> int;
   auto initCoredumpSession(GdbSessionInfo &sessionInfo) -> int;
   auto initRemoteSession(GdbSessionInfo &sessionInfo) -> int;
   void terminateSession();
   /*****************************************************************************************************/
   /* Breakpoint Commands */
   /*
     -break-info breakpoint
     -break-passcount tracepoint-number passcount
     -break-watch [ -a | -r ]
   */
   auto gdbBreakAfter(int bkptNo, int count) -> bool;                                                /* -break-after number count */
   auto gdbBreakCommands(const std::string &bpNo, const std::vector<std::string> &commands) -> bool; /* -break-commands number [ command1 ... commandN ] */
   auto gdbBreakCondition(int bkptNo, const std::string &expression, bool force = false) -> bool;    /* -break-condition [ --force ] number [ expr ] */
   auto gdbBreakDelete(const std::vector<std::string> &bpList) -> bool;                              /* -break-delete ( breakpoint )+ */
   auto gdbBreakDisable(const std::vector<std::string> &bpList) -> bool;                             /* -break-disable ( breakpoint )+ */
   auto gdbBreakEnable(const std::vector<std::string> &bpList) -> bool;                              /* -break-enable ( breakpoint )+ */
   auto gdbBreakList(std::vector<BreakPoint *> &bpList) -> bool;                                     /* -break-list */
   auto gdbBreakInsert(const std::string &location = "", bool isTemp = false, bool isHardware = false, bool isPending = false, bool isDisabled = false, const std::string &condition = "",
                       bool isForceCondition = false, int ignoreCount = -1)
       -> bool; /*-break-insert [ -t ] [ -h ] [ -f ] [ -d ] [ -a ] [ --qualified ] [ -c condition ] [ --force-condition ] [ -i ignore-count ] [ -p thread-id ] [ location ]*/

   /*****************************************************************************************************/
   /* Stack Manipulation Commands */
   auto gdbEnableFrameFilters() -> bool;
   auto gdbStackInfoFrame(struct StackFrameEntry &frame) -> bool;
   auto gdbStackInfoDepth(int &stackDepth) -> bool;
   auto gdbStackListVariables() -> std::vector<std::string>;
   void gdbStackListVariables(VariableWatchList_t &varWatchList);
   auto gdbStackListFrames(int low_frame, int high_frame, std::vector<struct StackFrameEntry> &stackList) -> bool;
   auto gdbStackSelectFrame(int frameNo) -> bool;
   /*****************************************************************************************************/
   /* File Commands */
   auto gdbFileExecAndSymbols(const std::string &filePath) -> bool;
   auto gdbFileListExecSourceFile(/*struct to come*/) -> bool;
   auto gdbFileListExecSourceFiles(const std::string &args, SourceFileList_t &sourceFilesList) -> bool;
   auto gdbFileListSharedLibraries(const std::string &regExp /*struct to come*/) -> bool;
   auto gdbFileSymbolFile(const std::string &filePath) -> bool;

   /*****************************************************************************************************/
   /* Variable Objects */
   void gdbEnablePrettyPrinting();
   void gdbVarCreate(const std::string &varName);
   auto gdbVarDelete(const std::string &varObjId, bool isDeleteOnlyChild = true) -> bool;
   auto gdbVarListChildren(const std::string &varObjId, VariableWatchList_t &varList) -> bool;
   auto gdbVarUpdate() -> bool;
   auto gdbVarAssign(const std::string &name, const std::string &expression) -> bool;

   /*****************************************************************************************************/
   /* Target Manipulation Commands */
   auto gdbTargetAttach(int pid) -> bool;
   auto gdbTargetDetach(int pid) -> bool;
   auto gdbTargetDisconnect() -> bool;
   auto gdbTargetSelect(const std::string &type, const std::vector<std::string> &parameters) -> bool;

   /*****************************************************************************************************/
   /* Miscellaneous GDB/MI Commands */
   void gdbExit();
   auto gdbSet(const std::string &variable) -> bool;
   void gdbShow(const std::string &variable);
   void gdbVersion();
   auto gdbInfoOsProcesses(OsInfoProcessesList_t &processList) -> bool;
   auto gdbInferiorTtySet(const std::string &tty) -> bool;

   /*****************************************************************************************************/
   /* Program Context - COMPLETED */
   auto gdbExecArguments(const std::string &args) -> bool;
   auto gdbEnvironmentCd(const std::string &dir) -> bool;

   /*****************************************************************************************************/
   /* Program Execution - COMPLETED */
   auto gdbExecContinue() -> bool;
   auto gdbExecFinish() -> bool;
   auto gdbExecInterrupt() -> bool;
   auto gdbExecJump(const std::string &location) -> bool;
   auto gdbExecNext() -> bool;
   auto gdbExecNextInstruction() -> bool;
   auto gdbExecReturn() -> bool;
   auto gdbExecRun(bool isBreakAtMain = false) -> bool;
   auto gdbExecStep() -> bool;
   auto gdbExecStepInstruction() -> bool;
   auto gdbExecUntil(const std::string &location) -> bool;
   /*****************************************************************************************************/

   void writeCommandBlock(const std::string &command);

   void loadBreakpoints();
   auto SetBreakpoint(const wxString &func) -> int;
   auto SetBreakpoint(const wxString &filename, int lineNo) -> int;
   auto ToggleBreakpoint(const wxString &filename, int lineNo) -> int;
   auto SetBreakpoint(const wxString &lineSpecLocation, bool isTemp, bool isDisabled) -> int;
   void RemoveBreakpoint(const std::string &bpNo);
   void EnableBreakpoint(std::vector<std::string> &bkpt);
   void DisableBreakpoint(std::vector<std::string> &bkpt);
   void EnableAllBreakpoints();
   void DisableAllBreakpoints();
   void UpdateBreakpointList();
   auto getBreakpointList() -> std::vector<BreakPoint *> & { return _breakpointsList; }
   auto getBreakpoint(const std::string &no) -> BreakPoint *;

   void updateLocalVariables();
   auto getLocalVariableList() -> VariableWatchList_t & { return _localVariableList; }

   auto getThreadList() -> std::vector<ThreadInfo>;

   auto getVarWatchInfo(const wxString &watchId) -> VarWatch *;
   auto getWatchChildren(VarWatch &parentWatch) -> std::vector<VarWatch *>;
   void gdbRemoveVarWatch(const wxString &watchId, bool removeOblyChildren = false);
   auto gdbChangeWatchVariable(const wxString &watchId, const wxString &newValue) -> int;

   void gdbStop();
   auto gdbJump(const wxString &fileName, int lineNo) -> int;
   void gdbStepIn();
   void gdbStepOut();
   void gdbContinue();
   void gdbNext();
   void gdbRunOrContinue();
   void gdbRun();

   void gdbGetStackFrames(int low_frame = 0, int high_frame = 0);
   void gdbSelectFrame(int selectedFrameIdx);

   auto gdbGetFiles(SourceFileList_t &sourceFilesList) -> bool;

 private:
   void OnGdbProcessEnd(AsyncProcessEvent &evt);
   void OnGdbProcessData(AsyncProcessEvent &evt);

   void handleInferiorData(std::string &data) override;
   void handleMiOutput(GdbMiOutput *gdbMiOutput) override;
   void handleTerminated() override;

   auto doneResultCommand(const std::string &command) -> bool;
   auto simpleResultCommand(const std::string &command, int resultClass) -> bool;
   void handleAsyncRecord(GdbMiAsyncRecord *asyncRecord);
   void handleStopped(GdbMgr::StopReason reasonEnum, GdbStoppedInfoBase &stoppedInfo);
   auto startGdbProcess(GdbSessionInfo &sessionInfo);
   auto getNextWatchId() -> int { return _varWatchLastId++; }
   void deleteResultRecord();
   void notifyStateChanged(int newState);

   static auto parseReasonString(const wxString &reasonString) -> StopReason;

 public:
   GdbSessionInfo          _sessionInfo;
   TargetState             _targetState     = TARGET_DISCONNECTED;
   int                     _currentFrameIdx = -1;
   int                     _varWatchLastId  = 0;
   bool                    _isRemote        = false; //!< True if "remote target" or false if it is a "local target".
   int                     _memDepth        = 64;    //!< The memory depth. (Either 64 or 32).
   int                     _stackDepth      = 0;
   wxString                _currentFunctionName;
   wxString                _currentFilePath;
   wxString                _prevFunctionName;
   wxString                _prevFilePath;
   int                     _prevStackDepth = 0;
   int                     _currentLineNo  = 0;
   ThreadInfoMap_t         _threadList;
   BreakpointList_t        _breakpointsList;
   VariableWatchList_t     _localVariableList;
   GdbMiProcess           *_mi          = nullptr;
   GdbMiResultRecord      *_gdbMiOutput = nullptr;
   std::mutex              _gdbCmdMutex;
   std::condition_variable _gdbCmdCondVar;
};
