#include "jaeTextEditControlBase.h"
#include "jaeScintilla.h"
#include "wx/dcclient.h"
#include "wx/dcgraph.h"
#include "wx/event.h"
#include <cstdint>
#include <wx/buffer.h>
#include <wx/listbox.h>
#include <wx/vlbox.h>
#include <wx/stack.h>
#include <jaeScintilla.h>
#include "jaeTextEditControlEvent.h"
#include "wx/gtk/brush.h"
#include "wx/gtk/colour.h"
#include "wx/gtk/window.h"
#include "wx/string.h"
#include <wx/dcbuffer.h>

using namespace Scintilla;
using namespace Scintilla::Internal;

jaeTextEditControlBase::jaeTextEditControlBase(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style, const wxString &name) {
   Create(parent, id, pos, size, style, name);
   InitEventHandlers();

   SetFnPtr(&jaeScintilla::DirectStatusFunction, (intptr_t)_scintillaObj);
}

jaeTextEditControlBase::~jaeTextEditControlBase() {}

auto jaeTextEditControlBase::Create(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style, const wxString &name) -> bool {
   style |= wxVSCROLL | wxHSCROLL;

   if (!wxControl::Create(parent, id, pos, size, style | wxWANTS_CHARS | wxCLIP_CHILDREN, wxDefaultValidator, name)) {
      return false;
   }
   _scintillaObj = new jaeScintilla(this);
   _stopWatch.Start();
   _lastKeyDownConsumed = false;
   _vScrollBar          = nullptr;
   _hScrollBar          = nullptr;
   SetInitialSize(size);

   // Reduces flicker on GTK+/X11
   SetBackgroundStyle(wxBG_STYLE_PAINT);

   // Make sure it can take the focus
   SetCanFocus(true);

   // STC doesn't support RTL languages at all
   SetLayoutDirection(wxLayout_LeftToRight);

   return true;
}

void jaeTextEditControlBase::HandleNotifyChange() {
   jaeTextEditControlEvent evt(wxEVT_JAE_TEXT_EDIT_CONTROL_CHANGE, GetId());
   evt.SetEventObject(this);
   GetEventHandler()->ProcessEvent(evt);
}

static void SetEventText(jaeTextEditControlEvent &evt, const char *text, size_t length) {
   if (text == nullptr) {
      return;
   }

   evt.SetString(wxString::FromUTF8(text, length));
}

void jaeTextEditControlBase::HandleNotifyParent(Scintilla::NotificationData scn) {
   jaeTextEditControlEvent evt(0, GetId());

   evt.SetEventObject(this);
   evt.SetPosition(scn.position);
   evt.SetKey(scn.ch);
   evt.SetModifiers((int)scn.modifiers);

   switch (scn.nmhdr.code) {
   case Notification::StyleNeeded:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_STYLENEEDED);
      break;

   case Notification::CharAdded:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_CHARADDED);
      break;

   case Notification::SavePointReached:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_SAVEPOINTREACHED);
      break;

   case Notification::SavePointLeft:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_SAVEPOINTLEFT);
      break;

   case Notification::ModifyAttemptRO:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_ROMODIFYATTEMPT);
      break;

   case Notification::DoubleClick:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_DOUBLECLICK);
      evt.SetLine(scn.line);
      break;

   case Notification::UpdateUI:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_UPDATEUI);
      evt.SetUpdated((int)scn.updated);
      break;

   case Notification::Modified:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_MODIFIED);
      evt.SetModificationType((int)scn.modificationType);
      SetEventText(evt, scn.text, scn.length);
      evt.SetLength(scn.length);
      evt.SetLinesAdded(scn.linesAdded);
      evt.SetLine(scn.line);
      evt.SetFoldLevelNow((int)scn.foldLevelNow);
      evt.SetFoldLevelPrev((int)scn.foldLevelPrev);
      evt.SetToken(scn.token);
      evt.SetAnnotationLinesAdded(scn.annotationLinesAdded);
      break;

   case Notification::MacroRecord:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_MACRORECORD);
      evt.SetMessage((int)scn.message);
      evt.SetWParam((int)scn.wParam);
      evt.SetLParam((int)scn.lParam);
      break;

   case Notification::MarginClick:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_MARGINCLICK);
      evt.SetMargin(scn.margin);
      break;

   case Notification::NeedShown:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_NEEDSHOWN);
      evt.SetLength((int)scn.length);
      break;

   case Notification::Painted:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_PAINTED);
      break;

   case Notification::UserListSelection:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_USERLISTSELECTION);
      evt.SetListType(scn.listType);
      SetEventText(evt, scn.text, strlen(scn.text));
      evt.SetPosition(scn.lParam);
      evt.SetListCompletionMethod((int)scn.listCompletionMethod);
      break;

   case Notification::DwellStart:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_DWELLSTART);
      evt.SetX(scn.x);
      evt.SetY(scn.y);
      break;

   case Notification::DwellEnd:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_DWELLEND);
      evt.SetX(scn.x);
      evt.SetY(scn.y);
      break;

   case Notification::Zoom:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_ZOOM);
      break;

   case Notification::HotSpotClick:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_HOTSPOT_CLICK);
      break;

   case Notification::HotSpotDoubleClick:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_HOTSPOT_DCLICK);
      break;

   case Notification::HotSpotReleaseClick:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_HOTSPOT_RELEASE_CLICK);
      break;

   case Notification::IndicatorClick:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_INDICATOR_CLICK);
      break;

   case Notification::IndicatorRelease:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_INDICATOR_RELEASE);
      break;

   case Notification::CallTipClick:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_CALLTIP_CLICK);
      break;

   case Notification::AutoCSelection:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_AUTOCOMP_SELECTION);
      evt.SetListType(scn.listType);
      SetEventText(evt, scn.text, strlen(scn.text));
      evt.SetPosition((int)scn.lParam);
      evt.SetListCompletionMethod((int)scn.listCompletionMethod);
      break;

   case Notification::AutoCCancelled:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_AUTOCOMP_CANCELLED);
      break;

   case Notification::AutoCCharDeleted:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_AUTOCOMP_CHAR_DELETED);
      break;

   case Notification::AutoCCompleted:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_AUTOCOMP_COMPLETED);
      evt.SetListType(scn.listType);
      SetEventText(evt, scn.text, strlen(scn.text));
      evt.SetPosition((int)scn.lParam);
      evt.SetListCompletionMethod((int)scn.listCompletionMethod);
      break;

   case Notification::MarginRightClick:
      evt.SetEventType(wxEVT_JAE_TEXT_EDIT_CONTROL_MARGIN_RIGHT_CLICK);
      evt.SetMargin(scn.margin);
      break;

   default:
      return;
   }

   GetEventHandler()->ProcessEvent(evt);
}

void jaeTextEditControlBase::InitEventHandlers() {
   Bind(wxEVT_PAINT, &jaeTextEditControlBase::OnPaint, this);

   Bind(wxEVT_SCROLLWIN_TOP, &jaeTextEditControlBase::OnScrollWin, this);
   Bind(wxEVT_SCROLLWIN_BOTTOM, &jaeTextEditControlBase::OnScrollWin, this);
   Bind(wxEVT_SCROLLWIN_LINEUP, &jaeTextEditControlBase::OnScrollWin, this);
   Bind(wxEVT_SCROLLWIN_LINEDOWN, &jaeTextEditControlBase::OnScrollWin, this);
   Bind(wxEVT_SCROLLWIN_PAGEUP, &jaeTextEditControlBase::OnScrollWin, this);
   Bind(wxEVT_SCROLLWIN_PAGEDOWN, &jaeTextEditControlBase::OnScrollWin, this);
   Bind(wxEVT_SCROLLWIN_THUMBTRACK, &jaeTextEditControlBase::OnScrollWin, this);
   Bind(wxEVT_SCROLLWIN_THUMBRELEASE, &jaeTextEditControlBase::OnScrollWin, this);

   Bind(wxEVT_SCROLL_TOP, &jaeTextEditControlBase::OnScroll, this);
   Bind(wxEVT_SCROLL_BOTTOM, &jaeTextEditControlBase::OnScroll, this);
   Bind(wxEVT_SCROLL_LINEUP, &jaeTextEditControlBase::OnScroll, this);
   Bind(wxEVT_SCROLL_LINEDOWN, &jaeTextEditControlBase::OnScroll, this);
   Bind(wxEVT_SCROLL_PAGEUP, &jaeTextEditControlBase::OnScroll, this);
   Bind(wxEVT_SCROLL_PAGEDOWN, &jaeTextEditControlBase::OnScroll, this);
   Bind(wxEVT_SCROLL_THUMBTRACK, &jaeTextEditControlBase::OnScroll, this);
   Bind(wxEVT_SCROLL_THUMBRELEASE, &jaeTextEditControlBase::OnScroll, this);
   Bind(wxEVT_SCROLL_CHANGED, &jaeTextEditControlBase::OnScroll, this);

   Bind(wxEVT_SIZE, &jaeTextEditControlBase::OnSize, this);
   Bind(wxEVT_LEFT_DOWN, &jaeTextEditControlBase::OnMouseLeftDown, this);
   Bind(wxEVT_LEFT_UP, &jaeTextEditControlBase::OnMouseLeftUp, this);
   Bind(wxEVT_LEFT_DCLICK, &jaeTextEditControlBase::OnMouseLeftDown, this);
   Bind(wxEVT_RIGHT_DOWN, &jaeTextEditControlBase::OnMouseRightDown, this);
   Bind(wxEVT_MOTION, &jaeTextEditControlBase::OnMouseMove, this);
   Bind(wxEVT_CONTEXT_MENU, &jaeTextEditControlBase::OnContextMenu, this);
   Bind(wxEVT_MOUSEWHEEL, &jaeTextEditControlBase::OnMouseWheel, this);
   Bind(wxEVT_MIDDLE_UP, &jaeTextEditControlBase::OnMouseMiddleUp, this);
   Bind(wxEVT_CHAR, &jaeTextEditControlBase::OnChar, this);
   Bind(wxEVT_KEY_DOWN, &jaeTextEditControlBase::OnKeyDown, this);
   Bind(wxEVT_KILL_FOCUS, &jaeTextEditControlBase::OnLoseFocus, this);
   Bind(wxEVT_SET_FOCUS, &jaeTextEditControlBase::OnGainFocus, this);
   Bind(wxEVT_DPI_CHANGED, &jaeTextEditControlBase::OnDPIChanged, this);
   Bind(wxEVT_ERASE_BACKGROUND, &jaeTextEditControlBase::OnEraseBackground, this);
   Bind(wxEVT_LISTBOX_DCLICK, &jaeTextEditControlBase::OnListBox, this);
   Bind(wxEVT_MOUSE_CAPTURE_LOST, &jaeTextEditControlBase::OnMouseCaptureLost, this);

   Bind(wxEVT_MENU, &jaeTextEditControlBase::OnMenu, this, 10, 16);
}

auto jaeTextEditControlBase::getGraphicsCtx() -> wxGraphicsContext * {
   wxGraphicsContext *context  = nullptr;
   auto *             renderer = wxGraphicsRenderer::GetCairoRenderer();

   if (renderer != nullptr) {
      context = renderer->CreateContext(this);

      if (context != nullptr) {
         context->SetAntialiasMode(wxANTIALIAS_DEFAULT);
      }
   }

   return context;
}

void jaeTextEditControlBase::OnPaint(wxPaintEvent & /*evt*/) {
   wxPaintDC dc(this);
   wxGCDC    gcdc{};
   auto *    graphicsCtx = getGraphicsCtx();

   if (graphicsCtx != nullptr) {
      gcdc.SetGraphicsContext(graphicsCtx);
   }
   _scintillaObj->DoPaint(&gcdc, GetUpdateRegion().GetBox());

   if (IsActive() == false) {
      wxBrush brush{wxColour{0x1F, 0x1F, 0x1F, 0xF}};
      gcdc.SetBrush(brush);
      gcdc.DrawRectangle(GetClientRect());
   }
}

void jaeTextEditControlBase::OnScrollWin(wxScrollWinEvent &evt) {
   if (evt.GetOrientation() == wxHORIZONTAL) {
      _scintillaObj->DoHScroll(evt.GetEventType(), evt.GetPosition());
   } else {
      _scintillaObj->DoVScroll(evt.GetEventType(), evt.GetPosition());
   }
}

void jaeTextEditControlBase::OnScroll(wxScrollEvent &evt) {
   auto *sb = dynamic_cast<wxScrollBar *>(evt.GetEventObject());
   if (sb != nullptr) {
      if (sb->IsVertical()) {
         _scintillaObj->DoVScroll(evt.GetEventType(), evt.GetPosition());
      } else {
         _scintillaObj->DoHScroll(evt.GetEventType(), evt.GetPosition());
      }
   }
}

void jaeTextEditControlBase::OnSize(wxSizeEvent & /*evt*/) {
   if (_scintillaObj != nullptr) {
      wxSize sz = GetClientSize();
      _scintillaObj->DoSize(sz.x, sz.y);
   }
}

void jaeTextEditControlBase::OnMouseLeftDown(wxMouseEvent &evt) {
   wxWindow::SetFocus();
   wxPoint pt = evt.GetPosition();
   _scintillaObj->DoLeftButtonDown(Point(pt.x, pt.y), _stopWatch.Time(), evt.ShiftDown(), evt.ControlDown(), evt.AltDown());
}

void jaeTextEditControlBase::OnMouseRightDown(wxMouseEvent &evt) {
   wxWindow::SetFocus();
   wxPoint pt = evt.GetPosition();
   _scintillaObj->DoRightButtonDown(Point(pt.x, pt.y), _stopWatch.Time(), evt.ShiftDown(), evt.ControlDown(), evt.AltDown());
   // We need to call evt.Skip() to allow generating EVT_CONTEXT_MENU
   evt.Skip();
}

void jaeTextEditControlBase::OnMouseMove(wxMouseEvent &evt) {
   wxPoint pt = evt.GetPosition();
   _scintillaObj->DoLeftButtonMove(Point(pt.x, pt.y), _stopWatch.Time(), evt.ShiftDown(), evt.ControlDown(), evt.AltDown());
}

void jaeTextEditControlBase::OnMouseLeftUp(wxMouseEvent &evt) {
   wxPoint pt = evt.GetPosition();
   _scintillaObj->DoLeftButtonUp(Point(pt.x, pt.y), _stopWatch.Time(), evt.ShiftDown(), evt.ControlDown(), evt.AltDown());
}

void jaeTextEditControlBase::OnMouseMiddleUp(wxMouseEvent &evt) {
   wxPoint pt = evt.GetPosition();
   _scintillaObj->DoMiddleButtonUp(Point(pt.x, pt.y), _stopWatch.Time(), evt.ShiftDown(), evt.ControlDown(), evt.AltDown());
}

void jaeTextEditControlBase::OnContextMenu(wxContextMenuEvent &evt) { DoContextMenu(evt); }

void jaeTextEditControlBase::OnMouseWheel(wxMouseEvent &evt) {
   // The default action of this method is to call m_swx->DoMouseWheel.
   // However, it might be necessary to do something else depending on whether
   //     1) the mouse wheel captures for the STC,
   //     2) the event's position is in the STC's rect, and
   //     3) and an autocompletion list is currently being shown.
   // This table summarizes when each action is needed.

   // InRect | MouseWheelCaptures | Autocomp Active |      action
   // -------+--------------------+-----------------+-------------------
   //  true  |       true         |      true       | scroll ac list
   //  true  |       true         |      false      | default
   //  true  |       false        |      true       | scroll ac list
   //  true  |       false        |      false      | default
   //  false |       true         |      true       | scroll ac list
   //  false |       true         |      false      | default
   //  false |       false        |      true       | forward to parent
   //  false |       false        |      false      | forward to parent

   // if the mouse wheel is not captured, test if the mouse
   // pointer is over the editor window and if not, don't
   // handle the message but pass it on.

   auto isAutoCompleteActive = AutoCActive();
   auto isMouseWheelCaptures = MouseWheelCaptures();

   if (!isMouseWheelCaptures && !GetRect().Contains(evt.GetPosition())) {
      wxWindow *parent = GetParent();

      if (parent != nullptr) {
         wxMouseEvent newevt(evt);
         newevt.SetPosition(parent->ScreenToClient(ClientToScreen(evt.GetPosition())));
         parent->ProcessWindowEvent(newevt);
      }
   } else if (isAutoCompleteActive) {
      // When the autocompletion popup is active, Scintilla uses the mouse
      // wheel to scroll the autocomp list instead of the editor.

      // First try to find the list. It will be a wxVListBox named
      // "AutoCompListBox".
      wxWindow *curWin    = this;
      wxWindow *acListBox = nullptr;

      wxStack<wxWindow *> windows;

      windows.push(curWin);

      while (!windows.empty()) {
         curWin = windows.top();
         windows.pop();

         if (curWin->IsKindOf(wxCLASSINFO(wxVListBox)) && curWin->GetName() == "AutoCompListBox") {
            acListBox = curWin;
            break;
         }

         wxWindowList &         children = curWin->GetChildren();
         wxWindowList::iterator it;

         for (it = children.begin(); it != children.end(); ++it) {
            windows.push(*it);
         }
      }

      // Next if the list was found, send it a copy of this event.
      if (acListBox != nullptr) {
         wxMouseEvent newevt(evt);
         newevt.SetPosition(acListBox->ScreenToClient(ClientToScreen(evt.GetPosition())));
         acListBox->ProcessWindowEvent(newevt);
      }
   } else {
      _scintillaObj->DoMouseWheel(evt.GetWheelAxis(), evt.GetWheelRotation(), evt.GetWheelDelta(), evt.GetLinesPerAction(), evt.GetColumnsPerAction(), evt.ControlDown(), evt.IsPageScroll());
   }
}

void jaeTextEditControlBase::OnChar(wxKeyEvent &evt) {
   // On (some?) non-US PC keyboards the AltGr key is required to enter some
   // common characters.  It comes to us as both Alt and Ctrl down so we need
   // to let the char through in that case, otherwise if only ctrl or only
   // alt let's skip it.
   bool ctrl = evt.ControlDown();
   bool alt  = evt.AltDown();
   bool skip = ((ctrl || alt) && !(ctrl && alt));

#if wxUSE_UNICODE
   // apparently if we don't do this, Unicode keys pressed after non-char
   // ASCII ones (e.g. Enter, Tab) are not taken into account (patch 1615989)
   if (_lastKeyDownConsumed && evt.GetUnicodeKey() > 255) {
      _lastKeyDownConsumed = false;
   }
#endif

   if (!_lastKeyDownConsumed && !skip) {
#if wxUSE_UNICODE
      int  key   = evt.GetUnicodeKey();
      bool keyOk = true;

      // if the unicode key code is not really a unicode character (it may
      // be a function key or etc., the platforms appear to always give us a
      // small value in this case) then fallback to the ascii key code but
      // don't do anything for function keys or etc.
      if (key <= 127) {
         key   = evt.GetKeyCode();
         keyOk = (key <= 127);
      }
      if (keyOk) {
         _scintillaObj->DoAddChar(key);
         return;
      }
#else
      int key = evt.GetKeyCode();
      if (key < WXK_START) {
         m_swx->DoAddChar(key);
         return;
      }
#endif
   }

   evt.Skip();
}

void jaeTextEditControlBase::OnKeyDown(wxKeyEvent &evt) {
   int processed = _scintillaObj->DoKeyDown(evt, &_lastKeyDownConsumed);

   if ((processed == 0) && !_lastKeyDownConsumed) {
      evt.Skip();
   }
}

void jaeTextEditControlBase::OnLoseFocus(wxFocusEvent &evt) {
   _scintillaObj->DoLoseFocus();
   evt.Skip();
}

void jaeTextEditControlBase::OnGainFocus(wxFocusEvent &evt) {
   _scintillaObj->DoGainFocus();
   evt.Skip();
}

void jaeTextEditControlBase::OnDPIChanged(wxDPIChangedEvent &evt) {
   _scintillaObj->DoInvalidateStyleData();

   // trigger a cursor change, so any cursors created by wxWidgets (like reverse arrow) will be recreated
   auto oldCursor = Scintilla::ScintillaCall::Cursor();
   Scintilla::ScintillaCall::SetCursor(Scintilla::CursorShape::Normal);
   Scintilla::ScintillaCall::SetCursor(oldCursor);

   // adjust the margins to the new DPI
   for (int i = 0; i < SC_MAX_MARGIN; ++i) {
      auto tempCurMargin = MarginWidthN(i);
      auto margin        = (int)wxMulDivInt32(tempCurMargin, evt.GetNewDPI().y, evt.GetOldDPI().y);

      SetMarginWidthN(i, margin);
   }

   // Hide auto-complete popup, there is no (easy) way to set it to the correct size and position
   if (AutoCActive()) {
      AutoCCancel();
   }
}

void jaeTextEditControlBase::OnSysColourChanged(wxSysColourChangedEvent & /*evt*/) { _scintillaObj->DoInvalidateStyleData(); }

void jaeTextEditControlBase::OnEraseBackground(wxEraseEvent &evt) {}

void jaeTextEditControlBase::OnMenu(wxCommandEvent &evt) { _scintillaObj->DoCommand(evt.GetId()); }

void jaeTextEditControlBase::OnListBox(wxCommandEvent & /*evt*/) { _scintillaObj->DoOnListBox(); }

void jaeTextEditControlBase::OnIdle(wxIdleEvent &evt) { _scintillaObj->DoOnIdle(evt); }

void jaeTextEditControlBase::OnMouseCaptureLost(wxMouseCaptureLostEvent & /*evt*/) { _scintillaObj->DoMouseCaptureLost(); }

auto jaeTextEditControlBase::Send(unsigned int iMessage, uptr_t wParam, sptr_t lParam) const -> sptr_t { return _scintillaObj->WndProc(static_cast<Message>(iMessage), wParam, lParam); }

void jaeTextEditControlBase::DoContextMenu(wxContextMenuEvent &evt) {
   wxPoint pt = evt.GetPosition();
   ScreenToClient(&pt.x, &pt.y);
   /*
     Show context menu at event point if it's within the window,
     or at caret location if not
   */
   wxHitTest ht = this->HitTest(pt);
   if (ht != wxHT_WINDOW_INSIDE) {
      auto pos = CurrentPos();
      auto x   = PointXFromPosition(pos);
      auto y   = PointYFromPosition(pos);
      pt       = wxPoint(static_cast<int>(x), static_cast<int>(y));
   }
   if (!_scintillaObj->DoContextMenu(Point(pt.x, pt.y))) {
      evt.Skip();
   }
}
