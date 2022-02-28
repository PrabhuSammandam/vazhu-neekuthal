#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <algorithm>
#include <memory>
#include <sstream>
#include <cstdint>
#include "Geometry.h"
#include "Sci_Position.h"
#include "ScintillaTypes.h"
#include "Debugging.h"
#include "Platform.h"

#include "wx/systhemectrl.h"
#include "wx/vlbox.h"

using CallBackAction = void (*)(void *);

using namespace Scintilla::Internal;

class jaePortListBoxVisualData {
 public:
   explicit jaePortListBoxVisualData(int d);
   virtual ~jaePortListBoxVisualData();

   // ListBoxImpl implementation
   void SetDesiredVisibleRows(int d);
   auto GetDesiredVisibleRows() const -> int;
   void RegisterImage(int type, const wxBitmap &bmp);
   void RegisterImage(int, const char *);
   void RegisterRGBAImage(int, int, int, const unsigned char *);
   void ClearRegisteredImages();

   // Image data
   auto GetImage(int i) const -> const wxBitmap *;
   auto GetImageAreaWidth() const -> int;
   auto GetImageAreaHeight() const -> int;

   // Colour data
   void ComputeColours();
   auto GetBorderColour() const -> const wxColour &;
   void SetColours(const wxColour &, const wxColour &, const wxColour &, const wxColour &);
   auto GetBgColour() const -> const wxColour &;
   auto GetTextColour() const -> const wxColour &;
   auto GetHighlightBgColour() const -> const wxColour &;
   auto GetHighlightTextColour() const -> const wxColour &;

   // ListCtrl Style
   void UseListCtrlStyle(bool, const wxColour &, const wxColour &);
   auto HasListCtrlAppearance() const -> bool;
   auto GetCurrentBgColour() const -> const wxColour &;
   auto GetCurrentTextColour() const -> const wxColour &;

   // Data needed for SELECTION_CHANGE event
   void SetSciListData(int *, int *, int *);
   auto GetListType() const -> int;
   auto GetPosStart() const -> int;
   auto GetStartLen() const -> int;

 private:
   WX_DECLARE_HASH_MAP(int, wxBitmap, wxIntegerHash, wxIntegerEqual, ImgList);

   int     m_desiredVisibleRows;
   ImgList m_imgList;
   wxSize  m_imgAreaSize;

   wxColour m_borderColour;
   wxColour m_bgColour;
   wxColour m_textColour;
   wxColour m_highlightBgColour;
   wxColour m_highlightTextColour;
   bool     m_useDefaultBgColour;
   bool     m_useDefaultTextColour;
   bool     m_useDefaultHighlightBgColour;
   bool     m_useDefaultHighlightTextColour;
   bool     m_hasListCtrlAppearance;
   wxColour m_currentBgColour;
   wxColour m_currentTextColour;
   bool     m_useDefaultCurrentBgColour;
   bool     m_useDefaultCurrentTextColour;
   int *    m_listType;
   int *    m_posStart;
   int *    m_startLen;
};

// The class is intended to look like a standard listbox (with an optional
// icon). However, it needs to look like it has focus even when it doesn't.
class jaePortListBox : public wxSystemThemedControl<wxVListBox> {
 public:
   jaePortListBox(wxWindow *, jaePortListBoxVisualData *, int);

   // wxWindow overrides
   auto AcceptsFocus() const -> bool override;
   void SetFocus() override;

   // Setters
   void SetContainerBorderSize(int);

   // ListBoxImpl implementation
   virtual void SetListBoxFont(const Scintilla::Internal::Font *font);
   void         SetAverageCharWidth(int width);
   auto         GetDesiredRect() const -> PRectangle;
   auto         CaretFromEdge() const -> int;
   void         Clear();
   void         Append(char *s, int type = -1);
   auto         Length() const -> int;
   void         Select(int n);
   auto         GetValue(int n) -> std::string;
   void         SetList(const char *list, char separator, char typesep);
   void         SetDoubleClickAction(CallBackAction, void *);

 protected:
   // Helpers
   void         AppendHelper(const wxString &text, int type);
   void         SelectHelper(int i);
   void         RecalculateItemHeight();
   auto         TextBoxFromClientEdge() const -> int;
   virtual void OnDrawItemText(wxDC &, const wxRect &, const wxString &, const wxColour &) const;

   // Event handlers
   void OnSelection(wxCommandEvent &);
   void OnDClick(wxCommandEvent &);
   void OnSysColourChanged(wxSysColourChangedEvent &event);
   void OnDPIChanged(wxDPIChangedEvent &event);
   void OnMouseMotion(wxMouseEvent &event);
   void OnMouseLeaveWindow(wxMouseEvent &event);

   // wxVListBox overrides
   auto OnMeasureItem(size_t /*n*/) const -> wxCoord override;
   void OnDrawItem(wxDC & /*dc*/, const wxRect & /*rect*/, size_t /*n*/) const override;
   void OnDrawBackground(wxDC & /*dc*/, const wxRect & /*rect*/, size_t /*n*/) const override;

   jaePortListBoxVisualData *m_visualData;
   wxVector<wxString>        m_labels;
   wxVector<int>             m_imageNos;
   size_t                    m_maxStrWidth;
   int                       m_currentRow;

   CallBackAction m_doubleClickAction;
   void *         m_doubleClickActionData;
   int            m_aveCharWidth;
   // These drawing parameters are computed or set externally.
   int m_borderSize;
   int m_textHeight;
   int m_itemHeight;
   int m_textTopGap;
   // These drawing parameters are set internally and can be changed if needed
   // to better match the appearance of a list box on a specific platform.
   int m_imagePadding;
   int m_textBoxToTextGap;
   int m_textExtraVerticalPadding;
};
