#include "CustomEvents.h"
#include "wx/event.h"
#include <memory>

wxDEFINE_EVENT(wxEVT_PROCESS_OUTPUT, AsyncProcessEvent);
wxDEFINE_EVENT(wxEVT_PROCESS_STDERR, AsyncProcessEvent);
wxDEFINE_EVENT(wxEVT_PROCESS_TERMINATED, AsyncProcessEvent);

wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_PROCESS_ENDED, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_NOTIFICATION, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_LOCAL_VARIABLE_CHANGED, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_LOCAL_VARIABLE_ADDED, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_STATE_CHANGED, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_SIGNALED, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_STOPPED, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_FRAME_VARIABLE_RESET, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_FRAME_VARIABLE_CHANGED, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_FRAME_ID_CHANGED, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_THREAD_ID_CHANGED, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_THREAD_LIST_CHANGED, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_BREAKPOINT_CHANGED, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_VARIABLE_WATCH_CHANGED, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_CONSOLE_LOG, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_STACK_CHANGED, DebuggerEvent);

wxIMPLEMENT_DYNAMIC_CLASS(AsyncProcessEvent, wxCommandEvent);
wxIMPLEMENT_DYNAMIC_CLASS(DebuggerEvent, wxCommandEvent);

auto DebuggerEvent::makeGdbStoppedEvent(int reason, const wxString &fileName, const wxString &funcName, int stackDepth, int lineNo) -> DebuggerEvent {
   DebuggerEvent evt;
   evt.SetEventType(wxEVT_DEBUGGER_GDB_STOPPED);
   evt._reason       = reason;
   evt._fileName     = fileName;
   evt._functionName = funcName;
   evt._stackDepth   = stackDepth;
   evt._lineNo       = lineNo;
   return evt;
}

auto DebuggerEvent::makeMessageEvent(const wxString &message) -> DebuggerEvent {
   DebuggerEvent evt;
   evt.SetEventType(wxEVT_DEBUGGER_GDB_NOTIFICATION);
   evt._message = message;
   return evt;
}

auto DebuggerEvent::makeThreadIdChangedEvent(int threadId) -> DebuggerEvent {
   DebuggerEvent evt;
   evt.SetEventType(wxEVT_DEBUGGER_GDB_THREAD_ID_CHANGED);
   evt._threadId = threadId;
   return evt;
}

auto DebuggerEvent::makeFrameIdChangedEvent(int frameId) -> DebuggerEvent {
   DebuggerEvent evt;
   evt.SetEventType(wxEVT_DEBUGGER_GDB_FRAME_ID_CHANGED);
   evt._frameId = frameId;
   return evt;
}

auto DebuggerEvent::makeVariableWatchChangedEvent(VarWatch *varWatch) -> DebuggerEvent {
   DebuggerEvent evt;
   evt.SetEventType(wxEVT_DEBUGGER_GDB_VARIABLE_WATCH_CHANGED);
   evt._variableWatch = varWatch;
   return evt;
}

auto DebuggerEvent::makeLocalVariableAddedEvent(VarWatch *varWatch) -> DebuggerEvent {
   DebuggerEvent evt;
   evt.SetEventType(wxEVT_DEBUGGER_GDB_LOCAL_VARIABLE_ADDED);
   evt._variableWatch = varWatch;
   return evt;
}

auto DebuggerEvent::makeConsoleLogEvent(const wxString &consoleLog) -> DebuggerEvent {
   DebuggerEvent evt;
   evt.SetEventType(wxEVT_DEBUGGER_GDB_CONSOLE_LOG);
   evt._message = consoleLog;
   return evt;
}
