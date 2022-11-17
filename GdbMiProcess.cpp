#include "GdbMiProcess.h"
#include <GdbMiDriver.hpp>
#include <array>
#include <cstring>
#include <pty.h>
#include <regex>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#define BUFF_SIZE 1024 * 64

const char DELIMITER = '\n';

void tokenize(const std::string &str, std::vector<std::string> &token_v) {
   ssize_t start = str.find_first_not_of(DELIMITER);
   ssize_t end   = start;

   while (start != std::string::npos) {
      // Find next occurence of delimiter
      end = str.find(DELIMITER, start);
      // Push back the token found into vector
      token_v.push_back(str.substr(start, end - start));
      // Skip all occurences of the delimiter to find new start
      start = str.find_first_not_of(DELIMITER, end);
   }
}

static void readerThread(GdbMiProcess *_gdbProcess);

GdbMiProcess::GdbMiProcess() {}

void GdbMiProcess::startGdbProcess(const std::string &gdbPath, std::vector<std::string> &args) {
   int                               i              = 0;
   int                               _gdbTtySlaveFd = 0;
   std::array<char, SLAVE_NAME_SIZE> slavename{};
   std::vector<std::string>          argList;

   argList.push_back(gdbPath);
   argList.emplace_back("--nw");
   argList.emplace_back("--quiet");
   argList.emplace_back("-i=mi3");
   argList.insert(argList.end(), args.begin(), args.end());

   slavename.fill(0);

   auto *local_argv = new char *[argList.size() + 1];

   for (const auto &a : argList) {
      local_argv[i++] = strdup(a.c_str());
   }
   local_argv[i] = nullptr;

   slavename.fill(0);
   openpty(&_gdbInferiorFd, &_gdbTtySlaveFd, &slavename[0], nullptr, nullptr);

   slavename.fill(0);
   _gdbProcessPid = forkpty(&_gdbMiFd, &slavename[0], nullptr, nullptr);

   if (_gdbProcessPid == -1) { /* error, free memory and return  */
      if (_gdbInferiorFd != -1) {
         close(_gdbInferiorFd);
      }
      if (_gdbMiFd != -1) {
         close(_gdbMiFd);
      }
   } else if (_gdbProcessPid == 0) { /* child */
      execvp(local_argv[0], local_argv);
      exit(0);
   }

   for (auto i = 0; i < (int)argList.size() + 1; i++) {
      delete local_argv[i];
   }

   // disable ECHO
   struct termios termio {};

   tcgetattr(_gdbMiFd, &termio);
   cfmakeraw(&termio);

   termio.c_lflag = ICANON;
   termio.c_oflag = ONOCR | ONLRET;

   tcsetattr(_gdbMiFd, TCSANOW, &termio);

   _t = new std::thread{readerThread, this};
}

void GdbMiProcess::terminate() {
   _quit = true;
   _t->join();
   delete _t;
   _t = nullptr;

   close(_gdbMiFd);
   close(_gdbInferiorFd);
   _gdbInferiorFd = -1;
   _gdbMiFd       = -1;
   _pendingLine.clear();
   _quit = false;

   kill(_gdbProcessPid, SIGTERM);
   usleep(100000);

   int status(0);

   if (waitpid(_gdbProcessPid, &status, WNOHANG) != _gdbProcessPid) {
      kill(_gdbProcessPid, SIGKILL);
      waitpid(_gdbProcessPid, &status, 0);
   }

   _gdbProcessPid = -1;

   std::cout << "GDB process terminated" << std::endl;
   if (_imiprocess != nullptr) {
      _imiprocess->handleTerminated();
   }
}

static void writeData(const std::string &data, int fd) {
   std::string tmpbuf     = data;
   const int   chunk_size = 1024;

   while (!tmpbuf.empty()) {
      auto bytes_written = ::write(fd, tmpbuf.c_str(), tmpbuf.length() > chunk_size ? chunk_size : tmpbuf.length());
      if (bytes_written <= 0) {
         return;
      }
      tmpbuf.erase(0, bytes_written);
   }
}

void GdbMiProcess::writeInferiorData(const std::string &data) { writeData(data, _gdbInferiorFd); }

void GdbMiProcess::writeMiData(const std::string &data) { writeData(data + "\n", _gdbMiFd); }

auto GdbMiProcess::pumpData() -> bool {
   fd_set  rs;
   timeval timeout{};

   FD_ZERO(&rs);
   FD_SET(_gdbMiFd, &rs);
   FD_SET(_gdbInferiorFd, &rs);

   timeout.tv_sec  = 0;     // 0 seconds
   timeout.tv_usec = 50000; // 50 ms

   int errCode(0);
   errno = 0;

   int maxFd = 0;
   maxFd     = maxFd > _gdbMiFd ? maxFd : _gdbMiFd;
   maxFd     = maxFd > _gdbInferiorFd ? maxFd : _gdbInferiorFd;

   int rc = select(maxFd + 1, &rs, nullptr, nullptr, &timeout);

   errCode = errno;

   if (rc == 0) {
      return true; // timeout
   }
   if (errCode == EINTR || errCode == EAGAIN) {
      return true;
   }
   if (rc == -1 && errCode == EINTR) {
      return true; /* if the signal interrupted system call keep going */
   }
   if (rc == -1) {
      return false;
   }

   if (FD_ISSET(_gdbInferiorFd, &rs)) {
      return handleData(_gdbInferiorFd);
   }
   if (FD_ISSET(_gdbMiFd, &rs)) {
      return handleData(_gdbMiFd);
   }

   // Process terminated
   // the exit code will be set in the sigchld event handler
   return false;
}

auto GdbMiProcess::handleData(int fileDescriptor) -> bool {
   const int                  buffSize = BUFF_SIZE + 1;
   std::array<char, buffSize> buff{};
   auto                       bytesRead = ::read(fileDescriptor, &buff[0], buffSize);

   if (bytesRead > 0) {
      std::string data(&buff[0], bytesRead);

      if (fileDescriptor == _gdbInferiorFd) {

         if (_imiprocess != nullptr) {
            _imiprocess->handleInferiorData(data);
         }
      } else if (fileDescriptor == _gdbMiFd) {
         //         printf("%s", &buff[0]);
         //       fflush(stdout);

         std::vector<std::string> lines;
         int                      lastCutPos = 0;
         std::size_t              found      = 0;

         if (!_pendingLine.empty()) {
            //            std::cout << "===========================" << data;
            data = _pendingLine + data;
         }

         do {
            found = data.find('\n', lastCutPos);

            if (found != std::string::npos) {
               // std::cout << "last cut position= " << lastCutPos << "found at =" << found << std::endl;
               auto str = data.substr(lastCutPos, (found + 1) - lastCutPos) + '\n';
               // std::cout << "found the line " << str << std::endl;
               lastCutPos = found + 1;
               lines.push_back(str);
               _pendingLine.clear();
            }
         } while (found != std::string::npos);

         if (lastCutPos != data.size()) {
            _pendingLine = data.substr(lastCutPos, data.size() - lastCutPos);
            // std::cout << "-----------------------------------------------------------------------------------------------------" << std::endl;
            // std::cout << "===================pending line " << _pendingLine;
            // std::cout << "-----------------------------------------------------------------------------------------------------" << std::endl;
         }

         for (const auto &line : lines) {
            //            std::cout << x;
            JA::GdbMiDriver drv;
            auto           *output = drv.parse(line);

            if (_imiprocess != nullptr && (output != nullptr)) {
               //      output->print();
               _imiprocess->handleMiOutput(output);
            }
         }
      }
      return true;
   }
   return false;
}

auto GdbMiProcess::getInferiorTtyName() -> std::string {
   auto *name = ptsname(_gdbInferiorFd);
   return std::string{name};
}

void readerThread(GdbMiProcess *_gdbProcess) {
   while (true) {
      // Did we get a request to terminate?
      if (_gdbProcess->_quit || !_gdbProcess->pumpData()) {
         break;
      }
   }
}
