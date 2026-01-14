#pragma once

#include <duorou/ui/base_render.hpp>

#include <algorithm>
#include <initializer_list>

namespace duorou::ui {

SizeF measure_node(const ViewNode &node, ConstraintsF constraints);
LayoutNode layout_node(const ViewNode &node, RectF frame);

inline ViewNode ScrollView(std::initializer_list<ViewNode> children) {
  auto b = view("ScrollView");
  b.prop("clip", true);
  b.children(children);
  return std::move(b).build();
}

inline bool measure_node_scrollview(const ViewNode &node, ConstraintsF constraints,
                                   SizeF &out) {
  if (node.type != "ScrollView") {
    return false;
  }
  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto spacing = prop_as_float(node.props, "spacing", 0.0f);
  const auto default_w = prop_as_float(node.props, "default_width", 160.0f);
  const auto default_h = prop_as_float(node.props, "default_height", 160.0f);

  const auto inner_max_w = std::max(0.0f, constraints.max_w - padding * 2.0f);
  const auto inner = ConstraintsF{inner_max_w, 1000000.0f};

  float content_w = 0.0f;
  float content_h = 0.0f;
  for (std::size_t i = 0; i < node.children.size(); ++i) {
    const auto cs = measure_node(node.children[i], inner);
    content_w = std::max(content_w, cs.w);
    content_h += cs.h;
    if (i + 1 < node.children.size()) {
      content_h += spacing;
    }
  }

  const auto w = std::max(default_w, content_w + padding * 2.0f);
  const auto h = std::max(default_h, std::min(constraints.max_h, content_h + padding * 2.0f));

  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  return true;
}

inline bool layout_children_scrollview(const ViewNode &node, RectF frame,
                                      LayoutNode &out) {
  if (node.type != "ScrollView") {
    return false;
  }

  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto spacing = prop_as_float(node.props, "spacing", 0.0f);
  const auto scroll_y = prop_as_float(node.props, "scroll_y", 0.0f);

  const auto inner_x = frame.x + padding;
  const auto inner_y = frame.y + padding;
  const auto inner_w = std::max(0.0f, frame.w - padding * 2.0f);
  const auto inner_h = std::max(0.0f, frame.h - padding * 2.0f);
  const auto inner = ConstraintsF{inner_w, 1000000.0f};

  float content_h = 0.0f;
  std::vector<SizeF> child_sizes;
  child_sizes.reserve(node.children.size());
  for (std::size_t i = 0; i < node.children.size(); ++i) {
    const auto cs = measure_node(node.children[i], inner);
    child_sizes.push_back(cs);
    content_h += cs.h;
    if (i + 1 < node.children.size()) {
      content_h += spacing;
    }
  }

  const auto max_scroll =
      std::max(0.0f, static_cast<float>(content_h) - inner_h);
  const auto sy = clampf(static_cast<float>(scroll_y), 0.0f, max_scroll);

  out.scroll_y = sy;
  out.scroll_content_h = static_cast<float>(content_h);
  out.scroll_max_y = max_scroll;

  float y = inner_y - sy;
  for (std::size_t i = 0; i < node.children.size(); ++i) {
    RectF child_frame{inner_x, y, child_sizes[i].w, child_sizes[i].h};
    out.children.push_back(layout_node(node.children[i], child_frame));
    y += child_sizes[i].h;
    if (i + 1 < node.children.size()) {
      y += spacing;
    }
  }

  return true;
}

inline bool emit_render_ops_scrollview(const ViewNode &v, const LayoutNode &l,
                                       std::vector<RenderOp> &out) {
  if (v.type != "ScrollView") {
    return false;
  }
  if (!prop_as_bool(v.props, "scroll_indicator", true)) {
    return true;
  }

  const auto padding = prop_as_float(v.props, "padding", 0.0f);
  const float inner_w = std::max(0.0f, l.frame.w - padding * 2.0f);
  const float inner_h = std::max(0.0f, l.frame.h - padding * 2.0f);
  if (!(l.scroll_max_y > 0.0f) || !(inner_h > 0.0f) || !(inner_w > 0.0f)) {
    return true;
  }

  const float bar_w = prop_as_float(v.props, "scrollbar_width", 6.0f);
  const float margin = prop_as_float(v.props, "scrollbar_margin", 2.0f);

  const float x = l.frame.x + padding + std::max(0.0f, inner_w - bar_w - margin);
  const float y = l.frame.y + padding + margin;
  const float h = std::max(0.0f, inner_h - margin * 2.0f);
  if (!(h > 0.0f)) {
    return true;
  }

  const float thumb_h =
      std::max(12.0f, h * (inner_h / std::max(inner_h, l.scroll_content_h)));
  const float denom = std::max(1e-6f, l.scroll_max_y);
  const float t = clampf(l.scroll_y / denom, 0.0f, 1.0f);
  const float thumb_y = y + (h - thumb_h) * t;

  const auto track = prop_as_color(v.props, "scrollbar_track", ColorU8{20, 20, 20, 140});
  const auto thumb = prop_as_color(v.props, "scrollbar_thumb", ColorU8{220, 220, 220, 160});
  out.push_back(DrawRect{RectF{x, y, bar_w, h}, track});
  out.push_back(DrawRect{RectF{x, thumb_y, bar_w, thumb_h}, thumb});
  return true;
}

} // namespace duorou::ui
