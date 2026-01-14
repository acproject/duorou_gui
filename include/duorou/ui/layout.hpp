#pragma once

#include <duorou/ui/base_layout.hpp>

#include <ostream>

#include <duorou/ui/component_box.hpp>
#include <duorou/ui/component_button.hpp>
#include <duorou/ui/component_canvas.hpp>
#include <duorou/ui/component_checkbox.hpp>
#include <duorou/ui/component_column.hpp>
#include <duorou/ui/component_divider.hpp>
#include <duorou/ui/component_geometryreader.hpp>
#include <duorou/ui/component_grid.hpp>
#include <duorou/ui/component_image.hpp>
#include <duorou/ui/component_row.hpp>
#include <duorou/ui/component_scrollview.hpp>
#include <duorou/ui/component_slider.hpp>
#include <duorou/ui/component_spacer.hpp>
#include <duorou/ui/component_text.hpp>
#include <duorou/ui/component_textfield.hpp>

namespace duorou::ui {

inline SizeF measure_leaf(const ViewNode &node, ConstraintsF constraints) {
  SizeF out{};
  if (measure_leaf_divider(node, constraints, out)) {
    return out;
  }
  if (measure_leaf_image(node, constraints, out)) {
    return out;
  }
  if (measure_leaf_text(node, constraints, out)) {
    return out;
  }
  if (measure_leaf_button(node, constraints, out)) {
    return out;
  }
  if (measure_leaf_stepper(node, constraints, out)) {
    return out;
  }
  if (measure_leaf_checkbox(node, constraints, out)) {
    return out;
  }
  if (measure_leaf_slider(node, constraints, out)) {
    return out;
  }
  if (measure_leaf_progressview(node, constraints, out)) {
    return out;
  }
  if (measure_leaf_canvas(node, constraints, out)) {
    return out;
  }
  if (measure_leaf_textfield(node, constraints, out)) {
    return out;
  }
  if (measure_leaf_texteditor(node, constraints, out)) {
    return out;
  }
  if (measure_leaf_spacer(node, constraints, out)) {
    return out;
  }
  return apply_explicit_size(node, constraints, SizeF{0.0f, 0.0f});
}

inline SizeF measure_node(const ViewNode &node, ConstraintsF constraints) {
  SizeF out{};
  if (measure_node_column(node, constraints, out)) {
    return out;
  }
  if (measure_node_row(node, constraints, out)) {
    return out;
  }
  if (measure_node_overlay(node, constraints, out)) {
    return out;
  }
  if (measure_node_box(node, constraints, out)) {
    return out;
  }
  if (measure_node_scrollview(node, constraints, out)) {
    return out;
  }
  if (measure_node_geometryreader(node, constraints, out)) {
    return out;
  }
  if (measure_node_grid(node, constraints, out)) {
    return out;
  }

  if (!node.children.empty()) {
    const auto padding = prop_as_float(node.props, "padding", 0.0f);
    const auto inner_max_w =
        std::max(0.0f, constraints.max_w - padding * 2.0f);
    const auto inner_max_h =
        std::max(0.0f, constraints.max_h - padding * 2.0f);
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
    return apply_explicit_size(
        node, constraints,
        SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  }

  return measure_leaf(node, constraints);
}

inline LayoutNode layout_node(const ViewNode &node, RectF frame) {
  LayoutNode out;
  out.id = node.id;
  out.key = node.key;
  out.type = node.type;
  out.frame = frame;

  if (layout_children_column(node, frame, out)) {
    return out;
  }
  if (layout_children_row(node, frame, out)) {
    return out;
  }
  if (layout_children_overlay(node, frame, out)) {
    return out;
  }
  if (layout_children_box(node, frame, out)) {
    return out;
  }
  if (layout_children_scrollview(node, frame, out)) {
    return out;
  }
  if (layout_children_geometryreader(node, frame, out)) {
    return out;
  }
  if (layout_children_grid(node, frame, out)) {
    return out;
  }

  if (!node.children.empty()) {
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
  }

  return out;
}

inline LayoutNode layout_tree(const ViewNode &root, SizeF viewport) {
  RectF root_frame{0.0f, 0.0f, viewport.w, viewport.h};
  return layout_node(root, root_frame);
}

inline void dump_layout(std::ostream &os, const LayoutNode &node,
                        int indent_spaces = 0) {
  for (int i = 0; i < indent_spaces; ++i) {
    os.put(' ');
  }

  os << node.type << "#" << node.id << " [" << node.frame.x << ","
     << node.frame.y << " " << node.frame.w << "x" << node.frame.h << "]\n";

  for (const auto &c : node.children) {
    dump_layout(os, c, indent_spaces + 2);
  }
}

} // namespace duorou::ui
