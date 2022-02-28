class wxEvtHandler;
class IProcess;

#include <wx/string.h>
#include "unixprocess_impl.h"

IProcess *CreateAsyncProcess(wxEvtHandler *parent, const wxString &cmd, size_t flags, const wxString &workingDir, const clEnvList_t *env, wxUIntPtr clientData) {
   clEnvironment e(env);
   return UnixProcessImpl::Execute(parent, cmd, flags, workingDir);
}

IProcess *CreateAsyncProcessCB(wxEvtHandler *parent, IProcessCallback *cb, const wxString &cmd, size_t flags, const wxString &workingDir, const clEnvList_t *env) {
   clEnvironment e(env);
   return UnixProcessImpl::Execute(parent, cmd, flags, workingDir, cb);
}

IProcess *CreateSyncProcess(const wxString &cmd, size_t flags, const wxString &workingDir, const clEnvList_t *env) {
   clEnvironment e(env);
   return UnixProcessImpl::Execute(NULL, cmd, flags | IProcessCreateSync, workingDir);
}

// Static methods:
bool IProcess::GetProcessExitCode(int pid, int &exitCode) {
   wxUnusedVar(pid);
   wxUnusedVar(exitCode);

   exitCode = 0;
   return true;
}

void IProcess::SetProcessExitCode(int pid, int exitCode) {
   wxUnusedVar(pid);
   wxUnusedVar(exitCode);
}

void IProcess::WaitForTerminate(wxString &output) {
   if (IsRedirect()) {
      wxString buff;
      wxString buffErr;

      while (Read(buff, buffErr)) {
         output << buff;

         if (!buff.IsEmpty() && !buffErr.IsEmpty()) {
            output << "\n";
         }
         output << buffErr;
      }
   } else {
      // Just wait for the process to terminate in a busy loop
      while (IsAlive()) {
         wxThread::Sleep(10);
      }
   }
}
