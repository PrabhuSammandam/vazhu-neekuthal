#include "KeyProcessor.h"

wxEventFunction jaeEventFunctor::GetEvtMethod() const { return nullptr; }
auto            jaeEventFunctor::GetEvtHandler() const -> wxEvtHandler            *{ return nullptr; }

KeyProcessor::KeyProcessor() { _curKeyMaps.reserve(100); }

void KeyProcessor::addGlobalKeyMap(jaeEventFunctor *keyMap) { _globalKeyMaps.push_back(keyMap); }

auto KeyProcessor::Process(uint32_t keyCode) -> bool {
   bool isConsumedKey = false;

   if (_inProgress) {
      bool someKeyMatched = false;
      bool completed      = false;
      auto tempKeys       = std::move(_curKeyMaps);

      _curKeyMaps.clear();

      for (auto *km : tempKeys) {
         auto matchState = km->Check(keyCode);

         if (matchState == KEY_MATCH_COMPLETED) {
            std::cout << "[INPROGRESS] Completed" << std::endl;
            completed = true;
            break;
         }

         switch (matchState) {
         case KEY_MATCH_IN_PROGRESS:
            std::cout << "[INPROGRESS] Partial" << std::endl;
            isConsumedKey  = true;
            someKeyMatched = true;
            _curKeyMaps.push_back(km);
            break;
         case KEY_NOT_MATCHED:
            std::cout << "[INPROGRESS] Not Matched" << std::endl;
            km->ResetIndex();
            break;
         default:
            break;
         }
      }

      if (completed || !someKeyMatched) {
         for (auto *km : tempKeys) {
            km->ResetIndex(); // need reset all the curently partial matched keys maps
         }
         _curKeyMaps.clear();
         _inProgress = false;
         return true; // consume key
      }
   }
   for (auto *km : _globalKeyMaps) {
      auto matchState = km->Check(keyCode);

      if (matchState == KEY_MATCH_IN_PROGRESS) {
         _inProgress = true;
         _curKeyMaps.push_back(km);
         isConsumedKey = true;
         std::cout << "[FIRST] Partial" << std::endl;
      } else if (matchState == KEY_MATCH_COMPLETED) {
         _inProgress = false;
         km->ResetIndex();
         isConsumedKey = true;
         _curKeyMaps.clear();
         std::cout << "[FIRST] Completed" << std::endl;
      }
   }
   return isConsumedKey;
}
