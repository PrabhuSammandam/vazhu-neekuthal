#include "jaePortFont.h"
#include "wx/encconv.h"
#include "wx/font.h"

using namespace Scintilla::Internal;

jaeScintillaFont::jaeScintillaFont(const Scintilla::Internal::FontParameters &fp) {
   // The minus one is done because since Scintilla uses SC_CHARSET_DEFAULT
   // internally and we need to have wxFONENCODING_DEFAULT == SC_SHARSET_DEFAULT
   // so we adjust the encoding before passing it to Scintilla.  See also
   // wxStyledTextCtrl::StyleSetCharacterSet
   int  characterSet = static_cast<int>(fp.characterSet);
   auto encoding     = (wxFontEncoding)(characterSet - 1);

   wxFontEncodingArray ea = wxEncodingConverter::GetPlatformEquivalents(encoding);
   if (ea.GetCount() != 0U) {
      encoding = ea[0];
   }

   wxFontWeight weight = wxFONTWEIGHT_LIGHT;
   switch (fp.weight) {
   case Scintilla::FontWeight::Normal:
      weight = wxFONTWEIGHT_NORMAL;
      break;
   case Scintilla::FontWeight::SemiBold:
      weight = wxFONTWEIGHT_LIGHT;
      break;
   case Scintilla::FontWeight::Bold:
      weight = wxFONTWEIGHT_BOLD;
      break;
   }

   _font = wxFont(wxRound(fp.size), wxFONTFAMILY_DEFAULT, fp.italic ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL, weight, false, wxString::FromUTF8(fp.faceName), encoding);
}

auto Font::Allocate(const FontParameters &fp) -> std::shared_ptr<Font> { return std::make_shared<jaeScintillaFont>(fp); }
