#include "CustomEvents.h"
#include "processreaderthread.h"
#include "wx/event.h"
#include <memory>

wxDEFINE_EVENT(wxEVT_PROCESS_OUTPUT, AsyncProcessEvent);
wxDEFINE_EVENT(wxEVT_PROCESS_STDERR, AsyncProcessEvent);
wxDEFINE_EVENT(wxEVT_PROCESS_TERMINATED, AsyncProcessEvent);

wxDEFINE_EVENT(wxEVT_GDB_RESPONSE, GdbResponseEvent);

wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_PROCESS_ENDED, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_NOTIFICATION, DebuggerEvent);
wxDEFINE_EVENT(wxEVT_DEBUGGER_GDB_LOCAL_VARIABLE_CHANGED, DebuggerEvent);
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

wxDEFINE_EVENT(wxEVT_BREAKPOINT_DELETE_ITEM, BreakpointEvent);
wxDEFINE_EVENT(wxEVT_BREAKPOINT_DELETE_ALL, BreakpointEvent);

wxIMPLEMENT_DYNAMIC_CLASS(AsyncProcessEvent, wxCommandEvent);
wxIMPLEMENT_DYNAMIC_CLASS(BreakpointEvent, wxCommandEvent);
wxIMPLEMENT_DYNAMIC_CLASS(GdbResponseEvent, wxCommandEvent);
wxIMPLEMENT_DYNAMIC_CLASS(DebuggerEvent, wxCommandEvent);

auto DebuggerEvent::makeGdbStoppedEvent(int reason, const wxString &fileName, const wxString &funcName, int stackDepth, int lineNo) -> DebuggerEvent {
   DebuggerEvent e;
   e.SetEventType(wxEVT_DEBUGGER_GDB_STOPPED);
   e._reason       = reason;
   e._fileName     = fileName;
   e._functionName = funcName;
   e._stackDepth   = stackDepth;
   e._lineNo       = lineNo;
   return e;
}

auto DebuggerEvent::makeMessageEvent(const wxString &message) -> DebuggerEvent {
   DebuggerEvent e;
   e.SetEventType(wxEVT_DEBUGGER_GDB_NOTIFICATION);
   e._message = message;
   return e;
}

auto DebuggerEvent::makeThreadIdChangedEvent(int threadId) -> DebuggerEvent {
   DebuggerEvent e;
   e.SetEventType(wxEVT_DEBUGGER_GDB_THREAD_ID_CHANGED);
   e._threadId = threadId;
   return e;
}

auto DebuggerEvent::makeFrameIdChangedEvent(int frameId) -> DebuggerEvent {
   DebuggerEvent e;
   e.SetEventType(wxEVT_DEBUGGER_GDB_FRAME_ID_CHANGED);
   e._frameId = frameId;
   return e;
}

auto DebuggerEvent::makeVariableWatchChangedEvent(VarWatch *varWatch) -> DebuggerEvent {
   DebuggerEvent e;
   e.SetEventType(wxEVT_DEBUGGER_GDB_VARIABLE_WATCH_CHANGED);
   e._variableWatch = varWatch;
   return e;
}

auto DebuggerEvent::makeConsoleLogEvent(const wxString &consoleLog) -> DebuggerEvent {
   DebuggerEvent e;
   e.SetEventType(wxEVT_DEBUGGER_GDB_CONSOLE_LOG);
   e._message = consoleLog;
   return e;
}
