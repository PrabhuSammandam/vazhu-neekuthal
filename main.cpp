#include "wx/event.h"
#include "wx/window.h"
#include "wx/panel.h"
#include "wx/scrolwin.h"
#include <cstring>
#include <iostream>
#include <ostream>
#include <utility>
#include <vector>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/aui/aui.h>
#include <wx/ffile.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include "MainFrame.h"

class MyApp : public wxApp {
 public:
   auto OnInit() -> bool override {
      if (!wxApp::OnInit()) {
         return (false);
      }
      SetVendorName("JinjuAmla");
      SetAppName("vazhu-neekuthal");
      SetAppDisplayName("GDB debugger");

      auto *frame = new MainFrame();

      frame->SetSize(800, 800);
      frame->Maximize(true);
      frame->Show(true);

      SetTopWindow(frame);

      return (true);
   }
};
wxIMPLEMENT_APP(MyApp);
