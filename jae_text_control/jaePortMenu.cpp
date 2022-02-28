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

#include "wx/menu.h"
#include "wx/window.h"

using namespace Scintilla::Internal;

Menu::Menu() noexcept : mid(nullptr) {}

void Menu::CreatePopUp() {
   Destroy();
   mid = new wxMenu();
}

void Menu::Destroy() noexcept {
   if (mid != nullptr) {
      delete static_cast<wxMenu *>(mid);
   }
   mid = nullptr;
}

void Menu::Show(Point pt, const Window &w) {
   auto *window = static_cast<wxWindow *>(w.GetID());

   window->PopupMenu(static_cast<wxMenu *>(mid), wxRound(pt.x - 4), wxRound(pt.y));
   Destroy();
}
