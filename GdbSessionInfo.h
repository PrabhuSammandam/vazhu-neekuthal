#pragma once

#include "wx/fileconf.h"
#include "wx/string.h"

class GdbSessionInfo {
 public:
   enum GDB_SESSION_TYPE { GDB_SESSION_TYPE_EXE_RUN, GDB_SESSION_TYPE_ATTACH_PID, GDB_SESSION_TYPE_COREDUMP, GDB_SESSION_TYPE_REMOTE };

   GdbSessionInfo() = default;

   GdbSessionInfo(const wxString &gdbExePath, const wxString &gdbArgs, const wxString &exePath, const wxString &exeArgs)
       : _sessionType{GDB_SESSION_TYPE_EXE_RUN}, _gdbExePath(gdbExePath), _gdbArgs(gdbArgs), _exePath(exePath), _exeArgs(exeArgs) {}

   GdbSessionInfo(const wxString &gdbExePath, const wxString &gdbArgs, long exePid) : _sessionType{GDB_SESSION_TYPE_ATTACH_PID} {
      _gdbExePath = gdbExePath;
      _gdbArgs    = gdbArgs;
      _exePid     = exePid;
   }

   GdbSessionInfo(const wxString &gdbExePath, const wxString &gdbArgs, const wxString &coredumpPath) : _sessionType{GDB_SESSION_TYPE_COREDUMP} {
      _gdbExePath   = gdbExePath;
      _gdbArgs      = gdbArgs;
      _coredumpPath = coredumpPath;
   }

   GdbSessionInfo(const wxString &gdbExePath, const wxString &gdbArgs, const wxString &remoteHost, long remotePort) : _sessionType{GDB_SESSION_TYPE_REMOTE} {
      _gdbExePath = gdbExePath;
      _gdbArgs    = gdbArgs;
      _remoteHost = remoteHost;
      _remotePort = remotePort;
   }

   void saveToConfig(wxFileConfig &config) const {
      config.SetPath("session");
      config.Write("sessionName", _sessionName);
      config.Write("sessionType", (int)_sessionType);
      config.Write("gdb", _gdbExePath);
      config.Write("gdbArgs", _gdbArgs);
      config.Write("execFile", _exePath);
      config.Write("execArgs", _exeArgs);
      config.Write("execWorkingDir", _exeWorkingDir);
      config.Write("execPid", _exePid);
      config.Write("remoteHost", _remoteHost);
      config.Write("remotePort", _remotePort);
      config.Write("remoteProcess", _remoteProcess);
      config.Write("coredumpPath", _coredumpPath);
      config.Write("breakAtMain", _isBreakAtMain);
   }
   void loadFromConfig(wxFileConfig &config) {
      config.SetPath("session");
      _sessionName   = config.Read("sessionName", "");
      _sessionType   = (GDB_SESSION_TYPE)config.ReadLong("sessionType", 0);
      _gdbExePath    = config.Read("gdb");
      _gdbArgs       = config.Read("gdbArgs");
      _exePath       = config.Read("execFile");
      _exeArgs       = config.Read("execArgs");
      _exeWorkingDir = config.Read("execWorkingDir");
      _exePid        = config.ReadLong("execPid", -1);
      _remoteHost    = config.Read("remoteHost");
      _remotePort    = config.ReadLong("remotePort", -1);
      _remoteProcess = config.Read("remoteProcess");
      _coredumpPath  = config.Read("coredumpPath");
      _isBreakAtMain = config.ReadBool("breakAtMain", false);
   }

   void clear() {
      _gdbExePath    = "";
      _gdbArgs       = "";
      _exePath       = "";
      _exeArgs       = "";
      _exeWorkingDir = "";
      _exePid        = -1;
      _remoteHost    = "";
      _remotePort    = -1;
      _coredumpPath  = "";
      _isBreakAtMain = false;
      _isValid       = false;
   }

   GDB_SESSION_TYPE _sessionType;
   wxString         _sessionName;
   wxString         _sessionFullPath;
   wxString         _gdbExePath{};
   wxString         _gdbArgs{};
   wxString         _exePath{};
   wxString         _exeArgs{};
   wxString         _exeWorkingDir{};
   long             _exePid = -1;
   wxString         _remoteHost{};
   long             _remotePort{};
   wxString         _remoteProcess{};
   wxString         _coredumpPath{};
   bool             _isBreakAtMain = false;
   bool             _isValid       = false;
};
