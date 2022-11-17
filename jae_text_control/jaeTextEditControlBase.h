#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"

#include "ScintillaCall.h"
#include "jaeScintilla.h"
#include <wx/control.h>
#include <wx/scrolbar.h>
#include <wx/graphics.h>

class jaeScintilla;

class jaeTextEditControlBase : public wxControl, public Scintilla::ScintillaCall {
 public:
   explicit jaeTextEditControlBase(wxWindow *parent, wxWindowID id = wxID_ANY, const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxDefaultSize, long style = 0,
                                   const wxString &name = "jaeTextEditControl");

   ~jaeTextEditControlBase() override;

   jaeTextEditControlBase(const jaeTextEditControlBase &) = delete;
   jaeTextEditControlBase(jaeTextEditControlBase &&)      = delete;
   auto operator=(const jaeTextEditControlBase &) -> jaeTextEditControlBase & = delete;
   auto operator=(jaeTextEditControlBase &&) -> jaeTextEditControlBase & = delete;

   auto Create(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style, const wxString &name) -> bool;
   auto Send(unsigned int iMessage, uptr_t wParam, sptr_t lParam) const -> sptr_t;

   void SetActive(bool isActive) { _active = isActive; }
   auto IsActive() const -> bool { return _active; }

   void HandleNotifyChange();
   void HandleNotifyParent(Scintilla::NotificationData scn);

   // Can be used to prevent the EVT_CHAR handler from adding the char
   auto GetLastKeydownProcessed() const -> bool { return _lastKeyDownConsumed; }
   void SetLastKeydownProcessed(bool val) { _lastKeyDownConsumed = val; }

   /* window event handling */
   void OnPaint(wxPaintEvent &evt);
   void OnScrollWin(wxScrollWinEvent &evt);
   void OnScroll(wxScrollEvent &evt);
   void OnSize(wxSizeEvent &evt);
   void OnMouseLeftDown(wxMouseEvent &evt);
   void OnMouseRightDown(wxMouseEvent &evt);
   void OnMouseMove(wxMouseEvent &evt);
   void OnMouseLeftUp(wxMouseEvent &evt);
   void OnMouseMiddleUp(wxMouseEvent &evt);
   void OnContextMenu(wxContextMenuEvent &evt);
   void OnMouseWheel(wxMouseEvent &evt);
   void OnChar(wxKeyEvent &evt);
   void OnKeyDown(wxKeyEvent &evt);
   void OnLoseFocus(wxFocusEvent &evt);
   void OnGainFocus(wxFocusEvent &evt);
   void OnDPIChanged(wxDPIChangedEvent &evt);
   void OnSysColourChanged(wxSysColourChangedEvent &evt);
   void OnEraseBackground(wxEraseEvent &evt);
   void OnMenu(wxCommandEvent &evt);
   void OnListBox(wxCommandEvent &evt);
   void OnIdle(wxIdleEvent &evt);
   void OnMouseCaptureLost(wxMouseCaptureLostEvent &evt);

 protected:
   virtual void DoContextMenu(wxContextMenuEvent &evt);

 private:
   void InitEventHandlers();
   auto getGraphicsCtx() -> wxGraphicsContext *;

 protected:
   wxScrollBar * _vScrollBar   = nullptr;
   wxScrollBar * _hScrollBar   = nullptr;
   jaeScintilla *_scintillaObj = nullptr;
   wxStopWatch   _stopWatch{};
   bool          _lastKeyDownConsumed = false;
   bool          _active              = true;

   friend class jaeScintilla;
};
