#include "jaeScintilla.h"
#include "jaePortListBox.h"
#include "Position.h"
#include "Scintilla.h"
#include "ScintillaTypes.h"
#include "jaeTextEditControlBase.h"
#include "jaeTextEditControlEvent.h"
#include "wx/string.h"
#include <wx/clipbrd.h>
#include <wx/scopedarray.h>
#include <wx/menu.h>
#include <wx/textfile.h>

using namespace Scintilla;
using namespace Scintilla::Internal;

const int H_SCROLL_STEP = 20;

class jaeTimer : public wxTimer {
 public:
   jaeTimer(jaeScintilla *swx, jaeScintilla::TickReason reason) {
      m_swx    = swx;
      m_reason = reason;
   }

   void Notify() override { m_swx->TickFor(m_reason); }

 private:
   jaeScintilla *           m_swx;
   jaeScintilla::TickReason m_reason;
};

#if wxUSE_DRAG_AND_DROP
class jaeDropTarget : public wxTextDropTarget {
 public:
   jaeDropTarget()           = default;
   ~jaeDropTarget() override = default;

   void SetScintilla(jaeScintilla *swx) { m_swx = swx; }

   auto OnDropText(wxCoord x, wxCoord y, const wxString &data) -> bool wxOVERRIDE { return m_swx->DoDropText(x, y, data); }
   auto OnEnter(wxCoord x, wxCoord y, wxDragResult def) -> wxDragResult wxOVERRIDE { return m_swx->DoDragEnter(x, y, def); }
   auto OnDragOver(wxCoord x, wxCoord y, wxDragResult def) -> wxDragResult wxOVERRIDE { return m_swx->DoDragOver(x, y, def); }
   void OnLeave() wxOVERRIDE { m_swx->DoDragLeave(); }

 private:
   jaeScintilla *m_swx = nullptr;
};
#endif

static auto wxConvertEOLMode(int scintillaMode) -> wxTextFileType {
   wxTextFileType type{};

   switch (scintillaMode) {
   case SC_EOL_CRLF:
      type = wxTextFileType_Dos;
      break;
   case SC_EOL_CR:
      type = wxTextFileType_Mac;
      break;
   case SC_EOL_LF:
      type = wxTextFileType_Unix;
      break;
   default:
      type = wxTextBuffer::typeDefault;
      break;
   }
   return type;
}

jaeScintilla::jaeScintilla(jaeTextEditControlBase *editControl) : Scintilla::Internal::ScintillaBase{}, _editControl{editControl} {
   Initialise();

   m_clipRectTextFormat = wxDataFormat(wxT("SECONDARY"));

   for (auto i = 0; i < 10; i++) {
      _timers[i] = nullptr;
   }

   wMain = _editControl;
}

jaeScintilla::~jaeScintilla() {
   for (auto i = 0; i < 10; i++) {
      delete _timers[i];
   }
}

void jaeScintilla::Initialise() {
   dropTarget = new jaeDropTarget;
   dropTarget->SetScintilla(this);
   _editControl->SetDropTarget(dropTarget);
   vs.extraFontFlag = Scintilla::FontQuality::QualityAntialiased; // UseAntiAliasing

   auto *autoCompleteLB = dynamic_cast<ListBoxImpl *>(ac.lb.get());
   autoCompleteLB->SetListInfo(&listType, (int *)&(ac.posStart), (int *)&(ac.startLen));
}

void jaeScintilla::Finalise() {
   ScintillaBase::Finalise();
   SetIdle(false);
}

void jaeScintilla::StartDrag() {
   wxString dragText = wxString::FromUTF8(drag.Data(), drag.Length());

   // Send an event to allow the drag text to be changed
   jaeTextEditControlEvent evt(wxEVT_JAE_TEXT_EDIT_CONTROL_START_DRAG, _editControl->GetId());

   evt.SetEventObject(_editControl);
   evt.SetString(dragText);
   evt.SetDragFlags(wxDrag_DefaultMove);
   evt.SetPosition(wxMin(_editControl->SelectionStart(), _editControl->SelectionEnd()));
   _editControl->GetEventHandler()->ProcessEvent(evt);
   dragText = evt.GetString();

   if (!dragText.empty()) {
      wxDropSource     source(_editControl);
      wxTextDataObject data(dragText);
      wxDragResult     result{};

      source.SetData(data);
      dropWentOutside = true;
      inDragDrop      = Scintilla::Internal::Editor::DragDrop::dragging;
      result          = source.DoDragDrop(evt.GetDragFlags());

      if (result == wxDragMove && dropWentOutside) {
         ClearSelection();
      }

      inDragDrop = Scintilla::Internal::Editor::DragDrop::none;
      SetDragPosition(SelectionPosition(Sci::invalidPosition));
   }
}

void jaeScintilla::SetMouseCapture(bool on) {
   if (mouseDownCaptures) {
      if (on && !_capturedMouse) {
         _editControl->CaptureMouse();
      } else if (!on && _capturedMouse && _editControl->HasCapture()) {
         _editControl->ReleaseMouse();
      }
      _capturedMouse = on;
   }
}
auto jaeScintilla::HaveMouseCapture() -> bool { return _capturedMouse; }

void jaeScintilla::ScrollText(Sci::Line linesToMove) {
   auto dy = static_cast<int>(vs.lineHeight * (linesToMove));
   _editControl->ScrollWindow(0, dy);
}

void jaeScintilla::SetVerticalScrollPos() {
   if (_editControl->_vScrollBar == nullptr) { // Use built-in scrollbar
      _editControl->SetScrollPos(wxVERTICAL, topLine);
   } else { // otherwise use the one that's been given to us
      _editControl->_vScrollBar->SetThumbPosition(topLine);
   }
}

void jaeScintilla::SetHorizontalScrollPos() {
   if (_editControl->_hScrollBar == nullptr) { // Use built-in scrollbar
      _editControl->SetScrollPos(wxHORIZONTAL, xOffset);
   } else { // otherwise use the one that's been given to us
      _editControl->_hScrollBar->SetThumbPosition(xOffset);
   }
}

auto jaeScintilla::ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) -> bool {
   bool modified = false;

   int vertEnd = nMax + 1;
   if (!verticalScrollBarVisible) {
      nPage = vertEnd + 1;
   }

   // Check the vertical scrollbar
   if (_editControl->_vScrollBar == nullptr) { // Use built-in scrollbar
      int sbMax   = _editControl->GetScrollRange(wxVERTICAL);
      int sbThumb = _editControl->GetScrollThumb(wxVERTICAL);
      int sbPos   = _editControl->GetScrollPos(wxVERTICAL);
      if (sbMax != vertEnd || sbThumb != nPage) {
         _editControl->SetScrollbar(wxVERTICAL, sbPos, nPage, vertEnd);
         modified = true;
      }
   } else { // otherwise use the one that's been given to us
      int sbMax  = _editControl->_vScrollBar->GetRange();
      int sbPage = _editControl->_vScrollBar->GetPageSize();
      int sbPos  = _editControl->_vScrollBar->GetThumbPosition();
      if (sbMax != vertEnd || sbPage != nPage) {
         _editControl->_vScrollBar->SetScrollbar(sbPos, nPage, vertEnd, nPage);
         modified = true;
      }
   }

   // Check the horizontal scrollbar
   PRectangle rcText   = GetTextRectangle();
   int        horizEnd = scrollWidth;

   if (horizEnd < 0) {
      horizEnd = 0;
   }

   int pageWidth = static_cast<int>(rcText.Width());

   if (!horizontalScrollBarVisible || Wrapping()) {
      pageWidth = horizEnd + 1;
   }

   if (_editControl->_hScrollBar == nullptr) { // Use built-in scrollbar
      int sbMax   = _editControl->GetScrollRange(wxHORIZONTAL);
      int sbThumb = _editControl->GetScrollThumb(wxHORIZONTAL);
      int sbPos   = _editControl->GetScrollPos(wxHORIZONTAL);
      if ((sbMax != horizEnd) || (sbThumb != pageWidth)) {
         _editControl->SetScrollbar(wxHORIZONTAL, sbPos, pageWidth, horizEnd);
         modified = true;
         if (scrollWidth < pageWidth) {
            HorizontalScrollTo(0);
         }
      }
   } else { // otherwise use the one that's been given to us
      int sbMax   = _editControl->_hScrollBar->GetRange();
      int sbThumb = _editControl->_hScrollBar->GetPageSize();
      int sbPos   = _editControl->_hScrollBar->GetThumbPosition();
      if ((sbMax != horizEnd) || (sbThumb != pageWidth)) {
         _editControl->_hScrollBar->SetScrollbar(sbPos, pageWidth, horizEnd, pageWidth);
         modified = true;
         if (scrollWidth < pageWidth) {
            HorizontalScrollTo(0);
         }
      }
   }

   return modified;
}

void jaeScintilla::Copy() {
   if (!sel.Empty()) {
      SelectionText st;
      CopySelectionRange(&st);
      CopyToClipboard(st);
   }
}

void jaeScintilla::Paste() {}

auto jaeScintilla::CanPaste() -> bool {
#if wxUSE_CLIPBOARD
   bool canPaste = false;
   bool didOpen  = false;

   if (Editor::CanPaste()) {
      wxTheClipboard->UsePrimarySelection(false);
      didOpen = !wxTheClipboard->IsOpened();
      if (didOpen) {
         wxTheClipboard->Open();
      }

      if (wxTheClipboard->IsOpened()) {
         canPaste = wxTheClipboard->IsSupported(wxUSE_UNICODE ? wxDF_UNICODETEXT : wxDF_TEXT);
         if (didOpen) {
            wxTheClipboard->Close();
         }
      }
   }
   return canPaste;
#else
   return false;
#endif // wxUSE_CLIPBOARD
}

void jaeScintilla::CopyToClipboard(const Scintilla::Internal::SelectionText &st) {
#if wxUSE_CLIPBOARD
   if (st.LengthWithTerminator() == 0U) {
      return;
   }

   // Send an event to allow the copied text to be changed
   jaeTextEditControlEvent evt(wxEVT_JAE_TEXT_EDIT_CONTROL_CLIPBOARD_COPY, _editControl->GetId());
   evt.SetEventObject(_editControl);
   evt.SetString(wxTextBuffer::Translate(wxString::FromUTF8(st.Data(), st.Length())));
   _editControl->GetEventHandler()->ProcessEvent(evt);

   wxClipboard::Get()->UsePrimarySelection(false);
   if (wxClipboard::Get()->Open()) {
      wxString text = evt.GetString();

#ifdef wxHAVE_STC_RECT_FORMAT
      if (st.rectangular) {
         // when copying the text to the clipboard, add extra meta-data that
         // tells the Paste() method that the user copied a rectangular
         // block of text, as opposed to a stream of text.
         auto *composite = new wxDataObjectComposite();
         composite->Add(new wxTextDataObject(text), true);
         composite->Add(new wxCustomDataObject(m_clipRectTextFormat));
         wxTheClipboard->SetData(composite);
      } else
#endif // wxHAVE_STC_RECT_FORMAT
      {
         wxClipboard::Get()->SetData(new wxTextDataObject(text));
      }
      wxTheClipboard->Close();
   }
#else
   wxUnusedVar(st);
#endif // wxUSE_CLIPBOARD
}

void jaeScintilla::ClaimSelection() {
#if wxUSE_CLIPBOARD
   // Put the selected text in the PRIMARY selection
   if (!sel.Empty()) {
      SelectionText st;
      CopySelectionRange(&st);
      wxTheClipboard->UsePrimarySelection(true);
      if (wxTheClipboard->Open()) {
         wxString text = wxString::FromUTF8(st.Data(), st.Length());
         wxTheClipboard->SetData(new wxTextDataObject(text));
         wxTheClipboard->Close();
      }
      wxTheClipboard->UsePrimarySelection(false);
   }
#endif // wxUSE_CLIPBOARD
}

void jaeScintilla::NotifyChange() { _editControl->HandleNotifyChange(); }
void jaeScintilla::NotifyParent(Scintilla::NotificationData scn) { _editControl->HandleNotifyParent(scn); }

auto jaeScintilla::DefWndProc(Scintilla::Message /*iMessage*/, uptr_t /*wParam*/, sptr_t /*lParam*/) -> sptr_t { return 0; }
auto jaeScintilla::WndProc(Scintilla::Message iMessage, uptr_t wParam, sptr_t lParam) -> sptr_t {
   try {
      switch (iMessage) {
      case Message::GetDirectFunction:
         return reinterpret_cast<sptr_t>(DirectFunction);

      case Message::GetDirectStatusFunction:
         return reinterpret_cast<sptr_t>(DirectStatusFunction);

      case Message::GetDirectPointer:
         return reinterpret_cast<sptr_t>(this);

      default:
         return ScintillaBase::WndProc(iMessage, wParam, lParam);
      }
   } catch (std::bad_alloc &) {
      errorStatus = Status::BadAlloc;
   } catch (...) {
      errorStatus = Status::Failure;
   }
   return 0;
}

auto jaeScintilla::SetIdle(bool on) -> bool {
   if (idler.state != on) {
      // connect or disconnect the EVT_IDLE handler
      if (on) {
         _editControl->Bind(wxEVT_IDLE, &jaeTextEditControlBase::OnIdle, _editControl);
      } else {
         _editControl->Unbind(wxEVT_IDLE, &jaeTextEditControlBase::OnIdle, _editControl);
      }
      idler.state = on;
   }
   return idler.state;
}

auto jaeScintilla::FineTickerRunning(TickReason reason) -> bool {
   bool  running = false;
   auto  index   = static_cast<size_t>(reason);
   auto *t       = _timers.at(index);

   if (t != nullptr) {
      running = t->IsRunning();
   }
   return running;
}

void jaeScintilla::FineTickerStart(TickReason reason, int millis, int /*tolerance*/) {
   FineTickerCancel(reason);

   auto  index = static_cast<size_t>(reason);
   auto *t     = _timers.at(index);

   if (t == nullptr) {
      t              = new jaeTimer{this, reason};
      _timers[index] = t;
   }
   t->Start(millis);
}

void jaeScintilla::FineTickerCancel(TickReason reason) {
   auto  index = static_cast<size_t>(reason);
   auto *t     = _timers.at(index);

   if (t != nullptr) {
      t->Stop();
   }
}

void jaeScintilla::AddToPopUp(const char *label, int cmd, bool enabled) {
   auto *menu = static_cast<wxMenu *>(popup.GetID());

   if (label[0] == 0) {
      menu->AppendSeparator();
   } else {
      menu->Append(cmd, wxGetTranslation(wxString::FromUTF8(label)));
   }

   if (!enabled) {
      menu->Enable(cmd, enabled);
   }
}

auto jaeScintilla::DirectFunction(sptr_t ptr, unsigned int iMessage, uptr_t wParam, sptr_t lParam) -> sptr_t {
   auto *sci = reinterpret_cast<jaeScintilla *>(ptr);
   return sci->WndProc(static_cast<Message>(iMessage), wParam, lParam);
}

auto jaeScintilla::DirectStatusFunction(sptr_t ptr, unsigned int iMessage, uptr_t wParam, sptr_t lParam, int *pStatus) -> sptr_t {
   auto *       sci         = reinterpret_cast<jaeScintilla *>(ptr);
   const sptr_t returnValue = sci->WndProc(static_cast<Message>(iMessage), wParam, lParam);
   *pStatus                 = static_cast<int>(sci->errorStatus);
   return returnValue;
}

static auto PRectangleFromwxRect(wxRect rc) -> PRectangle { return PRectangle(rc.GetLeft(), rc.GetTop(), rc.GetRight() + 1, rc.GetBottom() + 1); }

void jaeScintilla::FullPaint() {
   _editControl->Refresh(false);
   _editControl->Update();
}

void jaeScintilla::FullPaintDC(wxDC *dc) {
   paintState      = PaintState::painting;
   rcPaint         = GetClientRectangle();
   paintingAllText = true;
   AutoSurface surfaceWindow(dc, this);
   if (surfaceWindow != nullptr) {
      Paint(surfaceWindow, rcPaint);
      surfaceWindow->Release();
   }
   paintState = PaintState::notPainting;
}

void jaeScintilla::SetUseAntiAliasing(bool useAA) {
   vs.extraFontFlag = useAA ? Scintilla::FontQuality::QualityAntialiased : Scintilla::FontQuality::QualityNonAntialiased;
   InvalidateStyleRedraw();
}

auto jaeScintilla::GetUseAntiAliasing() -> bool { return vs.extraFontFlag == Scintilla::FontQuality::QualityAntialiased; }

/* Event Handling */
void jaeScintilla::DoPaint(wxDC *dc, wxRect rect) {
   paintState = PaintState::painting;
   AutoSurface surfaceWindow(dc, this);
   if (surfaceWindow != nullptr) {
      rcPaint             = PRectangleFromwxRect(rect);
      PRectangle rcClient = GetClientRectangle();
      paintingAllText     = rcPaint.Contains(rcClient);

      Paint(surfaceWindow, rcPaint);
      surfaceWindow->Release();
   }

   if (paintState == PaintState::abandoned) {
      // Painting area was insufficient to cover new styling or brace
      // highlight positions.  So trigger a new paint event that will
      // repaint the whole window.
      _editControl->Refresh(false);

      //#if wxALWAYS_NATIVE_DOUBLE_BUFFER
      // On systems using double buffering, we also need to finish the
      // current paint to make sure that everything is on the screen that
      // needs to be there between now and when the next paint event arrives.
      FullPaintDC(dc);
      //#endif
   }
   paintState = PaintState::notPainting;
}

void jaeScintilla::DoHScroll(int type, int pos) {
   int        xPos      = xOffset;
   PRectangle rcText    = GetTextRectangle();
   int        pageWidth = wxRound(rcText.Width() * 2 / 3);
   if (type == wxEVT_SCROLLWIN_LINEUP || type == wxEVT_SCROLL_LINEUP) {
      xPos -= H_SCROLL_STEP;
   } else if (type == wxEVT_SCROLLWIN_LINEDOWN || type == wxEVT_SCROLL_LINEDOWN) {
      xPos += H_SCROLL_STEP;
   } else if (type == wxEVT_SCROLLWIN_PAGEUP || type == wxEVT_SCROLL_PAGEUP) {
      xPos -= pageWidth;
   } else if (type == wxEVT_SCROLLWIN_PAGEDOWN || type == wxEVT_SCROLL_PAGEDOWN) {
      xPos += pageWidth;
      if (xPos > scrollWidth - rcText.Width()) {
         xPos = wxRound(scrollWidth - rcText.Width());
      }
   } else if (type == wxEVT_SCROLLWIN_TOP || type == wxEVT_SCROLL_TOP) {
      xPos = 0;
   } else if (type == wxEVT_SCROLLWIN_BOTTOM || type == wxEVT_SCROLL_BOTTOM) {
      xPos = scrollWidth;
   } else if (type == wxEVT_SCROLLWIN_THUMBTRACK || type == wxEVT_SCROLL_THUMBTRACK) {
      xPos = pos;
   }

   HorizontalScrollTo(xPos);
}

void jaeScintilla::DoVScroll(int type, int pos) {
   int topLineNew = topLine;

   if (type == wxEVT_SCROLLWIN_LINEUP || type == wxEVT_SCROLL_LINEUP) {
      topLineNew -= 1;
   } else if (type == wxEVT_SCROLLWIN_LINEDOWN || type == wxEVT_SCROLL_LINEDOWN) {
      topLineNew += 1;
   } else if (type == wxEVT_SCROLLWIN_PAGEUP || type == wxEVT_SCROLL_PAGEUP) {
      topLineNew -= LinesToScroll();
   } else if (type == wxEVT_SCROLLWIN_PAGEDOWN || type == wxEVT_SCROLL_PAGEDOWN) {
      topLineNew += LinesToScroll();
   } else if (type == wxEVT_SCROLLWIN_TOP || type == wxEVT_SCROLL_TOP) {
      topLineNew = 0;
   } else if (type == wxEVT_SCROLLWIN_BOTTOM || type == wxEVT_SCROLL_BOTTOM) {
      topLineNew = MaxScrollPos();
   } else if (type == wxEVT_SCROLLWIN_THUMBTRACK || type == wxEVT_SCROLL_THUMBTRACK) {
      topLineNew = pos;
   }

   ScrollTo(topLineNew);
}

void jaeScintilla::DoSize(int /*width*/, int /*height*/) { ChangeSize(); }

void jaeScintilla::DoLoseFocus() {
   _focusEvent = true;
   SetFocusState(false);
   _focusEvent = false;
}

void jaeScintilla::DoGainFocus() {
   _focusEvent = true;
   SetFocusState(true);
   _focusEvent = false;
}

void jaeScintilla::DoInvalidateStyleData() { InvalidateStyleData(); }

void jaeScintilla::DoLeftButtonDown(Scintilla::Internal::Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt) { ButtonDownWithModifiers(pt, curTime, ModifierFlags(shift, ctrl, alt)); }

void jaeScintilla::DoRightButtonDown(Scintilla::Internal::Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt) {
   if (!PointInSelection(pt)) {
      CancelModes();
      SetEmptySelection(PositionFromLocation(pt));
   }

   RightButtonDownWithModifiers(pt, curTime, ModifierFlags(shift, ctrl, alt));
}

void jaeScintilla::DoLeftButtonUp(Scintilla::Internal::Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt) { ButtonUpWithModifiers(pt, curTime, ModifierFlags(shift, ctrl, alt)); }

void jaeScintilla::DoLeftButtonMove(Scintilla::Internal::Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt) { ButtonMoveWithModifiers(pt, curTime, ModifierFlags(shift, ctrl, alt)); }

void jaeScintilla::DoMiddleButtonUp(Scintilla::Internal::Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt) {}

void jaeScintilla::DoMouseWheel(wxMouseWheelAxis axis, int rotation, int delta, int linesPerAction, int columnsPerAction, bool ctrlDown, bool isPageScroll) {
   auto topLineNew = topLine;
   int  lines      = 0;
   int  xPos       = xOffset;
   int  pixels     = 0;

   if (axis == wxMOUSE_WHEEL_HORIZONTAL) {

      wheelHRotation += wxRound(rotation * (columnsPerAction * vs.spaceWidth));
      pixels = wheelHRotation / delta;
      wheelHRotation -= pixels * delta;
      if (pixels != 0) {
         xPos += pixels;
         PRectangle rcText = GetTextRectangle();
         if (xPos > scrollWidth - rcText.Width()) {
            xPos = wxRound(scrollWidth - rcText.Width());
         }
         HorizontalScrollTo(xPos);
      }
   } else if (ctrlDown) { // Zoom the fonts if Ctrl key down
      if (rotation > 0) {
         KeyCommand(Scintilla::Message::ZoomIn);
      } else {
         KeyCommand(Scintilla::Message::ZoomOut);
      }
   } else { // otherwise just scroll the window
      if (delta == 0) {
         delta = 120;
      }
      wheelVRotation += rotation;
      lines = wheelVRotation / delta;
      wheelVRotation -= lines * delta;
      if (lines != 0) {
         if (isPageScroll) {
            lines = lines * LinesOnScreen(); // lines is either +1 or -1
         } else {
            lines *= linesPerAction;
         }
         topLineNew -= lines;
         ScrollTo(topLineNew);
      }
   }
}

void jaeScintilla::DoAddChar(int key) {
   // #if wxUSE_UNICODE
   //    wxChar wszChars[2];
   //    wszChars[0] = (wxChar)key;
   //    wszChars[1] = 0;
   //    const wxCharBuffer buf(wx2stc(wszChars));
   //    AddCharUTF(buf, buf.length());
   // #else
   AddChar((char)key);
   //#endif
}

auto jaeScintilla::DoKeyDown(const wxKeyEvent &evt, bool *consumed) -> int {
   int key = evt.GetKeyCode();

   if (evt.RawControlDown() && key >= 1 && key <= 26 && key != WXK_BACK) {
      key += 'A' - 1;
   }

   switch (key) {
   case WXK_DOWN:
      key = SCK_DOWN;
      break;
   case WXK_UP:
      key = SCK_UP;
      break;
   case WXK_LEFT:
      key = SCK_LEFT;
      break;
   case WXK_RIGHT:
      key = SCK_RIGHT;
      break;
   case WXK_HOME:
      key = SCK_HOME;
      break;
   case WXK_END:
      key = SCK_END;
      break;
   case WXK_PAGEUP:
      key = SCK_PRIOR;
      break;
   case WXK_PAGEDOWN:
      key = SCK_NEXT;
      break;
   case WXK_NUMPAD_DOWN:
      key = SCK_DOWN;
      break;
   case WXK_NUMPAD_UP:
      key = SCK_UP;
      break;
   case WXK_NUMPAD_LEFT:
      key = SCK_LEFT;
      break;
   case WXK_NUMPAD_RIGHT:
      key = SCK_RIGHT;
      break;
   case WXK_NUMPAD_HOME:
      key = SCK_HOME;
      break;
   case WXK_NUMPAD_END:
      key = SCK_END;
      break;
   case WXK_NUMPAD_PAGEUP:
      key = SCK_PRIOR;
      break;
   case WXK_NUMPAD_PAGEDOWN:
      key = SCK_NEXT;
      break;
   case WXK_NUMPAD_DELETE:
      key = SCK_DELETE;
      break;
   case WXK_NUMPAD_INSERT:
      key = SCK_INSERT;
      break;
   case WXK_DELETE:
      key = SCK_DELETE;
      break;
   case WXK_INSERT:
      key = SCK_INSERT;
      break;
   case WXK_ESCAPE:
      key = SCK_ESCAPE;
      break;
   case WXK_BACK:
      key = SCK_BACK;
      break;
   case WXK_TAB:
      key = SCK_TAB;
      break;
   case WXK_NUMPAD_ENTER:
      wxFALLTHROUGH;
   case WXK_RETURN:
      key = SCK_RETURN;
      break;
   case WXK_ADD:
      wxFALLTHROUGH;
   case WXK_NUMPAD_ADD:
      key = SCK_ADD;
      break;
   case WXK_SUBTRACT:
      wxFALLTHROUGH;
   case WXK_NUMPAD_SUBTRACT:
      key = SCK_SUBTRACT;
      break;
   case WXK_DIVIDE:
      wxFALLTHROUGH;
   case WXK_NUMPAD_DIVIDE:
      key = SCK_DIVIDE;
      break;
   case WXK_CONTROL:
   case WXK_ALT:
   case WXK_SHIFT:
      key = 0;
      break;
   case WXK_MENU:
      key = SCK_MENU;
      break;
   case WXK_NONE: {
      // This is a Unicode character not representable in Latin-1 or some key
      // without key code at all (e.g. dead key or VK_PROCESSKEY under MSW).
      if (consumed != nullptr) {
         *consumed = false;
      }
      return 0;
   }
   }

   int rv = KeyDownWithModifiers((Scintilla::Keys)key, ModifierFlags(evt.ShiftDown(), evt.ControlDown(), evt.AltDown()), consumed);

   if (key != 0) {
      return rv;
   }
   return 1;
}

void jaeScintilla::DoOnIdle(wxIdleEvent &evt) {
   if (Idle()) {
      evt.RequestMore();
   } else {
      SetIdle(false);
   }
}

void jaeScintilla::DoCommand(int ID) { Command(ID); }

auto jaeScintilla::DoContextMenu(Scintilla::Internal::Point pt) -> bool {
   if (ShouldDisplayPopup(pt)) {
      // To prevent generating EVT_MOUSE_CAPTURE_LOST.
      if (HaveMouseCapture()) {
         SetMouseCapture(false);
      }
      ContextMenu(pt);
      return true;
   }
   return false;
}

void jaeScintilla::DoOnListBox() { AutoCompleteCompleted(0, Scintilla::CompletionMethods::Command); }

void jaeScintilla::DoMouseCaptureLost() { _capturedMouse = false; }

auto jaeScintilla::DoDropText(long x, long y, const wxString &data) -> bool {
   SetDragPosition(SelectionPosition(Sci::invalidPosition));

   wxString text = wxTextBuffer::Translate(data, wxConvertEOLMode((int)pdoc->eolMode));

   // Send an event to allow the drag details to be changed
   jaeTextEditControlEvent evt(wxEVT_JAE_TEXT_EDIT_CONTROL_DO_DROP, _editControl->GetId());
   evt.SetEventObject(_editControl);
   evt.SetDragResult(dragResult);
   evt.SetX(x);
   evt.SetY(y);
   evt.SetPosition(PositionFromLocation(Point(x, y)));
   evt.SetString(text);
   _editControl->GetEventHandler()->ProcessEvent(evt);

   dragResult = evt.GetDragResult();
   if (dragResult == wxDragMove || dragResult == wxDragCopy) {
      DropAt(SelectionPosition(evt.GetPosition()), wxString::FromUTF8(evt.GetString()), dragResult == wxDragMove, false); // TODO: rectangular?
      return true;
   }
   return false;
}

auto jaeScintilla::DoDragEnter(wxCoord x, wxCoord y, wxDragResult def) -> wxDragResult {
   dragResult = def;
   return dragResult;
}

auto jaeScintilla::DoDragOver(wxCoord x, wxCoord y, wxDragResult def) -> wxDragResult {
   SetDragPosition(SelectionPosition(PositionFromLocation(Point(x, y))));

   // Send an event to allow the drag result to be changed
   jaeTextEditControlEvent evt(wxEVT_JAE_TEXT_EDIT_CONTROL_DRAG_OVER, _editControl->GetId());
   evt.SetEventObject(_editControl);
   evt.SetDragResult(def);
   evt.SetX(x);
   evt.SetY(y);
   evt.SetPosition(PositionFromLocation(Point(x, y)));
   _editControl->GetEventHandler()->ProcessEvent(evt);

   dragResult = evt.GetDragResult();
   return dragResult;
}

void jaeScintilla::DoDragLeave() { SetDragPosition(SelectionPosition(Sci::invalidPosition)); }

void jaeScintilla::DoScrollToLine(int line) { ScrollTo(line); }

void jaeScintilla::DoScrollToColumn(int column) { HorizontalScrollTo(wxRound(column * vs.spaceWidth)); }

void jaeScintilla::DoMarkerDefineBitmap(int markerNumber, const wxBitmap &bmp) {
   if (0 <= markerNumber && markerNumber <= MARKER_MAX) {
      // Build an RGBA buffer from bmp.
      const int                    totalPixels = bmp.GetWidth() * bmp.GetHeight();
      wxScopedArray<unsigned char> rgba(4 * bmp.GetWidth() * bmp.GetHeight());
      wxImage                      img         = bmp.ConvertToImage();
      int                          curRGBALoc  = 0;
      int                          curDataLoc  = 0;
      int                          curAlphaLoc = 0;

      if (img.HasMask()) {
         for (int y = 0; y < bmp.GetHeight(); ++y) {
            for (int x = 0; x < bmp.GetWidth(); ++x) {
               rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
               rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
               rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
               rgba[curRGBALoc++] = img.IsTransparent(x, y) ? wxALPHA_TRANSPARENT : wxALPHA_OPAQUE;
            }
         }
      } else if (img.HasAlpha()) {
         for (int i = 0; i < totalPixels; ++i) {
            rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
            rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
            rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
            rgba[curRGBALoc++] = img.GetAlpha()[curAlphaLoc++];
         }
      } else {
         for (int i = 0; i < totalPixels; ++i) {
            rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
            rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
            rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
            rgba[curRGBALoc++] = wxALPHA_OPAQUE;
         }
      }

      // Now follow the same procedure used for handling the
      // SCI_MARKERDEFINERGBAIMAGE message, except use the bitmap's width and
      // height instead of the values stored in sizeRGBAImage.
      Point bitmapSize = Point::FromInts(bmp.GetWidth(), bmp.GetHeight());
      vs.markers[markerNumber].SetRGBAImage(bitmapSize, 1.0F, rgba.get());
      vs.CalcLargestMarkerHeight();
   }
   InvalidateStyleData();
   RedrawSelMargin();
}

void jaeScintilla::DoRegisterImage(int type, const wxBitmap &bmp) {}
