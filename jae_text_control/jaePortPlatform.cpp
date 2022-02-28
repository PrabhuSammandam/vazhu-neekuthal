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
#include "Debugging.h"
#include "Platform.h"

#include "wx/display.h"
#include "wx/gdicmn.h"
#include "wx/settings.h"
#include "wx/window.h"
#include <wx/menu.h>

using namespace Scintilla::Internal;

auto Platform::Chrome() -> ColourRGBA {
   wxColour c;
   c = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
   return ColourRGBA(c.Red(), c.Green(), c.Blue());
}

auto Platform::ChromeHighlight() -> ColourRGBA {
   wxColour c;
   c = wxSystemSettings::GetColour(wxSYS_COLOUR_3DHIGHLIGHT);
   return ColourRGBA(c.Red(), c.Green(), c.Blue());
}

auto Platform::DefaultFont() -> const char * {
   static char buf[128];
   wxStrlcpy(buf, wxNORMAL_FONT->GetFaceName().mbc_str(), WXSIZEOF(buf));
   return buf;
}

auto Platform::DefaultFontSize() -> int { return wxNORMAL_FONT->GetPointSize(); }

auto Platform::DoubleClickTime() -> unsigned int {
   return 500; // **** ::GetDoubleClickTime();
}

void Platform::Assert(const char *c, const char *file, int line) noexcept {
#ifdef TRACE
   char buffer[2000];
   sprintf(buffer, "Assertion [%s] failed at %s %d", c, file, line);
   if (assertionPopUps) {
      /*int idButton = */
      wxMessageBox(stc2wx(buffer), wxT("Assertion failure"), wxICON_HAND | wxOK);
   } else {
      strcat(buffer, "\r\n");
      Platform::DebugDisplay(buffer);
      abort();
   }
#else
   wxUnusedVar(c);
   wxUnusedVar(file);
   wxUnusedVar(line);
#endif
}

void Platform::DebugPrintf(const char *format, ...) noexcept {
#ifdef TRACE
   char    buffer[2000];
   va_list pArguments;
   va_start(pArguments, format);
   vsprintf(buffer, format, pArguments);
   va_end(pArguments);
   Platform::DebugDisplay(buffer);
#else
   wxUnusedVar(format);
#endif
}

void Platform::DebugDisplay(const char *s) noexcept {
#ifdef TRACE
   wxLogDebug(stc2wx(s));
#else
   wxUnusedVar(s);
#endif
}
