#pragma once

#include "wx/control.h"
#include "wx/string.h"
#include "wx/textctrl.h"
#include <cstdint>

class jaeTextEditControl : public wxControl {

 public:
   explicit jaeTextEditControl(wxWindow *parent, wxWindowID id = wxID_ANY, const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxDefaultSize, long style = 0,
                               const wxString &name = "jaeTextEditControl");

   ~jaeTextEditControl() override;

   auto SendMsg(int msg, wxUIntPtr wp = 0, wxIntPtr lp = 0) const -> wxIntPtr;

   /* >>>>>>>>>>>> Text retrieval and modification >>>>>>>>>>> */
   auto GetText() -> wxString;
   auto GetText(intptr_t position) -> wxString;
   void SetText(const wxString &text) const;
   void SetSavePoint() const;
   auto GetLine(int lineNo) const -> wxString;
   void ReplaceSelection(const wxString &text) const;
   void SetReadOnly(bool isReadOnly);
   auto IsReadOnly() -> bool;
   auto GetTextRange(int start, int end) -> wxString;
   auto GetStyledText(int start, int end) -> wxString;
   void Allocate(int noOfBytes);
   void AllocateLines(int noOfLines);
   void AddText(const wxString &text);
   void AddStyledText(const wxString &text);
   void AppendText(const wxString &text);
   void InsertText(int position, const wxString &text);
   void ChangeInsertion(int position, const wxString &text);
   void ClearAll();
   void DeleteRange(int start, int end);
   void ClearDocumentStyle();
   auto GetCharAt(int position) -> int;
   auto GetStyleAt(int position) -> int;
   void ReleaseAllExtendedStyles();
   auto AllocateExtendedStyles(int noOfStyles) -> int;
   /* <<<<<<<<<< Text retrieval and modification <<<<<<<<<<<< */

   /* <<<<<<<<<< Searching <<<<<<<<<<<< */
   void SetTargetStart(int start);
   auto GetTargetStart() -> int;
   void SetTargetStartVirtualSpace(int start);
   auto GetTargetStartVirtualSpace() -> int;
   void SetTargetEnd(int end);
   auto GetTargetEnd() -> int;
   void SetTargetEndVirtualSpace(int start);
   auto GetTargetEndVirtualSpace() -> int;
   void SetTargetRange(int start, int end);
   void TargetFromSelection();
   void TargetWholeDocument();
   void SetSearchFlags(int searchFlags);
   auto GetSearchFlags() -> int;
   auto SearchInTarget(const wxString &text) -> int;
   auto getTargetText(const wxString &text) -> int;
   auto ReplaceTarget(const wxString &text) -> int;
   auto ReplaceTargetRegularExpr(const wxString &text) -> int;
   auto GetTag(int tagNumber, const wxString &text) -> int;

   /* >>>>>>>>>>>> Searching >>>>>>>>>>> */

   /* >>>>>>>>>>>> Overtype >>>>>>>>>>> */
   void SetOvertype(bool isOvertype);
   auto GetOvertype() -> bool;
   /* >>>>>>>>>>>> Overtype >>>>>>>>>>> */

   /* >>>>>>>>>>>> Cut, copy and Paste >>>>>>>>>>> */
   void Cut();
   void Copy();
   void Paste();
   void Clear();
   auto CanPaste() -> bool;
   void CopyRange(int start, int end);
   void CopyText(const wxString &text);
   void CopyAllowLine();
   void SetPasteConvertEndings(bool isConvert);
   auto GetPasteConvertEndings() -> bool;
   void ReplaceRectangle(const wxString &text);
   /* <<<<<<<<<< Cut, copy and Paste <<<<<<<<<<<< */

   /* >>>>>>>>>>>> Error handling  >>>>>>>>>>> */
   void SetStatus(int isOvertype);
   auto GetStatus() -> int;
   /* >>>>>>>>>>>> Error handling  >>>>>>>>>>> */

   /* >>>>>>>>>>>> Undo and Redo >>>>>>>>>>> */
   void Undo();
   auto CanUndo() -> bool;
   void EmptyUndoBuffer();
   void Redo();
   void CanRedo();
   void SetUndoCollection(bool collectUndo);
   auto GetUndoCollection() -> bool;
   void BeginUndoAction();
   void EndUndoAction();
   void AddUndoAction(int token, int flags);
   /* >>>>>>>>>>>> Undo and Redo >>>>>>>>>>> */

   /* >>>>>>>>>>>> Selection and information >>>>>>>>>>> */
   auto GetTextLength() const -> intptr_t;
   auto GetLength() -> int;
   auto GetLineCount() -> int;
   auto LinesOnScreen() -> int;
   auto GetModify() -> bool;
   void SetSelection(int anchor, int caret);
   void GoToPos(int caret);
   void GoToLine(int line);
   void SetCurrentPos(int caret);
   auto GetCurrentPos() -> int;
   void SetAnchor(int anchor);
   auto GetAnchor() -> int;
   void SetSelectionStart(int anchor);
   auto GetSelectionStart() -> int;
   void SetSelectionEnd(int anchor);
   auto GetSelectionEnd() -> int;

   auto LineLength(int lineNo) const -> intptr_t;
   /* >>>>>>>>>>>> Selection and information >>>>>>>>>>> */

#if 0
   /* wxTextAreaBase virtual functions */
   auto GetLineLength(long int lineNo) const -> int override;
   auto GetLineText(long int lineNo) const -> wxString override;
   auto GetNumberOfLines() const -> int override {}
   auto IsModified() const -> bool override {}
   void MarkDirty() override;
   void DiscardEdits() override;
   auto SetStyle(long int, long int, const wxTextAttr &) -> bool override;
   auto GetStyle(long int, wxTextAttr &) -> bool override;
   auto SetDefaultStyle(const wxTextAttr &) -> bool override;
   auto XYToPosition(long int, long int) const -> long int override;
   auto PositionToXY(long int, long int *, long int *) const -> bool override;
   void ShowPosition(long int) override;
   auto GetValue() const -> wxString override;
   void SetValue(const wxString &) override;
   auto IsValidPosition(long int) const -> bool override;

   /* wxTextEntryBase virtual functions */
   void WriteText(const wxString &text) override;
   void Remove(long int, long int) override;
   void Copy() override;
   void Cut() override;
   void Paste() override;
   void Undo() override;
   void Redo() override;
   auto CanUndo() const -> bool override;
   auto CanRedo() const -> bool override;
   void SetInsertionPoint(long int) override;
   auto GetInsertionPoint() const -> long int override;
   auto GetLastPosition() const -> long int override;
   void SetSelection(long int, long int) override;
   void GetSelection(long int *, long int *) const override;
   auto IsEditable() const -> bool override;
   void SetEditable(bool) override;
   auto DoGetValue() const -> wxString override;
   auto GetEditableWindow() -> wxWindow * override;
#endif
};
