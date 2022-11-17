#include "jaeCppTextDocument.h"
#include "Lexilla.h"
#include "SciLexer.h"
#include "jae_text_control/jaeTextEditControlEvent.h"
#include "wx/colour.h"
#include "wx/convauto.h"
#include "wx/event.h"
#include "wx/ffile.h"
#include "wx/filename.h"
#include "wx/gdicmn.h"
#include "wx/gtk/bitmap.h"
#include "wx/menu.h"
#include "wx/scopedarray.h"
#include "wx/string.h"
#include "wx/wfstream.h"
#include <cstdint>
#include <exception>
#include <string>
#include <wx/strconv.h>
#include <wx/stdpaths.h>
#include "ID.h"
#include "MainFrame.h"
//#include "nlohmann/json.hpp"
//#include "yaml-cpp/yaml.h"

using std::exception;
// using YAML::Load;

enum { MARKER_BREAKPOINT = 0, MARKER_CURRENT_EXECUTION_LINE };

jaeCppTextDocument::jaeCppTextDocument(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style) : jaeTextDocument{parent, id, pos, size, style} {
   _mainFrame  = dynamic_cast<MainFrame *>(parent);
   auto *lexer = CreateLexer("cpp");

   if (lexer != nullptr) {
      lexer->PropertySet("lexer.cpp.track.preprocessor", "0");
      lexer->PropertySet("fold", "1");
      lexer->PropertySet("fold.comment", "1");
      SetILexer((void *)lexer);
   }
#if 1
   StyleResetDefault();
   StyleClearAll();

   auto lineNoWidth = TextWidth(SC_MARGIN_NUMBER, "_9999999");
   SetMarginWidthN(0, lineNoWidth);
   SetMarginTypeN(0, Scintilla::MarginType::Number);

   SetMarginWidthN(1, 20);
   SetMarginTypeN(1, Scintilla::MarginType::Symbol);
   SetMarginSensitiveN(1, true);

   MarkerDefine(MARKER_CURRENT_EXECUTION_LINE, Scintilla::MarkerSymbol::ShortArrow);

   auto       userDataDir = wxStandardPaths::Get().GetExecutablePath();
   wxFileName fn{userDataDir};

   wxBitmap bmp;
   bmp.LoadFile(fn.GetPath() + "/resource/" + "16-breakpoint.png", wxBITMAP_TYPE_PNG);

   if (bmp.IsOk()) {
      const int                    totalPixels = bmp.GetWidth() * bmp.GetHeight();
      wxScopedArray<unsigned char> rgba(4 * bmp.GetWidth() * bmp.GetHeight());
      wxImage                      img         = bmp.ConvertToImage();
      int                          curRGBALoc  = 0;
      int                          curDataLoc  = 0;
      int                          curAlphaLoc = 0;

      if (img.HasMask()) {
         for (int y = 0; y < bmp.GetHeight(); ++y) {
            for (int x = 0; x < bmp.GetWidth(); ++x) {
               rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
               rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
               rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
               rgba[curRGBALoc++] = img.IsTransparent(x, y) ? wxALPHA_TRANSPARENT : wxALPHA_OPAQUE;
            }
         }
      } else if (img.HasAlpha()) {
         for (int i = 0; i < totalPixels; ++i) {
            rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
            rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
            rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
            rgba[curRGBALoc++] = img.GetAlpha()[curAlphaLoc++];
         }
      } else {
         for (int i = 0; i < totalPixels; ++i) {
            rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
            rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
            rgba[curRGBALoc++] = img.GetData()[curDataLoc++];
            rgba[curRGBALoc++] = wxALPHA_OPAQUE;
         }
      }
      MarkerDefine(MARKER_BREAKPOINT, Scintilla::MarkerSymbol::RgbaImage);
      RGBAImageSetWidth(bmp.GetWidth());
      RGBAImageSetHeight(bmp.GetHeight());
      MarkerDefineRGBAImage(MARKER_BREAKPOINT, (const char *)rgba.get());
   }

   // MarkerSetBack(MARKER_BREAKPOINT, 0x0000FF);
   // MarkerSetFore(MARKER_BREAKPOINT, 0x0000FF);
   // MarkerDefine(MARKER_BREAKPOINT, Scintilla::MarkerSymbol::Circle);

   MarkerDefine(SC_MARKNUM_FOLDER, Scintilla::MarkerSymbol::Plus);
   MarkerDefine(SC_MARKNUM_FOLDEROPEN, Scintilla::MarkerSymbol::Minus);
   MarkerDefine(SC_MARKNUM_FOLDEREND, Scintilla::MarkerSymbol::Empty);
   MarkerDefine(SC_MARKNUM_FOLDERMIDTAIL, Scintilla::MarkerSymbol::Empty);
   MarkerDefine(SC_MARKNUM_FOLDEROPENMID, Scintilla::MarkerSymbol::Empty);
   MarkerDefine(SC_MARKNUM_FOLDERSUB, Scintilla::MarkerSymbol::Empty);
   MarkerDefine(SC_MARKNUM_FOLDERTAIL, Scintilla::MarkerSymbol::Empty);

   SetFoldFlags(Scintilla::FoldFlag::LineAfterContracted);

   Bind(wxEVT_JAE_TEXT_EDIT_CONTROL_MARGINCLICK, &jaeCppTextDocument::OnMarginClick, this);
   Bind(wxEVT_JAE_TEXT_EDIT_CONTROL_MARGIN_RIGHT_CLICK, &jaeCppTextDocument::OnMarginRightClick, this);

   UsePopUp(Scintilla::PopUp::Never);
   StyleSetVisible(STYLE_INDENTGUIDE, true);
   StyleSetFore(STYLE_INDENTGUIDE, 0x00ff00);
   SetIndentationGuides(Scintilla::IndentView::Real);
   SetCodePage(SC_CP_UTF8);
   SetCharsDefault();
   SetCaretLineVisibleAlways(true);

#if 0
   wxFFile file("/home/psammandam/disk/projects/wxwidgets/jae/properties/cpp.yaml", wxS("rb"));
   if (file.IsOpened()) {
      wxString text;

      if (file.ReadAll(&text)) {
         YAML::Node config = Load(text.ToUTF8());
         if (config["styles"]) {
            std::cout << "styles found" << std::endl;

            for (auto s : config["styles"]) {
               auto styleNo = -1;

               if (s["no"]) {
                  styleNo = s["no"].as<int>();
               }
               if (styleNo == -1) {
                  continue;
               }
               if (s["fore"]) {
                  auto     foreColor = s["fore"].as<std::string>();
                  wxColour c{"#" + foreColor};
                  StyleSetFore(styleNo, c.GetRGBA());
               }
               if (s["back"]) {
                  auto     backColor = s["back"].as<std::string>();
                  wxColour c{"#" + backColor};
                  StyleSetBack(styleNo, c.GetRGBA());
               }
               if (s["font"]) {
                  auto font = s["font"];

                  if (font["name"]) {
                     auto fontName = font["name"].as<std::string>();
                     StyleSetFont(styleNo, wxString{fontName});
                  }
                  if (font["size"]) {
                     auto fontSize = font["size"].as<double>();
                     StyleSetSizeFractional(styleNo, static_cast<int>(fontSize * 100));
                  }
               }
            }
         }

         if (config["primary_keywords"]) {
            auto kw = config["primary_keywords"].as<std::string>();
            SetKeyWords(0, kw.c_str());
         }
         if (config["preprocessor_keywords"]) {
            auto kw = config["preprocessor_keywords"].as<std::string>();
            SetKeyWords(4, kw.c_str());
         }
         if (config["marker_keywords"]) {
            auto kw = config["marker_keywords"].as<std::string>();
            SetKeyWords(5, kw.c_str());
         }
      }
   }
#endif
#endif
}

jaeCppTextDocument::~jaeCppTextDocument() {}

void jaeCppTextDocument::DoContextMenu(wxContextMenuEvent &evt) {
   wxPoint pt = evt.GetPosition();
   ScreenToClient(&pt.x, &pt.y);
   /*
     Show context menu at event point if it's within the window,
     or at caret location if not
   */
   wxHitTest ht = this->HitTest(pt);
   if (ht != wxHT_WINDOW_INSIDE) {
      auto pos = CurrentPos();
      auto x   = PointXFromPosition(pos);
      auto y   = PointYFromPosition(pos);
      pt       = wxPoint(static_cast<int>(x), static_cast<int>(y));
   }

   wxMenu popup;

   popup.Append(CODE_VIEW_CONTEXT_MENU_TOGGLE_BREAKPOINT, "Toggle Breakpoint");
   popup.Append(CODE_VIEW_CONTEXT_MENU_RUN_TO_LINE, "Run To Line");
   popup.Append(wxID_COPY, "Copy");

   PopupMenu(&popup, pt);
   evt.Skip();
}

void jaeCppTextDocument::OnMarginClick(jaeTextEditControlEvent &event) {
   auto lineNo = LineFromPosition(event.GetPosition());

   switch (event.GetMargin()) {
   case 1: {
      if (_mainFrame != nullptr) {
         _mainFrame->getGdbMgr()->ToggleBreakpoint(getFilePath(), (int)lineNo + 1);
      }
   } break;
   }
}

void jaeCppTextDocument::OnMarginRightClick(jaeTextEditControlEvent &event) { event.Skip(); }

auto jaeCppTextDocument::LoadFile(const wxString &filename) -> bool {
   // load file in edit and clear undo
   if (!filename.empty()) {
      _filePath = filename;
   }

   wxFFile file(filename, wxS("rb"));

   if (file.IsOpened()) {
      wxString text;
      if (file.ReadAll(&text, wxConvAuto())) {
         auto                      utf8  = text.ToUTF8();
         const wxString::size_type posLF = text.find('\n');

         if (posLF != wxString::npos) {
            // Set EOL mode to ensure that the new lines inserted into the
            // text use the same EOLs as the existing ones.
            if (posLF > 0 && text[posLF - 1] == '\r') {
               SetEOLMode(Scintilla::EndOfLine::CrLf);
            } else {
               SetEOLMode(Scintilla::EndOfLine::Lf);
            }
         }
         SetText(utf8);
         EmptyUndoBuffer();
         SetSavePoint();
         SetReadOnly(true);

         auto       filePath = getFilePath();
         wxFileName fn{filePath};
         auto       extension = fn.GetExt();

         if (extension == "c" || extension == "cpp" || extension == "h" || extension == "cxx" || extension == "cc") {
            int      fontSize = 10;
            wxString fontName = "Noto Sans Mono";
            // default
            StyleSetFore(0, wxColor{"#000000"}.GetRGBA());
            StyleSetFont(0, fontName);
            StyleSetSizeFractional(0, static_cast<int>(fontSize * 100));
            // comment
            StyleSetFore(1, wxColor{"#0000FF"}.GetRGBA());
            StyleSetFont(1, fontName);
            StyleSetSizeFractional(1, static_cast<int>(fontSize * 100));
            // commentLine
            StyleSetFore(2, wxColor{"#0000FF"}.GetRGBA());
            StyleSetFont(2, fontName);
            StyleSetSizeFractional(2, static_cast<int>(fontSize * 100));
            // commentDoc
            StyleSetFore(3, wxColor{"#0000FF"}.GetRGBA());
            StyleSetFont(3, fontName);
            StyleSetSizeFractional(3, static_cast<int>(fontSize * 100));
            // number
            StyleSetFore(4, wxColor{"#FF0000"}.GetRGBA());
            StyleSetFont(4, fontName);
            StyleSetSizeFractional(4, static_cast<int>(fontSize * 100));
            // primary key word
            StyleSetFore(5, wxColor{"#630a59"}.GetRGBA());
            StyleSetFont(5, fontName);
            StyleSetSizeFractional(5, static_cast<int>(fontSize * 100));
            // string
            StyleSetFore(6, wxColor{"#000000"}.GetRGBA());
            StyleSetBack(6, wxColor{"#FFFF00"}.GetRGBA());
            StyleSetFont(6, fontName);
            StyleSetSizeFractional(6, static_cast<int>(fontSize * 100));
            // character
            StyleSetFont(7, fontName);
            StyleSetSizeFractional(7, static_cast<int>(fontSize * 100));
            // uuid
            StyleSetFont(8, fontName);
            StyleSetSizeFractional(8, static_cast<int>(fontSize * 100));
            // pre-processor
            StyleSetFore(9, wxColor{"#8a1203"}.GetRGBA());
            StyleSetFont(9, fontName);
            StyleSetSizeFractional(9, static_cast<int>(fontSize * 100));
            // operator
            StyleSetFont(10, fontName);
            StyleSetSizeFractional(10, static_cast<int>(fontSize * 100));
            // identifier
            StyleSetFore(11, wxColor{"#107505"}.GetRGBA());
            StyleSetFont(11, fontName);
            StyleSetSizeFractional(11, static_cast<int>(fontSize * 100));

            // primary keywords
            SetKeyWords(0, "alignas alignof and and_eq asm audit auto axiom bitand bitor bool break \
case catch char char8_t char16_t char32_t class compl concept \
const consteval constexpr const_cast continue co_await co_return co_yield \
decltype default delete do double dynamic_cast else enum explicit export extern false final float for \
friend goto if import inline int long module mutable namespace new noexcept not not_eq nullptr \
operator or or_eq override private protected public \
register reinterpret_cast requires return \
short signed sizeof static static_assert static_cast struct switch \
template this thread_local throw true try typedef typeid typename union unsigned using \
virtual void volatile wchar_t while xor xor_eq");

            // preprocessor-keywords
            SetKeyWords(4, "assert define else elseif endif  endinput  error  file \
include line pragma section tryinclude undef defined sizeof state tagof");

            // marker keywords
            SetKeyWords(5, "FIXME TODO HACK UNDONE");
         }
         return true;
      }
   }

   return true;
}

void jaeCppTextDocument::gotoLineAndEnsureVisible(int lineno) {
   auto noOfLines = LinesOnScreen();
   auto start     = lineno - noOfLines / 2;
   auto end       = lineno + noOfLines / 2;
   auto startPos  = PositionFromLine(start);
   auto endPos    = PositionFromLine(end);

   ScrollRange(startPos, endPos);
   GotoLine(lineno);
   MarkerDeleteAll(MARKER_CURRENT_EXECUTION_LINE);
   MarkerAdd(lineno, MARKER_CURRENT_EXECUTION_LINE);
}

void jaeCppTextDocument::disableLine() { MarkerDeleteAll(0); }

void jaeCppTextDocument::setBreakpoints(std::vector<int> &breakpointLineNoList) {
   MarkerDeleteAll(MARKER_BREAKPOINT);

   for (auto lineNo : breakpointLineNoList) {
      MarkerAdd(lineNo - 1, MARKER_BREAKPOINT);
   }
}

void jaeCppTextDocument::deleteAllMarkers() {
   MarkerDeleteAll(MARKER_BREAKPOINT);
   MarkerDeleteAll(MARKER_CURRENT_EXECUTION_LINE);
}
