#pragma once

#include "wx/string.h"
#include "wx/variant.h"
#include <iostream>
#include <string>
#include <utility>

class VarWatch {
 public:
   using DispFormat = enum { FMT_HEX = 1, FMT_DEC, FMT_BIN, FMT_CHAR, FMT_NATIVE };
   using Type       = enum { TYPE_HEX_INT = 1, TYPE_DEC_INT, TYPE_FLOAT, TYPE_STRING, TYPE_ENUM, TYPE_ERROR_MSG, TYPE_CHAR, TYPE_UNKNOWN };

   VarWatch() = default;
   VarWatch(const wxString &watchId_, const wxString &name_);
   VarWatch(const wxString &watchId, const wxString &varName, const wxString &varValue, const wxString &varType, bool hasChildren);

   auto getName() const -> wxString;
   void setName(const wxString &name) { m_name = name; }
   auto getWatchId() const -> wxString;
   void setHasChildren(bool hasChildren);
   auto hasChildren() const -> bool;
   auto inScope() const -> bool;
   void setInScope(bool inScope);
   void setVarType(const wxString &varType);
   auto getVarType() const -> wxString;
   void setValue(const wxString &value);
   auto getValue() const -> wxString;
   void valueFromGdbString(const wxString &data);
   auto getData() const -> wxString;
   void setData(Type type, const wxVariant &data);
   auto getPointerAddress() const -> long long;
   void setPointerAddress(long long addr);
   auto hasPointerAddress() const -> bool;
   auto getParentWatchId() const -> wxString;
   void setParentWatchId(const wxString &parentWatchId);
   auto isExpanded() const -> bool;
   auto isDynamic() const -> bool { return _isDynamic; }
   void setIsDynamic(bool isExpanded) { _isDynamic = isExpanded; }
   void setIsExpanded(bool isExpanded);
   auto getOriginalVariabeValue() -> wxString { return _originalVaraibleValue; }
   void setDisplayHint(const wxString &displayHint) { _displayhint = displayHint; }
   auto getDisplayHint() -> wxString { return _displayhint; }

 private:
   bool       m_inScope             = true;
   bool       m_hasChildren         = false;
   long long  m_address             = 0;
   Type       m_type                = VarWatch::TYPE_UNKNOWN;
   bool       m_addressValid        = false;
   bool       _isExpanded           = false;
   bool       _isDynamic            = false;
   DispFormat _currentDisplayFormat = DispFormat::FMT_NATIVE;
   wxVariant  m_data;
   wxString   m_watchId;
   wxString   m_name;
   wxString   m_varType;
   wxString   m_parentWatchId;
   wxString   _originalVaraibleValue;
   wxString   _displayhint;
};

struct StackFrameEntry {
 public:
   int      _level = 0;
   wxString m_functionName; //!< Eg: "main".
   int      m_line;         //!< The line number. Eg: 1.
   wxString m_sourcePath;   //!< The full path of the source file. Eg: "/test/file.c".
};

struct ThreadInfo {
   int      m_id;      //!< The numeric id assigned to the thread by GDB.
   wxString m_name;    //!< Target-specific string identifying the thread.
   wxString m_func;    //!< The name of the function (Eg: "func").
   wxString m_details; //!< Additional information about the thread provided by the target.
};

class GdbSourceFile {
 public:
   GdbSourceFile(std::string name, std::string fullName) : _name{std::move(name)}, _fullName{std::move(fullName)} {}

   std::string _name;
   std::string _fullName;
   std::string _fileName;
   bool        _isValid = true;
};

class BreakPoint {
 public:
   BreakPoint() = default;
   explicit BreakPoint(int number) : _number(number){};

   bool               _isEnabled    = true;
   bool               _isTemp       = false;
   bool               _isMulti      = false;
   bool               _isMultiOwner = false;
   int                _times        = 0;
   int                _number       = -1;
   int                _line         = -1;
   int                _ignoreCount  = -1;
   std::string        _no;
   std::string        _fullname;
   std::string        _file;
   std::string        _funcName;
   std::string        _origLoc;
   unsigned long long _addr{};
};

class GdbBreakpointInfo {
 public:
   wxString number;
   wxString type;
   wxString catch_type;
   wxString disp;
   wxString enabled;
   wxString addr;
   wxString addr_flags;
   wxString func;
   wxString filename;
   wxString fullname;
   wxString line;
   wxString at;
   wxString pending;
   wxString evaluated_by;
   wxString thread;
   wxString task;
   wxString cond;
   wxString ignore;
   wxString enable;
   wxString original_location;
   wxString times;
   wxString what;
};

class GdbBreakpointLocation {
 public:
   wxString number;
   wxString enabled;
   wxString addr;
   wxString addr_flags;
   wxString func;
   wxString file;
   wxString fullname;
   wxString line;
   wxString thread_groups;
};

using GdbStackFrameInfo = struct GdbStackFrameInfo_ {
   int         level;
   long long   addr;
   std::string func;
   std::string file;
   std::string fullname;
   int         line;
};

typedef struct GdbStoppedInfoBase_ {
   struct GdbFrame {
      long long   addr;
      std::string func;
      std::string args;
      std::string file;
      std::string fullname;
      int         line;
   } frame;
   int  thread_id;
   void print() {
      std::cout << "address = " << frame.addr << std::endl;
      std::cout << "function = " << frame.func << std::endl;
      std::cout << "args = " << frame.args << std::endl;
      std::cout << "file = " << frame.file << std::endl;
      std::cout << "fullname = " << frame.fullname << std::endl;
      std::cout << "line = " << frame.line << std::endl;
   }

} GdbStoppedInfoBase;

using GdbBreakpointHitInfo = struct GdbBreakpointHitInfo_ : public GdbStoppedInfoBase {
   std::string disposition;
   int         no;
};

using GdbEndSteppingRangeInfo = struct GdbEndSteppingRangeInfo_ : public GdbStoppedInfoBase { int thread_id = 0; };

using GdbSignalReceivedInfo = struct GdbSignalReceivedInfo_ : public GdbStoppedInfoBase {
   std::string signal_name;
   std::string signal_meaning;

   void print() {
      std::cout << "signalName = " << signal_name << std::endl;
      std::cout << "signal_meaning = " << signal_meaning << std::endl;
      GdbStoppedInfoBase::print();
   }
};

class GdbThreadInfo {
 public:
   wxString id;
   wxString target_id;
   wxString details;
   wxString name;
   wxString state;
   wxString frame;
   wxString core;
};

class GdbOsInfoProcesses {
 public:
   GdbOsInfoProcesses(std::string name, int pid) : _processName{std::move(name)}, _processPid{pid} {}
   std::string _processName;
   int         _processPid;
};

using ThreadInfoMap_t       = std::unordered_map<int, ThreadInfo>;
using SourceFileList_t      = std::vector<GdbSourceFile *>;
using BreakpointList_t      = std::vector<BreakPoint *>;
using VariableWatchList_t   = std::vector<VarWatch *>;
using OsInfoProcessesList_t = std::vector<GdbOsInfoProcesses>;
using StackFramesList_t     = std::vector<StackFrameEntry>;
