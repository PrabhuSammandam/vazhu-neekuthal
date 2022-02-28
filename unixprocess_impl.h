#pragma once

#include "asyncprocess.h"
#include "processreaderthread.h"

class wxTerminal;

class UnixProcessImpl : public IProcess {
   int                  m_readHandle;
   int                  m_stderrHandle = wxNOT_FOUND;
   int                  m_writeHandle;
   ProcessReaderThread *m_thr = nullptr;
   wxString             m_tty;
   friend class wxTerminal;

 private:
   void StartReaderThread();
   auto ReadFromFd(int fd, fd_set &rset, wxString &output) -> bool;

 public:
   explicit UnixProcessImpl(wxEvtHandler *parent);
   ~UnixProcessImpl() override;

   static auto Execute(wxEvtHandler *parent, const wxString &cmd, size_t flags, const wxString &workingDirectory = wxEmptyString, IProcessCallback *cb = nullptr) -> IProcess *;

   void SetReadHandle(const int &readHandle) { this->m_readHandle = readHandle; }
   void SetWriteHandler(const int &writeHandler) { this->m_writeHandle = writeHandler; }
   auto GetReadHandle() const -> int { return m_readHandle; }
   auto GetStderrHandle() const -> int { return m_stderrHandle; }
   auto GetWriteHandle() const -> int { return m_writeHandle; }
   void SetStderrHandle(int stderrHandle) { this->m_stderrHandle = stderrHandle; }
   void SetTty(const wxString &tty) { this->m_tty = tty; }
   auto GetTty() const -> const wxString & { return m_tty; }
   // overrides
   void Cleanup() override;
   auto IsAlive() -> bool override;
   auto Read(wxString &buff, wxString &buffErr) -> bool override;
   auto Write(const wxString &buff) -> bool override;
   auto Write(const std::string &buff) -> bool override;
   auto WriteRaw(const wxString &buff) -> bool override;
   auto WriteRaw(const std::string &buff) -> bool override;
   void Terminate() override;
   auto WriteToConsole(const wxString &buff) -> bool override;
   void Detach() override;
   void Signal(wxSignal sig) override;
};
