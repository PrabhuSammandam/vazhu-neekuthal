#include "jaePortSurface.h"
#include "jaePortFont.h"
#include "jaeUtils.h"
#include "Scintilla.h"

#include "wx/brush.h"
#include "wx/colour.h"
#include "wx/dc.h"
#include "wx/dcgraph.h"
#include "wx/dcmemory.h"
#include "wx/gdicmn.h"

using namespace Scintilla::Internal;

#define EXTENT_TEST wxT(" `~!@#$%^&*()-_=+\\|[]{};:\"\'<,>.?/1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")

auto Surface::Allocate(Technology /*unused*/) -> std::unique_ptr<Surface> //
{
   return std::make_unique<jaeScintillaSurface>();
}

class jaeMemoryDc : public wxMemoryDC {
 public:
   explicit jaeMemoryDc(wxWindow *window, int width, int height) : _window{window} {
      if (width < 1) {
         width = 1;
      }
      if (height < 1) {
         height = 1;
      }
      GetImpl()->SetWindow(_window);
      if (_window != nullptr) {
         _bitmap.CreateScaled(width, height, wxBITMAP_SCREEN_DEPTH, _window->GetContentScaleFactor());
      } else {
         _bitmap.Create(width, height, wxBITMAP_SCREEN_DEPTH);
      }
      SelectObject(_bitmap);
   }

 private:
   wxBitmap  _bitmap;
   wxWindow *_window = nullptr;
};

jaeScintillaSurface::jaeScintillaSurface() : Surface{} {}

jaeScintillaSurface::jaeScintillaSurface(int width, int height) {
   hdc      = new jaeMemoryDc{static_cast<wxWindow *>(_wid), width, height};
   hdcOwned = true;
}

jaeScintillaSurface::~jaeScintillaSurface() { Release(); }

/*
initialise this surface with the window ID. Here we are creating our own DC of size 1, since we are not checking for hdc as NULL.
*/

void jaeScintillaSurface::Init(WindowID wid) {
   Release();
   _wid     = wid;
   hdc      = new jaeMemoryDc{static_cast<wxWindow *>(_wid), 1, 1};
   hdcOwned = true;
}

/*
initialise this surface with the window ID.
Here we are not creating DC and we are using the passed DC. Usually the passed DC is client DC from Paint API.
This is very important to know the ideology followed in scitilla that for window painting or sepate memory paiting
they are using the surface implemention generically.
*/
void jaeScintillaSurface::Init(SurfaceID sid, WindowID wid) {
   Release();
   hdc      = static_cast<wxDC *>(sid);
   _wid     = wid;
   hdcOwned = false;
}

auto jaeScintillaSurface::AllocatePixMap(int width, int height) -> std::unique_ptr<Surface> { return std::make_unique<jaeScintillaSurface>(width, height); }

void jaeScintillaSurface::SetMode(SurfaceMode mode_) {
   _mode = mode_;

   unicodeMode = _mode.codePage == SC_CP_UTF8;
}

auto jaeScintillaSurface::SupportsFeature(Scintilla::Supports /*feature*/) noexcept -> int { return 0; }

void jaeScintillaSurface::Release() noexcept {
   if (hdcOwned) {
      delete hdc;
      hdc      = nullptr;
      hdcOwned = false;
   }
}

auto jaeScintillaSurface::Initialised() -> bool { return hdc != nullptr; }

auto jaeScintillaSurface::LogPixelsY() -> int { return hdc->GetPPI().y; }

void jaeScintillaSurface::LineDraw(Point start, Point end, Stroke stroke) {
   auto           pen = wxPen{stroke.colour.AsInteger(), static_cast<int>(stroke.width)};
   wxDCPenChanger pc{*hdc, pen};

   hdc->DrawLine(static_cast<int>(start.x), static_cast<int>(start.y), static_cast<int>(end.x), static_cast<int>(end.y));
}

void jaeScintillaSurface::PolyLine(const Point *pts, size_t npts, Stroke stroke) {

   auto           pen = wxPen{stroke.colour.AsInteger(), static_cast<int>(stroke.width)};
   wxDCPenChanger pc{*hdc, pen};

   int startX = static_cast<int>(pts[0].x);
   int startY = static_cast<int>(pts[0].y);

   for (auto i = 1; i < npts; i++) {
      hdc->DrawLine(startX, startY, static_cast<int>(pts[i].x), static_cast<int>(pts[i].y));
      startX = static_cast<int>(pts[i].x);
      startY = static_cast<int>(pts[i].y);
   }
}

void jaeScintillaSurface::Polygon(const Point *pts, size_t npts, FillStroke fillStroke) {
   auto pen   = wxPen{fillStroke.stroke.colour.AsInteger(), static_cast<int>(fillStroke.stroke.width)};
   auto brush = wxBrush{fillStroke.fill.colour.AsInteger()};

   wxDCPenChanger   pc{*hdc, pen};
   wxDCBrushChanger bc{*hdc, brush};

   auto *p = new wxPoint[npts];

   for (auto i = 0; i < npts; i++) {
      p[i].x = static_cast<int>(pts[i].x);
      p[i].y = static_cast<int>(pts[i].y);
   }
   hdc->DrawPolygon(npts, p);
   delete[] p;
}

void jaeScintillaSurface::RectangleDraw(PRectangle rc, FillStroke fillStroke) {
   auto pen   = wxPen{fillStroke.stroke.colour.AsInteger(), static_cast<int>(fillStroke.stroke.width)};
   auto brush = wxBrush{fillStroke.fill.colour.AsInteger()};

   wxDCPenChanger   pc{*hdc, pen};
   wxDCBrushChanger bc{*hdc, brush};

   hdc->DrawRectangle(static_cast<int>(rc.left), static_cast<int>(rc.top), static_cast<int>(rc.Width()), static_cast<int>(rc.Height()));
}

void jaeScintillaSurface::RectangleFrame(PRectangle rc, Stroke stroke) {
   auto pen = wxPen{stroke.colour.AsInteger(), static_cast<int>(stroke.width)};

   wxDCPenChanger   pc{*hdc, pen};
   wxDCBrushChanger bc{*hdc, wxNullBrush};

   hdc->DrawRectangle(static_cast<int>(rc.left), static_cast<int>(rc.top), static_cast<int>(rc.Width()), static_cast<int>(rc.Height()));
}

void jaeScintillaSurface::FillRectangle(PRectangle rc, Fill fill) {
   auto brush = wxBrush{fill.colour.AsInteger()};

   wxDCPenChanger   pc{*hdc, *wxTRANSPARENT_PEN};
   wxDCBrushChanger bc{*hdc, brush};

   hdc->DrawRectangle(static_cast<int>(rc.left), static_cast<int>(rc.top), static_cast<int>(rc.Width()), static_cast<int>(rc.Height()));
}

void jaeScintillaSurface::FillRectangle(PRectangle rc, Surface &surfacePattern) {
   wxBrush br;
   auto *  ourDc = dynamic_cast<jaeMemoryDc *>(dynamic_cast<jaeScintillaSurface &>(surfacePattern).hdc);

   if (ourDc->GetSelectedBitmap().IsOk()) {
      br = wxBrush(ourDc->GetSelectedBitmap());
   } else { // Something is wrong so display in red
      br = wxBrush(*wxRED);
   }

   wxDCPenChanger   pc{*hdc, *wxTRANSPARENT_PEN};
   wxDCBrushChanger bc{*hdc, br};

   hdc->DrawRectangle(static_cast<int>(rc.left), static_cast<int>(rc.top), static_cast<int>(rc.Width()), static_cast<int>(rc.Height()));
}

void jaeScintillaSurface::RoundedRectangle(PRectangle rc, FillStroke fillStroke) {
   auto pen   = wxPen{fillStroke.stroke.colour.AsInteger(), static_cast<int>(fillStroke.stroke.width)};
   auto brush = wxBrush{fillStroke.fill.colour.AsInteger()};

   wxDCPenChanger   pc{*hdc, pen};
   wxDCBrushChanger bc{*hdc, brush};

   hdc->DrawRoundedRectangle(static_cast<int>(rc.left), static_cast<int>(rc.top), static_cast<int>(rc.Width()), static_cast<int>(rc.Height()), 4);
}

void jaeScintillaSurface::AlphaRectangle(PRectangle rc, XYPOSITION cornerSize, FillStroke fillStroke) {
   wxGCDC dc(*dynamic_cast<wxMemoryDC *>(hdc));

   auto pen   = wxPen{fillStroke.stroke.colour.AsInteger(), static_cast<int>(fillStroke.stroke.width)};
   auto brush = wxBrush{fillStroke.fill.colour.AsInteger()};

   wxDCPenChanger   pc{dc, pen};
   wxDCBrushChanger bc{dc, brush};

   hdc->DrawRoundedRectangle(static_cast<int>(rc.left), static_cast<int>(rc.top), static_cast<int>(rc.Width()), static_cast<int>(rc.Height()), cornerSize);
}

void jaeScintillaSurface::GradientRectangle(PRectangle rc, const std::vector<ColourStop> & /*stops*/, GradientOptions /*options*/) {
   FillStroke fillStroke{ColourRGBA{0xFF, 0xFF, 0xFF}, ColourRGBA{0xFF, 0xFF, 0xFF}};
   RectangleDraw(rc, fillStroke);
}

void jaeScintillaSurface::DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) {
   wxRect   r   = wxRectFromPRectangle(rc);
   wxBitmap bmp = BitmapFromRGBAImage(width, height, pixelsImage);
   hdc->DrawBitmap(bmp, r.x, r.y, true);
}

void jaeScintillaSurface::Ellipse(PRectangle rc, FillStroke fillStroke) {
   auto pen   = wxPen{fillStroke.stroke.colour.AsInteger(), static_cast<int>(fillStroke.stroke.width)};
   auto brush = wxBrush{fillStroke.fill.colour.AsInteger()};

   wxDCPenChanger   pc{*hdc, pen};
   wxDCBrushChanger bc{*hdc, brush};
   hdc->DrawEllipse(wxRectFromPRectangle(rc));
}

void jaeScintillaSurface::Stadium(PRectangle rc, FillStroke fillStroke, Ends ends) {}

void jaeScintillaSurface::Copy(PRectangle rc, Point from, Surface &surfaceSource) {
   wxRect r = wxRectFromPRectangle(rc);
   hdc->Blit(r.x, r.y, r.width, r.height, (dynamic_cast<jaeScintillaSurface &>(surfaceSource)).hdc, wxRound(from.x), wxRound(from.y), wxCOPY);
}

auto jaeScintillaSurface::Ascent(const Font *font_) -> XYPOSITION {
   SetFont(font_);
   int w = 0;
   int h = 0;
   int d = 0;
   int e = 0;
   hdc->GetTextExtent(EXTENT_TEST, &w, &h, &d, &e);
   const int ascent = h - d;
   SetAscent(font_, ascent);
   return ascent;
}

auto jaeScintillaSurface::Descent(const Font *font_) -> XYPOSITION {
   SetFont(font_);
   int w = 0;
   int h = 0;
   int d = 0;
   int e = 0;
   hdc->GetTextExtent(EXTENT_TEST, &w, &h, &d, &e);
   return d;
}

auto jaeScintillaSurface::InternalLeading(const Font * /*font_*/) -> XYPOSITION { return 0; }

auto jaeScintillaSurface::Height(const Font *font_) -> XYPOSITION {
   SetFont(font_);
   return hdc->GetCharHeight() + 1;
}

auto jaeScintillaSurface::AverageCharWidth(const Font *font_) -> XYPOSITION {
   SetFont(font_);
   return hdc->GetCharWidth();
}

void jaeScintillaSurface::SetClip(PRectangle rc) { hdc->SetClippingRegion(wxRectFromPRectangle(rc)); }
void jaeScintillaSurface::PopClip() {}
void jaeScintillaSurface::FlushCachedState() {}
void jaeScintillaSurface::FlushDrawing() {}

void jaeScintillaSurface::DrawTextNoClip(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) {
   SetFont(font_);
   hdc->SetTextForeground(wxColour{fore.GetRed(), fore.GetGreen(), fore.GetBlue()});
   hdc->SetTextBackground(wxColour{back.GetRed(), back.GetGreen(), back.GetBlue()});
   FillRectangle(rc, back);

   // ybase is where the baseline should be, but wxWin uses the upper left
   // corner, so I need to calculate the real position for the text...

   hdc->DrawText(wxString::FromUTF8(text.data(), text.length()), wxRound(rc.left), wxRound(ybase - GetAscent(font_)));
}

void jaeScintillaSurface::DrawTextClipped(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) {
   SetFont(font_);
   hdc->SetTextForeground(wxColour{fore.GetRed(), fore.GetGreen(), fore.GetBlue()});
   hdc->SetTextBackground(wxColour{back.GetRed(), back.GetGreen(), back.GetBlue()});
   FillRectangle(rc, back);
   hdc->SetClippingRegion(wxRectFromPRectangle(rc));

   // ybase is where the baseline should be, but wxWin uses the upper left
   // corner, so I need to calculate the real position for the text...

   hdc->DrawText(wxString::FromUTF8(text.data(), text.length()), wxRound(rc.left), wxRound(ybase - GetAscent(font_)));

   hdc->DestroyClippingRegion();
}

void jaeScintillaSurface::DrawTextTransparent(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore) {
   SetFont(font_);
   hdc->SetTextForeground(wxColour{fore.GetRed(), fore.GetGreen(), fore.GetBlue()});
   hdc->SetBackgroundMode(wxBRUSHSTYLE_TRANSPARENT);

   // ybase is where the baseline should be, but wxWin uses the upper left
   // corner, so I need to calculate the real position for the text...
   hdc->DrawText(wxString::FromUTF8(text.data(), text.length()), wxRound(rc.left), wxRound(ybase - GetAscent(font_)));

   hdc->SetBackgroundMode(wxBRUSHSTYLE_SOLID);
}

void jaeScintillaSurface::MeasureWidths(const Font *font_, std::string_view text, XYPOSITION *positions) {
   wxString   str = wxString::FromUTF8(text.data(), text.length());
   wxArrayInt tpos;

   SetFont(font_);

   hdc->GetPartialTextExtents(str, tpos);

#if wxUSE_UNICODE
   // Map the widths back to the UTF-8 input string
   size_t utf8i = 0;
   for (size_t wxi = 0; wxi < str.size(); ++wxi) {
      wxUniChar c = str[wxi];

#if SIZEOF_WCHAR_T == 2
      // For surrogate pairs, the position for the lead surrogate is garbage
      // and we need to use the position of the trail surrogate for all four bytes
      if (c >= 0xD800 && c < 0xE000 && wxi + 1 < str.size()) {
         ++wxi;
         positions[utf8i++] = tpos[wxi];
         positions[utf8i++] = tpos[wxi];
         positions[utf8i++] = tpos[wxi];
         positions[utf8i++] = tpos[wxi];
         continue;
      }
#endif

      positions[utf8i++] = tpos[wxi];
      if (c >= 0x80) {
         positions[utf8i++] = tpos[wxi];
      }
      if (c >= 0x800) {
         positions[utf8i++] = tpos[wxi];
      }
      if (c >= 0x10000) {
         positions[utf8i++] = tpos[wxi];
      }
   }
#else  // !wxUSE_UNICODE
   // If not unicode then just use the widths we have
   for (int i = 0; i < len; i++) {
      positions[i] = tpos[i];
   }
#endif // wxUSE_UNICODE/!wxUSE_UNICODE
}

auto jaeScintillaSurface::WidthText(const Font *font_, std::string_view text) -> XYPOSITION {
   SetFont(font_);
   int w = 0;
   int h = 0;

   hdc->GetTextExtent(wxString::FromUTF8(text.data(), text.length()), &w, &h);
   return w;
}

void jaeScintillaSurface::DrawTextNoClipUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) {
   DrawTextNoClip(rc, font_, ybase, text, fore, back);
}

void jaeScintillaSurface::DrawTextClippedUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) {
   DrawTextClipped(rc, font_, ybase, text, fore, back);
}

void jaeScintillaSurface::DrawTextTransparentUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore) { DrawTextTransparent(rc, font_, ybase, text, fore); }

void jaeScintillaSurface::MeasureWidthsUTF8(const Font *font_, std::string_view text, XYPOSITION *positions) { MeasureWidths(font_, text, positions); }

auto jaeScintillaSurface::WidthTextUTF8(const Font *font_, std::string_view text) -> XYPOSITION { return WidthText(font_, text); }

auto jaeScintillaSurface::Layout(const IScreenLine * /*screenLine*/) -> std::unique_ptr<IScreenLineLayout> { return {}; }

void jaeScintillaSurface::SetFont(const Font *font_) { hdc->SetFont(((jaeScintillaFont *)font_)->getFont()); }

void jaeScintillaSurface::SetAscent(const Font *font_, int ascent) { ((jaeScintillaFont *)font_)->ascent(ascent); }

int jaeScintillaSurface::GetAscent(const Font *font_) { return ((jaeScintillaFont *)font_)->ascent(); }
