#pragma once

#include <duorou/ui/base_render.hpp>

#include <algorithm>
#include <initializer_list>

namespace duorou::ui {

SizeF measure_node(const ViewNode &node, ConstraintsF constraints);
LayoutNode layout_node(const ViewNode &node, RectF frame);

inline ViewNode Box(std::initializer_list<ViewNode> children) {
  auto b = view("Box");
  b.children(children);
  return std::move(b).build();
}

inline ViewNode ZStack(std::initializer_list<ViewNode> children) {
  return Box(children);
}

inline ViewNode Overlay(std::initializer_list<ViewNode> children) {
  auto b = view("Overlay");
  b.children(children);
  return std::move(b).build();
}

inline bool measure_node_box(const ViewNode &node, ConstraintsF constraints,
                             SizeF &out) {
  if (node.type != "Box") {
    return false;
  }
  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto inner_max_w = std::max(0.0f, constraints.max_w - padding * 2.0f);
  const auto inner_max_h = std::max(0.0f, constraints.max_h - padding * 2.0f);
  const auto inner = ConstraintsF{inner_max_w, inner_max_h};

  float w = 0.0f;
  float h = 0.0f;
  for (const auto &c : node.children) {
    const auto cs = measure_node(c, inner);
    w = std::max(w, cs.w);
    h = std::max(h, cs.h);
  }
  w += padding * 2.0f;
  h += padding * 2.0f;
  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  return true;
}

inline bool measure_node_overlay(const ViewNode &node, ConstraintsF constraints,
                                 SizeF &out) {
  if (node.type != "Overlay") {
    return false;
  }
  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto inner_max_w = std::max(0.0f, constraints.max_w - padding * 2.0f);
  const auto inner_max_h = std::max(0.0f, constraints.max_h - padding * 2.0f);
  const auto inner = ConstraintsF{inner_max_w, inner_max_h};

  float w = 0.0f;
  float h = 0.0f;
  for (const auto &c : node.children) {
    const auto cs = measure_node(c, inner);
    w = std::max(w, cs.w);
    h = std::max(h, cs.h);
  }
  w += padding * 2.0f;
  h += padding * 2.0f;
  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  return true;
}

inline bool layout_children_box(const ViewNode &node, RectF frame,
                               LayoutNode &out) {
  if (node.type != "Box") {
    return false;
  }
  const auto padding = prop_as_float(node.props, "padding", 0.0f);

  const auto inner_x = frame.x + padding;
  const auto inner_y = frame.y + padding;
  const auto inner_w = std::max(0.0f, frame.w - padding * 2.0f);
  const auto inner_h = std::max(0.0f, frame.h - padding * 2.0f);
  const auto inner = ConstraintsF{inner_w, inner_h};

  for (const auto &c : node.children) {
    const auto cs = measure_node(c, inner);
    RectF child_frame{inner_x, inner_y, cs.w, cs.h};
    out.children.push_back(layout_node(c, child_frame));
  }
  return true;
}

inline bool layout_children_overlay(const ViewNode &node, RectF frame,
                                   LayoutNode &out) {
  if (node.type != "Overlay") {
    return false;
  }
  const auto padding = prop_as_float(node.props, "padding", 0.0f);

  const auto inner_x = frame.x + padding;
  const auto inner_y = frame.y + padding;
  const auto inner_w = std::max(0.0f, frame.w - padding * 2.0f);
  const auto inner_h = std::max(0.0f, frame.h - padding * 2.0f);

  for (const auto &c : node.children) {
    RectF child_frame{inner_x, inner_y, inner_w, inner_h};
    out.children.push_back(layout_node(c, child_frame));
  }
  return true;
}

inline bool emit_render_ops_box(const ViewNode &v, const LayoutNode &l,
                               std::vector<RenderOp> &out) {
  if (v.type != "Box") {
    return false;
  }
  if (find_prop(v.props, "bg")) {
    const auto bg = prop_as_color(v.props, "bg", ColorU8{0, 0, 0, 0});
    out.push_back(DrawRect{l.frame, bg});
  }
  const auto bw = prop_as_float(v.props, "border_width", 0.0f);
  if (bw > 0.0f && find_prop(v.props, "border")) {
    const auto bc = prop_as_color(v.props, "border", ColorU8{80, 80, 80, 255});
    const auto t = bw;
    out.push_back(DrawRect{RectF{l.frame.x, l.frame.y, l.frame.w, t}, bc});
    out.push_back(DrawRect{RectF{l.frame.x, l.frame.y + l.frame.h - t, l.frame.w, t}, bc});
    out.push_back(DrawRect{RectF{l.frame.x, l.frame.y, t, l.frame.h}, bc});
    out.push_back(DrawRect{RectF{l.frame.x + l.frame.w - t, l.frame.y, t, l.frame.h}, bc});
  }
  return true;
}

} // namespace duorou::ui
