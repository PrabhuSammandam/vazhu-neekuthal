#pragma once

#include <cstdint>
#include <iostream>
#include <wx/event.h>

const uint8_t KEY_MATCH_IN_PROGRESS = 0;
const uint8_t KEY_MATCH_COMPLETED   = 1;
const uint8_t KEY_NOT_MATCHED       = 2;

class jaeEventFunctor {
 public:
   virtual ~jaeEventFunctor() = default;

   virtual void operator()(wxEvtHandler *, wxEvent &) = 0;
   virtual auto GetEvtHandler() const -> wxEvtHandler *;
   virtual auto GetEvtMethod() const -> wxEventFunction;
   virtual auto Check(uint32_t keyCode) -> uint8_t = 0;
   virtual void ResetIndex()                       = 0;
};

template <typename Class, typename EventArg> //
class KeyMap : public jaeEventFunctor {
 public:
   KeyMap(std::vector<uint32_t> keys, void (Class::*method)(EventArg &), wxEvtHandler *handler) : _keys{std::move(keys)}, _handler{handler}, _method{method} {}

   void operator()(wxEvtHandler *handler, wxEvent &event) override {
      Class *realHandler = nullptr;

      if (_handler == nullptr) {
         realHandler = static_cast<Class *>(handler);
      } else {
         realHandler = static_cast<Class *>(_handler);
      }

      // the real (run-time) type of event is EventClass and we checked in
      // the ctor that EventClass can be converted to EventArg, so this cast
      // is always valid
      (realHandler->*_method)(static_cast<EventArg &>(event));
   }

   auto Check(uint32_t keyCode) -> uint8_t override {
      std::cout << "Check for " << keyCode << " in [";
      for (auto loop_i : _keys) {
         std::cout << loop_i << ",";
      }
      std::cout << "]" << std::endl;

      if (_keys.at(_idx) == keyCode) {
         if (_idx == _keys.size() - 1) {
            Class         *realHandler = nullptr;
            wxCommandEvent temp_event;

            if (_handler != nullptr) {
               realHandler = static_cast<Class *>(_handler);
               (realHandler->*_method)(static_cast<EventArg &>(temp_event));
            }
            return KEY_MATCH_COMPLETED;
         }
         _idx++;
         return KEY_MATCH_IN_PROGRESS;
      }
      return KEY_NOT_MATCHED;
   }

   void ResetIndex() override { _idx = 0; }
   auto GetEvtHandler() const -> wxEvtHandler * override { return _handler; }
   auto GetEvtMethod() const -> wxEventFunction override { return static_cast<void (wxEvtHandler::*)(EventArg &)>(_method); }

   int                   _idx = 0;
   std::vector<uint32_t> _keys;
   wxEvtHandler         *_handler;
   void (Class::*_method)(EventArg &);
};

class KeyMapContainer {
 public:
   KeyMapContainer() {}
};

class KeyProcessor {
 public:
   KeyProcessor();

   void fixMaps() {}
   void addMinorKeyMap() {}
   void addGlobalKeyMap(jaeEventFunctor *keyMap);
   auto Process(uint32_t keyCode) -> bool;

   bool                           _inProgress = false;
   std::vector<jaeEventFunctor *> _globalKeyMaps;
   std::vector<jaeEventFunctor *> _curKeyMaps;
};

const uint32_t JAE_CONTROL_KEY_MASK = 0x00'01'00'00U;
const uint32_t JAE_ALT_KEY_MASK     = 0x00'02'00'00U;
const uint32_t JAE_SHIFT_KEY_MASK   = 0x00'04'00'00U;
