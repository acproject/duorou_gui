#pragma once

#include <duorou/ui/base_render.hpp>

#include <algorithm>

namespace duorou::ui {

inline ViewNode Slider(double value) {
  auto b = view("Slider");
  b.prop("value", value);
  return std::move(b).build();
}

inline ViewNode ProgressView(double value) {
  auto b = view("ProgressView");
  b.prop("value", value);
  return std::move(b).build();
}

inline bool measure_leaf_slider(const ViewNode &node, ConstraintsF constraints,
                                SizeF &out) {
  if (node.type != "Slider") {
    return false;
  }
  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto default_w = prop_as_float(node.props, "default_width", 160.0f);
  const auto track_h = prop_as_float(node.props, "track_height", 4.0f);
  const auto thumb = prop_as_float(node.props, "thumb_size", 14.0f);
  const auto w = default_w + padding * 2.0f;
  const auto h = std::max(track_h, thumb) + padding * 2.0f;
  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  return true;
}

inline bool measure_leaf_progressview(const ViewNode &node, ConstraintsF constraints,
                                      SizeF &out) {
  if (node.type != "ProgressView") {
    return false;
  }
  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto default_w = prop_as_float(node.props, "default_width", 160.0f);
  const auto bar_h = prop_as_float(node.props, "height", 8.0f);
  const auto w = default_w + padding * 2.0f;
  const auto h = bar_h + padding * 2.0f;
  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  return true;
}

inline bool emit_render_ops_slider(const ViewNode &v, const LayoutNode &l,
                                  std::vector<RenderOp> &out) {
  if (v.type != "Slider") {
    return false;
  }
  const auto padding = prop_as_float(v.props, "padding", 0.0f);
  const auto min_v = prop_as_float(v.props, "min", 0.0f);
  const auto max_v = prop_as_float(v.props, "max", 1.0f);
  const auto v0 = prop_as_float(v.props, "value", 0.0f);
  const auto thumb = prop_as_float(v.props, "thumb_size", 14.0f);
  const auto track_h = prop_as_float(v.props, "track_height", 4.0f);
  const auto denom = (max_v - min_v);
  const auto t =
      denom != 0.0f ? clampf((v0 - min_v) / denom, 0.0f, 1.0f) : 0.0f;
  const float x0 = l.frame.x + padding;
  const float x1 = l.frame.x + l.frame.w - padding;
  const float w = std::max(0.0f, x1 - x0);
  const float cy = l.frame.y + l.frame.h * 0.5f;
  const RectF track{x0, cy - track_h * 0.5f, w, track_h};
  out.push_back(DrawRect{track, ColorU8{60, 60, 60, 255}});
  out.push_back(DrawRect{RectF{track.x, track.y, track.w * t, track.h},
                         ColorU8{80, 140, 255, 255}});
  const float thumb_x = x0 + (w - thumb) * t;
  const RectF knob{thumb_x, cy - thumb * 0.5f, thumb, thumb};
  out.push_back(DrawRect{knob, ColorU8{200, 200, 200, 255}});
  return true;
}

inline bool emit_render_ops_progressview(const ViewNode &v, const LayoutNode &l,
                                         std::vector<RenderOp> &out) {
  if (v.type != "ProgressView") {
    return false;
  }
  const auto padding = prop_as_float(v.props, "padding", 0.0f);
  const auto t = clampf(prop_as_float(v.props, "value", 0.0f), 0.0f, 1.0f);
  const auto track = prop_as_color(v.props, "track", ColorU8{60, 60, 60, 255});
  const auto fill = prop_as_color(v.props, "fill", ColorU8{80, 140, 255, 255});

  const float x0 = l.frame.x + padding;
  const float y0 = l.frame.y + padding;
  const float w = std::max(0.0f, l.frame.w - padding * 2.0f);
  const float h = std::max(0.0f, l.frame.h - padding * 2.0f);
  const RectF bar{x0, y0, w, h};
  out.push_back(DrawRect{bar, track});
  out.push_back(DrawRect{RectF{bar.x, bar.y, bar.w * t, bar.h}, fill});
  return true;
}

} // namespace duorou::ui
