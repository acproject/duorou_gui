#pragma once

#include <duorou/ui/base_render.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <initializer_list>

namespace duorou::ui {

SizeF measure_node(const ViewNode &node, ConstraintsF constraints);
LayoutNode layout_node(const ViewNode &node, RectF frame);

inline ViewNode GeometryReader(std::initializer_list<ViewNode> children) {
  auto b = view("GeometryReader");
  b.children(children);
  return std::move(b).build();
}

inline ViewNode GeometryReader(std::function<ViewNode(SizeF)> content) {
  auto node = view("GeometryReader")
                  .prop("content_fn",
                        static_cast<std::int64_t>(reinterpret_cast<std::intptr_t>(
                            new std::function<ViewNode(SizeF)>(std::move(content)))))
                  .build();
  return node;
}

inline bool measure_node_geometryreader(const ViewNode &node,
                                       ConstraintsF constraints, SizeF &out) {
  if (node.type != "GeometryReader") {
    return false;
  }
  const float w = std::max(0.0f, constraints.max_w);
  const float h = std::max(0.0f, constraints.max_h);

  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});

  return true;
}

inline bool layout_children_geometryreader(const ViewNode &node, RectF frame,
                                          LayoutNode &out) {
  if (node.type != "GeometryReader") {
    return false;
  }

  const auto padding = prop_as_float(node.props, "padding", 0.0f);

  const auto inner_x = frame.x + padding;
  const auto inner_y = frame.y + padding;
  const auto inner_w = std::max(0.0f, frame.w - padding * 2.0f);
  const auto inner_h = std::max(0.0f, frame.h - padding * 2.0f);

  const std::size_t n = std::min<std::size_t>(1, node.children.size());
  for (std::size_t i = 0; i < n; ++i) {
    RectF child_frame{inner_x, inner_y, inner_w, inner_h};
    out.children.push_back(layout_node(node.children[i], child_frame));
  }

  return true;
}

} // namespace duorou::ui
