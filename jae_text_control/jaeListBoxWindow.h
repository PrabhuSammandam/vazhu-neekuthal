#pragma once

#include "jaeListBox.h"
#include "wx/dcclient.h"
#include "wx/sizer.h"
#include <wx/bitmap.h>
#include <wx/popupwin.h>

class jaePopupWindow : public wxPopupWindow {
 public:
   explicit jaePopupWindow(wxWindow *parent) : wxPopupWindow{parent} {}
   ~jaePopupWindow() override{};
   auto Destroy() -> bool wxOVERRIDE {
      if (!wxPendingDelete.Member(this)) {
         wxPendingDelete.Append(this);
      }

      return true;
   }
   auto AcceptsFocus() const -> bool wxOVERRIDE { return false; }

 protected:
   void DoSetSize(int x, int y, int width, int height, int sizeFlags = wxSIZE_AUTO) wxOVERRIDE {
      wxPoint pos(x, y);
      if (pos.IsFullySpecified() && !m_relPos.IsFullySpecified()) {
         m_relPos = GetParent()->ScreenToClient(pos);
      }

      m_absPos = GetParent()->ClientToScreen(m_relPos);

      wxPopupWindow::DoSetSize(m_absPos.x, m_absPos.y, width, height, sizeFlags);
   }

   void OnParentMove(wxMoveEvent &event) {
      SetPosition(m_absPos);
      event.Skip();
   }
#if defined(__WXOSX_COCOA__) || (defined(__WXGTK__) && !wxSTC_POPUP_IS_FRAME)
   void OnIconize(wxIconizeEvent &event) { Show(!event.IsIconized()); }
#elif !wxSTC_POPUP_IS_CUSTOM
   void OnFocus(wxFocusEvent &event) {
#if wxSTC_POPUP_IS_FRAME
      ActivateParent();
#endif

      GetParent()->SetFocus();
      event.Skip();
   }
#endif

 private:
   wxPoint   m_relPos;
   wxPoint   m_absPos;
   wxWindow *m_tlw = nullptr;
};

// A popup window to place the wxSTCListBox upon
class jaeListBoxWindow : public jaePopupWindow {
 public:
   jaeListBoxWindow(wxWindow *parent, jaePortListBox **lb, jaePortListBoxVisualData *v, int h, int /*tech*/) : jaePopupWindow(parent) {
      *lb = new jaePortListBox(this, v, h);

      const int borderThickness = FromDIP(1);
      auto *    bSizer          = new wxBoxSizer(wxVERTICAL);
      bSizer->Add(*lb, 1, wxEXPAND | wxALL, borderThickness);
      SetSizer(bSizer);
      (*lb)->SetContainerBorderSize(borderThickness);

      // When drawing highlighting in listctrl style with wxRendererNative on MSW,
      // the colours used seem to be based on the background of the parent window.
      // So manually paint this window to give it the border colour instead of
      // setting the background colour.
      m_visualData = v;
      Bind(wxEVT_PAINT, &jaeListBoxWindow::OnPaint, this);
      SetBackgroundStyle(wxBG_STYLE_PAINT);
   }

   ~jaeListBoxWindow() {}

 protected:
   void OnPaint(wxPaintEvent & /*unused*/) {
      wxPaintDC dc(this);
      dc.SetBackground(m_visualData->GetBorderColour());
      dc.Clear();
   }

 private:
   jaePortListBoxVisualData *m_visualData;
};
