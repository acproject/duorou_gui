#pragma once

#include <duorou/ui/base_render.hpp>

#include <algorithm>
#include <initializer_list>

namespace duorou::ui {

SizeF measure_node(const ViewNode &node, ConstraintsF constraints);
LayoutNode layout_node(const ViewNode &node, RectF frame);

inline ViewNode Row(std::initializer_list<ViewNode> children) {
  auto b = view("Row");
  b.children(children);
  return std::move(b).build();
}

inline ViewNode HStack(std::initializer_list<ViewNode> children) {
  return Row(children);
}

inline ViewNode LazyHStack(std::initializer_list<ViewNode> children) {
  return Row(children);
}

inline bool measure_node_row(const ViewNode &node, ConstraintsF constraints,
                             SizeF &out) {
  if (node.type != "Row") {
    return false;
  }
  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto spacing = prop_as_float(node.props, "spacing", 0.0f);
  const auto inner_max_w = std::max(0.0f, constraints.max_w - padding * 2.0f);
  const auto inner_max_h = std::max(0.0f, constraints.max_h - padding * 2.0f);
  const auto inner = ConstraintsF{inner_max_w, inner_max_h};

  float w = 0.0f;
  float h = 0.0f;
  std::size_t flex_spacers = 0;
  for (std::size_t i = 0; i < node.children.size(); ++i) {
    const auto &c = node.children[i];
    const auto cs = measure_node(c, inner);
    const bool is_flex_spacer =
        (c.type == "Spacer") && !find_prop(c.props, "width");
    if (!is_flex_spacer) {
      w += cs.w;
    } else {
      ++flex_spacers;
    }
    h = std::max(h, cs.h);
    if (i + 1 < node.children.size()) {
      w += spacing;
    }
  }
  if (flex_spacers > 0) {
    w = std::max(w, inner_max_w);
  }
  w += padding * 2.0f;
  h += padding * 2.0f;
  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  return true;
}

inline bool layout_children_row(const ViewNode &node, RectF frame, LayoutNode &out) {
  if (node.type != "Row") {
    return false;
  }

  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto spacing = prop_as_float(node.props, "spacing", 0.0f);

  const auto inner_x = frame.x + padding;
  const auto inner_y = frame.y + padding;
  const auto inner_w = std::max(0.0f, frame.w - padding * 2.0f);
  const auto inner_h = std::max(0.0f, frame.h - padding * 2.0f);
  const auto inner = ConstraintsF{inner_w, inner_h};

  const auto cross_align = prop_as_string(node.props, "cross_align", "stretch");

  float fixed_w = 0.0f;
  float spacer_min_total = 0.0f;
  std::size_t flex_spacers = 0;
  for (const auto &c : node.children) {
    const auto cs = measure_node(c, inner);
    const bool is_flex_spacer = (c.type == "Spacer") && !find_prop(c.props, "width");
    if (is_flex_spacer) {
      ++flex_spacers;
      spacer_min_total += prop_as_float(c.props, "min_length", 0.0f);
    } else {
      fixed_w += cs.w;
    }
  }

  const float spacing_total =
      node.children.size() > 1 ? spacing * static_cast<float>(node.children.size() - 1) : 0.0f;
  const float remaining =
      std::max(0.0f, inner_w - fixed_w - spacing_total);
  float flex_each = 0.0f;
  float flex_extra_each = 0.0f;
  if (flex_spacers > 0) {
    if (spacer_min_total <= remaining) {
      flex_each = spacer_min_total / static_cast<float>(flex_spacers);
      flex_extra_each = (remaining - spacer_min_total) / static_cast<float>(flex_spacers);
    } else {
      flex_each = remaining / static_cast<float>(flex_spacers);
      flex_extra_each = 0.0f;
    }
  }

  float cursor_x = inner_x;
  for (std::size_t i = 0; i < node.children.size(); ++i) {
    const auto &c = node.children[i];
    const auto cs = measure_node(c, inner);
    const bool is_flex_spacer = (c.type == "Spacer") && !find_prop(c.props, "width");
    float child_w = cs.w;
    if (is_flex_spacer) {
      const auto min_len = prop_as_float(c.props, "min_length", 0.0f);
      if (spacer_min_total <= remaining) {
        child_w = min_len + flex_extra_each;
      } else {
        child_w = flex_each;
      }
    }
    float child_h = inner_h;
    float child_y = inner_y;
    if (cross_align != "stretch") {
      child_h = cs.h;
      if (cross_align == "center") {
        child_y = inner_y + (inner_h - child_h) * 0.5f;
      } else if (cross_align == "end") {
        child_y = inner_y + (inner_h - child_h);
      }
    }
    RectF child_frame{cursor_x, child_y, child_w, child_h};
    out.children.push_back(layout_node(c, child_frame));
    cursor_x += child_w;
    if (i + 1 < node.children.size()) {
      cursor_x += spacing;
    }
  }
  return true;
}

} // namespace duorou::ui
