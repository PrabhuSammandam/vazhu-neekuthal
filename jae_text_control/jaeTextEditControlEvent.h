#pragma once

#include "wx/dnd.h"
#include "wx/event.h"

class jaeTextEditControlEvent : public wxCommandEvent {
 public:
   explicit jaeTextEditControlEvent(wxEventType commandType = 0, int id = 0);
   jaeTextEditControlEvent(const jaeTextEditControlEvent &event);
   ~jaeTextEditControlEvent() override = default;

   auto Clone() const -> wxEvent * override { return new jaeTextEditControlEvent(*this); }

   void SetPosition(int pos) { m_position = pos; }
   auto GetPosition() const -> int { return m_position; }

   void SetKey(int k) { m_key = k; }
   auto GetKey() const -> int { return m_key; }

   void SetModifiers(int m) { m_modifiers = m; }
   auto GetModifiers() const -> int { return m_modifiers; }

   void SetModificationType(int t) { m_modificationType = t; }
   auto GetModificationType() const -> int { return m_modificationType; }

   void SetLength(int len) { m_length = len; }
   auto GetLength() const -> int { return m_length; }

   void SetLinesAdded(int num) { m_linesAdded = num; }
   auto GetLinesAdded() const -> int { return m_linesAdded; }

   void SetLine(int val) { m_line = val; }
   auto GetLine() const -> int { return m_line; }

   void SetFoldLevelNow(int val) { m_foldLevelNow = val; }
   auto GetFoldLevelNow() const -> int { return m_foldLevelNow; }

   void SetFoldLevelPrev(int val) { m_foldLevelPrev = val; }
   auto GetFoldLevelPrev() const -> int { return m_foldLevelPrev; }

   void SetMargin(int val) { m_margin = val; }
   auto GetMargin() const -> int { return m_margin; }

   void SetMessage(int val) { m_message = val; }
   auto GetMessage() const -> int { return m_message; }

   void SetWParam(int val) { m_wParam = val; }
   auto GetWParam() const -> int { return m_wParam; }

   void SetLParam(int val) { m_lParam = val; }
   auto GetLParam() const -> int { return m_lParam; }

   void SetListType(int val) { m_listType = val; }
   auto GetListType() const -> int { return m_listType; }

   void SetX(int val) { m_x = val; }
   auto GetX() const -> int { return m_x; }

   void SetY(int val) { m_y = val; }
   auto GetY() const -> int { return m_y; }

   void SetToken(int val) { m_token = val; }
   auto GetToken() const -> int { return m_token; }

   void SetAnnotationLinesAdded(int val) { m_annotationLinesAdded = val; }
   auto GetAnnotationsLinesAdded() const -> int { return m_annotationLinesAdded; }

   void SetUpdated(int val) { m_updated = val; }
   auto GetUpdated() const -> int { return m_updated; }

   void SetListCompletionMethod(int val) { m_listCompletionMethod = val; }
   auto GetListCompletionMethod() const -> int { return m_listCompletionMethod; }

   void SetDragText(const wxString &val) { SetString(val); }
   auto GetDragText() -> wxString { return GetString(); }

   void SetDragFlags(int flags) { m_dragFlags = flags; }
   auto GetDragFlags() const -> int { return m_dragFlags; }

   void SetDragResult(wxDragResult val) { m_dragResult = val; }
   auto GetDragResult() const -> wxDragResult { return m_dragResult; }

   auto GetDragAllowMove() const -> bool { return (GetDragFlags() & wxDrag_AllowMove) != 0; }
   void SetDragAllowMove(bool allow);

   auto GetShift() const -> bool;
   auto GetControl() const -> bool;
   auto GetAlt() const -> bool;

   wxDECLARE_DYNAMIC_CLASS(jaeTextEditControlEvent);

   int          m_position;
   int          m_key;
   int          m_modifiers;
   int          m_modificationType; // wxEVT_STC_MODIFIED
   int          m_length;
   int          m_linesAdded;
   int          m_line;
   int          m_foldLevelNow;
   int          m_foldLevelPrev;
   int          m_margin;  // wxEVT_STC_MARGINCLICK
   int          m_message; // wxEVT_STC_MACRORECORD
   int          m_wParam;
   int          m_lParam;
   int          m_listType;
   int          m_x;
   int          m_y;
   int          m_token;                // wxEVT_STC__MODIFIED with SC_MOD_CONTAINER
   int          m_annotationLinesAdded; // wxEVT_STC_MODIFIED with SC_MOD_CHANGEANNOTATION
   int          m_updated;              // wxEVT_STC_UPDATEUI
   int          m_listCompletionMethod;
   int          m_dragFlags;  // wxEVT_STC_START_DRAG
   wxDragResult m_dragResult; // wxEVT_STC_DRAG_OVER,wxEVT_STC_DO_DROP
};

wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_CHANGE, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_STYLENEEDED, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_CHARADDED, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_SAVEPOINTREACHED, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_SAVEPOINTLEFT, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_ROMODIFYATTEMPT, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_DOUBLECLICK, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_UPDATEUI, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_MODIFIED, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_MACRORECORD, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_MARGINCLICK, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_NEEDSHOWN, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_PAINTED, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_USERLISTSELECTION, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_DWELLSTART, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_DWELLEND, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_START_DRAG, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_DRAG_OVER, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_DO_DROP, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_ZOOM, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_HOTSPOT_CLICK, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_HOTSPOT_DCLICK, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_CALLTIP_CLICK, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_AUTOCOMP_SELECTION, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_INDICATOR_CLICK, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_INDICATOR_RELEASE, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_AUTOCOMP_CANCELLED, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_AUTOCOMP_CHAR_DELETED, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_HOTSPOT_RELEASE_CLICK, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_CLIPBOARD_COPY, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_CLIPBOARD_PASTE, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_AUTOCOMP_COMPLETED, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_MARGIN_RIGHT_CLICK, jaeTextEditControlEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_STC, wxEVT_JAE_TEXT_EDIT_CONTROL_AUTOCOMP_SELECTION_CHANGE, jaeTextEditControlEvent);
