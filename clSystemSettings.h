#ifndef CLSYSTEMSETTINGS_H
#define CLSYSTEMSETTINGS_H

#include <wx/settings.h>
#include <wx/event.h>
#include "clColours.h"
#include "wx/dlimpexp.h"
#include <wx/sharedptr.h>

class clCommandEvent : public wxCommandEvent {
 protected:
   wxSharedPtr<wxClientData> m_ptr;
   wxArrayString             m_strings;
   wxString                  m_fileName;
   wxString                  m_oldName;
   bool                      m_answer;
   bool                      m_allowed;
   int                       m_lineNumber;
   bool                      m_selected;

 public:
   clCommandEvent(wxEventType commandType = wxEVT_NULL, int winid = 0) : wxCommandEvent(commandType, winid), m_answer(false), m_allowed(true), m_lineNumber(0), m_selected(false) {}
   clCommandEvent(const clCommandEvent &event) : wxCommandEvent(event), m_answer(false), m_allowed(true) { *this = event; }
   clCommandEvent &operator=(const clCommandEvent &src) {
      m_strings.clear();
      m_ptr = src.m_ptr;
      for (size_t i = 0; i < src.m_strings.size(); ++i) {
         m_strings.Add(src.m_strings.Item(i).c_str());
      }
      m_fileName   = src.m_fileName;
      m_answer     = src.m_answer;
      m_allowed    = src.m_allowed;
      m_oldName    = src.m_oldName;
      m_lineNumber = src.m_lineNumber;
      m_selected   = src.m_selected;

      // Copy wxCommandEvent members here
      m_eventType  = src.m_eventType;
      m_id         = src.m_id;
      m_cmdString  = src.m_cmdString;
      m_commandInt = src.m_commandInt;
      m_extraLong  = src.m_extraLong;
      return *this;
   }

   virtual ~clCommandEvent() { m_ptr.reset(); }

   clCommandEvent &SetSelected(bool selected) {
      this->m_selected = selected;
      return *this;
   }
   bool IsSelected() const { return m_selected; }

   // Veto management
   void Veto() { this->m_allowed = false; }
   void Allow() { this->m_allowed = true; }

   // Hides wxCommandEvent::Set{Get}ClientObject
   void SetClientObject(wxClientData *clientObject) { m_ptr = clientObject; }

   wxClientData *   GetClientObject() const { return m_ptr.get(); }
   virtual wxEvent *Clone() const {
      clCommandEvent *new_event = new clCommandEvent(*this);
      return new_event;
   }

   clCommandEvent &SetLineNumber(int lineNumber) {
      this->m_lineNumber = lineNumber;
      return *this;
   }
   int             GetLineNumber() const { return m_lineNumber; }
   clCommandEvent &SetAllowed(bool allowed) {
      this->m_allowed = allowed;
      return *this;
   }
   clCommandEvent &SetAnswer(bool answer) {
      this->m_answer = answer;
      return *this;
   }
   clCommandEvent &SetFileName(const wxString &fileName) {
      this->m_fileName = fileName;
      return *this;
   }
   clCommandEvent &SetOldName(const wxString &oldName) {
      this->m_oldName = oldName;
      return *this;
   }
   clCommandEvent &SetPtr(const wxSharedPtr<wxClientData> &ptr) {
      this->m_ptr = ptr;
      return *this;
   }
   clCommandEvent &SetStrings(const wxArrayString &strings) {
      this->m_strings = strings;
      return *this;
   }
   bool                             IsAllowed() const { return m_allowed; }
   bool                             IsAnswer() const { return m_answer; }
   const wxString &                 GetFileName() const { return m_fileName; }
   const wxString &                 GetOldName() const { return m_oldName; }
   const wxSharedPtr<wxClientData> &GetPtr() const { return m_ptr; }
   const wxArrayString &            GetStrings() const { return m_strings; }
   wxArrayString &                  GetStrings() { return m_strings; }
};

typedef void (wxEvtHandler::*clCommandEventFunction)(clCommandEvent &);
#define clCommandEventHandler(func) wxEVENT_HANDLER_CAST(clCommandEventFunction, func)

wxDECLARE_EXPORTED_EVENT(WXIMPORT, wxEVT_SYS_COLOURS_CHANGED, clCommandEvent);

class clSystemSettings : public wxEvtHandler, public wxSystemSettings {
   static bool      m_useCustomColours;
   static clColours m_customColours;

 protected:
   void OnColoursChanged(clCommandEvent &event);
   clSystemSettings();

 public:
   static clSystemSettings &Get();

   virtual ~clSystemSettings();
   static wxColour GetColour(wxSystemColour index);
};

#endif // CLSYSTEMSETTINGS_H
