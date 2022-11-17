#pragma once

#include "wx/dcclient.h"
#include "wx/sizer.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "Geometry.h"
#include "Sci_Position.h"
#include "ScintillaTypes.h"
#include "ScintillaTypes.h"
#include "Platform.h"

using namespace Scintilla::Internal;

class jaeScintillaSurface : public Surface {
 private:
   WindowID    _wid        = nullptr;
   wxDC *      hdc         = nullptr;
   bool        hdcOwned    = false;
   int         x           = 0;
   int         y           = 0;
   bool        unicodeMode = false;
   SurfaceMode _mode;

 public:
   jaeScintillaSurface();
   jaeScintillaSurface(int width, int height);
   ~jaeScintillaSurface() override;

   // Deleted so SurfaceImpl objects can not be copied.
   jaeScintillaSurface(const jaeScintillaSurface &) = delete;
   jaeScintillaSurface(jaeScintillaSurface &&)      = delete;
   auto operator=(const jaeScintillaSurface &) -> jaeScintillaSurface & = delete;
   auto operator=(jaeScintillaSurface &&) -> jaeScintillaSurface & = delete;

   void Init(WindowID wid) override;
   void Init(SurfaceID sid, WindowID wid) override;
   auto AllocatePixMap(int width, int height) -> std::unique_ptr<Surface> override;
   auto SupportsFeature(Scintilla::Supports feature) noexcept -> int override;
   void SetMode(SurfaceMode mode_) override;
   void Release() noexcept override;
   auto Initialised() -> bool override;
   auto LogPixelsY() -> int override;
   auto PixelDivisions() -> int override { return 1; }
   auto DeviceHeightFont(int points) -> int override { return points; }
   void LineDraw(Point start, Point end, Stroke stroke) override;
   void PolyLine(const Point *pts, size_t npts, Stroke stroke) override;
   void Polygon(const Point *pts, size_t npts, FillStroke fillStroke) override;
   void RectangleDraw(PRectangle rc, FillStroke fillStroke) override;
   void RectangleFrame(PRectangle rc, Stroke stroke) override;
   void FillRectangle(PRectangle rc, Fill fill) override;
   void FillRectangleAligned(PRectangle rc, Fill fill) override { FillRectangle(PixelAlign(rc, 1), fill); }
   void FillRectangle(PRectangle rc, Surface &surfacePattern) override;
   void RoundedRectangle(PRectangle rc, FillStroke fillStroke) override;
   void AlphaRectangle(PRectangle rc, XYPOSITION cornerSize, FillStroke fillStroke) override;
   void GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops, GradientOptions options) override;
   void DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) override;
   void Ellipse(PRectangle rc, FillStroke fillStroke) override;
   void Stadium(PRectangle rc, FillStroke fillStroke, Ends ends) override;
   void Copy(PRectangle rc, Point from, Surface &surfaceSource) override;

   auto Layout(const IScreenLine *screenLine) -> std::unique_ptr<IScreenLineLayout> override;

   auto Ascent(const Font *font_) -> XYPOSITION override;
   auto Descent(const Font *font_) -> XYPOSITION override;
   auto InternalLeading(const Font *font_) -> XYPOSITION override;
   auto Height(const Font *font_) -> XYPOSITION override;
   auto AverageCharWidth(const Font *font_) -> XYPOSITION override;

   void SetClip(PRectangle rc) override;
   void PopClip() override;
   void FlushCachedState() override;
   void FlushDrawing() override;

   void DrawTextNoClip(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) override;
   void DrawTextClipped(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) override;
   void DrawTextTransparent(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore) override;
   void MeasureWidths(const Font *font_, std::string_view text, XYPOSITION *positions) override;
   auto WidthText(const Font *font_, std::string_view text) -> XYPOSITION override;

   void DrawTextNoClipUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) override;
   void DrawTextClippedUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) override;
   void DrawTextTransparentUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore) override;
   void MeasureWidthsUTF8(const Font *font_, std::string_view text, XYPOSITION *positions) override;
   auto WidthTextUTF8(const Font *font_, std::string_view text) -> XYPOSITION override;

   void SetFont(const Font *font_);
   void SetAscent(const Font *font_, int ascent);
   auto GetAscent(const Font *font_) -> int;

#if 0
   virtual void PenColour(ColourDesired fore) wxOVERRIDE;
   virtual void MoveTo(int x_, int y_) wxOVERRIDE;
   virtual void LineTo(int x_, int y_) wxOVERRIDE;
   virtual void RectangleDraw(PRectangle rc, ColourDesired fore, ColourDesired back) wxOVERRIDE;
   virtual void FillRectangle(PRectangle rc, ColourDesired back) wxOVERRIDE;
   virtual void FillRectangle(PRectangle rc, Surface &surfacePattern) wxOVERRIDE;
   virtual void RoundedRectangle(PRectangle rc, ColourDesired fore, ColourDesired back) wxOVERRIDE;
   virtual void AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill, int alphaFill, ColourDesired outline, int alphaOutline, int flags) wxOVERRIDE;
   virtual void DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) wxOVERRIDE;
   virtual void Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back) wxOVERRIDE;
   virtual void Copy(PRectangle rc, Point from, Surface &surfaceSource) wxOVERRIDE;

   virtual void       DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore, ColourDesired back) wxOVERRIDE;
   virtual void       DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore, ColourDesired back) wxOVERRIDE;
   virtual void       DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore) wxOVERRIDE;
   virtual void       MeasureWidths(Font &font_, const char *s, int len, XYPOSITION *positions) wxOVERRIDE;
   virtual XYPOSITION WidthText(Font &font_, const char *s, int len) wxOVERRIDE;
   virtual XYPOSITION WidthChar(Font &font_, char ch) wxOVERRIDE;
   virtual XYPOSITION Ascent(Font &font_) wxOVERRIDE;
   virtual XYPOSITION Descent(Font &font_) wxOVERRIDE;
   virtual XYPOSITION InternalLeading(Font &font_) wxOVERRIDE;
   virtual XYPOSITION ExternalLeading(Font &font_) wxOVERRIDE;
   virtual XYPOSITION Height(Font &font_) wxOVERRIDE;
   virtual XYPOSITION AverageCharWidth(Font &font_) wxOVERRIDE;

   virtual void SetClip(PRectangle rc) wxOVERRIDE;
   virtual void FlushCachedState() wxOVERRIDE;

   virtual void SetUnicodeMode(bool unicodeMode_) wxOVERRIDE;
   virtual void SetDBCSMode(int codePage) wxOVERRIDE;

   void BrushColour(ColourDesired back);
   void SetFont(Font &font_);
#endif
};
