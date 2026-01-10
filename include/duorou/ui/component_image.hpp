#pragma once

#include <duorou/ui/base_render.hpp>

#include <algorithm>

namespace duorou::ui {

inline ViewNode Image(TextureHandle texture) {
  auto b = view("Image");
  b.prop("texture", static_cast<std::int64_t>(texture));
  return std::move(b).build();
}

inline bool measure_leaf_image(const ViewNode &node, ConstraintsF constraints,
                               SizeF &out) {
  if (node.type != "Image") {
    return false;
  }
  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto default_w = prop_as_float(node.props, "default_width", 64.0f);
  const auto default_h = prop_as_float(node.props, "default_height", 64.0f);
  const auto w = default_w + padding * 2.0f;
  const auto h = default_h + padding * 2.0f;
  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  return true;
}

inline bool emit_render_ops_image(const ViewNode &v, const LayoutNode &l,
                                  std::vector<RenderOp> &out) {
  if (v.type != "Image") {
    return false;
  }

  TextureHandle tex = 0;
  if (const auto *pv = find_prop(v.props, "texture")) {
    if (const auto *i = std::get_if<std::int64_t>(pv)) {
      tex = static_cast<TextureHandle>(*i);
    } else if (const auto *d = std::get_if<double>(pv)) {
      tex = static_cast<TextureHandle>(static_cast<std::uint64_t>(*d));
    }
  }
  if (tex == 0) {
    return true;
  }

  const auto padding = prop_as_float(v.props, "padding", 0.0f);
  const auto u0 = prop_as_float(v.props, "u0", 0.0f);
  const auto v0 = prop_as_float(v.props, "v0", 0.0f);
  const auto u1 = prop_as_float(v.props, "u1", 1.0f);
  const auto v1 = prop_as_float(v.props, "v1", 1.0f);
  const auto tint = prop_as_color(v.props, "tint", ColorU8{255, 255, 255, 255});

  RectF rect{l.frame.x + padding, l.frame.y + padding,
             std::max(0.0f, l.frame.w - padding * 2.0f),
             std::max(0.0f, l.frame.h - padding * 2.0f)};
  out.push_back(DrawImage{rect, tex, RectF{u0, v0, u1 - u0, v1 - v0}, tint});
  return true;
}

} // namespace duorou::ui

