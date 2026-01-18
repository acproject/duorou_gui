#pragma once

#include <duorou/ui/runtime.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace duorou::ui::editor {

inline ViewNode *find_node_by_key_mut(ViewNode &root, std::string_view key) {
  if (key.empty()) {
    return nullptr;
  }
  if (root.key == key) {
    return &root;
  }
  for (auto &c : root.children) {
    if (auto *r = find_node_by_key_mut(c, key)) {
      return r;
    }
  }
  return nullptr;
}

inline const ViewNode *find_node_by_key(const ViewNode &root, std::string_view key) {
  if (key.empty()) {
    return nullptr;
  }
  if (root.key == key) {
    return &root;
  }
  for (const auto &c : root.children) {
    if (const auto *r = find_node_by_key(c, key)) {
      return r;
    }
  }
  return nullptr;
}

inline bool find_path_by_key_impl(const ViewNode &root, std::string_view key,
                                 std::vector<std::size_t> &path) {
  if (key.empty()) {
    return false;
  }
  if (root.key == key) {
    return true;
  }
  for (std::size_t i = 0; i < root.children.size(); ++i) {
    path.push_back(i);
    if (find_path_by_key_impl(root.children[i], key, path)) {
      return true;
    }
    path.pop_back();
  }
  return false;
}

inline std::vector<std::size_t> find_path_by_key(const ViewNode &root,
                                                 std::string_view key) {
  std::vector<std::size_t> path;
  (void)find_path_by_key_impl(root, key, path);
  return path;
}

inline ViewNode *node_at_path_mut(ViewNode &root,
                                  const std::vector<std::size_t> &path) {
  ViewNode *cur = &root;
  for (const auto idx : path) {
    if (idx >= cur->children.size()) {
      return nullptr;
    }
    cur = &cur->children[idx];
  }
  return cur;
}

inline bool node_contains_key(const ViewNode &root, std::string_view key) {
  if (root.key == key) {
    return true;
  }
  for (const auto &c : root.children) {
    if (node_contains_key(c, key)) {
      return true;
    }
  }
  return false;
}

inline std::optional<ViewNode> take_node_by_key_mut(ViewNode &root,
                                                    std::string_view key,
                                                    std::string *parent_key_out = nullptr,
                                                    std::int64_t *index_out = nullptr) {
  if (root.key == key) {
    return std::nullopt;
  }
  for (std::size_t i = 0; i < root.children.size(); ++i) {
    auto &ch = root.children[i];
    if (ch.key == key) {
      if (parent_key_out) {
        *parent_key_out = root.key;
      }
      if (index_out) {
        *index_out = static_cast<std::int64_t>(i);
      }
      auto out = std::move(ch);
      root.children.erase(root.children.begin() + static_cast<std::ptrdiff_t>(i));
      return out;
    }
  }
  for (auto &ch : root.children) {
    if (auto r = take_node_by_key_mut(ch, key, parent_key_out, index_out)) {
      return r;
    }
  }
  return std::nullopt;
}

} // namespace duorou::ui::editor

