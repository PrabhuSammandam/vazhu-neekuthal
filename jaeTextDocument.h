#pragma once

#include "jae_text_control/jaeTextEditControlBase.h"
#include <vector>

class jaeTextDocument : public jaeTextEditControlBase {

 public:
   jaeTextDocument(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style) : jaeTextEditControlBase{parent, id, pos, size, style} {}

   auto         getFilePath() -> wxString { return _filePath; }
   virtual void setBreakpoints(std::vector<int> &breakpointLineNoList) {}
   virtual void deleteAllMarkers() {}

 protected:
   wxString _filePath;
};
