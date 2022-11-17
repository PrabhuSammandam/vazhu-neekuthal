#include "GdbMiResp.h"
#include <utility>

GdbMiResult::GdbMiResult(int type) : _type{type} {}

void GdbMiResult::clear() {
   for (auto *child : _childrens) {
      delete child;
   }
   _childrens.clear();
}
void GdbMiResult::print(int count) {
   char buf[1024];
   std::snprintf(&buf[0], 1024, "+- %s='%s'", _variable.c_str(), _const.c_str());

   std::string text{&buf[0]};

   for (int i = 0; i < count; i++) {
      text = "    " + text;
   }

   std::cout << text << std::endl << std::flush;

   for (auto *child : _childrens) {
      child->print(count + 1);
   }
}

GdbMiResultRecord::GdbMiResultRecord(int resultClass, std::string &token, const std::vector<GdbMiResult *> &resultList)
    : GdbMiOutput{GDB_MI_OUTPUT_RESULT_RECORD}, _token{std::move(token)}, _resultClass{resultClass} {
   _childrens = resultList;
}

GdbMiResultRecord::~GdbMiResultRecord() {
   for (auto *child : _childrens) {
      delete child;
   }
}

void GdbMiResultRecord::print() {
   for (auto *child : _childrens) {
      child->print();
   }
}

GdbMiOutOfBandRecord::GdbMiOutOfBandRecord(int type) : GdbMiOutput{GDB_MI_OUTPUT_OOB_RECORD}, _type{type} {}

GdbMiOutOfBandRecord::~GdbMiOutOfBandRecord() = default;

void GdbMiOutOfBandRecord::print() {}

GdbMiAsyncRecord::GdbMiAsyncRecord() : GdbMiOutOfBandRecord{OOB_RECORD_TYPE_ASYNC} {};

GdbMiAsyncRecord::GdbMiAsyncRecord(int asyncType, std::string &token, int asyncClass, const std::vector<GdbMiResult *> &resultList)
    : GdbMiOutOfBandRecord{OOB_RECORD_TYPE_ASYNC}, _token{std::move(token)}, _asyncClass{asyncClass}, _asyncType{asyncType} {
   _childrens = resultList;
}

GdbMiAsyncRecord::~GdbMiAsyncRecord() {
   for (auto *r : _childrens) {
      delete r;
   }
}

auto GdbMiAsyncRecord::asyncType() const -> int { return _asyncType; }

auto GdbMiAsyncRecord::asyncType(int type) -> GdbMiAsyncRecord & {
   _asyncType = type;
   return *this;
}

auto GdbMiAsyncRecord::asyncClass() const -> int { return _asyncClass; }

auto GdbMiAsyncRecord::asyncClass(int type) -> GdbMiAsyncRecord & {
   _asyncClass = type;
   return *this;
}

auto GdbMiAsyncRecord::token(std::string &token) -> GdbMiAsyncRecord & {
   _token = token;
   return *this;
}

auto GdbMiAsyncRecord::token() const -> std::string { return _token; }

void GdbMiAsyncRecord::print() {
   for (auto *child : _childrens) {
      child->print();
   }
}

GdbMiStreamRecord::GdbMiStreamRecord(int streamType) : GdbMiOutOfBandRecord{OOB_RECORD_TYPE_STREAM}, _streamType{streamType} {}

GdbMiStreamRecord::GdbMiStreamRecord(int streamType, std::string &data) : GdbMiOutOfBandRecord{OOB_RECORD_TYPE_STREAM}, _streamType{streamType}, _data{std::move(data)} {}
void GdbMiStreamRecord::print() { std::cout << _streamType << " => " << _data << std::endl; }

GdbMiOutput::GdbMiOutput(int type) : _type{type} {}

GdbMiOutput::~GdbMiOutput() {}

void GdbMiOutput::print() {}

auto GdbMiAsyncRecord::getAsyncClass(const std::string &asyncStr) -> int {
   int returnCode = -1;

   if ("stopped" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_STOPPED;
   } else if ("running" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_RUNNING;
   } else if ("thread-group-added" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_THREAD_GROUP_ADDED;
   } else if ("thread-group-removed" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_THREAD_GROUP_REMOVED;
   } else if ("thread-group-started" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_THREAD_GROUP_STARTED;
   } else if ("thread-group-exited" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_THREAD_GROUP_EXITED;
   } else if ("thread-created" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_THREAD_CREATED;
   } else if ("thread-exited" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_THREAD_EXITED;
   } else if ("thread-selected" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_THREAD_SELECTED;
   } else if ("library-loaded" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_LIBRARY_LOADED;
   } else if ("library-unloaded" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_LIBRARY_UNLOADED;
   } else if ("traceframe-changed" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_TRACEFRAME_CHANGED;
   } else if ("tsv-created" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_TSV_CREATED;
   } else if ("tsv-modified" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_TSV_MODIFIED;
   } else if ("tsv-deleted" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_TSV_DELETED;
   } else if ("breakpoint-created" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_BREAKPOINT_CREATED;
   } else if ("breakpoint-modified" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_BREAKPOINT_MODIFIED;
   } else if ("breakpoint-deleted" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_BREAKPOINT_DELETED;
   } else if ("record-started" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_RECORD_STARTED;
   } else if ("record-stopped" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_RECORD_STOPPED;
   } else if ("cmd-param-changed" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_CMD_PARAM_CHANGED;
   } else if ("memory-changed" == asyncStr) {
      returnCode = GdbMiAsyncRecord::ASYNC_CLASS_MEMORY_CHANGED;
   }
   return returnCode;
}

auto GdbMiResultRecord::getResultClass(const std::string &resultClass) -> int {
   if (resultClass == "done") {
      return (int)GdbMiResultRecord::RESULT_CLASS_DONE;
   }
   if (resultClass == "running") {
      return (int)GdbMiResultRecord::RESULT_CLASS_RUNNING;
   }
   if (resultClass == "connected") {
      return (int)GdbMiResultRecord::RESULT_CLASS_CONNECTED;
   }
   if (resultClass == "error") {
      return (int)GdbMiResultRecord::RESULT_CLASS_ERROR;
   }
   if (resultClass == "exit") {
      return (int)GdbMiResultRecord::RESULT_CLASS_EXIT;
   }
   return -1;
}
