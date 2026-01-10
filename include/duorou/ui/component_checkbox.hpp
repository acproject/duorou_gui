#pragma once

#include <duorou/ui/base_render.hpp>

#include <algorithm>
#include <string>

namespace duorou::ui {

inline ViewNode Checkbox(std::string label, bool checked) {
  auto b = view("Checkbox");
  b.prop("label", std::move(label));
  b.prop("checked", checked);
  return std::move(b).build();
}

inline ViewNode Toggle(std::string label, bool on) {
  return Checkbox(std::move(label), on);
}

inline bool measure_leaf_checkbox(const ViewNode &node, ConstraintsF constraints,
                                  SizeF &out) {
  if (node.type != "Checkbox") {
    return false;
  }
  const auto font_size = prop_as_float(node.props, "font_size", 16.0f);
  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto char_w = font_size * 0.5f;
  const auto line_h = font_size * 1.2f;
  const auto label = prop_as_string(node.props, "label", "");
  const auto box = std::max(12.0f, font_size * 1.0f);
  const auto gap = prop_as_float(node.props, "gap", 8.0f);
  const auto w = box + gap + static_cast<float>(label.size()) * char_w +
                 padding * 2.0f;
  const auto h = std::max(box, line_h) + padding * 2.0f;
  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  return true;
}

inline bool emit_render_ops_checkbox(const ViewNode &v, const LayoutNode &l,
                                    std::vector<RenderOp> &out) {
  if (v.type != "Checkbox") {
    return false;
  }
  const auto padding = prop_as_float(v.props, "padding", 0.0f);
  const auto gap = prop_as_float(v.props, "gap", 8.0f);
  const auto font_px = prop_as_float(v.props, "font_size", 16.0f);
  const auto box =
      std::max(12.0f, std::min(l.frame.h - padding * 2.0f, font_px * 1.0f));
  const float bx = l.frame.x + padding;
  const float by = l.frame.y + (l.frame.h - box) * 0.5f;
  const RectF outer{bx, by, box, box};
  out.push_back(DrawRect{outer, ColorU8{40, 40, 40, 255}});
  if (box >= 2.0f) {
    out.push_back(DrawRect{
        RectF{bx + 1.0f, by + 1.0f, box - 2.0f, box - 2.0f},
        ColorU8{120, 120, 120, 255}});
  }
  const auto checked = prop_as_bool(v.props, "checked", false);
  if (checked && box >= 6.0f) {
    out.push_back(DrawRect{
        RectF{bx + 3.0f, by + 3.0f, box - 6.0f, box - 6.0f},
        ColorU8{30, 200, 120, 255}});
  }
  const auto label = prop_as_string(v.props, "label", "");
  const RectF tr{bx + box + gap, l.frame.y,
                 std::max(0.0f, l.frame.w - (box + gap + padding)), l.frame.h};
  out.push_back(
      DrawText{tr, label, ColorU8{230, 230, 230, 255}, font_px, 0.0f, 0.5f});
  return true;
}

} // namespace duorou::ui
