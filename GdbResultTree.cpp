#include "GdbResultTree.h"

auto Tree::getString(const wxString &path) const -> wxString { return m_root.getChildDataString(path); }

auto Tree::getInt(const wxString &path, int defaultValue) const -> int { return m_root.getChildDataInt(path, defaultValue); }

auto Tree::getLongLong(const wxString &path) const -> long long { return m_root.getChildDataLongLong(path); }

auto Tree::getChildAt(int idx) -> TreeNode * { return m_root.getChild(idx); };

auto Tree::getRootChildCount() const -> int { return m_root.getChildCount(); };

auto Tree::findChild(const wxString &path) const -> TreeNode * { return m_root.findChild(path); }

auto Tree::getRoot() -> TreeNode * { return &m_root; };

void Tree::copy(const Tree &other) { m_root.copy(other.m_root); }

void Tree::removeAll() { m_root.removeAll(); }

void Tree::dump() { m_root.dump(); };

TreeNode::TreeNode(const wxString &name) : m_name{name} {}

auto TreeNode::findChild(const wxString &path) const -> TreeNode * {
   wxString childName;
   wxString restPath;
   int      indexPos = 0;

   // Find the seperator in the string
   indexPos = path.Index('/');
   if (indexPos == 0) {
      return findChild(path.Mid(1));
   }

   // Get the first child name
   if (indexPos == -1) {
      childName = path;
   } else {
      childName = path.Left(indexPos);
      restPath  = path.Mid(indexPos + 1);
   }

   if (childName.StartsWith('#')) {
      wxString numStr = childName.Mid(1);
      int      idx    = atoi(numStr.ToAscii()) - 1;

      if (0 <= idx && idx < getChildCount()) {
         TreeNode *child = getChild(idx);
         if (restPath.IsEmpty()) {
            return child;
         }
         return child->findChild(restPath);
      }
   } else {
      TreeNode *child = nullptr;

      auto foundIter = m_childMap.find(childName);
      if (foundIter != m_childMap.end()) {
         child = foundIter->second;
         assert(child->m_name == childName);
         if (restPath.IsEmpty()) {
            return child;
         }
         return child->findChild(restPath);

      } // Look for the child
      for (int u = 0; u < getChildCount(); u++) {
         child = getChild(u);

         if (child->getName() == childName) {
            if (restPath.IsEmpty()) {
               return child;
            }
            return child->findChild(restPath);
         }
      }
   }

   return nullptr;
}

void TreeNode::addChild(TreeNode *child) {
   child->m_parent = this;

   m_children.push_back(child);
   m_childMap[child->m_name] = child;
}

auto TreeNode::getChild(int i) const -> TreeNode * { return m_children[i]; };

auto TreeNode::getChildCount() const -> int { return static_cast<int>(m_children.size()); };

auto TreeNode::getData() const -> wxString { return m_data; };

auto TreeNode::getDataInt(int defaultValue) const -> int {
   long val = 0;
   if (m_data.ToLong(&val)) {
      return static_cast<int>(val);
   }
   return defaultValue;
}

auto TreeNode::getChildDataString(const wxString &childName) const -> wxString {
   TreeNode *child = findChild(childName);
   if (child != nullptr) {
      return child->m_data;
   }
   return "";
}

auto TreeNode::getChildDataInt(const wxString &path, int defaultValue) const -> int {
   TreeNode *child = findChild(path);
   if (child != nullptr) {
      return child->getDataInt(defaultValue);
   }
   return defaultValue;
}

auto TreeNode::getChildDataLongLong(const wxString &path, long long defaultValue) const -> long long {
   TreeNode *child = findChild(path);
   if (child != nullptr) {
      return stringToLongLong(child->m_data);
   }
   return defaultValue;
}

void TreeNode::setData(const wxString &data) { m_data = data; };

void TreeNode::dump() { dump(0); }

auto TreeNode::getName() const -> wxString { return m_name; };

void TreeNode::removeAll() {
   for (auto *node : m_children) {
      delete node;
   }
   m_children.clear();
}

void TreeNode::copy(const TreeNode &other) {
   // Remove all children
   removeAll();

   // Set name and data
   m_name = other.m_name;
   m_data = other.m_data;

   // Copy all children
   for (auto *otherNode : other.m_children) {
      auto *thisNode = new TreeNode;
      thisNode->copy(*otherNode);

      addChild(thisNode);
   }
}

auto TreeNode::stringToLongLong(const char *str) -> long long {
   long long num  = 0;
   wxString  str2 = str;
   str2.Replace('_', "");
   str2.ToLongLong(&num, 0);
   return num;
}
