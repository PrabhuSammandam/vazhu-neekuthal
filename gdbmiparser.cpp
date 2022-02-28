#include <assert.h>
#include "gdbmiparser.h"
#include "wx/chartype.h"
#include "wx/string.h"
#include "wx/variant.h"
#include "wx/wxcrt.h"

auto GdbMiParser::tokenizeVarString(wxString str) -> TokenList_t {
   enum { IDLE, BLOCK, BLOCK_COLON, STRING, VAR, CHAR } state = IDLE;
   TokenList_t list;
   Token *     cur       = nullptr;
   char        prevC     = ' ';
   bool        isEscaped = false;

   if (str.IsEmpty()) {
      return list;
   }

   for (int i = 0; i < str.size(); i++) {
      char c = str[i];

      if (c == '\\' && prevC == '\\') {
      } else if (prevC == '\\') {
         isEscaped = true;
      } else if (c == '\\') {
         isEscaped = false;
         prevC     = c;
         continue;
      } else {
         isEscaped = false;
      }

      switch (state) {
      case IDLE: {
         if (c == '"') {
            cur = new Token(Token::C_STRING);
            list.push_back(cur);
            state = STRING;
         } else if (c == '\'') {
            cur = new Token(Token::C_CHAR);
            list.push_back(cur);
            state = CHAR;
         } else if (c == '<') {
            cur = new Token(Token::VAR);
            list.push_back(cur);
            state = BLOCK;
         } else if (c == '(') {
            cur = new Token(Token::VAR);
            list.push_back(cur);
            state = BLOCK_COLON;
         } else if (c == '=' || c == '{' || c == '}' || c == ',' || c == '[' || c == ']' || c == '+' || c == '^' || c == '~' || c == '@' || c == '&' || c == '*') {
            Token::Type type = Token::UNKNOWN;
            if (c == '=') {
               type = Token::KEY_EQUAL;
            }
            if (c == '{') {
               type = Token::KEY_LEFT_BRACE;
            }
            if (c == '}') {
               type = Token::KEY_RIGHT_BRACE;
            }
            if (c == '[') {
               type = Token::KEY_LEFT_BAR;
            }
            if (c == ']') {
               type = Token::KEY_RIGHT_BAR;
            }
            if (c == ',') {
               type = Token::KEY_COMMA;
            }
            if (c == '^') {
               type = Token::KEY_UP;
            }
            if (c == '+') {
               type = Token::KEY_PLUS;
            }
            if (c == '~') {
               type = Token::KEY_TILDE;
            }
            if (c == '@') {
               type = Token::KEY_SNABEL;
            }
            if (c == '&') {
               type = Token::KEY_AND;
            }
            if (c == '*') {
               type = Token::KEY_STAR;
            }
            cur = new Token(type);
            list.push_back(cur);
            cur->m_text += c;
            state = IDLE;
         } else if (c != ' ') {
            cur = new Token(Token::VAR);
            list.push_back(cur);
            cur->m_text = c;
            state       = VAR;
         }

      }; break;
      case CHAR: {
         if (isEscaped) {
            cur->m_text += '\\';
            cur->m_text += c;
         } else if (c == '\'') {
            state = IDLE;
         } else {
            cur->m_text += c;
         }
      }; break;
      case STRING: {
         if (isEscaped) {
            cur->m_text += '\\';
            cur->m_text += c;
         } else if (c == '"') {
            state = IDLE;
         } else {
            cur->m_text += c;
         }
      }; break;
      case BLOCK_COLON:
      case BLOCK: {
         if (isEscaped) {
            if (c == 'n') {
               cur->m_text += '\n';
            } else {
               cur->m_text += c;
            }
         } else if ((c == '>' && state == BLOCK) || (c == ')' && state == BLOCK_COLON)) {
            if (state == BLOCK_COLON) {
               int barIdx = cur->m_text.Index('|');
               if (barIdx != -1) {
                  cur->m_text = cur->m_text.Mid(barIdx + 1);
               }
            }
            state = IDLE;
         } else {
            cur->m_text += c;
         }
      }; break;
      case VAR: {
         if (c == ' ' || c == '=' || c == ',' || c == '{' || c == '}') {
            i--;
            cur->m_text = cur->m_text.Trim();
            state       = IDLE;
         } else {
            cur->m_text += c;
         }
      }; break;
      }
      prevC = c;
   }
   if (cur != nullptr) {
      if (cur->getType() == Token::VAR) {
         cur->m_text = cur->m_text.Trim();
      }
   }
   return list;
}

void GdbMiParser::setData(VarWatch *var, wxString data) {
   wxVariant      m_data = data;
   VarWatch::Type m_type = VarWatch::TYPE_UNKNOWN;

   // A parent?
   if (data == "...") {
      m_data = "{...}";
      m_type = VarWatch::TYPE_UNKNOWN;
   }
   // String?
   else if (data.StartsWith('"')) {
      if (data.EndsWith('"')) {
         data = data.Mid(1, data.length() - 2);
      }
      m_data = data;
      m_type = VarWatch::TYPE_STRING;
   }
   // Character?
   else if (data.StartsWith('\'')) {
      if (data.EndsWith('\'')) {
         data = data.Mid(1, data.length() - 2);
      } else {
         data = data.Mid(1);
      }

      if (data.StartsWith("\\0")) {
         long d;
         data.Mid(2).ToLong(&d);
         m_data = wxVariant{static_cast<int>(d)};
      } else {
         wxChar c = data[0];
         m_data   = wxVariant{c};
      }

      m_type = VarWatch::TYPE_CHAR;
   }
   // Gdb Error message?
   else if (data.EndsWith(">")) {
      m_data = data;
      m_type = VarWatch::TYPE_ERROR_MSG;
   }
   // Vector?
   else if (data.StartsWith("[")) {
      m_data = data;
      m_type = VarWatch::TYPE_UNKNOWN;
   } else if (data.length() > 0) {
      // Integer?
      if (wxIsdigit(data[0]) || data[0] == '-') {
         // Float?
         if (data.Contains(".")) {
            m_data = data;
            m_type = VarWatch::TYPE_FLOAT;
         } else { // or integer?
            if (data.StartsWith("0x")) {
               long long ll = 0;
               data.ToLongLong(&ll, 16);
               m_data = wxVariant{static_cast<long>(ll)};
               m_type = VarWatch::TYPE_HEX_INT;
            } else {
               int firstSpacePos = (int)data.Index(' ');
               if (firstSpacePos != -1) {
                  data = data.Left(firstSpacePos);
               }
               long long ll = 0;
               data.ToLongLong(&ll);
               m_data = wxVariant{static_cast<long>(ll)};
               m_type = VarWatch::TYPE_DEC_INT;
            }
         }
      } else {
         m_data = data;
         m_type = VarWatch::TYPE_ENUM;
      }
   } else {
      m_type = VarWatch::TYPE_UNKNOWN;
   }

   var->setData(m_type, m_data);
}

/**
 * @brief Parses a variable assignment block.
 */
template <typename T> auto removeFirstItem(std::vector<T *> &list) -> T * {
   T *i = list[0];
   list.erase(list.begin());
   return i;
}

int GdbMiParser::parseVariableData(VarWatch *var, std::vector<Token *> *tokenList) {
   if (tokenList->empty()) {
      return -1;
   }

   int rc = 0;

   auto *token = removeFirstItem<Token>(*tokenList);

   if (token == nullptr) {
      return -1;
   }

   if (token->getType() == Token::KEY_LEFT_BAR) {
      wxString data = "[";

      while (!tokenList->empty()) {
         if ((token = removeFirstItem<Token>(*tokenList)) != nullptr) {
            data += token->getString();
         }
      }
      setData(var, data);
      return 0;
   }
   if (token->getType() == Token::KEY_LEFT_BRACE) {
      wxString str;

      while (!tokenList->empty()) {
         str += token->getString();
         token = removeFirstItem<Token>(*tokenList);
      }
      str += token->getString();

      var->setData(VarWatch::TYPE_UNKNOWN, str);
   } else {
      wxString firstTokenStr = token->getString();

      // Did we not get an address followed by the data? (Eg: '0x0001 "string"' )
      if (tokenList->empty()) {
         if (token->getType() == Token::C_STRING) {
            var->setData(VarWatch::TYPE_STRING, token->getString());
         } else {
            setData(var, token->getString());
         }
         return 0;
      }

      Token *nextTok = tokenList->at(0);

      // A character (Eg: '90 'Z')
      if (nextTok->getType() == Token::C_CHAR) {
         long i = 0;
         token->getString().ToLong(&i);
         var->setData(VarWatch::TYPE_CHAR, (int)i);
      } else {
         wxString  valueStr;
         long long ll = 0;
         firstTokenStr.ToLongLong(&ll);
         var->setPointerAddress(ll);

         while (nextTok->getType() == Token::VAR || nextTok->getType() == Token::C_STRING) {
            nextTok = removeFirstItem<Token>(*tokenList);

            if (nextTok->getType() == Token::C_STRING) {
               var->setData(VarWatch::TYPE_STRING, nextTok->getString());
               return 0;
            }
            valueStr = nextTok->getString();

            if (valueStr.StartsWith("<")) {
               valueStr = firstTokenStr + " " + valueStr;
            }

            nextTok = tokenList->empty() ? nullptr : tokenList->at(0);

            if (nextTok == nullptr) {
               break;
            }
         }
         if (valueStr.IsEmpty()) {
            valueStr = firstTokenStr;
         }
         setData(var, valueStr);
      }
   }

   return rc;
}
