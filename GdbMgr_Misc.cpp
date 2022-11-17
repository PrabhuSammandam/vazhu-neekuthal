#include "GdbMgr.h"

/*****************************************************************************************************/
// Miscellaneous GDB/MI Commands
auto GdbMgr::gdbSet(const std::string &variable) -> bool { return doneResultCommand("-gdb-set " + variable); }

auto GdbMgr::gdbInferiorTtySet(const std::string &tty) -> bool { return doneResultCommand("-inferior-tty-set " + tty); }

/*
^done,OSDataTable={nr_rows="190",nr_cols="4",
hdr=[{width="10",alignment="-1",col_name="col0",colhdr="pid"},
     {width="10",alignment="-1",col_name="col1",colhdr="user"},
     {width="10",alignment="-1",col_name="col2",colhdr="command"},
     {width="10",alignment="-1",col_name="col3",colhdr="cores"}],
body=[item={col0="1",col1="root",col2="/sbin/init",col3="0"},
      item={col0="2",col1="root",col2="[kthreadd]",col3="1"},
      item={col0="3",col1="root",col2="[ksoftirqd/0]",col3="0"},
      ...
      item={col0="26446",col1="stan",col2="bash",col3="0"},
      item={col0="28152",col1="stan",col2="bash",col3="1"}]}
(gdb)*/

auto GdbMgr::gdbInfoOsProcesses(OsInfoProcessesList_t &processList) -> bool {
   writeCommandBlock("-info-os processes");

   auto isError = _gdbMiOutput->isErrorResult();

   if (!isError) {
      _gdbMiOutput->print();
      auto *dataTableResult = _gdbMiOutput->findResult("OSDataTable");

      if (dataTableResult != nullptr && dataTableResult->isTuple()) {
         auto *bodyResult = dataTableResult->findResult("body");

         if ((bodyResult != nullptr) && bodyResult->isList()) {
            for (auto *node : bodyResult->_childrens) {
               auto processName = node->asStr("col2");
               auto processPid  = node->asInt("col0");
               processList.push_back(GdbOsInfoProcesses{processName, processPid});
            }
         }
      }
   }
   deleteResultRecord();
   return !isError;
}
