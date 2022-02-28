#pragma once

#include "wx/string.h"
#include "GdbResultTree.h"

/*
output →
    ( out-of-band-record )* [ result-record ] "(gdb)" nl

result-record →
    [ token ] "^" result-class ( "," result )* nl

out-of-band-record →
    async-record | stream-record

async-record →
    exec-async-output | status-async-output | notify-async-output

exec-async-output →
    [ token ] "*" async-output nl

status-async-output →
    [ token ] "+" async-output nl

notify-async-output →
    [ token ] "=" async-output nl

async-output →
    async-class ( "," result )*

result-class →
    "done" | "running" | "connected" | "error" | "exit"

async-class →
    "stopped" | others (where others will be added depending on the needs—this is still in development).

result →
    variable "=" value

variable →
    string

value →
    const | tuple | list

const →
    c-string

tuple →
    "{}" | "{" result ( "," result )* "}"

list →
    "[]" | "[" value ( "," value )* "]" | "[" result ( "," result )* "]"

stream-record →
    console-stream-output | target-stream-output | log-stream-output

console-stream-output →
    "~" c-string nl

target-stream-output →
    "@" c-string nl

log-stream-output →
    "&" c-string nl

nl →
    CR | CR-LF

token →
    any sequence of digits.

Notes:

    1. All output sequences end in a single line containing a period.
    2. The token is from the corresponding request. Note that for all async output, while the token is allowed by the grammar and may be output by future versions of GDB       for select async output
messages, it is generally omitted. Frontends should treat all async output as reporting general changes in the state of the target and there should be no need to associate async output to
any prior command.
    3. status-async-output contains on-going status information about the progress of a slow operation. It can be discarded. All status output is prefixed by ‘+’.
    4. exec-async-output contains asynchronous state change on the target (stopped, started, disappeared). All async output is prefixed by ‘*’.
    5. notify-async-output contains supplementary information that the client should handle (e.g., a new breakpoint information). All notify output is prefixed by ‘=’.
    6. console-stream-output is output that should be displayed as is in the console. It is the textual response to a CLI command. All the console output is prefixed by        ‘~’.
    7. target-stream-output is the output produced by the target program. All the target output is prefixed by ‘@’.
    8. log-stream-output is output text coming from GDB’s internals, for instance messages that should be displayed as part of an error log. All the log output is              prefixed by ‘&’.
    9. New GDB/MI commands should only output lists containing values.
*/

enum GDB_RESULT_CLASS {
   GDB_RESULT_CLASS_DONE,      // "^done" [ "," results ]
   GDB_RESULT_CLASS_RUNNING,   // "^running"
   GDB_RESULT_CLASS_CONNECTED, // "^connected"
   GDB_RESULT_CLASS_ERROR,     // "^error" "," "msg=" c-string [ "," "code=" c-string ]
   GDB_RESULT_CLASS_EXIT       // "^exit"
};

enum GDB_ASYNC_CLASS {
   UNKNOWN,
   RUNNING,              // *running,thread-id="thread"
   STOPPED,              // *stopped,reason="reason",thread-id="id",stopped-threads="stopped",core="core"
   THREAD_GROUP_ADDED,   // =thread-group-added,id="id"
   THREAD_GROUP_REMOVED, // =thread-group-removed,id="id"
   THREAD_GROUP_STARTED, // =thread-group-started,id="id",pid="pid"
   THREAD_GROUP_EXITED,  // =thread-group-exited,id="id"[,exit-code="code"]
   THREAD_CREATED,       // =thread-created,id="id",group-id="gid"
   THREAD_EXITED,        // =thread-exited,id="id",group-id="gid"
   THREAD_SELECTED,      // =thread-selected,id="id"[,frame="frame"]
   LIBRARY_LOADED,       // =library-loaded,...
   LIBRARY_UNLOADED,     // =library-unloaded,...
   BREAKPOINT_CREATED,   // =breakpoint-created,bkpt={...}
   BREAKPOINT_MODIFIED,  // =breakpoint-modified,bkpt={...}
   BREAKPOINT_DELETED,   // =breakpoint-deleted,id=number
   RECORD_STARTED,       // =record-started,thread-group="id",method="method"[,format="format"]
   RECORD_STOPPED,       // =record-stopped,thread-group="id"
   CMD_PARAM_CHANGED,    // =cmd-param-changed,param=param,value=value
   MEMORY_CHANGED,       // =memory-changed,thread-group=id,addr=addr,len=len[,type="code"]
};

enum GDB_ASYNC_STOPPED_REASON {
   BREAKPOINT_HIT,
   WATCHPOINT_TRIGGER,
   READ_WATCHPOINT_TRIGGER,
   ACCESS_WATCHPOINT_TRIGGER,
   FUNCTION_FINISHED,
   LOCATION_REACHED,
   WATCHPOINT_SCOPE,
   END_STEPPING_RANGE,
   EXITED_SIGNALLED,
   EXITED,
   EXITED_NORMALLY,
   SIGNAL_RECEIVED,
   SOLIB_EVENT,
   FORK,
   VFORK,
   SYSCALL_ENTRY,
   SYSCALL_RETURN,
   EXEC
};

class GdbResponse {
 public:
   using Type = enum {
      UNKNOWN               = 0,
      RESULT                = 1,
      CONSOLE_STREAM_OUTPUT = 2,
      TARGET_STREAM_OUTPUT  = 3,
      LOG_STREAM_OUTPUT     = 4,
      TERMINATION           = 5,
      STATUS_ASYNC_OUTPUT   = 6,
      NOTIFY_ASYNC_OUTPUT   = 7,
      EXEC_ASYNC_OUTPUT     = 8,
   };

   GdbResponse() : m_type{UNKNOWN} {};
   explicit GdbResponse(Type type) : m_type{type} {};

   auto isConsoleStreamOutput() -> bool { return m_type == CONSOLE_STREAM_OUTPUT; }
   auto isTargetStreamOutput() -> bool { return m_type == TARGET_STREAM_OUTPUT; }
   auto isLogStreamoutput() -> bool { return m_type == LOG_STREAM_OUTPUT; }
   auto isTermination() -> bool { return m_type == TERMINATION; };
   auto isStatusAsyncOutput() -> bool { return m_type == STATUS_ASYNC_OUTPUT; }
   auto isNotifyAsyncOutput() -> bool { return m_type == NOTIFY_ASYNC_OUTPUT; }
   auto isExecAsyncOutput() -> bool { return m_type == EXEC_ASYNC_OUTPUT; }
   auto isResult() -> bool { return m_type == RESULT; }
   auto getType() -> Type { return m_type; };
   void setType(Type t) { m_type = t; };
   auto getString() -> wxString { return m_str; };
   void setString(const wxString &str) { m_str = str; };
   auto typeToStr() -> wxString;
   auto reassonToString() const -> wxString;

 private:
   Type     m_type{};
   wxString m_str;

 public:
   bool             _cmdIdAvailable = false;
   wxString         _cmdId{};
   Tree             tree{};
   GDB_ASYNC_CLASS  reason{};
   GDB_RESULT_CLASS m_result{};
};

class Token {
 public:
   enum Type {
      UNKNOWN,
      C_STRING,        // "string"
      C_CHAR,          // 'c'
      KEY_EQUAL,       // '='
      KEY_LEFT_BRACE,  // '{'
      KEY_RIGHT_BRACE, // '}'
      KEY_LEFT_BAR,    // '['
      KEY_RIGHT_BAR,   // ']'
      KEY_UP,          // '^'
      KEY_PLUS,        // '-'
      KEY_COMMA,       // ','
      KEY_TILDE,       // '~'
      KEY_SNABEL,      // '@'
      KEY_STAR,        // '*'
      KEY_AND,         // '&'
      END_CODE,
      VAR
   };

   explicit Token(Type type) : m_type(type){};

   static auto typeToString(Type type) -> const char *;
   static auto createToken(char c) -> Token *;
   static auto isDelimiter(char c) -> bool;
   auto        getType() const -> Type { return m_type; };
   void        setType(Type type) { m_type = type; };
   auto        getString() const -> wxString { return m_text; };
   auto        toString() -> const char *;

 private:
   Type m_type;

 public:
   wxString m_text;
};

using TokenList_t = std::vector<Token *>;

class GdbParser {
 public:
   explicit GdbParser(const wxString &gdbOutput);

   auto tokenize() -> GdbParser &;
   auto peek_token() -> Token *;
   auto pop_token() -> Token *;
   auto isTokenPending() -> bool;
   auto checkToken(Token::Type type) -> Token *;
   auto eatToken(Token::Type type) -> Token *;

   auto parseOutput() -> GdbResponse *;
   auto parseResultRecord() -> GdbResponse *;
   auto parseAsyncOutput(GdbResponse *resp, GDB_ASYNC_CLASS *ac) -> int;
   auto parseResult(TreeNode *parent) -> int;
   auto parseValue(TreeNode *item) -> int;

 private:
   wxString    _gdbOutput;
   TokenList_t m_list;
   TokenList_t m_freeTokens;
};
