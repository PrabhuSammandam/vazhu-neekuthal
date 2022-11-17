#pragma once

#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#include "GdbMiParser.hpp"
#include "location.hh"

namespace JA {
class GdbMiScanner : public yyFlexLexer {
 public:
   explicit GdbMiScanner(std::istream *inStream) : yyFlexLexer(inStream){};
   ~GdbMiScanner() override = default;

   // get rid of override virtual function warning
   using FlexLexer::yylex;

   virtual auto yylex(JA::GdbMiParser::semantic_type *lval, JA::GdbMiParser::location_type *location) -> int;
   // YY_DECL defined in mc_lexer.l
   // Method body created by flex in mc_lexer.yy.cc

 private:
   /* yyval ptr */
   JA::GdbMiParser::semantic_type *yylval = nullptr;
};
} // namespace JA
