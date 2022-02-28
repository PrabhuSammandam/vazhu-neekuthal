#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "Geometry.h"
#include "ScintillaTypes.h"
#include "Platform.h"

#include "jaeUtils.h"
#include "wx/display.h"
#include "wx/gdicmn.h"
#include "wx/settings.h"
#include "wx/window.h"
#include <wx/menu.h>

using namespace Scintilla::Internal;

Window::~Window() {}

void Window::Destroy() noexcept {
   if (wid != nullptr) {
      Show(false);
      auto *window = static_cast<wxWindow *>(wid);
      window->Destroy();
   }
   wid = nullptr;
}

auto Window::GetPosition() const -> PRectangle {
   if (wid == nullptr) {
      return PRectangle();
   }
   auto * window = static_cast<wxWindow *>(wid);
   wxRect rc(window->GetPosition(), window->GetSize());

   return PRectangleFromwxRect(rc);
}

void Window::SetPosition(PRectangle rc) {
   wxRect r = wxRectFromPRectangle(rc);
   static_cast<wxWindow *>(wid)->SetSize(r);
}

void Window::SetPositionRelative(PRectangle rc, const Window *relativeTo) {
   auto *relativeWin = static_cast<wxWindow *>(relativeTo->wid);

   wxPoint position = relativeWin->GetScreenPosition();
   position.x       = wxRound(position.x + rc.left);
   position.y       = wxRound(position.y + rc.top);

   const wxRect displayRect = wxDisplay(relativeWin).GetClientArea();

   if (position.x < displayRect.GetLeft()) {
      position.x = displayRect.GetLeft();
   }

   const int width = rc.Width();
   if (width > displayRect.GetWidth()) {
      // We want to show at least the beginning of the window.
      position.x = displayRect.GetLeft();
   } else if (position.x + width > displayRect.GetRight()) {
      position.x = displayRect.GetRight() - width;
   }

   const int height = rc.Height();
   if (position.y + height > displayRect.GetBottom()) {
      position.y = displayRect.GetBottom() - height;
   }

   auto *window = static_cast<wxWindow *>(wid);
   window->SetSize(position.x, position.y, width, height);
}

auto Window::GetClientPosition() const -> PRectangle {
   if (wid == nullptr) {
      return PRectangle();
   }
   wxSize sz = static_cast<wxWindow *>(wid)->GetClientSize();
   return PRectangle(0, 0, sz.x, sz.y);
}

void Window::Show(bool show) { static_cast<wxWindow *>(wid)->Show(show); }

void Window::InvalidateAll() { static_cast<wxWindow *>(wid)->Refresh(false); }

void Window::InvalidateRectangle(PRectangle rc) {
   if (wid != nullptr) {
      wxRect r = wxRectFromPRectangle(rc);
      static_cast<wxWindow *>(wid)->Refresh(false, &r);
   }
}

void Window::SetCursor(Cursor curs) {
   wxStockCursor cursorId = wxCURSOR_DEFAULT;

   switch (curs) {
   case Cursor::text:
      cursorId = wxCURSOR_IBEAM;
      break;
   case Cursor::arrow:
   case Cursor::up:
      cursorId = wxCURSOR_ARROW; // ** no up arrow...  wxCURSOR_UPARROW;
      break;
   case Cursor::wait:
      cursorId = wxCURSOR_WAIT;
      break;
   case Cursor::horizontal:
      cursorId = wxCURSOR_SIZEWE;
      break;
   case Cursor::vertical:
      cursorId = wxCURSOR_SIZENS;
      break;
   case Cursor::reverseArrow:
      cursorId = wxCURSOR_RIGHT_ARROW;
      break;
   case Cursor::hand:
      cursorId = wxCURSOR_HAND;
      break;
   default:
      cursorId = wxCURSOR_ARROW;
      break;
   }

   if (curs != cursorLast) {
      wxCursor wc = wxCursor(cursorId);
      if (wid != nullptr) {
         static_cast<wxWindow *>(wid)->SetCursor(wc);
      }
      cursorLast = curs;
   }
}

// Returns rectangle of monitor pt is on
auto Window::GetMonitorRect(Point pt) -> PRectangle {
   if (wid == nullptr) {
      return PRectangle();
   }
   // Get the display the point is found on
   int       n = wxDisplay::GetFromPoint(wxPoint(wxRound(pt.x), wxRound(pt.y)));
   wxDisplay dpy(n == wxNOT_FOUND ? 0 : n);
   return PRectangleFromwxRect(dpy.GetGeometry());
}
