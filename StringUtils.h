#pragma once

#include <wx/arrstr.h>
#include <wx/string.h>

class StringUtils {
 public:
   static auto ToStdString(const wxString &str) -> std::string;
   static void StripTerminalColouring(const std::string &buffer, std::string &modbuffer);
   static void StripTerminalColouring(const wxString &buffer, wxString &modbuffer);
   static auto BuildArgv(const wxString &str, int &argc) -> char **;
   static auto BuildArgv(const wxString &str) -> wxArrayString;
   static void FreeArgv(char **argv, int argc);
};
