#include "GdbMgr.h"
#include <cstdio>

/******************************************************************************************************/
/* Variable Objects */
void GdbMgr::gdbEnablePrettyPrinting() { doneResultCommand("-enable-pretty-printing"); }

// 00000010^done,name="w0",numchild="0",value="1",type="int",has_more="0"
void GdbMgr::gdbVarCreate(const std::string &varName) {
   writeCommandBlock(wxString::Format("-var-create L%d @ %s", getNextWatchId(), varName).ToStdString());

   if (!_gdbMiOutput->isErrorResult()) {
      wxString watchId       = _gdbMiOutput->asStr("name");
      wxString variableValue = _gdbMiOutput->asStr("value");
      wxString variableType  = _gdbMiOutput->asStr("type");
      int      numChild      = _gdbMiOutput->asInt("numchild");

      auto *newVariableWatch = new VarWatch{watchId, varName, variableValue, variableType, numChild > 0};

      _localVariableList.push_back(newVariableWatch);
   }
   deleteResultRecord();
}

auto GdbMgr::gdbVarDelete(const std::string &varObjId, bool isDeleteOnlyChild) -> bool {
   auto cmd = "-var-delete " + std::string{(isDeleteOnlyChild ? "-c" : "")} + " " + varObjId;
   return doneResultCommand(cmd);
}

/*
-var-list-children n
^done,numchild=n,children=[child={name=name,exp=exp, numchild=n,type=type},(repeats N times)]
(gdb)
*/

auto GdbMgr::gdbVarListChildren(const std::string &varObjId, VariableWatchList_t &varList) -> bool {
   writeCommandBlock("-var-list-children --all-values " + std::string{'"'} + varObjId + std::string{'"'});

   if (!_gdbMiOutput->isErrorResult()) {
      auto *childrenResult = _gdbMiOutput->findResult("children");

      if ((childrenResult != nullptr) && childrenResult->isList()) {
         for (auto *child : childrenResult->_childrens) {
            std::string name        = child->asStr("name");
            VarWatch   *varWatchObj = getVarWatchInfo(name);

            if (varWatchObj == nullptr) {
               varWatchObj = new VarWatch(name, child->asStr("exp"), child->asStr("value"), child->asStr("type"), child->asInt("numchild") > 0);
               varWatchObj->setParentWatchId(varObjId);
               varWatchObj->setIsDynamic(child->asInt("dynamic") != 0);
               varWatchObj->setDisplayHint(child->asStr("displayhint"));
               varList.push_back(varWatchObj);
            }
         }
      }
   }
   deleteResultRecord();
   return true;
}

/*
-var-update --all-values var1
^done,changelist=[{name="var1",value="3",in_scope="true",type_changed="false"}]
(gdb)*/
auto GdbMgr::gdbVarUpdate() -> bool { return true; }

auto GdbMgr::gdbVarAssign(const std::string &name, const std::string &expression) -> bool { return doneResultCommand("-var-assign " + name + " " + expression); }
