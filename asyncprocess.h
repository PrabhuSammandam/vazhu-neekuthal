#pragma once

#include <map>
#include <vector>
#include <wx/event.h>
#include <wx/sharedptr.h>
#include <wx/string.h>
#include <wx/utils.h>

typedef std::vector<std::pair<wxString, wxString>> clEnvList_t;
typedef std::unordered_map<wxString, wxString>     wxStringTable_t;
typedef wxStringTable_t                            wxStringMap_t; // aliases

enum IProcessCreateFlags {
   IProcessCreateDefault           = (1 << 0), // Default: create process with no console window
   IProcessCreateConsole           = (1 << 1), // Create with console window shown
   IProcessCreateWithHiddenConsole = (1 << 2), // Create process with a hidden console
   IProcessCreateSync              = (1 << 3), // Create a synchronous process (i.e. there is no background thread that performs the reads)
   IProcessCreateAsSuperuser       = (1 << 4), // On platforms that support it, start the process as superuser
   IProcessNoRedirect              = (1 << 5),
   IProcessStderrEvent             = (1 << 6), // fire a separate event for stderr output
   IProcessRawOutput               = (1 << 7), // return the process output as is, don't strip anything. By default CodeLite strips terminal colours escape sequences
   IProcessCreateSSH               = (1 << 8), // Create a remote process, over SSH
   IProcessInteractiveSSH          = (1 << 9),
};

class IProcess;

// Helper class for applying the environment before launching the process
class clEnvironment {
   const clEnvList_t *m_env = nullptr;
   wxStringMap_t      m_oldEnv;

 public:
   clEnvironment(const clEnvList_t *env) : m_env(env) {
      if (m_env != nullptr) {
         for (const auto &p : (*m_env)) {
            const wxString &name  = p.first;
            const wxString &value = p.second;

            wxString oldValue;
            // If an environment variable with this name already exists, keep its old value
            // as we want to restore it later
            if (::wxGetEnv(name, &oldValue)) {
               m_oldEnv.insert({name, oldValue});
            }
            // set the new value
            ::wxSetEnv(name, value);
         }
      }
   }
   ~clEnvironment() {
      if (m_env != nullptr) {
         for (const auto &p : (*m_env)) {
            const wxString &name = p.first;
            if (m_oldEnv.count(name)) {
               ::wxSetEnv(name, m_oldEnv[name]);
            } else {
               ::wxUnsetEnv(name);
            }
         }
      }
      m_oldEnv.clear();
   }
};

class IProcessCallback : public wxEvtHandler {
 public:
   virtual void OnProcessOutput(const wxString &str) { wxUnusedVar(str); }
   virtual void OnProcessTerminated() {}
};

class IProcess : public wxEvtHandler {
 protected:
   wxEvtHandler *    m_parent   = nullptr;
   int               m_pid      = -1;
   bool              m_hardKill = true;
   IProcessCallback *m_callback = nullptr;
   size_t            m_flags    = 0;

 public:
   typedef wxSharedPtr<IProcess> Ptr_t;

 public:
   IProcess(wxEvtHandler *parent) : m_parent(parent) {}
   ~IProcess() override = default;

   // Handle process exit code. This is done this way this
   // under Linux / Mac the exit code is returned only after the signal child has been
   // handled by codelite
   static void SetProcessExitCode(int pid, int exitCode);
   static auto GetProcessExitCode(int pid, int &exitCode) -> bool;

   // Stop notifying the parent window about input/output from the process
   // this is useful when we wish to terminate the process onExit but we don't want
   // to know about its termination
   virtual void Detach() = 0;

   // Read from process stdout - return immediately if no data is available
   virtual auto Read(wxString &buff, wxString &buffErr) -> bool = 0;

   // Write to the process stdin
   // This version add LF to the buffer
   virtual auto Write(const wxString &buff) -> bool = 0;

   // ANSI version
   // This version add LF to the buffer
   virtual auto Write(const std::string &buff) -> bool = 0;

   // Write to the process stdin
   virtual auto WriteRaw(const wxString &buff) -> bool = 0;

   // ANSI version
   virtual auto WriteRaw(const std::string &buff) -> bool = 0;

   /**
    * @brief wait for process to terminate and return all its output to the caller
    * Note that this function is blocking
    */
   virtual void WaitForTerminate(wxString &output);

   /**
    * @brief this method is mostly needed on MSW where writing a password
    * is done directly on the console buffer rather than its stdin
    */
   virtual auto WriteToConsole(const wxString &buff) -> bool = 0;

   // Return true if the process is still alive
   virtual auto IsAlive() -> bool = 0;

   // Clean the process resources and kill the process if it is
   // still alive
   virtual void Cleanup() = 0;

   // Terminate the process. It is recommended to use this method
   // so it will invoke the 'Cleaup' procedure and the process
   // termination event will be sent out
   virtual void Terminate() = 0;

   void SetPid(int pid) { this->m_pid = pid; }
   auto GetPid() const -> int { return m_pid; }

   void SetHardKill(bool hardKill) { this->m_hardKill = hardKill; }
   auto GetHardKill() const -> bool { return m_hardKill; }
   auto GetCallback() -> IProcessCallback * { return m_callback; }

   /**
    * @brief send signal to the process
    */
   virtual void Signal(wxSignal sig) = 0;

   /**
    * @brief do we have process redirect enabled?
    */
   auto IsRedirect() const -> bool { return !(m_flags & IProcessNoRedirect); }
};

// Help method
/**
 * @brief start process
 * @param parent the parent. all events will be sent to this object
 * @param cmd command line to execute
 * @param flags possible creation flag
 * @param workingDir set the working directory of the executed process
 * @return
 */
auto CreateAsyncProcess(wxEvtHandler *parent, const wxString &cmd, size_t flags = IProcessCreateDefault, const wxString &workingDir = wxEmptyString, const clEnvList_t *env = nullptr,
                        wxUIntPtr clientData = 0) -> IProcess *;

/**
 * @brief create synchronus process
 * @param cmd command to execute
 * @param flags process creation flags
 * @param workingDir working directory for the new process
 * @return IPorcess handle on succcess
 */
auto CreateSyncProcess(const wxString &cmd, size_t flags = IProcessCreateDefault, const wxString &workingDir = wxEmptyString, const clEnvList_t *env = nullptr) -> IProcess *;

/**
 * @brief start process
 * @brief cb callback object. Instead of events, OnProcessOutput and OnProcessTerminated will be called respectively
 * @param parent the parent. all events will be sent to this object
 * @param cmd command line to execute
 * @param flags possible creation flag
 * @param workingDir set the working directory of the executed process
 * @return
 */
auto CreateAsyncProcessCB(wxEvtHandler *parent, IProcessCallback *cb, const wxString &cmd, size_t flags = IProcessCreateDefault, const wxString &workingDir = wxEmptyString,
                          const clEnvList_t *env = nullptr) -> IProcess *;
