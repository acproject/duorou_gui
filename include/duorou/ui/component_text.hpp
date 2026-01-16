#pragma once

#include <duorou/ui/base_render.hpp>

#include <algorithm>
#include <string>

namespace duorou::ui {

inline ViewNode Text(std::string value) {
  auto b = view("Text");
  b.prop("value", std::move(value));
  return std::move(b).build();
}

inline bool measure_leaf_text(const ViewNode &node, ConstraintsF constraints,
                              SizeF &out) {
  if (node.type != "Text") {
    return false;
  }
  const auto font_size = prop_as_float(node.props, "font_size", 16.0f);
  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto char_w = font_size * 0.5f;
  const auto line_h = font_size * 1.2f;
  const auto text = prop_as_string(node.props, "value", "");
  const auto w = static_cast<float>(text.size()) * char_w + padding * 2.0f;
  const auto h = line_h + padding * 2.0f;
  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  return true;
}

inline bool emit_render_ops_text(const ViewNode &v, const LayoutNode &l,
                                std::vector<RenderOp> &out) {
  if (v.type != "Text") {
    return false;
  }
  const auto text = prop_as_string(v.props, "value", "");
  const auto font_px = prop_as_float(v.props, "font_size", 16.0f);
  const auto color = prop_as_color(v.props, "color", ColorU8{255, 255, 255, 255});
  out.push_back(DrawText{l.frame, text, color, font_px});
  return true;
}

} // namespace duorou::ui
