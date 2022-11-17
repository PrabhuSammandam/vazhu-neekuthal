#include "GdbModels.h"
#include <algorithm>
#include <vector>

VarWatch::VarWatch(const wxString &watchId_, const wxString &name_) : m_watchId{watchId_}, m_name{name_} {}

VarWatch::VarWatch(const wxString &watchId, const wxString &varName, const wxString &varValue, const wxString &varType, bool hasChildren)
    : m_hasChildren{hasChildren}, m_watchId{watchId}, m_name{varName}, m_varType{varType}, _originalVaraibleValue(varValue) {
   setValue(varValue);
}

void VarWatch::setData(Type type, const wxVariant &data) {
   m_type = type;
   m_data = data;
}

void VarWatch::setPointerAddress(long long addr) {
   m_addressValid = true;
   m_address      = addr;
};

void VarWatch::valueFromGdbString(const wxString &data) {
   _originalVaraibleValue = data;
#if 0
    auto tokenList    = GdbMiParser::tokenizeVarString(data);
   auto originalList = tokenList;

   GdbMiParser::parseVariableData(this, &tokenList);
   std::for_each(originalList.begin(), originalList.end(), [](auto *t) { delete t; });
#endif
}

auto VarWatch::getData() const -> wxString {

   return _originalVaraibleValue;
   DispFormat fmt = _currentDisplayFormat;
   wxString   valueText;

   if (fmt == FMT_NATIVE) {
      if (m_type == TYPE_HEX_INT) {
         fmt = FMT_HEX;
      }
      if (m_type == TYPE_CHAR) {
         fmt = FMT_CHAR;
      }
   }

   if (m_type == TYPE_ENUM) {
      return m_data.GetString();
   }

   if (m_type == TYPE_CHAR || m_type == TYPE_HEX_INT || m_type == TYPE_DEC_INT) {
      if (fmt == FMT_CHAR) {
         auto c = m_data.GetChar();
         if (isprint(c) != 0) {
            valueText.sprintf("%d '%c'", (int)m_data.GetInteger(), c);
         } else {
            valueText.sprintf("%d ' '", (int)m_data.GetInteger());
         }
      } else if (fmt == FMT_BIN) {
         wxString subText;
         wxString reverseText;
         auto     val = m_data.GetLongLong();

         do {
            subText.sprintf("%d", (val & 0x1));
            reverseText = subText + reverseText;
            val         = val >> 1;
         } while (val > 0 || reverseText.length() % 8 != 0);

         for (int i = 0; i < reverseText.length(); i++) {
            valueText += reverseText[i];

            if (i % 4 == 3 && i + 1 != reverseText.length()) {
               valueText += "_";
            }
         }
         valueText = "0b" + valueText;
      } else if (fmt == FMT_HEX) {
         wxString text;
         text.sprintf("%llX", m_data.GetLongLong());

         // Prefix the string with suitable number of zeroes
         while (text.length() % 4 != 0 && text.length() > 4) {
            text = "0" + text;
         }

         if (text.length() % 2 != 0) {
            text = "0" + text;
         }

         for (int i = 0; i < text.length(); i++) {
            valueText = valueText + text[i];

            if (i % 4 == 3 && i + 1 != text.length()) {
               valueText += "_";
            }
         }
         valueText = "0x" + valueText;
      } else { // if(fmt == FMT_DEC)
         valueText = m_data.GetString();
      }
   } else if (m_type == TYPE_STRING) {
      valueText = '"' + m_data.GetString() + '"';
   } else {
      valueText = m_data.GetString();
   }

   return valueText;
}
auto VarWatch::getName() const -> wxString { return m_name; };
auto VarWatch::getWatchId() const -> wxString { return m_watchId; };
void VarWatch::setHasChildren(bool hasChildren) { m_hasChildren = hasChildren; }
auto VarWatch::hasChildren() const -> bool { return m_hasChildren; }
auto VarWatch::inScope() const -> bool { return m_inScope; };
void VarWatch::setVarType(const wxString &varType) { m_varType = varType; }
auto VarWatch::getVarType() const -> wxString { return m_varType; };
void VarWatch::setValue(const wxString &value) { valueFromGdbString(value); }
auto VarWatch::getValue() const -> wxString { return getData(); };
auto VarWatch::getPointerAddress() const -> long long { return m_address; }
auto VarWatch::hasPointerAddress() const -> bool { return m_addressValid; };
void VarWatch::setInScope(bool inScope) { m_inScope = inScope; }
auto VarWatch::getParentWatchId() const -> wxString { return m_parentWatchId; }
void VarWatch::setParentWatchId(const wxString &parentWatchId) { m_parentWatchId = parentWatchId; };
auto VarWatch::isExpanded() const -> bool { return _isExpanded; }
void VarWatch::setIsExpanded(bool isExpanded) { _isExpanded = isExpanded; }
