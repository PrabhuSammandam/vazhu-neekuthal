#include "GdbMgr.h"
#include <unordered_map>

/*****************************************************************************************************/
/*File Commands*/
auto GdbMgr::gdbFileExecAndSymbols(const std::string &filePath) -> bool { return doneResultCommand("-file-exec-and-symbols " + filePath); }

auto GdbMgr::gdbFileListExecSourceFile(/*struct to come*/) -> bool { return true; }

auto GdbMgr::gdbFileListExecSourceFiles(const std::string &args, SourceFileList_t &sourceFilesList) -> bool {
   writeCommandBlock("-file-list-exec-source-files");

   if (!_gdbMiOutput->isErrorResult()) {
      auto *filesResult = _gdbMiOutput->findResult("files");

      if ((filesResult != nullptr) && filesResult->isList()) {
         std::map<std::string, bool> filesMap;

         for (auto *fileObj : filesResult->_childrens) {
            if ((fileObj != nullptr) && fileObj->isTuple()) {
               auto file     = fileObj->asStr("file");
               auto fullname = fileObj->asStr("fullname");

               if (file.empty() || file[0] == '/') {
                  continue;
               }

               if (filesMap.find(fullname) != filesMap.end()) {
                  continue;
               }

               filesMap[fullname] = true;

               sourceFilesList.push_back(new GdbSourceFile{file, fullname});
               //               std::cout << "file = " << file << ", fullname = " << fullname << std::endl;
            }
         }
      }
   }
   deleteResultRecord();

   return true;
}

auto GdbMgr::gdbFileListSharedLibraries(const std::string &regExp /*struct to come*/) -> bool { return true; }

auto GdbMgr::gdbFileSymbolFile(const std::string &filePath) -> bool { return true; }
/*****************************************************************************************************/
