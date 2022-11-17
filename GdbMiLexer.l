%option c++
%option noyywrap
%option noline
%option nodefault
%option yyclass="JA::GdbMiScanner"

%{
#include <iostream>
#include "GdbMiResp.h"
#include "GdbMiScanner.hpp"

#undef  YY_DECL
#define YY_DECL int JA::GdbMiScanner::yylex( JA::GdbMiParser::semantic_type * const lval, JA::GdbMiParser::location_type *loc )

/* typedef to make the returns for the tokens shorter */
using token = JA::GdbMiParser::token;

/* define yyterminate as this instead of NULL */
#define yyterminate() return( 0 );

/* update location on matching */
#define YY_USER_ACTION loc->step(); loc->columns(yyleng);

%}

DIGIT       [0-9]
L           [a-zA-Z_]
T           [0-9a-zA-Z_-]
IDENTIFIER  {L}+{T}*

%%
%{          /** Code executed at the beginning of yylex **/
            yylval = lval;
%}

"^"                     {  return token::CARET; }
","                     {  return token::COMMA; }
"+"                     {  return token::PLUS; }
"*"                     {  return token::ASTERISK; }
"="                     {  return token::EQUAL; }
"~"                     {  return token::TILDE; }
"@"                     {  return token::AT_SYMBOL; }
"&"                     {  return token::AMPERSAND; }
"["                     {  return token::OPEN_BRACKET; }
"]"                     {  return token::CLOSED_BRACKET; }
"{"                     {  return token::OPEN_BRACE; }
"}"                     {  return token::CLOSED_BRACE; }
"("                     {  return token::OPEN_PAREN; }
")"                     {  return token::CLOSED_PAREN; }

\n                      {  return token::NL; }
\r\n                    {  return token::NL; }
\r                      {  return token::NL; }

{DIGIT}+                {  yylval->build< std::string >( yytext ); return token::INTEGER_LITERAL; }

[ \t\v\f]               {}
{IDENTIFIER}            {  yylval->build< std::string >( yytext ); return token::STRING_LITERAL;    }
.                       {  yylval->build< std::string >( yytext ); return token::STRING_LITERAL;    }
\"(\\.|[^\\"])*\"       {  yylval->build< std::string >( yytext ); return token::CSTRING; }

%%

