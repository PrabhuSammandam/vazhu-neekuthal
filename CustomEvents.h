#pragma once

#include "GdbModels.h"
#include "GdbMiResp.h"
#include "wx/arrstr.h"
#include "wx/dlimpexp.h"
#include "wx/event.h"
#include "wx/string.h"

class AsyncProcessEvent : public wxCommandEvent {
 public:
   explicit AsyncProcessEvent(wxEventType commandType = 0, int id = 0) : wxCommandEvent(commandType, id) {}
   AsyncProcessEvent(const AsyncProcessEvent &event) : wxCommandEvent(event) { *this = event; }
   ~AsyncProcessEvent() override = default;

   auto operator=(const AsyncProcessEvent &src) -> AsyncProcessEvent & {
      _asyncRecord = src._asyncRecord;
      return *this;
   }

   auto Clone() const -> wxEvent * override { return new AsyncProcessEvent(*this); }

   wxDECLARE_DYNAMIC_CLASS(AsyncProcessEvent);

   auto asyncRecord() -> GdbMiAsyncRecord * { return _asyncRecord; }
   void asyncRecord(GdbMiAsyncRecord *ar) { _asyncRecord = ar; }

 private:
   GdbMiAsyncRecord *_asyncRecord;
};

class VarWatch;

class DebuggerEvent : public wxCommandEvent {
 public:
   explicit DebuggerEvent(wxEventType commandType = 0, int id = 0) : wxCommandEvent(commandType, id) {}
   DebuggerEvent(wxEventType commandType, const wxArrayString &varList, const wxString &fileName, const wxString &funcName, int stackDepth)
       : wxCommandEvent(commandType, 0), _variablesList{varList}, _fileName{fileName}, _functionName{funcName}, _stackDepth{stackDepth} {}
   DebuggerEvent(const DebuggerEvent &event) : wxCommandEvent(event) { *this = event; }
   ~DebuggerEvent() override = default;

   auto operator=(const DebuggerEvent &src) -> DebuggerEvent & { return *this; }

   auto Clone() const -> wxEvent * override { return new DebuggerEvent(*this); }
   auto setNewState(int newState) -> DebuggerEvent & {
      _newState = newState;
      return *this;
   }

   static auto makeGdbStoppedEvent(int reason, const wxString &fileName, const wxString &funcName, int stackDepth, int lineNo) -> DebuggerEvent;
   static auto makeMessageEvent(const wxString &message) -> DebuggerEvent;
   static auto makeThreadIdChangedEvent(int threadId) -> DebuggerEvent;
   static auto makeFrameIdChangedEvent(int frameId) -> DebuggerEvent;
   static auto makeVariableWatchChangedEvent(VarWatch *varWatch) -> DebuggerEvent;
   static auto makeLocalVariableAddedEvent(VarWatch *varWatch) -> DebuggerEvent;
   static auto makeConsoleLogEvent(const wxString &consoleLog) -> DebuggerEvent;

   wxDECLARE_DYNAMIC_CLASS(DebuggerEvent);

   StackFramesList_t _stackFrameList;
   wxArrayString     _variablesList;
   wxString          _fileName;
   wxString          _functionName;
   int               _stackDepth = 0;
   int               _newState;
   wxString          _signalName;
   int               _lineNo   = 0;
   int               _reason   = 0;
   int               _threadId = -1;
   int               _frameId  = -1;
   wxString          _variableName;
   wxString          _variableValue;
   wxString          _message;
   bool              _isBreakpointToSave = true;
   VarWatch         *_variableWatch      = nullptr;
   StackFrameEntry   _frameInfo;
};

wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_PROCESS_OUTPUT, AsyncProcessEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_PROCESS_STDERR, AsyncProcessEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_PROCESS_TERMINATED, AsyncProcessEvent);

wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_PROCESS_ENDED, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_NOTIFICATION, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_LOCAL_VARIABLE_CHANGED, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_LOCAL_VARIABLE_ADDED, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_STATE_CHANGED, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_SIGNALED, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_STOPPED, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_FRAME_VARIABLE_CHANGED, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_FRAME_VARIABLE_RESET, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_FRAME_ID_CHANGED, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_BREAKPOINT_CHANGED, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_THREAD_ID_CHANGED, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_THREAD_LIST_CHANGED, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_VARIABLE_WATCH_CHANGED, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_CONSOLE_LOG, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_STACK_CHANGED, DebuggerEvent);
