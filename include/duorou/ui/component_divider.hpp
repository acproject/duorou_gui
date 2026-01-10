#pragma once

#include <duorou/ui/base_render.hpp>

namespace duorou::ui {

inline ViewNode Divider() {
  auto b = view("Divider");
  return std::move(b).build();
}

inline bool measure_leaf_divider(const ViewNode &node, ConstraintsF constraints,
                                 SizeF &out) {
  if (node.type != "Divider") {
    return false;
  }
  const auto thickness = prop_as_float(node.props, "thickness", 1.0f);
  const auto w = constraints.max_w;
  const auto h = thickness;
  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  return true;
}

inline bool emit_render_ops_divider(const ViewNode &v, const LayoutNode &l,
                                   std::vector<RenderOp> &out) {
  if (v.type != "Divider") {
    return false;
  }
  const auto col = prop_as_color(v.props, "color", ColorU8{60, 60, 60, 255});
  out.push_back(DrawRect{l.frame, col});
  return true;
}

} // namespace duorou::ui

