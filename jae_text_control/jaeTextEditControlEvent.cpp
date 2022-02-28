#include "jaeTextEditControlEvent.h"

// C++ wrappers of C standard library
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

// C++ standard library
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <forward_list>
#include <optional>
#include <algorithm>
#include <iterator>
#include <functional>
#include <memory>

#include "Sci_Position.h"
#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ILoader.h"
#include "ILexer.h"

// src platform interface
#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"
#include "KeyMap.h"

using namespace Scintilla::Internal;

wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_CHANGE, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_STYLENEEDED, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_CHARADDED, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_SAVEPOINTREACHED, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_SAVEPOINTLEFT, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_ROMODIFYATTEMPT, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_KEY, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_DOUBLECLICK, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_UPDATEUI, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_MODIFIED, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_MACRORECORD, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_MARGINCLICK, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_NEEDSHOWN, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_PAINTED, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_USERLISTSELECTION, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_URIDROPPED, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_DWELLSTART, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_DWELLEND, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_START_DRAG, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_DRAG_OVER, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_DO_DROP, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_ZOOM, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_HOTSPOT_CLICK, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_HOTSPOT_DCLICK, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_CALLTIP_CLICK, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_AUTOCOMP_SELECTION, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_INDICATOR_CLICK, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_INDICATOR_RELEASE, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_AUTOCOMP_CANCELLED, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_AUTOCOMP_CHAR_DELETED, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_HOTSPOT_RELEASE_CLICK, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_CLIPBOARD_COPY, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_CLIPBOARD_PASTE, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_AUTOCOMP_COMPLETED, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_MARGIN_RIGHT_CLICK, jaeTextEditControlEvent);
wxDEFINE_EVENT(wxEVT_JAE_TEXT_EDIT_CONTROL_AUTOCOMP_SELECTION_CHANGE, jaeTextEditControlEvent);

wxIMPLEMENT_DYNAMIC_CLASS(jaeTextEditControlEvent, wxCommandEvent);

jaeTextEditControlEvent::jaeTextEditControlEvent(wxEventType commandType, int id) : wxCommandEvent(commandType, id) {
   m_position             = 0;
   m_key                  = 0;
   m_modifiers            = 0;
   m_modificationType     = 0;
   m_length               = 0;
   m_linesAdded           = 0;
   m_line                 = 0;
   m_foldLevelNow         = 0;
   m_foldLevelPrev        = 0;
   m_margin               = 0;
   m_message              = 0;
   m_wParam               = 0;
   m_lParam               = 0;
   m_listType             = 0;
   m_x                    = 0;
   m_y                    = 0;
   m_token                = 0;
   m_annotationLinesAdded = 0;
   m_updated              = 0;
   m_listCompletionMethod = 0;
   m_dragFlags            = wxDrag_CopyOnly;
   m_dragResult           = wxDragNone;
}

jaeTextEditControlEvent::jaeTextEditControlEvent(const jaeTextEditControlEvent &event) : wxCommandEvent(event) {
   m_position             = event.m_position;
   m_key                  = event.m_key;
   m_modifiers            = event.m_modifiers;
   m_modificationType     = event.m_modificationType;
   m_length               = event.m_length;
   m_linesAdded           = event.m_linesAdded;
   m_line                 = event.m_line;
   m_foldLevelNow         = event.m_foldLevelNow;
   m_foldLevelPrev        = event.m_foldLevelPrev;
   m_margin               = event.m_margin;
   m_message              = event.m_message;
   m_wParam               = event.m_wParam;
   m_lParam               = event.m_lParam;
   m_listType             = event.m_listType;
   m_x                    = event.m_x;
   m_y                    = event.m_y;
   m_token                = event.m_token;
   m_annotationLinesAdded = event.m_annotationLinesAdded;
   m_updated              = event.m_updated;
   m_listCompletionMethod = event.m_listCompletionMethod;
   m_dragFlags            = event.m_dragFlags;
   m_dragResult           = event.m_dragResult;
}

void jaeTextEditControlEvent::SetDragAllowMove(bool allow) {
   if (allow) {
      m_dragFlags |= wxDrag_AllowMove;
   } else {
      m_dragFlags &= ~(wxDrag_AllowMove | wxDrag_DefaultMove);
   }
}

auto jaeTextEditControlEvent::GetShift() const -> bool { return (m_modifiers & (int)Scintilla::KeyMod ::Shift) != 0; }
auto jaeTextEditControlEvent::GetControl() const -> bool { return (m_modifiers & (int)Scintilla::KeyMod ::Ctrl) != 0; }
auto jaeTextEditControlEvent::GetAlt() const -> bool { return (m_modifiers & (int)Scintilla::KeyMod ::Alt) != 0; }
