#include "GdbMiDriver.hpp"
#include "GdbMiParser.hpp"
#include <ostream>
#include <type_traits>

using std::string;

struct membuf : std::streambuf {
   membuf(char const *base, size_t size) {
      char *pointer(const_cast<char *>(base));

      this->setg(pointer, pointer, pointer + size);
   }
};

struct imemstream : virtual membuf, std::istream {
   imemstream(char const *base, size_t size) : membuf(base, size), std::istream(static_cast<std::streambuf *>(this)) {}
};

JA::GdbMiDriver::~GdbMiDriver() {
   delete _scanner;
   delete _parser;
   delete _output;
}

auto JA::GdbMiDriver::parse(const string &buffer) -> GdbMiOutput * {
   imemstream d(buffer.c_str(), buffer.size() - 1);
   _scanner = new GdbMiScanner{&d};
   _parser  = new GdbMiParser{*_scanner, *this};

   // _parser->set_debug_level(100);
   auto rc = _parser->parse();

   if (rc != 0) {
      std::cout << "parse failed" << std::endl;
      delete _output;
      _output = nullptr;
      return nullptr;
   }
   auto *temp = _output;
   _output    = nullptr; // memory should be owned by caller
   return temp;
}

auto JA::GdbMiDriver::createTuple(std::string &variableName, std::vector<GdbMiResult *> &resultList) -> GdbMiResult * {
   auto *tuple      = new GdbMiResult{GdbMiResult::VALUE_TYPE_TUPLE};
   tuple->_variable = variableName;
   std::copy(resultList.begin(), resultList.end(), std::back_inserter(tuple->_childrens));

   return tuple;
}

auto JA::GdbMiDriver::createList(std::string &variableName, std::vector<GdbMiResult *> &resultList) -> GdbMiResult * {
   auto *list      = new GdbMiResult{GdbMiResult::VALUE_TYPE_LIST};
   list->_variable = variableName;
   std::copy(resultList.begin(), resultList.end(), std::back_inserter(list->_childrens));

   return list;
}

auto JA::GdbMiDriver::createConst(std::string &variable, std::string &data) -> GdbMiResult * {
   auto *result      = new GdbMiResult{GdbMiResult::VALUE_TYPE_CONST};
   result->_const    = std::move(data);
   result->_variable = std::move(variable);
   return result;
}

auto JA::GdbMiDriver::createAsyncRecord(std::string &token, int asyncType, std::string &asyncClass, const std::vector<GdbMiResult *> &resultList) -> GdbMiOutOfBandRecord * {
   auto *nao = new GdbMiAsyncRecord{asyncType, token, GdbMiAsyncRecord::getAsyncClass(asyncClass), resultList};
   return nao;
}

auto JA::GdbMiDriver::createResultRecord(std::string &token, std::string &resultClass, const std::vector<GdbMiResult *> &resultList) -> GdbMiResultRecord * {
   auto *nao = new GdbMiResultRecord{GdbMiResultRecord::getResultClass(resultClass), token, resultList};
   return nao;
}

auto JA::GdbMiDriver::createStreamRecord(int streamType, std::string &data) -> GdbMiStreamRecord * { return new GdbMiStreamRecord{streamType, data}; }

auto JA::GdbMiDriver::setOutput(GdbMiOutput *output) -> GdbMiOutput * {
   _output = output;
   return _output;
}

auto JA::GdbMiDriver::unescapeString(const std::string &str) -> std::string {
   char  *result = nullptr;
   size_t r      = 0;
   size_t s      = 0;
   size_t length = 0;

   result = new char[str.length() + 1];
   length = str.length();

   /* a CSTRING should start and end with a quote */
   /*assert(result);*/
   /*assert(length >= 2);*/

   for (r = 0, s = 1; s < length - 1; ++s) {
      if (str[s] == '\\') {
         switch (str[s + 1]) {
         case 'n':
            result[r++] = '\n';
            ++s;
            break;
         case 'b':
            result[r++] = '\b';
            ++s;
            break;
         case 't':
            result[r++] = '\t';
            ++s;
            break;
         case 'f':
            result[r++] = '\f';
            ++s;
            break;
         case 'r':
            result[r++] = '\r';
            ++s;
            break;
         case 'e':
            result[r++] = '\033';
            ++s;
            break;
         case 'a':
            result[r++] = '\007';
            ++s;
            break;
         case '"':
            result[r++] = '\"';
            ++s;
            break;
         case '\\':
            result[r++] = '\\';
            ++s;
            break;
         default:
            result[r++] = str[s];
            break;
         }
      } else {
         result[r++] = str[s];
      }
   }

   result[r] = 0;

   return result;
}
