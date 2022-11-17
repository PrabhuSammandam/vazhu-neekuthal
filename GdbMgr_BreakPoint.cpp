#include "GdbMgr.h"

/******************************************************************************************************/
/* Breakpoint Commands */

auto GdbMgr::gdbBreakAfter(int bkptNo, int count) -> bool {
   std::array<char, 100> command{};
   std::snprintf(command.data(), 100, "-break-after %d %d", bkptNo, count);
   return simpleResultCommand(command.data(), GdbMiResultRecord::RESULT_CLASS_DONE);
}

auto GdbMgr::gdbBreakCommands(const std::string &bpNo, const std::vector<std::string> &commands) -> bool {
   std::string command;

   for (const auto &breakPoint : commands) {
      command += (breakPoint + " ");
   }
   return simpleResultCommand("-break-commands " + bpNo + command, GdbMiResultRecord::RESULT_CLASS_DONE);
}

auto GdbMgr::gdbBreakCondition(int bkptNo, const std::string &expression, bool force) -> bool {
   std::array<char, 100> command{};
   std::snprintf(command.data(), 100, "-break-condition %s %d %s", (force == true ? "--force" : ""), bkptNo, expression.data());
   return simpleResultCommand(command.data(), GdbMiResultRecord::RESULT_CLASS_DONE);
}

auto GdbMgr::gdbBreakDelete(const std::vector<std::string> &bpList) -> bool {
   std::string command;

   for (const auto &breakPoint : bpList) {
      command += (breakPoint + " ");
   }
   return simpleResultCommand("-break-delete " + command, GdbMiResultRecord::RESULT_CLASS_DONE);
}

auto GdbMgr::gdbBreakDisable(const std::vector<std::string> &bpList) -> bool {
   std::string command;

   for (const auto &breakPoint : bpList) {
      command += (breakPoint + " ");
   }
   return simpleResultCommand("-break-disable " + command, GdbMiResultRecord::RESULT_CLASS_DONE);
}

auto GdbMgr::gdbBreakEnable(const std::vector<std::string> &bpList) -> bool {
   std::string command;

   for (const auto &breakPoint : bpList) {
      command += (breakPoint + " ");
   }
   return simpleResultCommand("-break-enable " + command, GdbMiResultRecord::RESULT_CLASS_DONE);
}

/* -break-insert [ -t ] [ -h ] [ -f ] [ -d ] [ --qualified ] [ -c condition ] [ --force-condition ] [ -i ignore-count ] [ -p thread-id ] [ location ] */
auto GdbMgr::gdbBreakInsert(const std::string &location, bool isTemp, bool isHardware, bool isPending, bool isDisabled, const std::string &condition, bool isForceCondition, int ignoreCount) -> bool {
   std::array<char, 100> ignoreCountStr{};

   if (ignoreCount > 0) {
      std::snprintf(ignoreCountStr.data(), 100, "-i %d", ignoreCount);
   }

   std::array<char, 1024> c{};
   std::snprintf(c.data(), 1024, "-break-insert %s %s %s %s %s %s %s %s", (isTemp ? "-t" : ""), (isHardware ? "-h" : ""), (isPending ? "-f" : ""), (isDisabled ? "-d" : ""), condition.c_str(),
                 (isForceCondition ? "--force-condition" : ""), ignoreCountStr.data(), location.c_str());
   return simpleResultCommand(c.data(), GdbMiResultRecord::RESULT_CLASS_DONE);
}

auto createMultiBpChild(GdbMiResult *resultRecord) -> BreakPoint * {
   auto *tempBpObj       = new BreakPoint{};
   tempBpObj->_isMulti   = true;
   tempBpObj->_no        = resultRecord->asStr("number");
   tempBpObj->_isEnabled = resultRecord->asStr("enabled") == "y";
   tempBpObj->_addr      = strtoll(resultRecord->asStr("addr").c_str(), nullptr, 16);
   tempBpObj->_funcName  = resultRecord->asStr("func");
   tempBpObj->_file      = resultRecord->asStr("file");
   tempBpObj->_fullname  = resultRecord->asStr("fullname");
   tempBpObj->_line      = resultRecord->asInt("line");

   return tempBpObj;
}

auto createBreakpointObj(GdbMiResult *bpList) -> BreakPoint * {
   auto *tempBpObj       = new BreakPoint{};
   tempBpObj->_no        = bpList->asStr("number");
   tempBpObj->_isTemp    = bpList->asStr("disp") != "keep";
   tempBpObj->_isEnabled = bpList->asStr("enabled") == "y";
   tempBpObj->_origLoc   = bpList->asStr("original-location");
   tempBpObj->_times     = bpList->asInt("times");
   tempBpObj->_addr      = strtoll(bpList->asStr("addr").c_str(), nullptr, 16);
   tempBpObj->_file      = bpList->asStr("file");
   tempBpObj->_fullname  = bpList->asStr("fullname");
   tempBpObj->_funcName  = bpList->asStr("func");
   tempBpObj->_line      = bpList->asInt("line");

   return tempBpObj;
}
void createMultiBreakpointObj(GdbMiResult *bpList, std::vector<BreakPoint *> &breakpointList) {
   auto *tempBpObj          = new BreakPoint{};
   tempBpObj->_no           = bpList->asStr("number");
   tempBpObj->_isTemp       = bpList->asStr("disp") != "keep";
   tempBpObj->_isEnabled    = bpList->asStr("enabled") == "y";
   tempBpObj->_origLoc      = bpList->asStr("original-location");
   tempBpObj->_times        = bpList->asInt("times");
   tempBpObj->_addr         = 0;
   tempBpObj->_isMulti      = true;
   tempBpObj->_isMultiOwner = true;
   breakpointList.push_back(tempBpObj);

   auto *locationsResult = bpList->findResult("locations");

   if ((locationsResult != nullptr) && locationsResult->isList()) {
      for (auto *locResult : locationsResult->_childrens) {
         auto *tempBpObj = createMultiBpChild(locResult);
         breakpointList.push_back(tempBpObj);
      }
   }
}

/*
-break-list

(gdb)
-break-list
^done,BreakpointTable={nr_rows="2",nr_cols="6",hdr=[
                                                   {width="3",alignment="-1",col_name="number",colhdr="Num"}
                                                   ,{width="40",alignment="2",col_name="what",colhdr="What"}],

                                                   body=[bkpt={number="1",type="breakpoint",disp="keep",enabled="y", addr="0x000100d0"
                                                               ,func="main",file="hello.c",line="5",thread-groups=["i1"],times="0"},
                                                         bkpt={number="2",type="breakpoint",disp="keep",enabled="y",addr="0x00010114",func="foo"
                                                              ,file="hello.c",fullname="/home/foo/hello.c",line="13",thread-groups=["i1"],times="0"}
                                                        ]
                      }
(gdb)
*/
auto GdbMgr::gdbBreakList(std::vector<BreakPoint *> &breakpointList) -> bool {
   breakpointList.clear();
   writeCommandBlock("-break-list");

   auto isError = _gdbMiOutput->isErrorResult();

   if (!isError) {
      auto *breakPointTableResult = _gdbMiOutput->findResult("BreakpointTable");

      if ((breakPointTableResult != nullptr) && breakPointTableResult->isTuple()) {
         auto *bodyResult = breakPointTableResult->findResult("body");

         if ((bodyResult != nullptr) && bodyResult->isList()) {
            for (auto *breakPoint : bodyResult->_childrens) {
               if (breakPoint->_variable == "bkpt") {
                  if (breakPoint->asStr("addr") == "<MULTIPLE>") {
                     createMultiBreakpointObj(breakPoint, breakpointList);
                  } else {
                     auto *tempBpObj = createBreakpointObj(breakPoint);
                     breakpointList.push_back(tempBpObj);
                  }
               }
            }
         }
      }
   }
   deleteResultRecord();
   return !isError;
}
