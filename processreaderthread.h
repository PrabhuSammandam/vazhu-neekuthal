#pragma once

#include "wx/event.h"
#include "wx/thread.h"
#include <deque>

class IProcess;

class ProcessReaderThread : public wxThread {
 protected:
   wxEvtHandler *m_notifiedWindow = nullptr;
   IProcess *    m_process        = nullptr;

 protected:
   void NotifyTerminated();

 public:
   ProcessReaderThread();
   ~ProcessReaderThread() override;

   auto Entry() -> void * override;
   void OnExit() override{};
   void SetNotifyWindow(wxEvtHandler *evtHandler) { m_notifiedWindow = evtHandler; }
   void Stop();
   void Start(int priority = WXTHREAD_DEFAULT_PRIORITY);
   void SetProcess(IProcess *proc) { m_process = proc; }
};
