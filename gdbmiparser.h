#pragma once

#include "GdbParser.h"
#include "GdbModels.h"
#include "GdbResultTree.h"
#include "wx/string.h"
#include <vector>

class GdbMiParser {
 public:
   GdbMiParser() = default;

   static auto parseVariableData(VarWatch *var, std::vector<Token *> *tokenList) -> int;
   static auto tokenizeVarString(wxString str) -> std::vector<Token *>;
   static void setData(VarWatch *var, wxString data);
};
