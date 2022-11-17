#include "KeyProcessor.h"
#include "wx/event.h"
#include "wx/window.h"
#include <cstring>
#include <iostream>
#include <ostream>
#include <utility>
#include <vector>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include "MainFrame.h"
#include <wx/aui/aui.h>
#include <wx/cmdline.h>
#include <wx/ffile.h>
#include <wx/sizer.h>
#include <wx/splitter.h>

static auto getModifiers(wxKeyEvent &keyEvent) -> uint32_t {
   uint32_t keyCode = 0;

   if (keyEvent.ControlDown()) {
      keyCode |= JAE_CONTROL_KEY_MASK;
   }

   if (keyEvent.ShiftDown()) {
      keyCode |= JAE_SHIFT_KEY_MASK;
   }

   if (keyEvent.AltDown()) {
      keyCode |= JAE_ALT_KEY_MASK;
   }
   return keyCode;
}

class MyApp : public wxApp {
 public:
   auto OnInit() -> bool override {
      if (!wxApp::OnInit()) {
         return (false);
      }
      SetVendorName("JinjuAmla");
      SetAppName("vazhu-neekuthal");
      SetAppDisplayName("GDB debugger");

      auto *frame = new MainFrame(cmdLineOption);

      // addShortcutKeys(frame);

      frame->SetSize(800, 800);
      frame->Maximize(true);
      frame->Show(true);

      SetTopWindow(frame);

      return (true);
   }

   void OnInitCmdLine(wxCmdLineParser &parser) wxOVERRIDE {
      parser.AddOption("s", "session", "session name or full path of the session");
      parser.AddOption("v", "verbose", "session name or full path of the session");
   }

   auto OnCmdLineParsed(wxCmdLineParser &parser) -> bool wxOVERRIDE {
      if (!wxApp::OnCmdLineParsed(parser)) {
         return false;
      }

      parser.Found("s", &cmdLineOption._session);

      return true;
   }

   auto FilterEvent(wxEvent &event) -> int override {
      return -1;
      if ((event.GetEventType() == wxEVT_KEY_DOWN)) {
         auto keyEvent = dynamic_cast<wxKeyEvent &>(event);

         if (!keyEvent.HasModifiers() && !_keyProcessor._inProgress) {
            return -1;
         }

         auto     key     = static_cast<uint32_t>(keyEvent.GetKeyCode());
         uint32_t keyCode = getModifiers(keyEvent);

         if (key == wxKeyCode::WXK_CONTROL) {
            return -1;
         }

         keyCode |= (key & 0xFF'FFU);

         if (_keyProcessor.Process(keyCode)) {
            return 1;
         }
         return -1;
      }

      return -1;
   }

   void addShortcutKeys(MainFrame *mainframe) //
   {
      std::vector<uint32_t> goToLineKeys{};
      goToLineKeys.push_back((JAE_CONTROL_KEY_MASK | (uint32_t)'G'));
      goToLineKeys.push_back((JAE_CONTROL_KEY_MASK | (uint32_t)'G'));
      //      auto *keyMap1 = new KeyMap<MainFrame, wxEvent>{goToLineKeys, &MainFrame::goToLine, mainframe};

      //    _keyProcessor.addGlobalKeyMap(keyMap1);
   }

 private:
   CmdLineOptions cmdLineOption;
   KeyProcessor   _keyProcessor;
};

wxIMPLEMENT_APP(MyApp);
