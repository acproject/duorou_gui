#pragma once

#include <duorou/ui/base_node.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace duorou::ui {

struct SizeF {
  float w{};
  float h{};
};

struct RectF {
  float x{};
  float y{};
  float w{};
  float h{};
};

struct ConstraintsF {
  float max_w{};
  float max_h{};
};

struct LayoutNode {
  NodeId id{};
  std::string key;
  std::string type;
  RectF frame;
  std::vector<LayoutNode> children;
};

inline float clampf(float v, float lo, float hi) {
  return std::min(std::max(v, lo), hi);
}

inline const PropValue *find_prop(const Props &props, const std::string &key) {
  const auto it = props.find(key);
  if (it == props.end()) {
    return nullptr;
  }
  return &it->second;
}

inline float prop_as_float(const Props &props, const std::string &key,
                           float fallback) {
  const auto *pv = find_prop(props, key);
  if (!pv) {
    return fallback;
  }

  if (const auto *d = std::get_if<double>(pv)) {
    return static_cast<float>(*d);
  }
  if (const auto *i = std::get_if<std::int64_t>(pv)) {
    return static_cast<float>(*i);
  }
  if (const auto *b = std::get_if<bool>(pv)) {
    return *b ? 1.0f : 0.0f;
  }
  return fallback;
}

inline std::string prop_as_string(const Props &props, const std::string &key,
                                  std::string fallback = {}) {
  const auto *pv = find_prop(props, key);
  if (!pv) {
    return fallback;
  }
  if (const auto *s = std::get_if<std::string>(pv)) {
    return *s;
  }
  return fallback;
}

inline bool prop_as_bool(const Props &props, const std::string &key,
                         bool fallback) {
  const auto it = props.find(key);
  if (it == props.end()) {
    return fallback;
  }
  if (const auto *b = std::get_if<bool>(&it->second)) {
    return *b;
  }
  if (const auto *i = std::get_if<std::int64_t>(&it->second)) {
    return *i != 0;
  }
  if (const auto *d = std::get_if<double>(&it->second)) {
    return *d != 0.0;
  }
  return fallback;
}

inline SizeF apply_explicit_size(const ViewNode &node, ConstraintsF constraints,
                                 SizeF s) {
  if (find_prop(node.props, "width")) {
    const auto w = prop_as_float(node.props, "width", s.w);
    s.w = clampf(w, 0.0f, constraints.max_w);
  }
  if (find_prop(node.props, "height")) {
    const auto h = prop_as_float(node.props, "height", s.h);
    s.h = clampf(h, 0.0f, constraints.max_h);
  }
  return s;
}

} // namespace duorou::ui

