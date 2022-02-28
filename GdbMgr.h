#pragma once

#include "wx/arrstr.h"
#include "CustomEvents.h"
#include "GdbModels.h"
#include "GdbParser.h"
#include "GdbResultTree.h"
#include "GdbSessionInfo.h"
#include "asyncprocess.h"
#include "processreaderthread.h"
#include "wx/event.h"
#include "wx/process.h"
#include "wx/string.h"
#include "wx/timer.h"
#include <iostream>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>

class DebugCmdHandler;
class MainFrame;

using HandlersMap_t    = std::map<wxString, DebugCmdHandler *>;
using GdbResponseMap_t = std::map<wxString, GdbResponse *>;

class GdbMgr : public wxEvtHandler {
 public:
   enum TargetState { TARGET_STOPPED, TARGET_STARTING, TARGET_RUNNING, TARGET_FINISHED };

   enum StopReason { UNKNOWN, END_STEPPING_RANGE, BREAKPOINT_HIT, SIGNAL_RECEIVED, EXITED_NORMALLY, FUNCTION_FINISHED, EXITED };

   explicit GdbMgr();

   static auto isValidGdbOutput(wxString gdbOutputStr) -> bool;

   auto startGdbProcess(GdbSessionInfo &sessionInfo);
   auto isValidSession() const -> bool;
   auto initExeSession(GdbSessionInfo &sessionInfo) -> int;
   auto initAttachPidSession(GdbSessionInfo &sessionInfo) -> int;
   auto initCoredumpSession(GdbSessionInfo &sessionInfo) -> int;
   auto initRemoteSession(GdbSessionInfo &sessionInfo) -> int;

   void writeCommand(const wxString &command);
   void writeCommand(const wxString &command, DebugCmdHandler *handler);

   void        dispatchResponse(GdbResponse *resp);
   static auto parseReasonString(const wxString &reasonString) -> StopReason;

   void onStatusAsyncOut(Tree &tree, GDB_ASYNC_CLASS ac);
   void onNotifyAsyncOut(Tree &tree, GDB_ASYNC_CLASS ac);
   void onExecAsyncOut(Tree &tree, GDB_ASYNC_CLASS ac);
   void onResult(Tree &tree);
   void onConsoleStreamOutput(wxString str);
   void onTargetStreamOutput(wxString str);
   void onLogStreamOutput(wxString str);

   auto getNextWatchId() -> int { return m_varWatchLastId++; }
   void processBreakpointDeleted(int id);
   void processBreakpointChanged(Tree &tree, bool isToSave = true);
   auto getThreadList() -> std::vector<ThreadInfo>;
   auto getBreakpointList() -> std::vector<BreakPoint *> &;
   void removeBreakpoint(BreakPoint *bpObj);
   void loadBreakpoints();
   auto getVarWatchInfo(const wxString &watchId) -> VarWatch *;
   auto getWatchChildren(VarWatch &parentWatch) -> std::vector<VarWatch *>;
   auto checkGdbRunningOrStarting() -> bool;
   void ensureStopped();
   void stop();
   void quit();

   auto gdbJump(const wxString &fileName, int lineNo) -> int;
   void gdbStepIn();
   void gdbStepOut();
   void gdbNext();
   void gdbContinue();
   void gdbRun();
   auto gdbSetBreakpointAtFunc(const wxString &func) -> int;
   auto gdbSetBreakpoint(const wxString &filename, int lineNo) -> int;
   auto gdbToggleBreakpoint(const wxString &filename, int lineNo) -> int;
   auto gdbSetBreakpoint(const wxString &lineSpecLocation, bool isTemp, bool isDisabled) -> int;
   void gdbRemoveAllBreakpoints();
   void gdbRemoveBreakpoint(BreakPoint *bkpt);
   void gdbEnableBreakpoint(std::vector<BreakPoint *> &bkpt);
   void gdbDisableBreakpoint(std::vector<BreakPoint *> &bkpt);
   void gdbEnableAllBreakpoints();
   void gdbDisableAllBreakpoints();
   void gdbGetThreadList();
   void gdbGetStackFrames();
   auto gdbAddVariableWatch(const wxString &variableName) -> bool;
   void gdbSelectThread(int threadId);
   auto gdbGetFiles() -> bool;
   void gdbSelectFrame(int selectedFrameIdx);
   void gdbRemoveVarWatch(const wxString &watchId);
   auto gdbChangeWatchVariable(const wxString &watchId, const wxString &newValue) -> int;

   void OnGdbProcessEnd(AsyncProcessEvent &e);
   void OnGdbProcessData(AsyncProcessEvent &e);
   void OnKillGDB(wxCommandEvent &e);

   void addCmdHandler(const wxString &id, DebugCmdHandler *handler);
   auto removeCmdHandler(const wxString &id) -> DebugCmdHandler *;
   void clearCmdHandlers();

 public:
   GdbSessionInfo        _sessionInfo;
   wxArrayString         m_pendingComands;
   bool                  m_scanSources      = false;
   int                   m_pid              = 0;
   int                   m_selectedThreadId = -1;
   TargetState           m_targetState      = TARGET_STOPPED;
   TargetState           m_lastTargetState  = TARGET_FINISHED;
   int                   m_currentFrameIdx  = -1;
   int                   m_varWatchLastId   = 0;
   bool                  m_isRemote         = false; //!< True if "remote target" or false if it is a "local target".
   int                   m_memDepth         = 64;    //!< The memory depth. (Either 64 or 32).
   IProcess *            m_gdbProcess       = nullptr;
   bool                  m_goingDown        = false;
   int                   _stackDepth        = 0;
   wxString              _currentFunctionName;
   wxString              _currentFilePath;
   ThreadInfoMap_t       m_threadList;
   SourceFileList_t      m_sourceFiles;
   BreakpointList_t      m_breakpoints;
   VariableWatchList_t   m_watchList;
   std::vector<wxString> m_localVars;
   wxArrayString         m_gdbOutputArr;
   HandlersMap_t         _cmdHandlers;
   wxString              m_gdbOutputIncompleteLine;
};

// clang-format off

#define DECLARE_DEFAULT_CLASS(__x__) virtual ~__x__() = default;  __x__(__x__ &other)  = delete;   __x__(__x__ &&other) = delete; auto operator=(__x__ &&other) -> __x__ & = delete;auto operator=(__x__ &other) -> __x__ & = delete;
// clang-format on

class DebugCmdHandler {
 public:
   explicit DebugCmdHandler(GdbMgr *gdbMgr) : _gdbMgr{gdbMgr} {}
   DECLARE_DEFAULT_CLASS(DebugCmdHandler);

   virtual auto processResponse(GdbResponse * /*response*/) -> bool { return true; }

   GdbMgr *_gdbMgr = nullptr;
   friend GdbMgr;
};

class DebugCmdSelectThread : public DebugCmdHandler {
 public:
   explicit DebugCmdSelectThread(GdbMgr *gdbMgr) : DebugCmdHandler{gdbMgr} {}
   DECLARE_DEFAULT_CLASS(DebugCmdSelectThread);

   auto processResponse(GdbResponse * /*response*/) -> bool override {
      _gdbMgr->m_selectedThreadId = 0;
      return true;
   }
};

class DebugCmdBreakpoint : public DebugCmdHandler {
 public:
   explicit DebugCmdBreakpoint(GdbMgr *gdbMgr, bool isToSave = true) : DebugCmdHandler{gdbMgr}, _isToSave{isToSave} {}
   DECLARE_DEFAULT_CLASS(DebugCmdBreakpoint);

   auto processResponse(GdbResponse *response) -> bool override;
   bool _isToSave = true;
};

class DebugCmdProcessPids : public DebugCmdHandler {
 public:
   explicit DebugCmdProcessPids(GdbMgr *gdbMgr) : DebugCmdHandler{gdbMgr} {}
   DECLARE_DEFAULT_CLASS(DebugCmdProcessPids);

   auto     processResponse(GdbResponse *response) -> bool override;
   wxString _processName = "mor_epg_BeinIVP";
};

class DebugCmdGetThreadList : public DebugCmdHandler {
 public:
   explicit DebugCmdGetThreadList(GdbMgr *gdbMgr) : DebugCmdHandler{gdbMgr} {}
   DECLARE_DEFAULT_CLASS(DebugCmdGetThreadList);

   auto processResponse(GdbResponse *response) -> bool override;
};

class DebugCmdHandlerStackList : public DebugCmdHandler {
 public:
   explicit DebugCmdHandlerStackList(GdbMgr *gdbMgr) : DebugCmdHandler{gdbMgr} {}
   DECLARE_DEFAULT_CLASS(DebugCmdHandlerStackList);

   auto processResponse(GdbResponse *response) -> bool override;
};

class DebugCmdHandlerGetFiles : public DebugCmdHandler {
 public:
   explicit DebugCmdHandlerGetFiles(GdbMgr *gdbMgr) : DebugCmdHandler{gdbMgr} {}
   DECLARE_DEFAULT_CLASS(DebugCmdHandlerGetFiles);

   auto processResponse(GdbResponse *response) -> bool override;
};
