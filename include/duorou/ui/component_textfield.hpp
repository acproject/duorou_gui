#pragma once

#include <duorou/ui/base_render.hpp>

#include <algorithm>
#include <string>

namespace duorou::ui {

inline ViewNode TextField(std::string value, std::string placeholder = {}) {
  auto b = view("TextField");
  b.prop("value", std::move(value));
  if (!placeholder.empty()) {
    b.prop("placeholder", std::move(placeholder));
  }
  return std::move(b).build();
}

inline bool measure_leaf_textfield(const ViewNode &node, ConstraintsF constraints,
                                   SizeF &out) {
  if (node.type != "TextField") {
    return false;
  }
  const auto font_size = prop_as_float(node.props, "font_size", 16.0f);
  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto char_w = font_size * 0.5f;
  const auto line_h = font_size * 1.2f;
  const auto value = prop_as_string(node.props, "value", "");
  const auto placeholder = prop_as_string(node.props, "placeholder", "");
  const auto text_len = std::max<std::size_t>(value.size(), placeholder.size());
  const auto min_w = prop_as_float(node.props, "min_width", 140.0f);
  const auto w =
      std::max(min_w,
               static_cast<float>(text_len) * char_w + padding * 2.0f + 24.0f);
  const auto h = std::max(28.0f, line_h + 12.0f) + padding * 2.0f;
  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  return true;
}

inline bool emit_render_ops_textfield(const ViewNode &v, const LayoutNode &l,
                                     std::vector<RenderOp> &out) {
  if (v.type != "TextField") {
    return false;
  }
  const auto padding = prop_as_float(v.props, "padding", 10.0f);
  const auto font_px = prop_as_float(v.props, "font_size", 16.0f);
  const auto focused = prop_as_bool(v.props, "focused", false);
  const auto value = prop_as_string(v.props, "value", "");
  const auto placeholder = prop_as_string(v.props, "placeholder", "");
  out.push_back(DrawRect{l.frame, focused ? ColorU8{55, 55, 55, 255}
                                         : ColorU8{45, 45, 45, 255}});
  const RectF tr{l.frame.x + padding, l.frame.y,
                 std::max(0.0f, l.frame.w - padding * 2.0f), l.frame.h};
  if (value.empty()) {
    out.push_back(
        DrawText{tr, placeholder, ColorU8{160, 160, 160, 255}, font_px, 0.0f, 0.5f});
  } else {
    DrawText t{tr, value, ColorU8{235, 235, 235, 255}, font_px, 0.0f, 0.5f};
    t.caret_end = focused;
    out.push_back(std::move(t));
  }
  if (focused && value.empty()) {
    DrawText t{tr, std::string{}, ColorU8{235, 235, 235, 255}, font_px, 0.0f, 0.5f};
    t.caret_end = true;
    out.push_back(std::move(t));
  }
  return true;
}

} // namespace duorou::ui

