#pragma once

#include <duorou/ui/base_render.hpp>

#include <algorithm>
#include <string>

namespace duorou::ui {

inline ViewNode Button(std::string title) {
  auto b = view("Button");
  b.prop("title", std::move(title));
  return std::move(b).build();
}

inline bool measure_leaf_button(const ViewNode &node, ConstraintsF constraints,
                                SizeF &out) {
  if (node.type != "Button") {
    return false;
  }
  const auto font_size = prop_as_float(node.props, "font_size", 16.0f);
  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto char_w = font_size * 0.5f;
  const auto line_h = font_size * 1.2f;
  const auto title = prop_as_string(node.props, "title", "");
  const auto w =
      static_cast<float>(title.size()) * char_w + 24.0f + padding * 2.0f;
  const auto h = std::max(28.0f, line_h + 12.0f) + padding * 2.0f;
  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  return true;
}

inline bool emit_render_ops_button(const ViewNode &v, const LayoutNode &l,
                                  std::vector<RenderOp> &out) {
  if (v.type != "Button") {
    return false;
  }
  const auto pressed = prop_as_bool(v.props, "pressed", false);
  out.push_back(DrawRect{l.frame, pressed ? ColorU8{120, 120, 120, 255}
                                          : ColorU8{80, 80, 80, 255}});
  const auto title = prop_as_string(v.props, "title", "");
  const auto font_px = prop_as_float(v.props, "font_size", 16.0f);
  out.push_back(DrawText{l.frame, title, ColorU8{255, 255, 255, 255}, font_px});
  return true;
}

} // namespace duorou::ui

