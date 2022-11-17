#pragma once

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

using namespace std;

#include "Geometry.h"
#include "ScintillaTypes.h"
#include "Platform.h"

#include "jaeListBox.h"

using namespace Scintilla::Internal;

class ListBoxImpl : public Scintilla::Internal::ListBox {
 private:
   jaePortListBox *          m_listBox;
   jaePortListBoxVisualData *m_visualData;

 public:
   ListBoxImpl();
   ~ListBoxImpl() override;

   void SetFont(const Font *font) override;
   void Create(Window &parent, int ctrlID, Point location_, int lineHeight_, bool unicodeMode_, Scintilla::Technology technology_) override;
   void SetAverageCharWidth(int width) override;
   void SetVisibleRows(int rows) override;
   auto GetVisibleRows() const -> int override;
   auto GetDesiredRect() -> PRectangle override;
   auto CaretFromEdge() -> int override;
   void Clear() noexcept override;
   void Append(char *s, int type = -1) override;
   auto Length() -> int override;
   void Select(int n) override;
   auto GetSelection() -> int override;
   auto Find(const char *prefix) -> int override;
   auto GetValue(int n) -> std::string override;
   void RegisterImage(int type, const char *xpm_data) override;
   void RegisterImageHelper(int type, const wxBitmap &bmp);
   void RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage) override;
   void ClearRegisteredImages() override;
   void SetList(const char *list, char separator, char typesep) override;
   void SetListInfo(int *, int *, int *);
   void SetDelegate(IListBoxDelegate *lbDelegate) override;
   void SetOptions(ListOptions options_) override;
   void SetDoubleClickAction(CallBackAction, void *);
};
