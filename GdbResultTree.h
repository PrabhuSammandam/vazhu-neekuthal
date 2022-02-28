#pragma once

#include "wx/string.h"

class TreeNode {
 public:
   TreeNode() = default;
   explicit TreeNode(const wxString &name);
   virtual ~TreeNode() = default;

   auto findChild(const wxString &path) const -> TreeNode *;
   void addChild(TreeNode *child);
   auto getChild(int i) const -> TreeNode *;
   auto getChildCount() const -> int;
   auto getData() const -> wxString;
   auto getDataInt(int defaultValue = 0) const -> int;
   auto getChildDataString(const wxString &childName) const -> wxString;
   auto getChildDataInt(const wxString &path, int defaultValue = 0) const -> int;
   auto getChildDataLongLong(const wxString &path, long long defaultValue = 0) const -> long long;
   void setData(const wxString &data);
   void dump();
   auto getName() const -> wxString;
   void removeAll();
   void copy(const TreeNode &other);

 private:
   TreeNode(const TreeNode & /*unused*/){};
   void dump(int parentCnt) {}

   static auto stringToLongLong(const char *str) -> long long;

   TreeNode *                               m_parent{};
   wxString                                 m_name{};
   wxString                                 m_data{};
   std::vector<TreeNode *>                  m_children{};
   std::unordered_map<wxString, TreeNode *> m_childMap{};
};

class Tree {
 public:
   Tree() = default;

   auto getString(const wxString &path) const -> wxString;
   auto getInt(const wxString &path, int defaultValue = 0) const -> int;
   auto getLongLong(const wxString &path) const -> long long;
   auto getChildAt(int idx) -> TreeNode *;
   auto getRootChildCount() const -> int;
   auto findChild(const wxString &path) const -> TreeNode *;
   auto getRoot() -> TreeNode *;
   void copy(const Tree &other);
   void removeAll();
   void dump();

 private:
   Tree(const Tree & /*unused*/){};

   TreeNode m_root;
};
