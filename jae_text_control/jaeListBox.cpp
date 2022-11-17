#include "jaeListBox.h"
#include "jaeScintilla.h"
#include "wx/dc.h"
#include "wx/mstream.h"
#include "wx/renderer.h"
#include "wx/settings.h"
#include "wx/tokenzr.h"
#include "wx/wupdlock.h"
#include "wx/xpmdecod.h"
#include <wx/rawbmp.h>
#include "jaePortFont.h"
#include "jaeTextEditControlEvent.h"
#include "jaeTextEditControlBase.h"

#define EXTENT_TEST wxT(" `~!@#$%^&*()-_=+\\|[]{};:\"\'<,>.?/1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
#if defined(__WXMSW__) || defined(__WXMAC__)
#define wxPy_premultiply(p, a) ((p) * (a) / 0xff)
#else
#define wxPy_premultiply(p, a) (p)
#endif

static auto BitmapFromRGBAImage(int width, int height, const unsigned char *pixelsImage) -> wxBitmap;
auto        stc2wx(const char *str, size_t len) -> wxString;
auto        stc2wx(const char *str) -> wxString;
auto        wx2stc(const wxString &str) -> wxWX2MBbuf;

jaePortListBoxVisualData::jaePortListBoxVisualData(int d)
    : m_desiredVisibleRows(d), m_useDefaultBgColour(true), m_useDefaultTextColour(true), m_useDefaultHighlightBgColour(true), m_useDefaultHighlightTextColour(true), m_hasListCtrlAppearance(true),
      m_useDefaultCurrentBgColour(true), m_useDefaultCurrentTextColour(true), m_listType(NULL), m_posStart(NULL), m_startLen(NULL) {
   ComputeColours();
}

jaePortListBoxVisualData::~jaePortListBoxVisualData() { m_imgList.clear(); }

void jaePortListBoxVisualData::SetDesiredVisibleRows(int d) { m_desiredVisibleRows = d; }

int jaePortListBoxVisualData::GetDesiredVisibleRows() const { return m_desiredVisibleRows; }

void jaePortListBoxVisualData::RegisterImage(int type, const wxBitmap &bmp) {
   if (!bmp.IsOk())
      return;

   ImgList::iterator it                           = m_imgList.find(type);
   bool              preExistingWithDifferentSize = false;
   if (it != m_imgList.end()) {
      if (it->second.GetSize() != bmp.GetSize()) {
         preExistingWithDifferentSize = true;
      }

      m_imgList.erase(it);
   }

   m_imgList[type] = bmp;

   if (preExistingWithDifferentSize) {
      m_imgAreaSize.Set(0, 0);

      for (ImgList::iterator imgIt = m_imgList.begin(); imgIt != m_imgList.end(); ++imgIt) {
         m_imgAreaSize.IncTo(it->second.GetSize());
      }
   } else {
      m_imgAreaSize.IncTo(bmp.GetSize());
   }
}

void jaePortListBoxVisualData::RegisterImage(int type, const char *xpm_data) {
   wxXPMDecoder dec;
   wxImage      img;

   // This check is borrowed from src/stc/scintilla/src/XPM.cpp.
   // Test done is two parts to avoid possibility of overstepping the memory
   // if memcmp implemented strangely. Must be 4 bytes at least at destination.
   if ((0 == memcmp(xpm_data, "/* X", 4)) && (0 == memcmp(xpm_data, "/* XPM */", 9))) {
      wxMemoryInputStream stream(xpm_data, strlen(xpm_data) + 1);
      img = dec.ReadFile(stream);
   } else
      img = dec.ReadData(reinterpret_cast<const char *const *>(xpm_data));

   wxBitmap bmp(img);
   RegisterImage(type, bmp);
}

void jaePortListBoxVisualData::RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage) {
   wxBitmap bmp = BitmapFromRGBAImage(width, height, pixelsImage);
   RegisterImage(type, bmp);
}

void jaePortListBoxVisualData::ClearRegisteredImages() {
   m_imgList.clear();
   m_imgAreaSize.Set(0, 0);
}

const wxBitmap *jaePortListBoxVisualData::GetImage(int i) const {
   ImgList::const_iterator it = m_imgList.find(i);

   if (it != m_imgList.end())
      return &(it->second);
   else
      return NULL;
}

int jaePortListBoxVisualData::GetImageAreaWidth() const { return m_imgAreaSize.GetWidth(); }

int jaePortListBoxVisualData::GetImageAreaHeight() const { return m_imgAreaSize.GetHeight(); }

void jaePortListBoxVisualData::ComputeColours() {
   // wxSYS_COLOUR_BTNSHADOW seems to be the closest match with most themes.
   m_borderColour = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);

   if (m_useDefaultBgColour)
      m_bgColour = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX);

   if (m_useDefaultTextColour)
      m_textColour = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);

   if (m_hasListCtrlAppearance) {
      // If m_highlightBgColour and/or m_currentBgColour are not
      // explicitly set, set them to wxNullColour to indicate that they
      // should be drawn with wxRendererNative.
      if (m_useDefaultHighlightBgColour)
         m_highlightBgColour = wxNullColour;

      if (m_useDefaultCurrentBgColour)
         m_currentBgColour = wxNullColour;

#ifdef __WXMSW__
      if (m_useDefaultHighlightTextColour)
         m_highlightTextColour = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);
#else
      if (m_useDefaultHighlightTextColour)
         m_highlightTextColour = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT);
#endif

      if (m_useDefaultCurrentTextColour)
         m_currentTextColour = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);
   } else {
#ifdef __WXOSX_COCOA__
      if (m_useDefaultHighlightBgColour)
         m_highlightBgColour = GetListHighlightColour();
#else
      if (m_useDefaultHighlightBgColour)
         m_highlightBgColour = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
#endif

      if (m_useDefaultHighlightTextColour)
         m_highlightTextColour = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT);
   }
}

static void SetColourHelper(bool &isDefault, wxColour &itemColour, const wxColour &newColour) {
   isDefault  = !newColour.IsOk();
   itemColour = newColour;
}

void jaePortListBoxVisualData::SetColours(const wxColour &bg, const wxColour &txt, const wxColour &hlbg, const wxColour &hltext) {
   SetColourHelper(m_useDefaultBgColour, m_bgColour, bg);
   SetColourHelper(m_useDefaultTextColour, m_textColour, txt);
   SetColourHelper(m_useDefaultHighlightBgColour, m_highlightBgColour, hlbg);
   SetColourHelper(m_useDefaultHighlightTextColour, m_highlightTextColour, hltext);
   ComputeColours();
}

const wxColour &jaePortListBoxVisualData::GetBorderColour() const { return m_borderColour; }

const wxColour &jaePortListBoxVisualData::GetBgColour() const { return m_bgColour; }

const wxColour &jaePortListBoxVisualData::GetTextColour() const { return m_textColour; }

const wxColour &jaePortListBoxVisualData::GetHighlightBgColour() const { return m_highlightBgColour; }

const wxColour &jaePortListBoxVisualData::GetHighlightTextColour() const { return m_highlightTextColour; }

void jaePortListBoxVisualData::UseListCtrlStyle(bool useListCtrlStyle, const wxColour &curBg, const wxColour &curText) {
   m_hasListCtrlAppearance = useListCtrlStyle;
   SetColourHelper(m_useDefaultCurrentBgColour, m_currentBgColour, curBg);
   SetColourHelper(m_useDefaultCurrentTextColour, m_currentTextColour, curText);
   ComputeColours();
}

bool jaePortListBoxVisualData::HasListCtrlAppearance() const { return m_hasListCtrlAppearance; }

const wxColour &jaePortListBoxVisualData::GetCurrentBgColour() const { return m_currentBgColour; }

const wxColour &jaePortListBoxVisualData::GetCurrentTextColour() const { return m_currentTextColour; }

void jaePortListBoxVisualData::SetSciListData(int *type, int *pos, int *len) {
   m_listType = type;
   m_posStart = pos;
   m_startLen = len;
}

int jaePortListBoxVisualData::GetListType() const { return (m_listType ? *m_listType : 0); }

int jaePortListBoxVisualData::GetPosStart() const { return (m_posStart ? *m_posStart : 0); }

int jaePortListBoxVisualData::GetStartLen() const { return (m_startLen ? *m_startLen : 0); }

jaePortListBox::jaePortListBox(wxWindow *parent, jaePortListBoxVisualData *v, int ht)
    : wxSystemThemedControl<wxVListBox>(), m_visualData(v), m_maxStrWidth(0), m_currentRow(wxNOT_FOUND), m_doubleClickAction(nullptr), m_doubleClickActionData(nullptr), m_aveCharWidth(8),
      m_textHeight(ht), m_itemHeight(ht), m_textTopGap(0) {
   wxVListBox::Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE, "AutoCompListBox");

   m_imagePadding             = FromDIP(1);
   m_textBoxToTextGap         = FromDIP(3);
   m_textExtraVerticalPadding = FromDIP(1);

   SetBackgroundColour(m_visualData->GetBgColour());

   Bind(wxEVT_LISTBOX, &jaePortListBox::OnSelection, this);
   Bind(wxEVT_LISTBOX_DCLICK, &jaePortListBox::OnDClick, this);
   Bind(wxEVT_SYS_COLOUR_CHANGED, &jaePortListBox::OnSysColourChanged, this);
   Bind(wxEVT_DPI_CHANGED, &jaePortListBox::OnDPIChanged, this);

   if (m_visualData->HasListCtrlAppearance()) {
      EnableSystemTheme();
      Bind(wxEVT_MOTION, &jaePortListBox::OnMouseMotion, this);
      Bind(wxEVT_LEAVE_WINDOW, &jaePortListBox::OnMouseLeaveWindow, this);

#ifdef __WXMSW__
      // On MSW when using wxRendererNative to draw items in list control
      // style, the colours used seem to be based on the parent's
      // background colour. So set the popup's background.
      parent->SetOwnBackgroundColour(m_visualData->GetBgColour());
#endif
   }
}

bool jaePortListBox::AcceptsFocus() const { return false; }

// Do nothing in response to an attempt to set focus.
void jaePortListBox::SetFocus() {}

void jaePortListBox::SetContainerBorderSize(int s) { m_borderSize = s; }

void jaePortListBox::SetListBoxFont(const Font *font) {
   SetFont(((jaeScintillaFont *)font)->getFont());
   int w;
   GetTextExtent(EXTENT_TEST, &w, &m_textHeight);
   RecalculateItemHeight();
}

void jaePortListBox::SetAverageCharWidth(int width) { m_aveCharWidth = width; }

PRectangle jaePortListBox::GetDesiredRect() const {
   int maxw = m_maxStrWidth * m_aveCharWidth;
   int maxh;

   // give it a default if there are no lines, and/or add a bit more
   if (maxw == 0)
      maxw = 100;

   maxw += TextBoxFromClientEdge() + m_textBoxToTextGap + m_aveCharWidth * 3;

   // estimate a desired height
   const int count              = Length();
   const int desiredVisibleRows = m_visualData->GetDesiredVisibleRows();
   if (count) {
      if (count <= desiredVisibleRows)
         maxh = count * m_itemHeight;
      else
         maxh = desiredVisibleRows * m_itemHeight;
   } else
      maxh = 100;

   // Add space for a scrollbar if needed.
   if (count > desiredVisibleRows)
      maxw += wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, this);

   // Add borders.
   maxw += 2 * m_borderSize;
   maxh += 2 * m_borderSize;

   PRectangle rc;
   rc.top    = 0;
   rc.left   = 0;
   rc.right  = maxw;
   rc.bottom = maxh;
   return rc;
}

int jaePortListBox::CaretFromEdge() const { return m_borderSize + TextBoxFromClientEdge() + m_textBoxToTextGap; }

void jaePortListBox::Clear() {
   m_labels.clear();
   m_imageNos.clear();
}

void jaePortListBox::Append(char *s, int type) {
   AppendHelper(stc2wx(s), type);
   RecalculateItemHeight();
}

int jaePortListBox::Length() const { return GetItemCount(); }

void jaePortListBox::Select(int n) {
   SetSelection(n);
   SelectHelper(n);
}

auto jaePortListBox::GetValue(int n) -> std::string { return m_labels[n].ToStdString(); }

void jaePortListBox::SetDoubleClickAction(CallBackAction action, void *data) {
   m_doubleClickAction     = action;
   m_doubleClickActionData = data;
}

void jaePortListBox::SetList(const char *list, char separator, char typesep) {
   wxWindowUpdateLocker noUpdates(this);
   Clear();
   wxStringTokenizer tkzr(stc2wx(list), (wxChar)separator);
   while (tkzr.HasMoreTokens()) {
      wxString token = tkzr.GetNextToken();
      long     type  = -1;
      int      pos   = token.Find(typesep);
      if (pos != -1) {
         token.Mid(pos + 1).ToLong(&type);
         token.Truncate(pos);
      }
      AppendHelper(token, (int)type);
   }

   RecalculateItemHeight();
}

void jaePortListBox::AppendHelper(const wxString &text, int type) {
   m_maxStrWidth = wxMax(m_maxStrWidth, text.length());
   m_labels.push_back(text);
   m_imageNos.push_back(type);
   SetItemCount(m_labels.size());
}

void jaePortListBox::SelectHelper(int i) {
   // This method is used to trigger the wxEVT_STC_AUTOCOMP_SELECTION_CHANGE
   // event. This event is generated directly here since the version of
   // Scintilla currently used does not support it.

   // If the Scintilla component is updated, it should be sufficient to:
   // 1) Change this method to use a callback to let Scintilla generate the
   //    event.
   // 2) Remove the SELECTION_CHANGE event data from the wxSTCListBoxVisualData
   //    class and the SetListInfo method from the ListBoxImpl class since they
   //    will no longer be needed.

   jaeTextEditControlBase *stc = wxDynamicCast(GetGrandParent(), jaeTextEditControlBase);

   if (stc != nullptr) {
      jaeTextEditControlEvent evt(wxEVT_JAE_TEXT_EDIT_CONTROL_AUTOCOMP_SELECTION_CHANGE, stc->GetId());
      evt.SetEventObject(stc);
      evt.SetListType(m_visualData->GetListType());

      evt.SetPosition(m_visualData->GetPosStart() - m_visualData->GetStartLen());

      if (0 <= i && i < static_cast<int>(m_labels.size())) {
         evt.SetString(m_labels[i]);
      }

      stc->ProcessWindowEvent(evt);
   }
}

void jaePortListBox::RecalculateItemHeight() {
   m_itemHeight = wxMax(m_textHeight + 2 * m_textExtraVerticalPadding, m_visualData->GetImageAreaHeight() + 2 * m_imagePadding);
   m_textTopGap = (m_itemHeight - m_textHeight) / 2;
}

int jaePortListBox::TextBoxFromClientEdge() const {
   const int width = m_visualData->GetImageAreaWidth();
   return (width == 0 ? 0 : width + 2 * m_imagePadding);
}

void jaePortListBox::OnSelection(wxCommandEvent &event) { SelectHelper(event.GetSelection()); }

void jaePortListBox::OnDClick(wxCommandEvent &WXUNUSED(event)) {
   if (m_doubleClickAction)
      m_doubleClickAction(m_doubleClickActionData);
}

void jaePortListBox::OnSysColourChanged(wxSysColourChangedEvent &WXUNUSED(event)) {
   m_visualData->ComputeColours();
   GetParent()->SetOwnBackgroundColour(m_visualData->GetBgColour());
   SetBackgroundColour(m_visualData->GetBgColour());
   GetParent()->Refresh();
}

void jaePortListBox::OnDPIChanged(wxDPIChangedEvent &WXUNUSED(event)) {
   m_imagePadding             = FromDIP(1);
   m_textBoxToTextGap         = FromDIP(3);
   m_textExtraVerticalPadding = FromDIP(1);

   int w;
   GetTextExtent(EXTENT_TEST, &w, &m_textHeight);

   RecalculateItemHeight();
}

void jaePortListBox::OnMouseLeaveWindow(wxMouseEvent &event) {
   const int old = m_currentRow;
   m_currentRow  = wxNOT_FOUND;

   if (old != wxNOT_FOUND)
      RefreshRow(old);

   event.Skip();
}

void jaePortListBox::OnMouseMotion(wxMouseEvent &event) {
   const int old = m_currentRow;
   m_currentRow  = VirtualHitTest(event.GetY());

   if (old != m_currentRow) {
      if (m_currentRow != wxNOT_FOUND)
         RefreshRow(m_currentRow);

      if (old != wxNOT_FOUND)
         RefreshRow(old);
   }

   event.Skip();
}

wxCoord jaePortListBox::OnMeasureItem(size_t WXUNUSED(n)) const { return static_cast<wxCoord>(m_itemHeight); }

// This control will be drawn so that a typical row of pixels looks like:
//
//    +++++++++++++++++++++++++   =====ITEM TEXT================
//  |         |                 |    |
//  |       imageAreaWidth      |    |
//  |                           |    |
// m_imagePadding               |   m_textBoxToTextGap
//                              |
//                   m_imagePadding
//
//
// m_imagePadding            : Used to give a little extra space between the
//                             client edge and an item's bitmap.
// imageAreaWidth            : Computed as the width of the largest registered
//                             bitmap (part of wxSTCListBoxVisualData).
// m_textBoxToTextGap        : Used so that item text does not begin immediately
//                             at the edge of the highlight box.
//
// Images are drawn centered in the image area.
// If a selection rectangle is drawn, its left edge is at x=0 if there are
// no bitmaps. Otherwise
//       x = m_imagePadding + m_imageAreaWidth + m_imagePadding.
// Text is drawn at x + m_textBoxToTextGap and centered vertically.

void jaePortListBox::OnDrawItemText(wxDC &dc, const wxRect &rect, const wxString &label, const wxColour &textCol) const {
   wxDCTextColourChanger tcc(dc, textCol);

   wxString ellipsizedlabel = wxControl::Ellipsize(label, dc, wxELLIPSIZE_END, rect.GetWidth());
   dc.DrawText(ellipsizedlabel, rect.GetLeft(), rect.GetTop());
}

void jaePortListBox::OnDrawItem(wxDC &dc, const wxRect &rect, size_t n) const {
   wxString label;
   int      imageNo = -1;
   if (n < m_labels.size()) {
      label   = m_labels[n];
      imageNo = m_imageNos[n];
   }

   int topGap  = m_textTopGap;
   int leftGap = TextBoxFromClientEdge() + m_textBoxToTextGap;

   wxColour textCol;

   if (IsSelected(n))
      textCol = m_visualData->GetHighlightTextColour();
   else if (static_cast<int>(n) == m_currentRow)
      textCol = m_visualData->GetCurrentTextColour();
   else
      textCol = m_visualData->GetTextColour();

   wxRect rect2(rect.GetLeft() + leftGap, rect.GetTop() + topGap, rect.GetWidth() - leftGap, m_textHeight);

   OnDrawItemText(dc, rect2, label, textCol);

   const wxBitmap *b = m_visualData->GetImage(imageNo);
   if (b) {
      const int width = m_visualData->GetImageAreaWidth();
      topGap          = (m_itemHeight - b->GetHeight()) / 2;
      leftGap         = m_imagePadding + (width - b->GetWidth()) / 2;
      dc.DrawBitmap(*b, rect.GetLeft() + leftGap, rect.GetTop() + topGap, true);
   }
}

void jaePortListBox::OnDrawBackground(wxDC &dc, const wxRect &rect, size_t n) const {
   if (IsSelected(n)) {
      wxRect          selectionRect(rect);
      const wxColour &highlightBgColour = m_visualData->GetHighlightBgColour();

#ifdef __WXMSW__
      if (!m_visualData->HasListCtrlAppearance()) {
         // On windows the selection rectangle in Scintilla's
         // autocompletion list only covers the text and not the icon.

         const int textBoxFromClientEdge = TextBoxFromClientEdge();
         selectionRect.SetLeft(rect.GetLeft() + textBoxFromClientEdge);
         selectionRect.SetWidth(rect.GetWidth() - textBoxFromClientEdge);
      }
#endif // __WXMSW__

      if (highlightBgColour.IsOk()) {
         wxDCBrushChanger bc(dc, highlightBgColour);
         wxDCPenChanger   pc(dc, highlightBgColour);
         dc.DrawRectangle(selectionRect);
      } else {
         wxRendererNative::GetDefault().DrawItemSelectionRect(const_cast<jaePortListBox *>(this), dc, selectionRect, wxCONTROL_SELECTED | wxCONTROL_FOCUSED);
      }

      if (!m_visualData->HasListCtrlAppearance())
         wxRendererNative::GetDefault().DrawFocusRect(const_cast<jaePortListBox *>(this), dc, selectionRect);
   } else if (static_cast<int>(n) == m_currentRow) {
      const wxColour &currentBgColour = m_visualData->GetCurrentBgColour();

      if (currentBgColour.IsOk()) {
         wxDCBrushChanger bc(dc, currentBgColour);
         wxDCPenChanger   pc(dc, currentBgColour);
         dc.DrawRectangle(rect);
      } else {
         wxRendererNative::GetDefault().DrawItemSelectionRect(const_cast<jaePortListBox *>(this), dc, rect, wxCONTROL_CURRENT | wxCONTROL_FOCUSED);
      }
   }
}

#ifdef wxHAS_RAW_BITMAP
wxBitmap BitmapFromRGBAImage(int width, int height, const unsigned char *pixelsImage) {
   int              x, y;
   wxBitmap         bmp(width, height, 32);
   wxAlphaPixelData pixData(bmp);

   wxAlphaPixelData::Iterator p(pixData);
   for (y = 0; y < height; y++) {
      p.MoveTo(pixData, 0, y);
      for (x = 0; x < width; x++) {
         unsigned char red   = *pixelsImage++;
         unsigned char green = *pixelsImage++;
         unsigned char blue  = *pixelsImage++;
         unsigned char alpha = *pixelsImage++;

         p.Red()   = wxPy_premultiply(red, alpha);
         p.Green() = wxPy_premultiply(green, alpha);
         p.Blue()  = wxPy_premultiply(blue, alpha);
         p.Alpha() = alpha;
         ++p;
      }
   }
   return bmp;
}
#else
wxBitmap BitmapFromRGBAImage(int width, int height, const unsigned char *pixelsImage) {
   const int totalPixels = width * height;
   wxScopedArray<unsigned char> data(3 * totalPixels);
   wxScopedArray<unsigned char> alpha(totalPixels);
   int curDataLocation = 0, curAlphaLocation = 0, curPixelsImageLocation = 0;

   for (int i = 0; i < totalPixels; ++i) {
      data[curDataLocation++] = pixelsImage[curPixelsImageLocation++];
      data[curDataLocation++] = pixelsImage[curPixelsImageLocation++];
      data[curDataLocation++] = pixelsImage[curPixelsImageLocation++];
      alpha[curAlphaLocation++] = pixelsImage[curPixelsImageLocation++];
   }

   wxImage img(width, height, data.get(), alpha.get(), true);
   wxBitmap bmp(img);

   return bmp;
}
#endif

auto stc2wx(const char *str, size_t len) -> wxString { return wxString::FromUTF8(str, len); }

auto stc2wx(const char *str) -> wxString { return wxString::FromUTF8(str); }

auto wx2stc(const wxString &str) -> wxWX2MBbuf { return str.utf8_str(); }
