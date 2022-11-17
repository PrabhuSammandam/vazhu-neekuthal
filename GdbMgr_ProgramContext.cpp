#include "GdbMgr.h"

/*****************************************************************************************************/
/* Program Context */

auto GdbMgr::gdbExecArguments(const std::string &args) -> bool { return doneResultCommand("-exec-arguments " + args); }

auto GdbMgr::gdbEnvironmentCd(const std::string &dir) -> bool { return doneResultCommand("-environment-cd " + dir); }
