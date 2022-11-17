#include "GdbMgr.h"
#include <cstdio>

/******************************************************************************************************/
/* Stack Manipulation Commands */
auto GdbMgr::gdbEnableFrameFilters() -> bool { return simpleResultCommand("-enable-frame-filters", GdbMiResultRecord::RESULT_CLASS_DONE); }

/*
-stack-info-frame

-stack-info-frame
^done,frame={level="1",addr="0x0001076c",func="callee3",file="../../../devo/gdb/testsuite/gdb.mi/basics.c"
             ,fullname="/home/foo/bar/devo/gdb/testsuite/gdb.mi/basics.c",line="17",arch="i386:x86_64"}
(gdb)
*/
auto GdbMgr::gdbStackInfoFrame(struct StackFrameEntry &frameEntry) -> bool {
   writeCommandBlock("-stack-info-frame");

   auto isError = _gdbMiOutput->isErrorResult();

   if (!isError) {
      auto *frameResult = _gdbMiOutput->findResult("frame");

      if ((frameResult != nullptr) && frameResult->isTuple()) {
         frameEntry._level         = frameResult->asInt("level");
         frameEntry.m_functionName = frameResult->asStr("func");
         frameEntry.m_sourcePath   = frameResult->asStr("fullname");
         frameEntry.m_line         = frameResult->asInt("line");
      }
   }
   deleteResultRecord();
   return !isError;
}

/*
-stack-info-depth [ max-depth ]

-stack-info-depth
^done,depth="12"
(gdb)
-stack-info-depth 4
^done,depth="4"
(gdb)
*/
auto GdbMgr::gdbStackInfoDepth(int &stackDepth) -> bool {
   writeCommandBlock("-stack-info-depth");
   if (_gdbMiOutput->_resultClass == GdbMiResultRecord::RESULT_CLASS_DONE) {
      stackDepth = _gdbMiOutput->asInt("depth");
   }
   deleteResultRecord();
   return true;
}

/*
-stack-list-variables [ --no-frame-filters ] [ --skip-unavailable ] print-values

-stack-list-variables --thread 1 --frame 0 --all-values
^done,variables=[{name="x",value="11"},{name="s",value="{a = 1, b = 2}"}]
(gdb)
*/
auto GdbMgr::gdbStackListVariables() -> std::vector<std::string> {
   std::vector<std::string> varList{};

   writeCommandBlock("-stack-list-variables --simple-values");

   auto isError = _gdbMiOutput->isErrorResult();

   if (!isError) {
      auto *variableresult = _gdbMiOutput->findResult("variables");

      if ((variableresult != nullptr) && variableresult->isList()) {
         for (auto *result : variableresult->_childrens) {
            auto variableName = result->asStr("name");
            varList.push_back(variableName);
         }
      }
   }
   deleteResultRecord();
   return varList;
}

void GdbMgr::gdbStackListVariables(VariableWatchList_t &varWatchList) {
   writeCommandBlock("-stack-list-variables --simple-values");

   auto isError = _gdbMiOutput->isErrorResult();

   if (!isError) {
      auto *variableresult = _gdbMiOutput->findResult("variables");

      if ((variableresult != nullptr) && variableresult->isList()) {
         for (auto *result : variableresult->_childrens) {
            auto *varObj  = new VarWatch{};
            auto  varName = result->asStr("name");
            auto  varType = result->asStr("type");
            varObj->setName(varName);
            varObj->setVarType(varType);
            varWatchList.push_back(varObj);
         }
      }
   }
   deleteResultRecord();
}

/*
(gdb)
-stack-list-frames
^done,stack=
[frame={level="0",addr="0x0001076c",func="foo",
  file="recursive2.c",fullname="/home/foo/bar/recursive2.c",line="11",
  arch="i386:x86_64"},
frame={level="1",addr="0x000107a4",func="foo",
  file="recursive2.c",fullname="/home/foo/bar/recursive2.c",line="14",
  arch="i386:x86_64"}]
(gdb)*/
auto GdbMgr::gdbStackListFrames(int low_frame, int high_frame, std::vector<struct StackFrameEntry> &stackList) -> bool {
   if (low_frame < -1 || low_frame > high_frame) {
      return false;
   }

   if (high_frame == 0) {
      writeCommandBlock("-stack-list-frames");
   } else {
      char buff[100];
      std::snprintf(&buff[0], 100, "-stack-list-frames %d %d", low_frame, high_frame);
      writeCommandBlock(&buff[0]);
   }
   stackList.clear();

   auto isError = _gdbMiOutput->isErrorResult();

   if (!isError) {
      auto *stackResult = _gdbMiOutput->findResult("stack");

      if ((stackResult != nullptr) && stackResult->isList()) {
         for (auto *result : stackResult->_childrens) {
            StackFrameEntry entry;
            entry.m_functionName = result->asStr("func");
            entry.m_sourcePath   = result->asStr("fullname");
            entry.m_line         = result->asInt("line");
            entry._level         = result->asInt("level");

            stackList.push_back(entry);
         }
      }
   }
   deleteResultRecord();
   return !isError;
}

/*
-stack-select-frame 2
^done
(gdb)*/
auto GdbMgr::gdbStackSelectFrame(int frameNo) -> bool {
   char buf[100];
   std::snprintf(&buf[0], 100, "-stack-select-frame %d", frameNo);
   return doneResultCommand(&buf[0]);
}
