#include "GdbMgr.h"

/* Target Manipulation Commands */

auto GdbMgr::gdbTargetAttach(int pid) -> bool {
   std::array<char, 100> command{};
   std::snprintf(command.data(), 100, "-target-attach %d", pid);

   return simpleResultCommand(command.data(), GdbMiResultRecord::RESULT_CLASS_DONE);
}

auto GdbMgr::gdbTargetDetach(int pid) -> bool {
   std::array<char, 100> command{};
   std::snprintf(command.data(), 100, "-target-detach %d", pid);

   return simpleResultCommand(command.data(), GdbMiResultRecord::RESULT_CLASS_DONE);
}

auto GdbMgr::gdbTargetDisconnect() -> bool { return simpleResultCommand("-target-disconnect ", GdbMiResultRecord::RESULT_CLASS_DONE); }

auto GdbMgr::gdbTargetSelect(const std::string &type, const std::vector<std::string> &parameters) -> bool {
   std::string paramStr = " ";

   for (const auto &param : parameters) {
      paramStr += (param + " ");
   }

   return simpleResultCommand("-target-select " + type + paramStr, GdbMiResultRecord::RESULT_CLASS_CONNECTED);
}
