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

} // namespace duorou::ui

