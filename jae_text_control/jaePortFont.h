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
#include "ScintillaTypes.h"
#include "Platform.h"

#include <wx/font.h>

class jaeScintillaFont : public Scintilla::Internal::Font {
 public:
   jaeScintillaFont() noexcept = default;
   explicit jaeScintillaFont(const Scintilla::Internal::FontParameters &fp);

   jaeScintillaFont(const Font &) = delete;
   jaeScintillaFont(Font &&)      = delete;
   auto operator=(const Font &) -> jaeScintillaFont & = delete;
   auto operator=(Font &&) -> jaeScintillaFont & = delete;

   auto getFont() -> wxFont & { return _font; }

   void ascent(int ascent) { m_ascent = ascent; }
   auto ascent() const -> int { return m_ascent; }

 private:
   wxFont _font;
   int    m_ascent = 0;
};
