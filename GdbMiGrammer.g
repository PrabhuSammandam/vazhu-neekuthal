/*
The bison whether c++ or c requires the yylex function. There are many forms of yylex with different arguments type.
First identify the prototype of the yylex function by running the bison to create a outputc or cpp file and check in the c or cpp file.

in generated code the yylex is not defined or declared as function. It is just the macro name. we need #define the yylex macro with real
function name only.

Say for example in the generated code the yylex was like 
            yyla.kind_ = yytranslate_ (yylex (&yyla.value, &yyla.location));

So we need to declare and define a function with arguments same as above yylex call.
            int my_yylex( JA::GdbMiParser::semantic_type * const lval, JA::GdbMiParser::location_type *location );

Then we need to tell the bison that yylex is replaced by our function name my_yylex like below
#undef yylex
#define yylex my_yylex

This is the first toughest part in using the bison. we can define the grammers and we will stuck on how to test and where to give input to test.
*/

%skeleton "lalr1.cc"
%require "3.2"
%language "c++"
%defines
%no-lines
%define api.value.type variant
%define api.namespace {JA}
%define api.parser.class {   GdbMiParser }
%define parse.error verbose
%define parse.trace
%locations
%define parse.lac full

%code requires { namespace JA { class GdbMiScanner; class GdbMiDriver; } }

%parse-param{JA::GdbMiScanner & scanner}
%parse-param{JA::GdbMiDriver & driver}

%code top {
#include "GdbMiResp.h"
#include "GdbMiScanner.hpp"
#include "GdbMiDriver.hpp"

#undef yylex
#define yylex scanner.yylex
}

%token OPEN_BRACE      /* { */
%token CLOSED_BRACE    /* } */
%token OPEN_PAREN      /* ( */
%token CLOSED_PAREN    /* ) */
%token OPEN_BRACKET    /* [ */
%token CLOSED_BRACKET  /* ] */
%token PLUS            /* + */
%token ASTERISK        /* * */
%token EQUAL           /* = */
%token TILDE           /* ~ */
%token AT_SYMBOL       /* @ */
%token AMPERSAND       /* & */
%token NL              /* \n \r\n \r */
%token INTEGER_LITERAL /* A number 1234 */
%token STRING_LITERAL  /* A string literal */
%token CSTRING         /* "a string like \" this " */
%token COMMA           /* , */
%token CARET           /* ^ */

%type<std::string>                          STRING_LITERAL CSTRING INTEGER_LITERAL cstring

%nterm<std::string>                         optionalToken token resultClass asyncClass variable optionalVariable
%nterm<GdbMiResult *>                       result 
%nterm<std::vector<GdbMiResult *>>          resultList tuple list
%nterm<GdbMiOutOfBandRecord *>              oobRecord streamRecord asyncRecord
%nterm<GdbMiResultRecord *>                 resultRecord
%nterm<GdbMiOutput *>                       output outputVariant
%nterm<int>                                 asyncRecordClass streamRecordClass

%start output

%%
output                 : outputVariant NL                                           { $$ = driver.setOutput($1); };
outputVariant          : oobRecord                                                  { $$ = $1; };
outputVariant          : resultRecord                                               { $$ = $1; };
outputVariant          : OPEN_PAREN variable CLOSED_PAREN                           { };
resultRecord           : optionalToken CARET resultClass                            { $$ = driver.createResultRecord($1, $3, std::vector<GdbMiResult *>{} ); };
resultRecord           : optionalToken CARET resultClass COMMA resultList           { $$ = driver.createResultRecord($1, $3, $5);};
oobRecord              : asyncRecord                                                { $$ = $1; };
oobRecord              : streamRecord                                               { $$ = $1; };
asyncRecord            : optionalToken asyncRecordClass asyncClass                  { $$ = driver.createAsyncRecord($1, $2, $3, std::vector<GdbMiResult *>{} ); };
asyncRecord            : optionalToken asyncRecordClass asyncClass COMMA resultList { $$ = driver.createAsyncRecord($1, $2, $3, $5); };
asyncRecordClass       : ASTERISK                                                   { $$ = GdbMiAsyncRecord::ASYNC_TYPE_EXEC; };
asyncRecordClass       : PLUS                                                       { $$ = GdbMiAsyncRecord::ASYNC_TYPE_STATUS; };
asyncRecordClass       : EQUAL                                                      { $$ = GdbMiAsyncRecord::ASYNC_TYPE_NOTIFY; };
resultClass            : STRING_LITERAL                                             { $$ = std::move($1); };
asyncClass             : STRING_LITERAL                                             { $$ = std::move($1); };
optionalVariable       : %empty                                                     { $$ = std::string{}; };
optionalVariable       : variable EQUAL                                             { $$ = std::move($1); };
resultList             : result                                                     { $$ = std::vector<GdbMiResult *>{}; $$.push_back($1); };
resultList             : resultList COMMA result                                    { $$ = std::move($1); $$.push_back($3);};
result                 : optionalVariable cstring                                   { $$ = driver.createConst($1, $2); };
result                 : optionalVariable tuple                                     { $$ = driver.createTuple($1, $2); };
result                 : optionalVariable list                                      { $$ = driver.createList($1, $2); };
variable               : STRING_LITERAL                                             { $$ = std::move($1); };
tuple                  : OPEN_BRACE CLOSED_BRACE                                    { $$ = std::vector<GdbMiResult *>{}; };
tuple                  : OPEN_BRACE resultList CLOSED_BRACE                         { $$ = std::move($2); };
list                   : OPEN_BRACKET CLOSED_BRACKET                                { $$ = std::vector<GdbMiResult *>{}; };
list                   : OPEN_BRACKET resultList CLOSED_BRACKET                     { $$ = std::move($2); };
streamRecord           : streamRecordClass cstring                                  { $$ = driver.createStreamRecord($1, $2); };
streamRecordClass      : TILDE                                                      { $$ = GdbMiStreamRecord::STREAM_TYPE_CONSOLE; };
streamRecordClass      : AT_SYMBOL                                                  { $$ = GdbMiStreamRecord::STREAM_TYPE_TARGET; };
streamRecordClass      : AMPERSAND                                                  { $$ = GdbMiStreamRecord::STREAM_TYPE_LOG; };
optionalToken          : %empty                                                     { $$ = std::string{}; };
optionalToken          : token                                                      { $$ = std::move($1); };
token                  : INTEGER_LITERAL                                            { $$ = std::move($1); };
cstring                : CSTRING                                                    { $$ = driver.unescapeString($1); }

%%
void JA::GdbMiParser::error(const location_type &l, const std::string &err_message) {
   std::cerr << "Error: " << err_message << " at " << l << "\n";
}
