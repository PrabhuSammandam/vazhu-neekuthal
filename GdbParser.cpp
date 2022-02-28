#include "GdbParser.h"

auto Token::typeToString(Type type) -> const char * {
   const char *str = "?";
   switch (type) {
   case UNKNOWN:
      str = "unknown";
      break;
   case C_STRING:
      str = "c_string";
      break;
   case C_CHAR:
      str = "c_char";
      break;
   case KEY_EQUAL:
      str = "=";
      break;
   case KEY_LEFT_BRACE:
      str = "{";
      break;
   case KEY_RIGHT_BRACE:
      str = "}";
      break;
   case KEY_LEFT_BAR:
      str = "[";
      break;
   case KEY_RIGHT_BAR:
      str = "]";
      break;
   case KEY_UP:
      str = "^";
      break;
   case KEY_PLUS:
      str = "+";
      break;
   case KEY_COMMA:
      str = ",";
      break;
   case KEY_TILDE:
      str = "~";
      break;
   case KEY_SNABEL:
      str = "@";
      break;
   case KEY_STAR:
      str = "*";
      break;
   case KEY_AND:
      str = "&";
      break;
   case END_CODE:
      str = "endcode";
      break;
   case VAR:
      str = "var";
      break;
   }
   return str;
}

auto Token::createToken(char c) -> Token * {
   Token::Type type = Token::UNKNOWN;
   switch (c) {
   case '=':
      type = Token::KEY_EQUAL;
      break;
   case '{':
      type = Token::KEY_LEFT_BRACE;
      break;
   case '}':
      type = Token::KEY_RIGHT_BRACE;
      break;
   case '[':
      type = Token::KEY_LEFT_BAR;
      break;
   case ']':
      type = Token::KEY_RIGHT_BAR;
      break;
   case ',':
      type = Token::KEY_COMMA;
      break;
   case '^':
      type = Token::KEY_UP;
      break;
   case '+':
      type = Token::KEY_PLUS;
      break;
   case '~':
      type = Token::KEY_TILDE;
      break;
   case '@':
      type = Token::KEY_SNABEL;
      break;
   case '&':
      type = Token::KEY_AND;
      break;
   case '*':
      type = Token::KEY_STAR;
      break;
   default:
      break;
   }

   auto *cur = new Token(type);
   return cur;
}

auto Token::isDelimiter(char c) -> bool { return (c == '=' || c == '{' || c == '}' || c == ',' || c == '[' || c == ']' || c == '+' || c == '^' || c == '~' || c == '@' || c == '&' || c == '*'); }

/* ------------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------------ */

GdbParser::GdbParser(const wxString &gdbOutput) : _gdbOutput{gdbOutput} {}

auto GdbParser::tokenize() -> GdbParser & {
   auto &str                                  = _gdbOutput;
   enum { IDLE, END_CODE, STRING, VAR } state = IDLE;
   Token *     cur                            = nullptr;
   bool        prevCharIsEscCode              = false;
   std::string sttt                           = "Hello World";

   if (str.IsEmpty()) {
      return *this;
   }

   for (int i = 0; i < str.size(); i++) {
      char c         = str[i];
      bool isEscaped = false;

      if (c == '\\' && prevCharIsEscCode) {
         prevCharIsEscCode = false;
      } else if (prevCharIsEscCode) {
         isEscaped         = true;
         prevCharIsEscCode = false;
      } else if (c == '\\') {
         prevCharIsEscCode = true;
         continue;
      }

      switch (state) {
      case IDLE: {
         if (c == '"') {
            cur = new Token(Token::C_STRING);
            m_list.push_back(cur);
            state = STRING;
         } else if (c == '(') {
            cur = new Token(Token::END_CODE);
            m_list.push_back(cur);
            cur->m_text += c;
            state = END_CODE;
         } else if (Token::isDelimiter(c)) {
            cur = Token::createToken(c);
            m_list.push_back(cur);
            cur->m_text += c;
            state = IDLE;
         } else if (c != ' ') {
            cur = new Token(Token::VAR);
            m_list.push_back(cur);
            cur->m_text = c;
            state       = VAR;
         }
      }; break;
      case END_CODE: {
         wxString codeEndStr = "(gdb)";
         cur->m_text += c;

         if (cur->m_text.length() == codeEndStr.length()) {
            state = IDLE;
         } else if (cur->m_text.compare(codeEndStr.Left(cur->m_text.length())) != 0) {
            cur->setType(Token::VAR);
            state = IDLE;
         }
      }; break;
      case STRING: {
         if (c == '"' && isEscaped == false) {
            state = IDLE;
         } else if (isEscaped) {
            if (c == 'n') {
               cur->m_text += '\n';
            } else if (c == 't') {
               cur->m_text += '\t';
            } else {
               cur->m_text += c;
            }
         } else {
            cur->m_text += c;
         }
      }; break;
      case VAR: {
         if (c == '=' || c == ',' || c == '{' || c == '}' || c == '^') {
            i--;
            cur->m_text = cur->m_text.Trim();
            state       = IDLE;
         } else {
            cur->m_text += c;
         }
      }; break;
      }
   }
   if (cur != nullptr && (cur->getType() == Token::VAR)) {
      cur->m_text = cur->m_text.Trim();
   }
   return *this;
}

auto GdbParser::pop_token() -> Token * {
   if (m_list.empty()) {
      return nullptr;
   }
   auto *tok = m_list.at(0);
   m_freeTokens.push_back(tok);
   m_list.erase(m_list.begin());

   return tok;
}

auto GdbParser::peek_token() -> Token * { return m_list.empty() ? nullptr : m_list.at(0); }

auto GdbParser::isTokenPending() -> bool { return peek_token() != nullptr; }

auto GdbParser::checkToken(Token::Type type) -> Token * {
   if (m_list.empty() || m_list.at(0)->getType() != type) {
      return nullptr;
   }
   return pop_token();
}

auto GdbParser::eatToken(Token::Type type) -> Token * { return checkToken(type); }

/*
output →
    ( out-of-band-record )* [ result-record ] "(gdb)" nl
*/
auto GdbParser::parseOutput() -> GdbResponse * {
   GdbResponse *resp  = nullptr;
   Token *      cmdId = nullptr;

   if (m_list.empty()) {
      return resp;
   }
   cmdId = checkToken(Token::VAR);

   auto *tok = peek_token();

   switch (tok->getType()) {
   case Token::KEY_UP: {
      eatToken(Token::KEY_UP);
      resp = parseResultRecord();
   } break;
   case Token::KEY_TILDE: {
      eatToken(Token::KEY_TILDE);
      resp = new GdbResponse{GdbResponse::CONSOLE_STREAM_OUTPUT};
      tok  = eatToken(Token::C_STRING);
      resp->setString(tok->getString());
   } break;
   case Token::KEY_SNABEL: {
      eatToken(Token::KEY_SNABEL);
      resp = new GdbResponse{GdbResponse::TARGET_STREAM_OUTPUT};
      tok  = eatToken(Token::C_STRING);
      resp->setString(tok->getString());
   } break;
   case Token::KEY_AND: {
      eatToken(Token::KEY_AND);
      resp = new GdbResponse{GdbResponse::LOG_STREAM_OUTPUT};
      tok  = eatToken(Token::C_STRING);
      resp->setString(tok->getString());
   } break;
   case Token::KEY_STAR: {
      /*
      exec-async-output →
          [ token ] "*" async-output nl
      */
      eatToken(Token::KEY_STAR);
      resp = new GdbResponse{GdbResponse::EXEC_ASYNC_OUTPUT};
      parseAsyncOutput(resp, &resp->reason);
   } break;
   case Token::KEY_PLUS: {
      /*
      status-async-output →
          [ token ] "+" async-output nl
      */
      eatToken(Token::KEY_PLUS);
      resp = new GdbResponse{GdbResponse::STATUS_ASYNC_OUTPUT};
      parseAsyncOutput(resp, &resp->reason);
   } break;
   case Token::KEY_EQUAL: {
      /*
      notify-async-output →
          [ token ] "=" async-output nl
      */
      eatToken(Token::KEY_EQUAL);
      resp = new GdbResponse{GdbResponse::NOTIFY_ASYNC_OUTPUT};
      parseAsyncOutput(resp, &resp->reason);
   } break;
   case Token::END_CODE:
      eatToken(Token::END_CODE);
      resp = new GdbResponse{GdbResponse::TERMINATION};
      break;
   case Token::UNKNOWN:
   case Token::C_STRING:
   case Token::C_CHAR:
   case Token::KEY_LEFT_BRACE:
   case Token::KEY_RIGHT_BRACE:
   case Token::KEY_LEFT_BAR:
   case Token::KEY_RIGHT_BAR:
   case Token::KEY_COMMA:
   case Token::VAR:
      break;
   }

   if (resp != nullptr && cmdId != nullptr) {
      resp->_cmdIdAvailable = true;
      resp->_cmdId          = cmdId->m_text;
   }

   std::for_each(m_freeTokens.begin(), m_freeTokens.end(), [](auto *t) { delete t; });
   m_freeTokens.clear();

   return resp;
}

/*
result-record →
    [ token ] "^" result-class ( "," result )* nl
*/
auto GdbParser::parseResultRecord() -> GdbResponse * {
   int rc = 0;

   // Parse 'result class'
   auto *tok = eatToken(Token::VAR);

   if (tok == nullptr) {
      return nullptr;
   }

   wxString         resultClassString = tok->getString();
   GDB_RESULT_CLASS resultClass       = GDB_RESULT_CLASS_DONE;

   if (resultClassString == "done") {
      resultClass = GDB_RESULT_CLASS_DONE;
   } else if (resultClassString == "running") {
      resultClass = GDB_RESULT_CLASS_RUNNING;
   } else if (resultClassString == "connected") {
      resultClass = GDB_RESULT_CLASS_CONNECTED;
   } else if (resultClassString == "error") {
      resultClass = GDB_RESULT_CLASS_ERROR;
   } else if (resultClassString == "exit") {
      resultClass = GDB_RESULT_CLASS_EXIT;
   } else {
      return nullptr;
   }
   auto *resp     = new GdbResponse;
   resp->m_result = resultClass;

   while (checkToken(Token::KEY_COMMA) != nullptr && rc == 0) {
      rc = parseResult(resp->tree.getRoot());
   }

   resp->setType(GdbResponse::RESULT);

   return resp;
}

/*
async-output →
    async-class ( "," result )*
*/
auto GdbParser::parseAsyncOutput(GdbResponse *resp, GDB_ASYNC_CLASS *ac) -> int {
   // Get the class
   auto *tokVar = eatToken(Token::VAR);

   if (tokVar == nullptr) {
      return -1;
   }
   wxString acString = tokVar->getString();

   if (acString == "stopped") {
      *ac = GDB_ASYNC_CLASS::STOPPED;
   } else if (acString == "running") {
      *ac = GDB_ASYNC_CLASS::RUNNING;
   } else if (acString == "thread-created") {
      *ac = GDB_ASYNC_CLASS::THREAD_CREATED;
   } else if (acString == "thread-group-added") {
      *ac = GDB_ASYNC_CLASS::THREAD_GROUP_ADDED;
   } else if (acString == "thread-group-started") {
      *ac = GDB_ASYNC_CLASS::THREAD_GROUP_STARTED;
   } else if (acString == "library-loaded") {
      *ac = GDB_ASYNC_CLASS::LIBRARY_LOADED;
   } else if (acString == "breakpoint-modified") {
      *ac = GDB_ASYNC_CLASS::BREAKPOINT_MODIFIED;
   } else if (acString == "breakpoint-deleted") {
      *ac = GDB_ASYNC_CLASS::BREAKPOINT_DELETED;
   } else if (acString == "thread-exited") {
      *ac = GDB_ASYNC_CLASS::THREAD_EXITED;
   } else if (acString == "thread-group-exited") {
      *ac = GDB_ASYNC_CLASS::THREAD_GROUP_EXITED;
   } else if (acString == "library-unloaded") {
      *ac = GDB_ASYNC_CLASS::LIBRARY_UNLOADED;
   } else if (acString == "thread-selected") {
      *ac = GDB_ASYNC_CLASS::THREAD_SELECTED;
   } else if (acString == "cmd-param-changed") {
      *ac = GDB_ASYNC_CLASS::CMD_PARAM_CHANGED;
   } else {
      /* warnMsg("Unexpected response '%s'", stringToCStr(acString)); */
      // assert(0);
      *ac = GDB_ASYNC_CLASS::UNKNOWN;
   }

   while (checkToken(Token::KEY_COMMA) != nullptr) {
      parseResult(resp->tree.getRoot());
   }

   return 0;
}

/*
result →
    variable "=" value
*/
auto GdbParser::parseResult(TreeNode *parent) -> int {
   wxString name;

   if (!m_list.empty() && m_list.at(0)->getType() == Token::KEY_LEFT_BRACE) {
   } else {
      //
      Token *tokVar = eatToken(Token::VAR);

      if (tokVar == nullptr) {
         return -1;
      }
      name = tokVar->getString();

      if (eatToken(Token::KEY_EQUAL) == nullptr) {
         return -1;
      }
   }

   auto *item = new TreeNode(name);
   parent->addChild(item);

   parseValue(item);

   return 0;
}

/*
value →
    const | tuple | list
const →
    c-string
tuple →
    "{}" | "{" result ( "," result )* "}"
list →
    "[]" | "[" value ( "," value )* "]" | "[" result ( "," result )* "]"
*/
auto GdbParser::parseValue(TreeNode *item) -> int {
   Token *tok = nullptr;
   int    rc  = 0;

   tok = pop_token();

   // Const?
   if (tok->getType() == Token::C_STRING) {
      item->setData(tok->getString());
   }
   // Tuple -> "{}" | "{" result ( "," result )* "}"
   else if (tok->getType() == Token::KEY_LEFT_BRACE) {
      do {
         parseResult(item);
      } while (checkToken(Token::KEY_COMMA) != nullptr);

      if (eatToken(Token::KEY_RIGHT_BRACE) == nullptr) {
         return -1;
      }
   }
   // List -> "[]" | "[" value ( "," value )* "]" | "[" result ( "," result )* "]"
   else if (tok->getType() == Token::KEY_LEFT_BAR) {
      if (checkToken(Token::KEY_RIGHT_BAR) != nullptr) {
         return 0;
      }

      tok = peek_token();

      if (tok->getType() == Token::VAR) {
         do {
            parseResult(item);
         } while (checkToken(Token::KEY_COMMA) != nullptr);
      } else {
         int      idx = 1;
         wxString name;

         do {
            name.sprintf("%d", idx++);
            auto *node = new TreeNode(name);
            item->addChild(node);
            rc = parseValue(node);
         } while (checkToken(Token::KEY_COMMA) != nullptr);
      }

      if (eatToken(Token::KEY_RIGHT_BAR) == nullptr) {
         return -1;
      }
   }
   return rc;
}

auto GdbResponse::typeToStr() -> wxString {
   switch (m_type) {
   case UNKNOWN:
      return "UNKNOWN";
   case RESULT:
      return "RESULT";
   case CONSOLE_STREAM_OUTPUT:
      return "CONSOLE_STREAM_OUTPUT";
   case TARGET_STREAM_OUTPUT:
      return "TARGET_STREAM_OUTPUT";
   case LOG_STREAM_OUTPUT:
      return "LOG_STREAM_OUTPUT";
   case TERMINATION:
      return "TERMINATION";
   case STATUS_ASYNC_OUTPUT:
      return "STATUS_ASYNC_OUTPUT";
   case NOTIFY_ASYNC_OUTPUT:
      return "NOTIFY_ASYNC_OUTPUT";
   case EXEC_ASYNC_OUTPUT:
      return "EXEC_ASYNC_OUTPUT";
   }
}

auto GdbResponse::reassonToString() const -> wxString {
   switch (reason) {
   case GDB_ASYNC_CLASS::UNKNOWN:
      return "UNKOWN";
   case GDB_ASYNC_CLASS::RUNNING:
      return "RUNNING";
   case GDB_ASYNC_CLASS::STOPPED:
      return "STOPPED";
   case GDB_ASYNC_CLASS::THREAD_GROUP_ADDED:
      return "THREAD_GROUP_ADDED";
   case GDB_ASYNC_CLASS::THREAD_GROUP_REMOVED:
      return "THREAD_GROUP_REMOVED";
   case GDB_ASYNC_CLASS::THREAD_GROUP_STARTED:
      return "THREAD_GROUP_STARTED";
   case GDB_ASYNC_CLASS::THREAD_GROUP_EXITED:
      return "THREAD_GROUP_EXITED";
   case GDB_ASYNC_CLASS::THREAD_CREATED:
      return "THREAD_CREATED";
   case GDB_ASYNC_CLASS::THREAD_EXITED:
      return "THREAD_EXITED";
   case GDB_ASYNC_CLASS::THREAD_SELECTED:
      return "THREAD_SELECTED";
   case GDB_ASYNC_CLASS::LIBRARY_LOADED:
      return "LIBRARY_LOADED";
   case GDB_ASYNC_CLASS::LIBRARY_UNLOADED:
      return "LIBRARY_UNLOADED";
   case GDB_ASYNC_CLASS::BREAKPOINT_CREATED:
      return "BREAKPOINT_CREATED";
   case GDB_ASYNC_CLASS::BREAKPOINT_MODIFIED:
      return "BREAKPOINT_MODIFIED";
   case GDB_ASYNC_CLASS::BREAKPOINT_DELETED:
      return "BREAKPOINT_DELETED";
   case GDB_ASYNC_CLASS::RECORD_STARTED:
      return "RECORD_STARTED";
   case GDB_ASYNC_CLASS::RECORD_STOPPED:
      return "RECORD_STOPPED";
   case GDB_ASYNC_CLASS::CMD_PARAM_CHANGED:
      return "CMD_PARAM_CHANGED";
   case GDB_ASYNC_CLASS::MEMORY_CHANGED:
      return "MEMORY_CHANGED";
   }
}
