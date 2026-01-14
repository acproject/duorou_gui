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

inline ViewNode Sheet(std::initializer_list<ViewNode> sheet_children,
                      std::uint64_t scrim_pointer_up = 0,
                      float sheet_height = 280.0f,
                      std::int64_t scrim_bg = 0x99000000,
                      std::int64_t sheet_bg = 0xFF202020) {
  auto scrim = view("Box")
                   .prop("bg", scrim_bg)
                   .event("pointer_up", scrim_pointer_up)
                   .build();

  auto content = view("Column")
                     .prop("spacing", 10.0)
                     .prop("cross_align", "start")
                     .children(sheet_children)
                     .build();

  auto panel =
      view("Box")
          .prop("padding", 16.0)
          .prop("bg", sheet_bg)
          .prop("border", 0xFF3A3A3A)
          .prop("border_width", 1.0)
          .prop("height", sheet_height)
          .children({std::move(content)})
          .build();

  auto sheet_layer = view("Column")
                         .prop("cross_align", "stretch")
                         .prop("hit_test", false)
                         .children([&](auto &c) {
                           c.add(view("Spacer").prop("hit_test", false).build());
                           c.add(std::move(panel));
                         })
                         .build();

  return view("Overlay")
      .children([&](auto &c) {
        c.add(std::move(scrim));
        c.add(std::move(sheet_layer));
      })
      .build();
}

inline ViewNode FullScreenCover(std::initializer_list<ViewNode> children,
                                std::uint64_t bg_pointer_up = 0,
                                std::int64_t bg = 0xFF101010) {
  auto background =
      view("Box").prop("bg", bg).event("pointer_up", bg_pointer_up).build();

  auto content = view("Box").children(children).build();

  return view("Overlay")
      .children([&](auto &c) {
        c.add(std::move(background));
        c.add(std::move(content));
      })
      .build();
}

inline ViewNode AlertDialog(std::initializer_list<ViewNode> alert_children,
                            std::uint64_t scrim_pointer_up = 0,
                            float width = 360.0f,
                            std::int64_t scrim_bg = 0x99000000,
                            std::int64_t alert_bg = 0xFF202020) {
  auto scrim = view("Box")
                   .prop("bg", scrim_bg)
                   .event("pointer_up", scrim_pointer_up)
                   .build();

  auto content = view("Column")
                     .prop("spacing", 10.0)
                     .prop("cross_align", "start")
                     .children(alert_children)
                     .build();

  auto panel =
      view("Box")
          .prop("padding", 16.0)
          .prop("bg", alert_bg)
          .prop("border", 0xFF3A3A3A)
          .prop("border_width", 1.0)
          .prop("width", width)
          .children({std::move(content)})
          .build();

  auto center =
      view("Column")
          .prop("cross_align", "stretch")
          .prop("hit_test", false)
          .children([&](auto &c) {
            c.add(view("Spacer").prop("hit_test", false).build());
            c.add(view("Row")
                      .prop("cross_align", "stretch")
                      .prop("hit_test", false)
                      .children([&](auto &r) {
                        r.add(view("Spacer").prop("hit_test", false).build());
                        r.add(std::move(panel));
                        r.add(view("Spacer").prop("hit_test", false).build());
                      })
                      .build());
            c.add(view("Spacer").prop("hit_test", false).build());
          })
          .build();

  return view("Overlay")
      .children([&](auto &c) {
        c.add(std::move(scrim));
        c.add(std::move(center));
      })
      .build();
}

inline ViewNode Popover(std::initializer_list<ViewNode> pop_children,
                        float anchor_x, float anchor_y,
                        std::uint64_t scrim_pointer_down = 0,
                        std::uint64_t scrim_pointer_up = 0,
                        std::int64_t scrim_bg = 0x22000000,
                        std::int64_t bubble_bg = 0xFF202020) {
  auto scrim = view("Box")
                   .prop("bg", scrim_bg)
                   .event("pointer_down", scrim_pointer_down)
                   .event("pointer_up", scrim_pointer_up)
                   .build();

  auto content = view("Column")
                     .prop("spacing", 8.0)
                     .prop("cross_align", "start")
                     .children(pop_children)
                     .build();

  auto bubble = view("Box")
                    .prop("padding", 12.0)
                    .prop("bg", bubble_bg)
                    .prop("border", 0xFF3A3A3A)
                    .prop("border_width", 1.0)
                    .children({std::move(content)})
                    .build();

  auto positioned =
      view("Column")
          .prop("cross_align", "stretch")
          .prop("hit_test", false)
          .children([&](auto &c) {
            c.add(view("Spacer").prop("height", anchor_y).prop("hit_test", false).build());
            c.add(view("Row")
                      .prop("cross_align", "stretch")
                      .prop("hit_test", false)
                      .children([&](auto &r) {
                        r.add(view("Spacer").prop("width", anchor_x).prop("hit_test", false).build());
                        r.add(std::move(bubble));
                        r.add(view("Spacer").prop("hit_test", false).build());
                      })
                      .build());
            c.add(view("Spacer").prop("hit_test", false).build());
          })
          .build();

  return view("Overlay")
      .children([&](auto &c) {
        c.add(std::move(scrim));
        c.add(std::move(positioned));
      })
      .build();
}

#if !defined(Alert)
inline ViewNode Alert(std::initializer_list<ViewNode> alert_children,
                      std::uint64_t scrim_pointer_up = 0,
                      float width = 360.0f,
                      std::int64_t scrim_bg = 0x99000000,
                      std::int64_t alert_bg = 0xFF202020) {
  return AlertDialog(alert_children, scrim_pointer_up, width, scrim_bg, alert_bg);
}
#endif

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
