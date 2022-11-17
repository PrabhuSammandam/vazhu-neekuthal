#pragma once

#include "GdbMiResp.h"
#include "GdbMiParser.hpp"
#include "GdbMiScanner.hpp"
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace JA {
class GdbMiDriver {
 public:
   GdbMiDriver() = default;

   virtual ~GdbMiDriver();
   auto parse(const std::string &buffer) -> GdbMiOutput *;
   void print();

   auto unescapeString(const std::string &str) -> std::string;
   auto createTuple(std::string &variableName, std::vector<GdbMiResult *> &resultList) -> GdbMiResult *;
   auto createList(std::string &variableName, std::vector<GdbMiResult *> &resultList) -> GdbMiResult *;
   auto createConst(std::string &variable, std::string &data) -> GdbMiResult *;

   auto createAsyncRecord(std::string &token, int asyncRecordType, std::string &asyncClass, const std::vector<GdbMiResult *> &resultList) -> GdbMiOutOfBandRecord *;
   auto createResultRecord(std::string &token, std::string &resultClass, const std::vector<GdbMiResult *> &resultList) -> GdbMiResultRecord *;
   auto createStreamRecord(int streamType, std::string &data) -> GdbMiStreamRecord *;

   auto setOutput(GdbMiOutput *output) -> GdbMiOutput *;

 private:
   JA::GdbMiParser  *_parser  = nullptr;
   JA::GdbMiScanner *_scanner = nullptr;
   GdbMiOutput      *_output  = nullptr;
};
} // namespace JA
