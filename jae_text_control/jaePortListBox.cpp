#include "jaePortListBox.h"
#include "jaeListBox.h"
#include "jaeListBoxWindow.h"
#include "wx/window.h"
#include <string>

using std::make_unique;

using namespace Scintilla::Internal;

ListBoxImpl::ListBoxImpl() : m_listBox(nullptr), m_visualData(new jaePortListBoxVisualData(5)) {}

ListBoxImpl::~ListBoxImpl() { delete m_visualData; }

void ListBoxImpl::SetFont(const Font *font) { m_listBox->SetListBoxFont(font); }

void ListBoxImpl::Create(Window &parent, int /*ctrlID*/, Point /*location_*/, int lineHeight_, bool /*unicodeMode_*/, Scintilla::Technology technology_) {
   wid = new jaeListBoxWindow((wxWindow *)parent.GetID(), &m_listBox, m_visualData, lineHeight_, (int)technology_);
}

void ListBoxImpl::SetAverageCharWidth(int width) { m_listBox->SetAverageCharWidth(width); }

void ListBoxImpl::SetVisibleRows(int rows) { m_visualData->SetDesiredVisibleRows(rows); }

auto ListBoxImpl::GetVisibleRows() const -> int { return m_visualData->GetDesiredVisibleRows(); }

auto ListBoxImpl::GetDesiredRect() -> PRectangle { return m_listBox->GetDesiredRect(); }

auto ListBoxImpl::CaretFromEdge() -> int { return m_listBox->CaretFromEdge(); }

void ListBoxImpl::Clear() noexcept { m_listBox->Clear(); }

void ListBoxImpl::Append(char *s, int type) { m_listBox->Append(s, type); }

void ListBoxImpl::SetList(const char *list, char separator, char typesep) { m_listBox->SetList(list, separator, typesep); }

auto ListBoxImpl::Length() -> int { return m_listBox->Length(); }

void ListBoxImpl::Select(int n) { m_listBox->Select(n); }

auto ListBoxImpl::GetSelection() -> int { return m_listBox->GetSelection(); }

auto ListBoxImpl::Find(const char *WXUNUSED(prefix) /*prefix*/) -> int {
   // No longer used
   return wxNOT_FOUND;
}

auto ListBoxImpl::GetValue(int n) -> std::string { return m_listBox->GetValue(n); }

void ListBoxImpl::RegisterImageHelper(int type, const wxBitmap &bmp) { m_visualData->RegisterImage(type, bmp); }

void ListBoxImpl::RegisterImage(int type, const char *xpm_data) { m_visualData->RegisterImage(type, xpm_data); }

void ListBoxImpl::RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage) { m_visualData->RegisterRGBAImage(type, width, height, pixelsImage); }

void ListBoxImpl::ClearRegisteredImages() { m_visualData->ClearRegisteredImages(); }

void ListBoxImpl::SetDoubleClickAction(CallBackAction action, void *data) { m_listBox->SetDoubleClickAction(action, data); }

void ListBoxImpl::SetListInfo(int *listType, int *posStart, int *startLen) { m_visualData->SetSciListData(listType, posStart, startLen); }

//----------------------------------------------------------------------

ListBox::ListBox() noexcept = default;

ListBox::~ListBox() = default;

auto ListBox::Allocate() -> std::unique_ptr<ListBox> { return make_unique<ListBoxImpl>(); }

void ListBoxImpl::SetDelegate(IListBoxDelegate *lbDelegate) { // CHECK
}

void ListBoxImpl::SetOptions(ListOptions options_) { // CHECK
}
