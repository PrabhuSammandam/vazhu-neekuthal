#if 0
         std::regex regexp(R"(\n\(gdb\) \n)");
         // finding all the match.
         for (std::sregex_iterator it = std::sregex_iterator(data.begin(), data.end(), regexp); it != std::sregex_iterator(); it++) {
            std::smatch match;
            match = *it;
            std::cout << "===================position" << match.position(0) << std::endl;
            std::cout << "===================sub string from " << lastCutPos << "to" << match.position(0) << std::endl;

            auto str = data.substr(lastCutPos, match.position(0) - lastCutPos);
            str += "\n(gdb) \n";
            std::cout << "*********************************************************************" << str;
            lines.push_back(str);
            lastCutPos = match.position(0) + 8;
            _pendingLine.clear();
         }

         std::cout << "===================lastCutPosition=" << lastCutPos << "input size=" << data.size() - 1 << std::endl;

         if (lastCutPos != (data.size() - 1)) {
            _pendingLine = data.substr(lastCutPos, data.size() - lastCutPos);
            std::cout << "-----------------------------------------------------------------------------------------------------" << std::endl;
            std::cout << "===================pending line " << _pendingLine;
            std::cout << "-----------------------------------------------------------------------------------------------------" << std::endl;
         }
#endif

// clang-format off

#define DECLARE_DEFAULT_CLASS(__x__) virtual ~__x__() = default;  \
    __x__(__x__ &other)  = delete;\
    __x__(__x__ &&other) = delete;\
    auto operator=(__x__ &&other) -> __x__ & = delete;\
    auto operator=(__x__ &other) -> __x__ & = delete;
// clang-format on

{
   for (auto partIndex = 0; partIndex < (int)tokens.size(); partIndex++) {
      thisWatchId += ((thisWatchId.empty() ? "" : ".") + tokens[partIndex]);

      wxTreeItemId foundItem = getTreeItemForWatchId(thisWatchId);
      wxTreeItemId tempTreeItem;

      if (!foundItem.IsOk()) {
         tempTreeItem = _treeListCtrl->AppendItem(rootItem, varWatchObj->getName());
         _treeListCtrl->SetItemText(tempTreeItem, 1, "");
         _treeListCtrl->SetItemText(tempTreeItem, 2, varWatchObj->getVarType());
         _treeListCtrl->SetItemData(tempTreeItem, new LocalVariableTreeItemData{thisWatchId});
         _treeListCtrl->SetItemHasChildren(tempTreeItem, (varWatchObj->hasChildren()));

         rootItem = tempTreeItem;
      } else {
         tempTreeItem = foundItem;
         rootItem     = foundItem;
      }

      if ((partIndex + 1) == (int)tokens.size()) {
         auto variablePath = getTreeItemPath(tempTreeItem);
         findOrAddIfNotFoundVariablePath(variablePath);

         auto valueString = varWatchObj->getOriginalVariabeValue();

         _treeListCtrl->SetItemHasChildren(tempTreeItem, (varWatchObj->hasChildren()));
         _treeListCtrl->SetItemText(tempTreeItem, 1, valueString);
         _treeListCtrl->SetItemTextColour(tempTreeItem, (_displayInfoMap[variablePath]._lastData != valueString) ? *wxRED : *wxBLACK);
         _displayInfoMap[variablePath]._lastData = valueString;

         std::cout << "watchid " << varWatchObj->getWatchId() << ", has children ->" << varWatchObj->hasChildren() << std::endl;

         if (_displayInfoMap[variablePath]._isExpanded && varWatchObj->hasChildren()) {
            _treeListCtrl->Expand(tempTreeItem);
         }
      }
   }
}
void GdbMgr::processBreakpointDeleted(int id) {
   auto found_itr = std::find_if(m_breakpoints.begin(), m_breakpoints.end(), [id](BreakPoint *bp) { return bp->_number == id; });

   if (found_itr != m_breakpoints.end()) {
      auto *bp = *found_itr;

      m_breakpoints.erase(found_itr);
      delete bp;

      DebuggerEvent e{wxEVT_DEBUGGER_GDB_BREAKPOINT_CHANGED};
      ProcessEvent(e);
   }
}

void GdbMgr::processBreakpointChanged(BreakPoint &bp, bool isToSave) {
   int         breakpointNo  = bp._number;
   BreakPoint *breakpointObj = nullptr;
   auto        found_itr     = std::find_if(m_breakpoints.begin(), m_breakpoints.end(), [breakpointNo](BreakPoint *bp) { return bp->_number == breakpointNo; });

   if (found_itr == m_breakpoints.end()) {
      breakpointObj = new BreakPoint(breakpointNo);
      m_breakpoints.push_back(breakpointObj);
   } else {
      breakpointObj = *found_itr;
   }
   *breakpointObj = bp;

   if (breakpointObj->_fullname.IsEmpty()) {
      wxString orgLoc = bp._origLoc;
      int      divPos = orgLoc.Find(':', true);
      if (divPos != -1) {
         breakpointObj->_fullname = orgLoc.Left(divPos);
      }
   }

   DebuggerEvent e{wxEVT_DEBUGGER_GDB_BREAKPOINT_CHANGED};
   e._isBreakpointToSave = isToSave;
   ProcessEvent(e);
}
void GdbMgr::processBreakpointChanged(Tree &tree, bool isToSave) {
   TreeNode *rootNode = tree.findChild("bkpt");

   if (rootNode == nullptr) {
      return;
   }

   int         breakpointNo  = rootNode->getChildDataInt("number");
   BreakPoint *breakpointObj = nullptr;
   auto        found_itr     = std::find_if(m_breakpoints.begin(), m_breakpoints.end(), [breakpointNo](BreakPoint *bp) { return bp->m_number == breakpointNo; });

   if (found_itr == m_breakpoints.end()) {
      breakpointObj = new BreakPoint(breakpointNo);
      m_breakpoints.push_back(breakpointObj);
   } else {
      breakpointObj = *found_itr;
   }

   breakpointObj->m_lineNo   = rootNode->getChildDataInt("line");
   breakpointObj->m_fullname = rootNode->getChildDataString("fullname");

   // We did not receive 'fullname' from gdb.
   // Lets try original-location instead...
   if (breakpointObj->m_fullname.IsEmpty()) {
      wxString orgLoc = rootNode->getChildDataString("original-location");
      int      divPos = orgLoc.Find(':', true);
      if (divPos != -1) {
         breakpointObj->m_fullname = orgLoc.Left(divPos);
      }
   }
   breakpointObj->m_funcName = rootNode->getChildDataString("func");
   breakpointObj->m_addr     = rootNode->getChildDataLongLong("addr");
   breakpointObj->_times     = rootNode->getChildDataInt("times");

   if (rootNode->findChild("enabled") != nullptr) {
      auto enabled              = rootNode->getChildDataString("enabled");
      breakpointObj->_isEnabled = enabled == "y";
   }

   if (rootNode->findChild("disp") != nullptr) {
      auto disp              = rootNode->getChildDataString("disp");
      breakpointObj->_isTemp = disp == "del";
   }

   DebuggerEvent e{wxEVT_DEBUGGER_GDB_BREAKPOINT_CHANGED};
   e._isBreakpointToSave = false;
   ProcessEvent(e);
}

auto DebugCmdHandlerGetFiles::processResponse(GdbResponse *response) -> bool {
   Tree                              &resultData = response->tree;
   std::unordered_map<wxString, bool> fileLookup;

   // Clear the old list
   for (auto *sourceFile : _gdbMgr->m_sourceFiles) {
      fileLookup[sourceFile->m_fullName] = false;
      delete sourceFile;
   }
   _gdbMgr->m_sourceFiles.clear();

   // Create the new list
   for (int k = 0; k < resultData.getRootChildCount(); k++) {
      TreeNode *rootNode = resultData.getChildAt(k);
      wxString  rootName = rootNode->getName();

      if (rootName == "files") {
         for (int j = 0; j < rootNode->getChildCount(); j++) {
            TreeNode *childNode = rootNode->getChild(j);
            wxString  name      = childNode->getChildDataString("file");
            wxString  fullname  = childNode->getChildDataString("fullname");

            if (fullname.IsEmpty()) {
               continue;
            }

            SourceFile *sourceFile = nullptr;

            if (!name.Contains("<built-in>") && name[0] != '/') {
               // Already added this file?
               bool alreadyAdded = false;

               if (fileLookup.find(fullname) != fileLookup.end()) {
                  if (fileLookup[fullname] == true) {
                     alreadyAdded = true;
                  }
               } else {
                  // modified = true;
               }

               if (!alreadyAdded) {
                  fileLookup[fullname] = true;

                  sourceFile = new SourceFile;

                  sourceFile->m_name     = name;
                  sourceFile->m_fullName = fullname;

                  _gdbMgr->m_sourceFiles.push_back(sourceFile);
               }
            }
         }
      }
   }
   return true;
}

auto DebugCmdGetThreadList::processResponse(GdbResponse *response) -> bool {
   auto &tree = response->tree;
   for (int treeChildIdx = 0; treeChildIdx < tree.getRootChildCount(); treeChildIdx++) {
      TreeNode *rootNode = tree.getChildAt(treeChildIdx);
      wxString  rootName = rootNode->getName();

      if (rootName == "threads") {
         _gdbMgr->m_threadList.clear();

         // Parse the result
         for (int cIdx = 0; cIdx < rootNode->getChildCount(); cIdx++) {
            TreeNode *child    = rootNode->getChild(cIdx);
            wxString  threadId = child->getChildDataString("id");
            wxString  targetId = child->getChildDataString("target-id");
            wxString  funcName = child->getChildDataString("frame/func");
            wxString  lineNo   = child->getChildDataString("frame/line");
            wxString  details  = child->getChildDataString("details");

            if (details.IsEmpty()) {
               if (!funcName.IsEmpty()) {
                  details = "Executing " + funcName + "()";

                  if (!lineNo.IsEmpty()) {
                     details += " @ L" + lineNo;
                  }
               }
            }

            long tempLong = 0;
            threadId.ToLong(&tempLong);

            ThreadInfo tinfo;
            tinfo.m_id                        = static_cast<int>(tempLong);
            tinfo.m_name                      = targetId;
            tinfo.m_details                   = details;
            tinfo.m_func                      = funcName;
            _gdbMgr->m_threadList[tinfo.m_id] = tinfo;
         }

         DebuggerEvent e{wxEVT_DEBUGGER_GDB_THREAD_LIST_CHANGED};
         _gdbMgr->ProcessEvent(e);
      }
   }
   return true;
}

auto DebugCmdProcessPids::processResponse(GdbResponse *response) -> bool {
   auto &tree = response->tree;
   for (int treeChildIdx = 0; treeChildIdx < tree.getRootChildCount(); treeChildIdx++) {
      TreeNode *rootNode = tree.getChildAt(treeChildIdx);
      wxString  rootName = rootNode->getName();
      bool      found    = false;

      if (rootName == "OSDataTable") {
         auto *bodyRootNode = rootNode->findChild("body");

         for (int i = 0; i < bodyRootNode->getChildCount(); i++) {
            auto *itemNode    = bodyRootNode->getChild(i);
            auto  processName = itemNode->getChildDataString("col2");

            //            std::cout << "process name" << processName << std::endl;

            if (processName.Contains(_processName)) {
               auto pid = itemNode->getChildDataInt("col0");
               std::cout << "found the pid" << pid << std::endl;
               _gdbMgr->writeCommand(wxString::Format("-target-attach %d", (int)pid), nullptr);
               found = true;
               _gdbMgr->loadBreakpoints();
               break;
            }
         }
         if (!found) {
            wxMessageBox("Failed to find the process " + _processName, "Attach Process", wxOK);
         }
      }
   }
   return true;
}

auto GdbMgr::startGdbProcess(GdbSessionInfo &sessionInfo) {
#if 0
   wxString gdbCommand = sessionInfo._gdbExePath + sessionInfo._gdbArgs + " --silent --nh";
   m_goingDown         = false;
   m_gdbProcess        = ::CreateAsyncProcess(this, gdbCommand, IProcessCreateDefault | IProcessStderrEvent, sessionInfo._exeWorkingDir);

   m_gdbProcess->SetHardKill(true);

   writeCommand("-enable-pretty-printing");
   writeCommand("-gdb-set mi-async on");
#else
   std::vector<std::string> args{};
   _mi->startGdbProcess(sessionInfo._gdbExePath.ToStdString(), args);
   writeCommand(wxString::Format("-inferior-tty-set %s", _mi->getInferiorTtyName()), nullptr);
#endif
}

void GdbMgr::OnGdbProcessData(AsyncProcessEvent &e) {
   //   if ((m_gdbProcess == nullptr) || !m_gdbProcess->IsAlive()) {
   //      return;
   //   }
   const wxString &bufferRead = e.GetOutput();

   std::cout << bufferRead << std::endl;
   gdbwire_push_data(_wire, bufferRead.data(), bufferRead.size());
   return;

   wxArrayString lines = wxStringTokenize(bufferRead, "\n", wxTOKEN_STRTOK);

   if (lines.IsEmpty()) {
      return;
   }
   // Prepend the partially saved line from previous iteration to the first line of this iteration
   if (!m_gdbOutputIncompleteLine.empty()) {
      lines.Item(0).Prepend(m_gdbOutputIncompleteLine);
      m_gdbOutputIncompleteLine.Clear();
   }
   // If the last line is in-complete, remove it from the array and keep it for next iteration
   if (!bufferRead.EndsWith(wxT("\n"))) {
      m_gdbOutputIncompleteLine = lines.Last();
      lines.RemoveAt(lines.GetCount() - 1);
   }
   for (const auto &line : lines) {
      if (line.IsEmpty() == false) {
         m_gdbOutputArr.Add(line);
         if (line.Contains("done,stack=[frame") || line.Contains("^done,threads=[")) {
            continue;
         }
         std::cout << "<<" << line << std::endl;
      }
   }
   for (const auto &row : m_gdbOutputArr) {
      if (isValidGdbOutput(row)) {
         auto *resp = GdbParser(row).tokenize().parseOutput();

         if (resp->_cmdIdAvailable && _cmdHandlers.find(resp->_cmdId) != _cmdHandlers.end()) {
            auto *handler = removeCmdHandler(resp->_cmdId);

            if (handler != nullptr) {
               handler->processResponse(resp);
               delete resp;
               delete handler;
            }
         } else {
            dispatchResponse(resp);
         }
      } else {
         onTargetStreamOutput(row);
      }
   }
   m_gdbOutputArr.clear();
}

auto GdbMgr::isValidGdbOutput(wxString gdbOutputStr) -> bool {
   auto strLen    = (int)gdbOutputStr.Len();
   char firstChar = gdbOutputStr[0];

   if (firstChar == '(') {
      return true;
   }
   if (firstChar == '^') {
      return true;
   }
   if (firstChar == '*') {
      return true;
   }
   if (firstChar == '+') {
      return true;
   }
   if (firstChar == '=') {
      return true;
   }
   if (firstChar == '&' || firstChar == '~' || firstChar == '@') {
      if (strLen > 2) {
         char secondChar = gdbOutputStr[1];
         char thirdChar  = gdbOutputStr[2];

         if (secondChar == '\\' && thirdChar == '"') {
            return true;
         }
      }
      return false;
   }

   if (strLen > 9 && gdbOutputStr[8] == '^') {
      auto startResult = gdbOutputStr.SubString(9, 50);

      if (startResult.StartsWith("done") || startResult.StartsWith("running") || startResult.StartsWith("connected") || startResult.StartsWith("error") || startResult.StartsWith("exit")) {
         return true;
      }
   }
   return false;
}

void GdbMgr::dispatchResponse(GdbResponse *resp) {
   if (resp == nullptr) {
      return;
   }
   switch (resp->getType()) {
   case GdbResponse::EXEC_ASYNC_OUTPUT:
      onExecAsyncOut(resp->tree, resp->reason);
      break;
   case GdbResponse::STATUS_ASYNC_OUTPUT:
      onStatusAsyncOut(resp->tree, resp->reason);
      break;
   case GdbResponse::NOTIFY_ASYNC_OUTPUT:
      onNotifyAsyncOut(resp->tree, resp->reason);
      break;
   case GdbResponse::LOG_STREAM_OUTPUT:
      onLogStreamOutput(resp->getString());
      break;
   case GdbResponse::TARGET_STREAM_OUTPUT:
      onTargetStreamOutput(resp->getString());
      break;
   case GdbResponse::CONSOLE_STREAM_OUTPUT:
      onConsoleStreamOutput(resp->getString());
      break;
   case GdbResponse::RESULT:
      // onResult(resp->tree);
      break;
   default:
      break;
   }
   delete resp;
}

void GdbMgr::onExecAsyncOut(Tree &tree, GDB_ASYNC_CLASS ac) {
   // The program has stopped
   if (ac == GDB_ASYNC_CLASS::STOPPED) {
      m_targetState = GdbMgr::TARGET_STOPPED;

      if (m_pid == 0) {
         // CHECK writeCommand("-list-thread-groups"); // get the debuggee pid
      }

      // Get the reason
      wxString           reasonString = tree.getString("reason");
      GdbMgr::StopReason reason       = reasonString.IsEmpty() ? GdbMgr::UNKNOWN : parseReasonString(reasonString);

      if (reason == GdbMgr::BREAKPOINT_HIT || reason == GdbMgr::END_STEPPING_RANGE) {
         _currentFunctionName = tree.getString("frame/func");
         _currentFilePath     = tree.getString("frame/fullname");
      }

      if (reason == GdbMgr::EXITED_NORMALLY || reason == GdbMgr::EXITED) {
         m_targetState = GdbMgr::TARGET_FINISHED;
      }

      wxString fileName = tree.getString("frame/fullname");
      int      lineNo   = tree.getInt("frame/line");

      if (reason == GdbMgr::SIGNAL_RECEIVED) {
         wxString signalName = tree.getString("signal-name");
         if (signalName == "SIGTRAP" && m_isRemote) {
            auto e = DebuggerEvent::makeGdbStoppedEvent(reason, fileName, _currentFunctionName, _stackDepth, lineNo);
            ProcessEvent(e);
         } else {
            if (signalName == "SIGSEGV") {
               m_targetState = GdbMgr::TARGET_FINISHED;
            }
            DebuggerEvent e{wxEVT_DEBUGGER_GDB_SIGNALED};
            e._signalName = signalName;
            ProcessEvent(e);
         }
      } else {
         if (m_targetState != GdbMgr::TARGET_FINISHED) {
            // writeCommand("-thread-info", new DebugCmdGetThreadList{this});
            // writeCommand("-var-update --all-values *"); // this is for custom added watch
            writeCommand("-stack-info-depth", new GdbCmdStackInfoDepth{this});
            writeCommand("-stack-list-variables --no-values", new GdbCmdStackListVariables{this});
         }
         auto e = DebuggerEvent::makeGdbStoppedEvent(reason, fileName, _currentFunctionName, _stackDepth, lineNo);
         ProcessEvent(e);
      }

      m_currentFrameIdx = tree.getInt("frame/level");

      auto e = DebuggerEvent::makeFrameIdChangedEvent((int)m_currentFrameIdx);
      ProcessEvent(e);

   } else if (ac == GDB_ASYNC_CLASS::RUNNING) {
      m_targetState = GdbMgr::TARGET_RUNNING;
   }

   // Get the current thread
   wxString threadIdStr = tree.getString("thread-id");
   if (threadIdStr.IsEmpty() == false) {
      long threadId = 0;
      threadIdStr.ToLong(&threadId);

      auto e = DebuggerEvent::makeThreadIdChangedEvent((int)threadId);
      ProcessEvent(e);
   }

   if (m_lastTargetState != m_targetState) {
      auto e      = DebuggerEvent{wxEVT_DEBUGGER_GDB_STATE_CHANGED};
      e._newState = m_targetState;

      ProcessEvent(e);
      m_lastTargetState = m_targetState;
   }
}

void GdbMgr::onNotifyAsyncOut(Tree &tree, GDB_ASYNC_CLASS ac) {
   if (ac == GDB_ASYNC_CLASS::BREAKPOINT_DELETED) {
      int id = tree.getInt("id");
      processBreakpointDeleted(id);
   } else if (ac == GDB_ASYNC_CLASS::BREAKPOINT_MODIFIED) {
      for (int i = 0; i < tree.getRootChildCount(); i++) {
         TreeNode *rootNode = tree.getChildAt(i);
         wxString  rootName = rootNode->getName();

         if (rootName == "bkpt") {
            processBreakpointChanged(tree);
         }
      }
   } else if (ac == GDB_ASYNC_CLASS::THREAD_CREATED) {
      if (checkGdbRunningOrStarting() == false) {
         writeCommand("-thread-info", new DebugCmdGetThreadList{this});
      }
   } else if (ac == GDB_ASYNC_CLASS::LIBRARY_LOADED) {
      m_scanSources = true;
   } else if (ac == GDB_ASYNC_CLASS::THREAD_GROUP_STARTED) {
      m_pid = tree.getInt("pid");
   }
}

void GdbMgr::onConsoleStreamOutput(wxString str) {
   str.Replace("\r", "");
   wxStringTokenizer tokenizer{str, '\n'};

   while (tokenizer.HasMoreTokens()) {
      wxString text = tokenizer.GetNextToken();
      if (text.IsEmpty() /* && i + 1 == list.size() */) {
         continue;
      }

      auto e = DebuggerEvent::makeConsoleLogEvent(text);
      ProcessEvent(e);
   }
}

void GdbMgr::onTargetStreamOutput(wxString str) {}

void GdbMgr::onLogStreamOutput(wxString str) {}

void GdbMgr::onStatusAsyncOut(Tree &tree, GDB_ASYNC_CLASS ac) {}

void GdbMgr::onResult(Tree &tree) {
   for (int treeChildIdx = 0; treeChildIdx < tree.getRootChildCount(); treeChildIdx++) {
      TreeNode *rootNode = tree.getChildAt(treeChildIdx);
      wxString  rootName = rootNode->getName();

      if (rootName == "changelist") {
         for (int j = 0; j < rootNode->getChildCount(); j++) {
            TreeNode *child       = rootNode->getChild(j);
            wxString  watchId     = child->getChildDataString("name");
            bool      typeChanged = child->getChildDataString("type_changed") == "true";
            VarWatch *varWatchObj = getVarWatchInfo(watchId);

            // If the type has changed then all of the children must be removed.
            if (varWatchObj != nullptr && typeChanged) {
               wxString                varName    = varWatchObj->getName();
               std::vector<VarWatch *> removeList = getWatchChildren(*varWatchObj);

               for (auto &cidx : removeList) {
                  gdbRemoveVarWatch(cidx->getWatchId());
               }

               varWatchObj->setValue("");
               varWatchObj->setVarType(child->getChildDataString("new_type"));
               varWatchObj->setHasChildren(child->getChildDataInt("new_num_children") > 0);

               auto e = DebuggerEvent::makeVariableWatchChangedEvent(varWatchObj);
               ProcessEvent(e);
            }
            // value changed?
            else if (varWatchObj != nullptr) {
               auto     newValue   = child->getChildDataString("value");
               wxString inScopeStr = child->getChildDataString("in_scope");

               varWatchObj->setValue(newValue);
               varWatchObj->setInScope((inScopeStr == "true" || inScopeStr.IsEmpty()));

               if (varWatchObj->getValue() == "{...}" && varWatchObj->hasChildren() == false) {
                  varWatchObj->setHasChildren(true);
               }
               auto e = DebuggerEvent::makeVariableWatchChangedEvent(varWatchObj);
               ProcessEvent(e);
            }
         }
      } else if (rootName == "current-thread-id") {
         // Get the current thread
         int threadId = rootNode->getDataInt(-1);
         if (threadId != -1) {
            auto e = DebuggerEvent::makeThreadIdChangedEvent((int)threadId);
            ProcessEvent(e);
         }
      } else if (rootName == "depth") {
         _stackDepth = rootNode->getDataInt(-1);
      } else if (rootName == "frame") {
         m_currentFrameIdx = rootNode->getChildDataInt("level");

         auto e = DebuggerEvent::makeGdbStoppedEvent(GdbMgr::UNKNOWN, rootNode->getChildDataString("fullname"), _currentFunctionName, _stackDepth, rootNode->getChildDataInt("line"));
         ProcessEvent(e);

         DebuggerEvent ee{wxEVT_DEBUGGER_GDB_FRAME_VARIABLE_RESET};
         ProcessEvent(ee);

         TreeNode *argsNode = rootNode->findChild("args");

         if (argsNode != nullptr) {
            for (int i = 0; i < argsNode->getChildCount(); i++) {
               TreeNode *child = argsNode->getChild(i);

               DebuggerEvent e{wxEVT_DEBUGGER_GDB_FRAME_VARIABLE_CHANGED};
               e._variableName  = child->getChildDataString("name");
               e._variableValue = child->getChildDataString("value");
               ProcessEvent(e);
            }
         }
      }
      // Local variables?
      else if (rootName == "variables") {
         wxArrayString localVariableList;

         for (int j = 0; j < rootNode->getChildCount(); j++) {
            TreeNode *child   = rootNode->getChild(j);
            wxString  varName = child->getChildDataString("name");

            localVariableList.push_back(varName);
         }

         DebuggerEvent e{wxEVT_DEBUGGER_GDB_LOCAL_VARIABLE_CHANGED, localVariableList, _currentFilePath, _currentFunctionName, _stackDepth};
         ProcessEvent(e);
      } else if (rootName == "msg") {
         wxString message = rootNode->getData();
         auto     e       = DebuggerEvent::makeMessageEvent(message);
         ProcessEvent(e);
      } else if (rootName == "groups") {
         if (m_pid == 0 && rootNode->getChildCount() > 0) {
            TreeNode *firstChild = rootNode->getChild(0);
            m_pid                = firstChild->getChildDataInt("pid", 0);
         }
      }
   }
}

auto GdbCmdStackListFrames::processResponse(GdbResponse *response) -> bool {
   auto &tree = response->tree;

   for (int treeChildIdx = 0; treeChildIdx < tree.getRootChildCount(); treeChildIdx++) {
      TreeNode *rootNode = tree.getChildAt(treeChildIdx);
      wxString  rootName = rootNode->getName();

      if (rootName == "stack") {
         std::vector<StackFrameEntry> stackFrameList;
         for (int j = 0; j < rootNode->getChildCount(); j++) {
            const TreeNode *child = rootNode->getChild(j);

            StackFrameEntry entry;
            entry.m_functionName = child->getChildDataString("func");
            entry.m_line         = child->getChildDataInt("line");
            entry.m_sourcePath   = child->getChildDataString("fullname");
            entry._level         = child->getChildDataInt("level");

            stackFrameList.push_back(entry);
         }

         DebuggerEvent ee{wxEVT_DEBUGGER_GDB_STACK_CHANGED};
         ee._stackFrameList = stackFrameList;
         _gdbMgr->ProcessEvent(ee);

         auto e = DebuggerEvent::makeFrameIdChangedEvent(_gdbMgr->m_currentFrameIdx);
         _gdbMgr->ProcessEvent(e);
      }
   }
   return true;
}

auto DebugCmdBreakpoint::processResponse(GdbResponse *response) -> bool {
   auto &tree = response->tree;
   for (int treeChildIdx = 0; treeChildIdx < tree.getRootChildCount(); treeChildIdx++) {
      TreeNode *rootNode = tree.getChildAt(treeChildIdx);
      wxString  rootName = rootNode->getName();

      if (rootName == "bkpt") {
         _gdbMgr->processBreakpointChanged(tree, _isToSave);
      }
   }
   return true;
}

#if 0
void GdbMgr::readTokens() {
   auto *myprocess = dynamic_cast<MyProcess *>(_gdbProcess);

   wxString m_inputBuffer;

   myprocess->ReadAll(m_inputBuffer);

   if (m_inputBuffer.Contains("*stopped")) {
      int a = 0;
      a++;
   }

   auto bufferSize = m_inputBuffer.length();

   // Any characters received?
   while (!m_inputBuffer.empty()) {
      std::cout << m_inputBuffer << std::endl;

      // Newline received?
      auto subLen = m_inputBuffer.Index('\n');
      if (subLen != -1) {
         wxString row  = wxString(m_inputBuffer.Left(subLen));
         m_inputBuffer = m_inputBuffer.Mid(subLen + 1);
         bufferSize    = m_inputBuffer.length();

         if (!row.IsEmpty()) {
            std::vector<Token *> list;
            char                 firstChar = row[0];

            // clang-format off
            if (firstChar == '(' ||
                firstChar == '^' ||
                firstChar == '*' ||
                firstChar == '+' ||
                firstChar == '~' ||
                firstChar == '@' ||
                firstChar == '&' ||
                firstChar == '=')
            {
               list = tokenize(row);
               m_list.insert(m_list.end(), list.begin(), list.end());
            } else {
               onTargetStreamOutput(row);
            }
            // clang-format on
         }
      }

      // Half a line received?
      if (m_inputBuffer.IsEmpty() == false) {
         int timeout = 20;
         // Wait for the complete line to be received
         while (m_inputBuffer.Index('\n') == -1) {
            wxMilliSleep(100);
            wxString tempOutput;
            myprocess->ReadAll(tempOutput);

            if (!tempOutput.IsEmpty()) {
               m_inputBuffer += tempOutput;
            }
            timeout--;
         }
      }
   }
}
class MyProcess : public wxProcess {
 public:
   explicit MyProcess(GdbMgr *parent) : wxProcess{wxPROCESS_REDIRECT}, _parent{parent} {}
   void OnTerminate(int pid, int status) override { _parent->onGdbProcessTerminate(pid, status); }

   void write(const wxString &text) {
      wxString s(text);
      s += '\n';
      auto *out = GetOutputStream();
      out->Write(s.c_str(), s.length());
   }

   auto HasInput() -> bool {
      if (IsInputAvailable()) {
         return true;
      }
      if (IsErrorAvailable()) {
         return true;
      }
      return false;
   }

   auto HasInput(wxString &input) -> bool {
      bool hasInput = false;
      bool cont1(true);
      bool cont2(true);
      while (cont1 || cont2) {
         cont1 = false;
         cont2 = false;
         while (IsInputAvailable()) {
            wxTextInputStream tis(*GetInputStream());
            // this assumes that the output is always line buffered
            wxChar ch = tis.GetChar();
            input << ch;
            hasInput = true;
            if (ch == wxT('\n')) {
               cont1 = false;
               break;
            }
            cont1 = true;
         }

         while (IsErrorAvailable()) {
            wxTextInputStream tis(*GetErrorStream());
            // this assumes that the output is always line buffered
            wxChar ch = tis.GetChar();
            input << ch;
            hasInput = true;
            if (ch == wxT('\n')) {
               cont2 = false;
               break;
            }
            cont2 = true;
         }
      }
      return hasInput;
   }

   auto ReadAll(wxString &input) -> bool {
      bool hasInput = false;
      bool cont1(true);
      bool cont2(true);

      wxTextInputStream tis(*GetInputStream());
      wxTextInputStream tie(*GetErrorStream());
      while (cont1 || cont2) {
         cont1 = false;
         cont2 = false;
         while (IsInputAvailable()) {
            // this assumes that the output is always line buffered
            wxChar ch = tis.GetChar();
            input << ch;
            hasInput = true;
            cont1    = true;
         }

         while (IsErrorAvailable()) {
            // this assumes that the output is always line buffered
            wxChar ch = tie.GetChar();
            input << ch;
            hasInput = true;
            cont2    = true;
         }
      }
      return hasInput;
   }

   GdbMgr *_parent = nullptr;
};
#endif

#if 0
auto GdbMgr::parseStreamRecord() -> GdbResponse * {
   GdbResponse *resp = nullptr;
   Token *      tok  = nullptr;

   if (checkToken(Token::KEY_TILDE) != nullptr) {
      resp = new GdbResponse{GdbResponse::CONSOLE_STREAM_OUTPUT};
      tok  = eatToken(Token::C_STRING);
      resp->setString(tok->getString());

   } else if (checkToken(Token::KEY_SNABEL) != nullptr) {
      resp = new GdbResponse{GdbResponse::TARGET_STREAM_OUTPUT};
      tok  = eatToken(Token::C_STRING);
      resp->setString(tok->getString());

   } else if (checkToken(Token::KEY_AND) != nullptr) {
      resp = new GdbResponse{GdbResponse::LOG_STREAM_OUTPUT};
      tok  = eatToken(Token::C_STRING);
      resp->setString(tok->getString());
   }

   return resp;
}

auto GdbMgr::parseExecAsyncOutput() -> GdbResponse * {
   if (m_list.size() >= 2 && m_list.at(0)->getType() == Token::VAR && m_list.at(0)->getType() != Token::KEY_STAR) {
      return nullptr;
   }

   checkToken(Token::VAR);

   if (checkToken(Token::KEY_STAR) == nullptr) {
      return nullptr;
   }

   auto *resp = new GdbResponse{GdbResponse::EXEC_ASYNC_OUTPUT};

   parseAsyncOutput(resp, &resp->reason);

   return resp;
}

auto GdbMgr::parseNotifyAsyncOutput() -> GdbResponse * {
   if (m_list.size() >= 2 && m_list.at(0)->getType() == Token::VAR && m_list.at(0)->getType() != Token::KEY_EQUAL) {
      return nullptr;
   }
   checkToken(Token::VAR);

   if (checkToken(Token::KEY_EQUAL) == nullptr) {
      return nullptr;
   }

   auto *resp = new GdbResponse{GdbResponse::NOTIFY_ASYNC_OUTPUT};

   parseAsyncOutput(resp, &resp->reason);
   return resp;
}

auto GdbMgr::parseStatusAsyncOutput() -> GdbResponse * {
   if (m_list.size() >= 2 && m_list.at(0)->getType() == Token::VAR && m_list.at(0)->getType() != Token::KEY_PLUS) {
      return nullptr;
   }
   checkToken(Token::VAR);

   if (checkToken(Token::KEY_PLUS) == nullptr) {
      return nullptr;
   }

   auto *resp = new GdbResponse{GdbResponse::STATUS_ASYNC_OUTPUT};

   parseAsyncOutput(resp, &resp->reason);
   return resp;
}
#endif
#if 0
auto GdbMgr::parseOutOfBandRecord() -> GdbResponse * {
   GdbResponse *resp = nullptr;

   if (isTokenPending() && resp == nullptr) {
      resp = parseAsyncRecord();
   }

   if (isTokenPending() && resp == nullptr) {
      resp = parseStreamRecord();
   }

   return resp;
}

auto GdbMgr::parseAsyncRecord() -> GdbResponse * {
   GdbResponse *resp = nullptr;

   if (isTokenPending() && resp == nullptr) {
      resp = parseExecAsyncOutput();
   }

   if (isTokenPending() && resp == nullptr) {
      resp = parseStatusAsyncOutput();
   }

   if (isTokenPending() && resp == nullptr) {
      resp = parseNotifyAsyncOutput();
   }

   return resp;
}
#if 0
auto GdbMgr::parseOutOfBandRecord() -> GdbResponse * {
   GdbResponse *resp = nullptr;

   if (isTokenPending() && resp == nullptr) {
      resp = parseAsyncRecord();
   }

   if (isTokenPending() && resp == nullptr) {
      resp = parseStreamRecord();
   }

   return resp;
}

auto GdbMgr::parseAsyncRecord() -> GdbResponse * {
   GdbResponse *resp = nullptr;

   if (isTokenPending() && resp == nullptr) {
      resp = parseExecAsyncOutput();
   }

   if (isTokenPending() && resp == nullptr) {
      resp = parseStatusAsyncOutput();
   }

   if (isTokenPending() && resp == nullptr) {
      resp = parseNotifyAsyncOutput();
   }

   return resp;
}
#endif

#if 0
   if (!m_gdbOutputArr.IsEmpty()) {
      for (auto gdbOutput : m_gdbOutputArr) {
         char firstChar = gdbOutput[0];

         if (firstChar == '*') { // exec-async-output
            if (gdbOutput.StartsWith("*running")) {
            } else if (gdbOutput.StartsWith("*stopped")) {
               gdbOutput   = gdbOutput.Mid(wxString("*stopped,reason=").Len());
               auto reason = gdbOutput.SubString(0, gdbOutput.Find(",") - 1);
               gdbOutput   = gdbOutput.Mid(reason.Len() + 1); // + 1 for ,

               wxGDB_STRIP_QUOATES(reason);

               if (reason == "breakpoint-hit") {
                  while (!gdbOutput.IsEmpty()) {
                     auto key  = gdbOutput.SubString(0, gdbOutput.Find("=") - 1);
                     gdbOutput = gdbOutput.Mid(key.Len() + 1); // + 1 for =

                     if (key == "disp") {
                        auto value = gdbOutput.SubString(0, gdbOutput.Find(",") - 1);
                        gdbOutput  = gdbOutput.Mid(value.Len() + 1); // + 1 for ,
                        wxGDB_STRIP_QUOATES(value);
                        continue;
                     }
                     if (key == "bkptno") {
                        auto value = gdbOutput.SubString(0, gdbOutput.Find(",") - 1);
                        gdbOutput  = gdbOutput.Mid(value.Len() + 1); // + 1 for ,
                        wxGDB_STRIP_QUOATES(value);
                        continue;
                     }
                     if (key == "frame") {
                        auto value = gdbOutput.SubString(0, gdbOutput.Find(",") - 1);
                        gdbOutput  = gdbOutput.Mid(value.Len() + 1); // + 1 for ,
                        wxGDB_STRIP_QUOATES(value);
                        continue;
                     }
                  }
               }

               wxArrayString result = wxStringTokenize(gdbOutput, ",", wxTOKEN_STRTOK);
               for (auto i = 1; i < result.size(); i++) {
                  auto value_pair = wxStringTokenize(result[i], "=", wxTOKEN_STRTOK);

                  if (value_pair[0] == "reason") {
                     continue;
                  }
                  if (value_pair[0] == "thread-id") {
                     continue;
                  }
                  if (value_pair[0] == "stopped-threads") {
                     continue;
                  }
                  if (value_pair[0] == "core") {
                     continue;
                  }
               }
            }
         } else if (firstChar == '+') { // status-async-output
         } else if (firstChar == '=') { // notify-async-output
         } else if (firstChar == '~') { // console-stream-output
         } else if (firstChar == '@') { // target-stream-output
         } else if (firstChar == '&') { // log-stream-output
         } else if (firstChar == '^') { // result-record
            if (gdbOutput.StartsWith("^done")) {
            } else if (gdbOutput.StartsWith("^running")) {
            } else if (gdbOutput.StartsWith("^connected")) {
            } else if (gdbOutput.StartsWith("^error")) {
            } else if (gdbOutput.StartsWith("^exit")) {
            }
         }
      }
   }
#endif

#if 0
class GdbResponseListener {
 public:
   GdbResponseListener(const GdbResponseListener &other) = delete;
   GdbResponseListener(GdbResponseListener &&other)      = delete;
   auto operator=(const GdbResponseListener &other) -> GdbResponseListener & = delete;
   auto operator=(GdbResponseListener &&other) -> GdbResponseListener & = delete;
   virtual ~GdbResponseListener()                                       = default;

   virtual void onStatusAsyncOut(Tree &tree, GDB_ASYNC_RECORD_TYPE ac) = 0;
   virtual void onNotifyAsyncOut(Tree &tree, GDB_ASYNC_RECORD_TYPE ac) = 0;
   virtual void onExecAsyncOut(Tree &tree, GDB_ASYNC_RECORD_TYPE ac)   = 0;
   virtual void onResult(Tree &tree)                                   = 0;
   virtual void onConsoleStreamOutput(wxString str)                    = 0;
   virtual void onTargetStreamOutput(wxString str)                     = 0;
   virtual void onLogStreamOutput(wxString str)                        = 0;
};
#endif

#if 0
   struct gdbwire_mi_command *bpList = nullptr;

   if (gdbwire_get_mi_command(GDBWIRE_MI_BREAK_INFO, result_record, &bpList) != GDBWIRE_OK) {
      return false;
   }
   auto *bp = bpList->variant.break_info.breakpoints;

   while (bp != nullptr) {
      auto *tempBpObj = new BreakPoint{};

      if (bp->multi) {
         tempBpObj->_isMulti      = true;
         tempBpObj->_isMultiOwner = true;
         tempBpObj->_fullname     = bp->fullname;
         tempBpObj->_line         = (int)bp->line;
         tempBpObj->_funcName     = bp->func_name;
         tempBpObj->_addr         = strtoll(bp->address, nullptr, 16);
         tempBpObj->_origLoc      = bp->original_location;
         tempBpObj->_isEnabled    = bp->enabled;
         tempBpObj->_times        = (int)bp->times;
         tempBpObj->_isTemp       = bp->disposition != GDBWIRE_MI_BP_DISP_KEEP;

         breakpointList.push_back(tempBpObj);

         auto *t = bp->multi_breakpoints;

         while (t != nullptr) {
            auto *tempBpObj          = new BreakPoint{};
            tempBpObj->_isMulti      = true;
            tempBpObj->_isMultiOwner = false;

            tempBpObj->_fullname  = t->fullname;
            tempBpObj->_line      = (int)t->line;
            tempBpObj->_funcName  = t->func_name;
            tempBpObj->_addr      = strtoll(t->address, nullptr, 16);
            tempBpObj->_origLoc   = t->original_location;
            tempBpObj->_isEnabled = t->enabled;
            tempBpObj->_times     = (int)t->times;
            tempBpObj->_isTemp    = t->disposition != GDBWIRE_MI_BP_DISP_KEEP;
            breakpointList.push_back(tempBpObj);
            t = t->next;
         }
      } else if (bp->pending) {
      } else {
         tempBpObj->_number    = atoi(bp->number);
         tempBpObj->_fullname  = bp->fullname;
         tempBpObj->_line      = (int)bp->line;
         tempBpObj->_funcName  = bp->func_name;
         tempBpObj->_addr      = strtoll(bp->address, nullptr, 16);
         tempBpObj->_origLoc   = bp->original_location;
         tempBpObj->_isEnabled = bp->enabled;
         tempBpObj->_times     = (int)bp->times;
         tempBpObj->_isTemp    = bp->disposition != GDBWIRE_MI_BP_DISP_KEEP;
         breakpointList.push_back(tempBpObj);
      }

      bp = bp->next;
   }
   gdbwire_mi_command_free(bpList);
#endif
