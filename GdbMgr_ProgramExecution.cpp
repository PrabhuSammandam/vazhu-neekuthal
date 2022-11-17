#include "GdbMgr.h"

/*****************************************************************************************************/
/* Program Execution */
auto GdbMgr::gdbExecContinue() -> bool { return simpleResultCommand("-exec-continue", GdbMiResultRecord::RESULT_CLASS_RUNNING); }

auto GdbMgr::gdbExecFinish() -> bool { return simpleResultCommand("-exec-finish", GdbMiResultRecord::RESULT_CLASS_RUNNING); }

auto GdbMgr::gdbExecInterrupt() -> bool { return simpleResultCommand("-exec-interrupt", GdbMiResultRecord::RESULT_CLASS_DONE); }

auto GdbMgr::gdbExecJump(const std::string &location) -> bool { return simpleResultCommand("-exec-jump " + location, GdbMiResultRecord::RESULT_CLASS_RUNNING); }

auto GdbMgr::gdbExecNext() -> bool { return simpleResultCommand("-exec-next", GdbMiResultRecord::RESULT_CLASS_RUNNING); }

auto GdbMgr::gdbExecNextInstruction() -> bool { return simpleResultCommand("-exec-next-instruction", GdbMiResultRecord::RESULT_CLASS_RUNNING); }

auto GdbMgr::gdbExecReturn() -> bool { return simpleResultCommand("-exec-return", GdbMiResultRecord::RESULT_CLASS_DONE); }

auto GdbMgr::gdbExecRun(bool isBreakAtMain) -> bool { return simpleResultCommand("-exec-run " + std::string{(isBreakAtMain ? "--start" : "")}, GdbMiResultRecord::RESULT_CLASS_RUNNING); }

auto GdbMgr::gdbExecStep() -> bool { return simpleResultCommand("-exec-step", GdbMiResultRecord::RESULT_CLASS_RUNNING); }

auto GdbMgr::gdbExecStepInstruction() -> bool { return simpleResultCommand("-exec-step-instruction", GdbMiResultRecord::RESULT_CLASS_RUNNING); }

auto GdbMgr::gdbExecUntil(const std::string &location) -> bool { return simpleResultCommand("-exec-until " + location, GdbMiResultRecord::RESULT_CLASS_RUNNING); }
