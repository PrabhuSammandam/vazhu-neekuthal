#pragma once

// C standard library
#include <array>
#include <stddef.h>
#include <stdint.h>

// C++ wrappers of C standard library
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cmath>
#include <climits>

// C++ standard library
#include <stdexcept>
#include <new>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <forward_list>
#include <optional>
#include <algorithm>
#include <iterator>
#include <functional>
#include <memory>
#include <numeric>
#include <chrono>
#include <regex>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <mutex>

// POSIX
#include <dlfcn.h>
#include <sys/time.h>

#include "Sci_Position.h"
#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaStructures.h"
#include "ILoader.h"
#include "ILexer.h"

// src platform interface
#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "Scintilla.h"
#include "ScintillaWidget.h"

#include "CharacterType.h"
#include "CharacterCategoryMap.h"
#include "Position.h"
#include "UniqueString.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "SparseVector.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "PerLine.h"
#include "CallTip.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"
#include "RESearch.h"
#include "CaseConvert.h"
#include "UniConversion.h"
#include "DBCS.h"
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"
#include "MarginView.h"
#include "EditView.h"
#include "Editor.h"
#include "ElapsedPeriod.h"

#include "AutoComplete.h"
#include "ScintillaBase.h"
#include "wx/hashmap.h"
#include <wx/timer.h>
#include <wx/dataview.h>

#define wxHAVE_STC_RECT_FORMAT

class jaeTextEditControlBase;
class jaeTimer;
class jaeDropTarget;

class jaeScintilla : public Scintilla::Internal::ScintillaBase {

 public:
   explicit jaeScintilla(jaeTextEditControlBase *editControl);
   ~jaeScintilla() override;

   jaeScintilla(const jaeScintilla &) = delete;
   jaeScintilla(jaeScintilla &&)      = delete;
   auto operator=(const jaeScintilla &) -> jaeScintilla & = delete;
   auto operator=(jaeScintilla &&) -> jaeScintilla & = delete;

   /* overrides of ScintillaBase */
   void Initialise() override;
   void Finalise() override;
   void StartDrag() override;
   void SetMouseCapture(bool on) override;
   auto HaveMouseCapture() -> bool override;
   void ScrollText(Sci::Line linesToMove) override;
   void SetVerticalScrollPos() override;
   void SetHorizontalScrollPos() override;
   auto ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) -> bool override;
   void Copy() override;
   void Paste() override;
   auto CanPaste() -> bool override;
   void CopyToClipboard(const Scintilla::Internal::SelectionText &selectedText) override;
   auto DefWndProc(Scintilla::Message iMessage, uptr_t wParam, sptr_t lParam) -> sptr_t override;
   auto WndProc(Scintilla::Message iMessage, uptr_t wParam, sptr_t lParam) -> sptr_t override;
   void ClaimSelection() override;
   void NotifyChange() override;
   void NotifyParent(Scintilla::NotificationData scn) override;
   auto SetIdle(bool on) -> bool override;
   auto FineTickerRunning(TickReason reason) -> bool override;
   void FineTickerStart(TickReason reason, int millis, int tolerance) override;
   void FineTickerCancel(TickReason reason) override;
   void CreateCallTipWindow(Scintilla::Internal::PRectangle rc) override {}
   void AddToPopUp(const char *label, int cmd = 0, bool enabled = true) override;
   auto UTF8FromEncoded(std::string_view encoded) const -> std::string override { return std::string{}; }
   auto EncodedFromUTF8(std::string_view utf8) const -> std::string override { return std::string{}; }

   static auto DirectFunction(sptr_t ptr, unsigned int iMessage, uptr_t wParam, sptr_t lParam) -> sptr_t;
   static auto DirectStatusFunction(sptr_t ptr, unsigned int iMessage, uptr_t wParam, sptr_t lParam, int *pStatus) -> sptr_t;

   /* utils */
   void FullPaint();
   void FullPaintDC(wxDC *dc);
   void SetUseAntiAliasing(bool useAA);
   auto GetUseAntiAliasing() -> bool;

   /* Event Handling */
   void DoPaint(wxDC *dc, wxRect rect);
   void DoHScroll(int type, int pos);
   void DoVScroll(int type, int pos);
   void DoSize(int width, int height);
   void DoLoseFocus();
   void DoGainFocus();
   void DoInvalidateStyleData();
   void DoLeftButtonDown(Scintilla::Internal::Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt);
   void DoRightButtonDown(Scintilla::Internal::Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt);
   void DoLeftButtonUp(Scintilla::Internal::Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt);
   void DoLeftButtonMove(Scintilla::Internal::Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt);
   void DoMiddleButtonUp(Scintilla::Internal::Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt);
   void DoMouseWheel(wxMouseWheelAxis axis, int rotation, int delta, int linesPerAction, int columnsPerAction, bool ctrlDown, bool isPageScroll);
   void DoAddChar(int key);
   auto DoKeyDown(const wxKeyEvent &event, bool *consumed) -> int;
   void DoOnIdle(wxIdleEvent &evt);
   void DoCommand(int ID);
   auto DoContextMenu(Scintilla::Internal::Point pt) -> bool;
   void DoOnListBox();
   void DoMouseCaptureLost();
   auto DoDropText(long x, long y, const wxString &data) -> bool;
   auto DoDragEnter(wxCoord x, wxCoord y, wxDragResult def) -> wxDragResult;
   auto DoDragOver(wxCoord x, wxCoord y, wxDragResult def) -> wxDragResult;
   void DoDragLeave();
   void DoScrollToLine(int line);
   void DoScrollToColumn(int column);
   void DoMarkerDefineBitmap(int markerNumber, const wxBitmap &bmp);
   void DoRegisterImage(int type, const wxBitmap &bmp);

 private:
   jaeTextEditControlBase *   _editControl   = nullptr;
   bool                       _capturedMouse = false;
   bool                       _focusEvent    = false;
   wxDragResult               dragResult     = wxDragNone;
   jaeDropTarget *            dropTarget     = nullptr;
   int                        wheelVRotation = 0;
   int                        wheelHRotation = 0;
   std::array<jaeTimer *, 10> _timers;
   wxDataFormat               m_clipRectTextFormat;

   friend class jaeTimer; // To get access to TickReason declaration
};
