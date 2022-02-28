#include <iostream>
#include "asyncprocess.h"
#include "processreaderthread.h"
#include "wx/event.h"
#include "CustomEvents.h"

#if defined(__WXGTK__) || defined(__WXMAC__)
#include <sys/wait.h>
#endif

ProcessReaderThread::ProcessReaderThread() : wxThread(wxTHREAD_JOINABLE) {}

ProcessReaderThread::~ProcessReaderThread() { m_notifiedWindow = nullptr; }

auto ProcessReaderThread::Entry() -> void * {
   while (true) {
      // Did we get a request to terminate?
      if (TestDestroy()) {
         break;
      }

      if (m_process != nullptr) {
         wxString buff;
         wxString buffErr;
         if (m_process->IsRedirect()) {
            if (m_process->Read(buff, buffErr)) {
               if (!buff.IsEmpty() || !buffErr.IsEmpty()) {
                  // If we got a callback object, use it
                  // std::cout << buff << std::endl;
                  // std::cout << buffErr << std::endl;
                  if ((m_process != nullptr) && (m_process->GetCallback() != nullptr)) {
                     m_process->GetCallback()->CallAfter(&IProcessCallback::OnProcessOutput, buff);

                  } else {
                     // We fire an event per data (stderr/stdout)
                     if (!buff.IsEmpty()) {
                        // fallback to the event system
                        // we got some data, send event to parent
                        AsyncProcessEvent e(wxEVT_PROCESS_OUTPUT);
                        e.SetOutput(buff);
                        e.SetProcess(m_process);
                        if (m_notifiedWindow != nullptr) {
                           m_notifiedWindow->AddPendingEvent(e);
                        }
                     }
                     if (!buffErr.IsEmpty()) {
                        // we got some data, send event to parent
                        AsyncProcessEvent e(wxEVT_PROCESS_STDERR);
                        e.SetOutput(buffErr);
                        e.SetProcess(m_process);
                        if (m_notifiedWindow != nullptr) {
                           m_notifiedWindow->AddPendingEvent(e);
                        }
                     }
                  }
               }
            } else {

               // Process terminated, exit
               // If we got a callback object, use it
               NotifyTerminated();
               break;
            }
         } else {
            // Check if the process is alive
            if (!m_process->IsAlive()) {
               // Notify about termination
               NotifyTerminated();
               break;
            }
            wxThread::Sleep(10);
         }
      } else {
         // No process??
         break;
      }
   }
   m_process = nullptr;
   return nullptr;
}

void ProcessReaderThread::Stop() {
   // Notify the thread to exit and
   // wait for it
   if (IsAlive()) {
      Delete(nullptr, wxTHREAD_WAIT_BLOCK);
   } else {
      Wait(wxTHREAD_WAIT_BLOCK);
   }
}

void ProcessReaderThread::Start(int priority) {
   Create();
   SetPriority(priority);
   Run();
}

void ProcessReaderThread::NotifyTerminated() {
   // Process terminated, exit
   // If we got a callback object, use it
   if ((m_process != nullptr) && (m_process->GetCallback() != nullptr)) {
      m_process->GetCallback()->CallAfter(&IProcessCallback::OnProcessTerminated);

   } else {
      // fallback to the event system
      AsyncProcessEvent e(wxEVT_PROCESS_TERMINATED);
      e.SetProcess(m_process);
      if (m_notifiedWindow != nullptr) {
         m_notifiedWindow->AddPendingEvent(e);
      }
   }
}
