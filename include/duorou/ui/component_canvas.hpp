#pragma once

#include <duorou/ui/base_render.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace duorou::ui {

using CanvasDrawFn = std::function<void(RectF, std::vector<RenderOp> &)>;

inline std::uint64_t canvas_hash64(std::string_view s) {
  std::uint64_t h = 14695981039346656037ull;
  for (unsigned char c : s) {
    h ^= static_cast<std::uint64_t>(c);
    h *= 1099511628211ull;
  }
  return h;
}

inline std::mutex &canvas_registry_mutex() {
  static std::mutex m;
  return m;
}

inline std::unordered_map<std::uint64_t, CanvasDrawFn> &canvas_registry_map() {
  static std::unordered_map<std::uint64_t, CanvasDrawFn> fns;
  return fns;
}

inline void register_canvas_draw(std::uint64_t id, CanvasDrawFn fn) {
  std::lock_guard<std::mutex> lock{canvas_registry_mutex()};
  canvas_registry_map().insert_or_assign(id, std::move(fn));
}

inline bool measure_leaf_canvas(const ViewNode &node, ConstraintsF constraints,
                                SizeF &out) {
  if (node.type != "Canvas") {
    return false;
  }
  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto default_w = prop_as_float(node.props, "default_width", 200.0f);
  const auto default_h = prop_as_float(node.props, "default_height", 200.0f);
  const auto w = default_w + padding * 2.0f;
  const auto h = default_h + padding * 2.0f;
  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  return true;
}

inline std::optional<std::int64_t> prop_as_i64_opt_canvas(const Props &props,
                                                          const std::string &key) {
  const auto *pv = find_prop(props, key);
  if (!pv) {
    return std::nullopt;
  }
  if (const auto *i = std::get_if<std::int64_t>(pv)) {
    return *i;
  }
  if (const auto *d = std::get_if<double>(pv)) {
    return static_cast<std::int64_t>(*d);
  }
  return std::nullopt;
}

inline bool emit_render_ops_canvas(const ViewNode &v, const LayoutNode &l,
                                  std::vector<RenderOp> &out) {
  if (v.type != "Canvas") {
    return false;
  }
  const auto id_raw = prop_as_i64_opt_canvas(v.props, "canvas_id");
  if (!id_raw || *id_raw == 0) {
    return true;
  }

  CanvasDrawFn fn;
  {
    std::lock_guard<std::mutex> lock{canvas_registry_mutex()};
    auto &m = canvas_registry_map();
    const auto it = m.find(static_cast<std::uint64_t>(*id_raw));
    if (it != m.end()) {
      fn = it->second;
    }
  }

  if (fn) {
    fn(l.frame, out);
  }
  return true;
}

} // namespace duorou::ui
