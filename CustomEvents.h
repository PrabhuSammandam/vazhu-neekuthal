#pragma once

#include "GdbModels.h"
#include "wx/arrstr.h"
#include "wx/dlimpexp.h"
#include "wx/event.h"
#include "asyncprocess.h"
#include "wx/string.h"
#include "wx/treebase.h"

class AsyncProcessEvent : public wxCommandEvent {
 public:
   explicit AsyncProcessEvent(wxEventType commandType = 0, int id = 0) : wxCommandEvent(commandType, id) {}
   AsyncProcessEvent(const AsyncProcessEvent &event) : wxCommandEvent(event) { *this = event; }
   ~AsyncProcessEvent() override = default;

   auto operator=(const AsyncProcessEvent &src) -> AsyncProcessEvent & {
      m_process = src.m_process;
      m_output  = src.m_output;
      return *this;
   }

   auto Clone() const -> wxEvent * override { return new AsyncProcessEvent(*this); }

   wxDECLARE_DYNAMIC_CLASS(AsyncProcessEvent);

   void SetOutput(const wxString &output) { this->m_output = output; }
   void SetProcess(IProcess *process) { this->m_process = process; }
   auto GetOutput() const -> const wxString & { return m_output; }
   auto GetProcess() -> IProcess * { return m_process; }

 private:
   wxString  m_output;
   IProcess *m_process = nullptr;
};

class BreakpointEvent : public wxCommandEvent {
 public:
   explicit BreakpointEvent(wxEventType commandType = 0, int id = 0) : wxCommandEvent(commandType, id) {}
   BreakpointEvent(const BreakpointEvent &event) : wxCommandEvent(event) { *this = event; }
   ~BreakpointEvent() override = default;

   auto operator=(const BreakpointEvent &src) -> BreakpointEvent & {
      _item        = src._item;
      _isDeleteAll = src._isDeleteAll;
      return *this;
   }

   auto Clone() const -> wxEvent * override { return new BreakpointEvent(*this); }

   wxDECLARE_DYNAMIC_CLASS(BreakpointEvent);

   wxTreeItemId _item;
   bool         _isDeleteAll = false;
};

class GdbResponse;
class VarWatch;

class GdbResponseEvent : public wxCommandEvent {
 public:
   explicit GdbResponseEvent(wxEventType commandType = 0, int id = 0) : wxCommandEvent(commandType, id) {}
   GdbResponseEvent(const GdbResponseEvent &event) : wxCommandEvent(event) { *this = event; }
   ~GdbResponseEvent() override = default;

   auto operator=(const GdbResponseEvent &src) -> GdbResponseEvent & {
      _gdbResponse = src._gdbResponse;
      return *this;
   }

   auto Clone() const -> wxEvent * override { return new GdbResponseEvent(*this); }

   wxDECLARE_DYNAMIC_CLASS(GdbResponseEvent);

   void SetResponse(GdbResponse *gdbResponse) { this->_gdbResponse = gdbResponse; }
   auto GetResponse() -> GdbResponse * { return _gdbResponse; }

 private:
   GdbResponse *_gdbResponse = nullptr;
};

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
   static auto makeConsoleLogEvent(const wxString &consoleLog) -> DebuggerEvent;

   wxDECLARE_DYNAMIC_CLASS(DebuggerEvent);

   std::vector<StackFrameEntry> _stackFrameList;
   wxArrayString                _variablesList;
   wxString                     _fileName;
   wxString                     _functionName;
   int                          _stackDepth = 0;
   int                          _newState;
   wxString                     _signalName;
   int                          _lineNo   = 0;
   int                          _reason   = 0;
   int                          _threadId = -1;
   int                          _frameId  = -1;
   wxString                     _variableName;
   wxString                     _variableValue;
   wxString                     _message;
   bool                         _isBreakpointToSave = true;
   VarWatch *                   _variableWatch      = nullptr;
};

wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_PROCESS_OUTPUT, AsyncProcessEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_PROCESS_STDERR, AsyncProcessEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_PROCESS_TERMINATED, AsyncProcessEvent);

wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_GDB_RESPONSE, GdbResponseEvent);

wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_PROCESS_ENDED, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_NOTIFICATION, DebuggerEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_DEBUGGER_GDB_LOCAL_VARIABLE_CHANGED, DebuggerEvent);
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

wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_BREAKPOINT_DELETE_ITEM, BreakpointEvent);
wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_BREAKPOINT_DELETE_ALL, BreakpointEvent);
