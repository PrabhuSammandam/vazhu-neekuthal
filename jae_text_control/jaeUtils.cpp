#include "jaeUtils.h"
#include "wx/rawbmp.h"
#include <wx/bitmap.h>

using Scintilla::Internal::PRectangle;

using namespace Scintilla::Internal;

auto wxRectFromPRectangle(PRectangle prc) -> wxRect {
   wxRect r(wxRound(prc.left), wxRound(prc.top), wxRound(prc.Width()), wxRound(prc.Height()));
   return r;
}

auto PRectangleFromwxRect(wxRect rc) -> PRectangle { return PRectangle(rc.GetLeft(), rc.GetTop(), rc.GetRight() + 1, rc.GetBottom() + 1); }

#if defined(__WXMSW__) || defined(__WXMAC__)
#define wxPy_premultiply(p, a) ((p) * (a) / 0xff)
#else
#define wxPy_premultiply(p, a) (p)
#endif

#ifdef wxHAS_RAW_BITMAP
wxBitmap BitmapFromRGBAImage(int width, int height, const unsigned char *pixelsImage) {
   int              x = 0;
   int              y = 0;
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
   const int                    totalPixels = width * height;
   wxScopedArray<unsigned char> data(3 * totalPixels);
   wxScopedArray<unsigned char> alpha(totalPixels);
   int                          curDataLocation = 0, curAlphaLocation = 0, curPixelsImageLocation = 0;

   for (int i = 0; i < totalPixels; ++i) {
      data[curDataLocation++]   = pixelsImage[curPixelsImageLocation++];
      data[curDataLocation++]   = pixelsImage[curPixelsImageLocation++];
      data[curDataLocation++]   = pixelsImage[curPixelsImageLocation++];
      alpha[curAlphaLocation++] = pixelsImage[curPixelsImageLocation++];
   }

   wxImage  img(width, height, data.get(), alpha.get(), true);
   wxBitmap bmp(img);

   return bmp;
}
#endif
