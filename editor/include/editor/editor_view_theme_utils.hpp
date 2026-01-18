#pragma once

#include <editor/editor_view_style.hpp>

#include <duorou/ui/runtime.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace duorou::ui::editor {

inline std::string unique_name_like(const std::vector<std::string> &names,
                                    std::string base) {
  std::unordered_set<std::string> set;
  set.reserve(names.size() * 2 + 1);
  for (const auto &n : names) {
    set.insert(n);
  }
  if (!set.contains(base)) {
    return base;
  }
  for (int i = 2; i < 10000; ++i) {
    std::string cand = base + " " + std::to_string(i);
    if (!set.contains(cand)) {
      return cand;
    }
  }
  return base + " Copy";
}

inline std::string format_theme_errors(const std::vector<StyleParseError> &errs) {
  if (errs.empty()) {
    return {};
  }
  std::string out;
  const std::size_t max_items = 8;
  const std::size_t take = std::min(max_items, errs.size());
  for (std::size_t i = 0; i < take; ++i) {
    const auto &e = errs[i];
    out += "line ";
    out += std::to_string(e.line);
    out += ":";
    out += std::to_string(e.column);
    out += " ";
    out += e.message;
    if (i + 1 < take) {
      out.push_back('\n');
    }
  }
  if (errs.size() > take) {
    out += "\n...";
  }
  return out;
}

inline std::unordered_map<std::string, PropValue>
flatten_sheet(const StyleSheetModel &sheet) {
  std::unordered_map<std::string, PropValue> out;
  out.reserve(sheet.global.size() + sheet.components.size() * 16);
  for (const auto &kv : sheet.global) {
    out.insert_or_assign("Global." + kv.first, kv.second);
  }
  for (const auto &ckv : sheet.components) {
    const auto &cname = ckv.first;
    const auto &comp = ckv.second;
    for (const auto &kv : comp.props) {
      out.insert_or_assign(cname + "." + kv.first, kv.second);
    }
    for (const auto &skv : comp.states) {
      for (const auto &kv : skv.second) {
        out.insert_or_assign(cname + "." + skv.first + "." + kv.first, kv.second);
      }
    }
    for (const auto &vkv : comp.variants) {
      const auto &vname = vkv.first;
      const auto &var = vkv.second;
      for (const auto &kv : var.props) {
        out.insert_or_assign(cname + "." + vname + "." + kv.first, kv.second);
      }
      for (const auto &skv : var.states) {
        for (const auto &kv : skv.second) {
          out.insert_or_assign(cname + "." + vname + "." + skv.first + "." + kv.first,
                               kv.second);
        }
      }
    }
  }
  return out;
}

inline std::string sheet_diff_summary(const StyleSheetModel &old_sheet,
                                      const StyleSheetModel &new_sheet) {
  const auto old_map = flatten_sheet(old_sheet);
  const auto new_map = flatten_sheet(new_sheet);

  std::size_t added = 0;
  std::size_t removed = 0;
  std::size_t changed = 0;
  std::vector<std::string> changed_keys;
  changed_keys.reserve(16);

  for (const auto &kv : new_map) {
    const auto it = old_map.find(kv.first);
    if (it == old_map.end()) {
      ++added;
      if (changed_keys.size() < 8) {
        changed_keys.push_back(std::string{"+"} + kv.first);
      }
      continue;
    }
    if (it->second != kv.second) {
      ++changed;
      if (changed_keys.size() < 8) {
        changed_keys.push_back(std::string{"~"} + kv.first);
      }
    }
  }
  for (const auto &kv : old_map) {
    if (!new_map.contains(kv.first)) {
      ++removed;
      if (changed_keys.size() < 8) {
        changed_keys.push_back(std::string{"-"} + kv.first);
      }
    }
  }

  std::string out =
      "diff: +" + std::to_string(added) + " -" + std::to_string(removed) +
      " ~" + std::to_string(changed) + " (total " +
      std::to_string(new_map.size()) + ")";
  if (!changed_keys.empty()) {
    out.push_back('\n');
    out += join_str(changed_keys, "\n");
  }
  return out;
}

} // namespace duorou::ui::editor

