#pragma once

#include "MainFrame.h"
#include "jae_text_control/jaeTextEditControlBase.h"
#include "jae_text_control/jaeTextEditControlEvent.h"
#include <vector>
#include "jaeTextDocument.h"
#include "wx/event.h"

class MainFrame;

class jaeCppTextDocument : public jaeTextDocument {
 public:
   explicit jaeCppTextDocument(wxWindow *parent, wxWindowID id = wxID_ANY, const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxDefaultSize, long style = 0);
   ~jaeCppTextDocument() override;

   void DoContextMenu(wxContextMenuEvent &evt) override;

   void OnMarginClick(jaeTextEditControlEvent &event);
   void OnMarginRightClick(jaeTextEditControlEvent &event);

   auto LoadFile(const wxString &filename) -> bool;
   void updateLastAccessStamp() { m_lastOpened = wxGetLocalTime(); };
   auto getLastAccessTime() const -> long { return m_lastOpened; };
   void gotoLineAndEnsureVisible(int lineNo);
   void disableLine();
   void setBreakpoints(std::vector<int> &breakpointLineNoList) override;
   void deleteAllMarkers() override;

   long       m_lastOpened;
   MainFrame *_mainFrame;
};
