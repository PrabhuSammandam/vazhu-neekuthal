#pragma once

//#include <cstddef>
//#include <cstdlib>
//#include <cstring>
//#include <cstdio>
//#include <cmath>
//#include <string>
#include <string_view>
//#include <vector>
//#include <map>
//#include <optional>
//#include <algorithm>
//#include <memory>
//#include <sstream>
//#include <cstdint>
#include "Geometry.h"
#include "wx/gdicmn.h"

auto wxRectFromPRectangle(Scintilla::Internal::PRectangle prc) -> wxRect;
auto PRectangleFromwxRect(wxRect rc) -> Scintilla::Internal::PRectangle;
auto BitmapFromRGBAImage(int width, int height, const unsigned char *pixelsImage) -> wxBitmap;
