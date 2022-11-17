#pragma once

#include "GdbMiResp.h"
#include <atomic>
#include <string>
#include <thread>
#include <vector>

constexpr int SLAVE_NAME_SIZE = 128;

class IGdbMiProcess {
 public:
   virtual ~IGdbMiProcess()                              = default;
   virtual void handleInferiorData(std::string &data)    = 0;
   virtual void handleTerminated()                       = 0;
   virtual void handleMiOutput(GdbMiOutput *gdbMiOutput) = 0;
};

class GdbMiProcess {
 public:
   GdbMiProcess();
   void startGdbProcess(const std::string &gdbPath, std::vector<std::string> &args);
   void terminate();
   void writeInferiorData(const std::string &data);
   void writeMiData(const std::string &data);
   auto pumpData() -> bool;
   auto handleData(int fileDescriptor) -> bool;
   auto getInferiorTtyName() -> std::string;

   int         _gdbInferiorFd = -1; /* read and write fd for gdb inferior which is called by command 'set inferior-tty /dev/pts/0' */
   int         _gdbMiFd       = -1; // read and write for the mi interpreter
   int         _gdbProcessPid = -1;
   std::string _pendingLine;

   IGdbMiProcess   *_imiprocess = nullptr;
   std::thread     *_t          = nullptr;
   std::atomic_bool _quit       = false;
};
