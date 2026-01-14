#pragma once

#include <duorou/ui/base_render.hpp>

#include <algorithm>
#include <initializer_list>
#include <vector>

namespace duorou::ui {

SizeF measure_node(const ViewNode &node, ConstraintsF constraints);
LayoutNode layout_node(const ViewNode &node, RectF frame);

inline ViewNode Grid(int columns, std::initializer_list<ViewNode> children) {
  auto b = view("Grid");
  b.prop("columns", columns);
  b.children(children);
  return std::move(b).build();
}

inline ViewNode LazyVGrid(int columns, std::initializer_list<ViewNode> children) {
  return Grid(columns, children);
}

inline ViewNode LazyHGrid(int rows, std::initializer_list<ViewNode> children) {
  auto b = view("Grid");
  b.prop("axis", "horizontal");
  b.prop("rows", rows);
  b.children(children);
  return std::move(b).build();
}

inline bool measure_node_grid(const ViewNode &node, ConstraintsF constraints,
                              SizeF &out) {
  if (node.type != "Grid") {
    return false;
  }

  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto spacing = prop_as_float(node.props, "spacing", 0.0f);
  const auto spacing_x = prop_as_float(node.props, "spacing_x", spacing);
  const auto spacing_y = prop_as_float(node.props, "spacing_y", spacing);
  const auto axis = prop_as_string(node.props, "axis", "vertical");

  const auto inner_max_w = std::max(0.0f, constraints.max_w - padding * 2.0f);
  const auto inner_max_h = std::max(0.0f, constraints.max_h - padding * 2.0f);

  if (node.children.empty()) {
    out = apply_explicit_size(node, constraints, SizeF{padding * 2.0f, padding * 2.0f});
    return true;
  }

  if (axis == "horizontal") {
    int rows = static_cast<int>(prop_as_float(node.props, "rows", 1.0f));
    rows = std::max(1, rows);

    const float cell_h =
        rows > 0 ? std::max(0.0f, (inner_max_h - spacing_y * (rows - 1)) / rows)
                 : inner_max_h;
    const auto cell_constraints = ConstraintsF{inner_max_w, cell_h};

    const std::size_t child_count = node.children.size();
    const std::size_t cols =
        static_cast<std::size_t>((child_count + static_cast<std::size_t>(rows) - 1) /
                                 static_cast<std::size_t>(rows));

    float max_row_w = 0.0f;
    for (int r = 0; r < rows; ++r) {
      float row_w = 0.0f;
      bool has_any = false;
      for (std::size_t c = 0; c < cols; ++c) {
        const std::size_t idx = c * static_cast<std::size_t>(rows) + static_cast<std::size_t>(r);
        if (idx >= child_count) {
          continue;
        }
        const auto cs = measure_node(node.children[idx], cell_constraints);
        if (has_any) {
          row_w += spacing_x;
        }
        row_w += cs.w;
        has_any = true;
      }
      max_row_w = std::max(max_row_w, row_w);
    }

    float content_h =
        cell_h * rows + spacing_y * static_cast<float>(std::max(0, rows - 1));
    float w = max_row_w + padding * 2.0f;
    float h = content_h + padding * 2.0f;
    out = apply_explicit_size(
        node, constraints,
        SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
    return true;
  }

  int columns = static_cast<int>(prop_as_float(node.props, "columns", 2.0f));
  columns = std::max(1, columns);
  const float cell_w =
      columns > 0 ? std::max(0.0f, (inner_max_w - spacing_x * (columns - 1)) / columns)
                  : inner_max_w;
  const auto cell_constraints = ConstraintsF{cell_w, inner_max_h};

  std::vector<float> row_heights;
  row_heights.reserve((node.children.size() + static_cast<std::size_t>(columns) - 1) /
                      static_cast<std::size_t>(columns));

  for (std::size_t i = 0; i < node.children.size(); ++i) {
    const auto cs = measure_node(node.children[i], cell_constraints);
    const std::size_t row = i / static_cast<std::size_t>(columns);
    if (row >= row_heights.size()) {
      row_heights.resize(row + 1, 0.0f);
    }
    row_heights[row] = std::max(row_heights[row], cs.h);
  }

  float content_h = 0.0f;
  for (std::size_t r = 0; r < row_heights.size(); ++r) {
    content_h += row_heights[r];
    if (r + 1 < row_heights.size()) {
      content_h += spacing_y;
    }
  }

  const std::size_t used_cols =
      std::min<std::size_t>(static_cast<std::size_t>(columns), node.children.size());
  float content_w = used_cols > 0 ? (cell_w * used_cols + spacing_x * (used_cols - 1))
                                  : 0.0f;

  float w = content_w + padding * 2.0f;
  float h = content_h + padding * 2.0f;
  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  return true;
}

inline bool layout_children_grid(const ViewNode &node, RectF frame,
                                 LayoutNode &out) {
  if (node.type != "Grid") {
    return false;
  }

  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto spacing = prop_as_float(node.props, "spacing", 0.0f);
  const auto spacing_x = prop_as_float(node.props, "spacing_x", spacing);
  const auto spacing_y = prop_as_float(node.props, "spacing_y", spacing);
  const auto axis = prop_as_string(node.props, "axis", "vertical");

  const auto inner_x = frame.x + padding;
  const auto inner_y = frame.y + padding;
  const auto inner_w = std::max(0.0f, frame.w - padding * 2.0f);
  const auto inner_h = std::max(0.0f, frame.h - padding * 2.0f);

  if (node.children.empty()) {
    return true;
  }

  if (axis == "horizontal") {
    int rows = static_cast<int>(prop_as_float(node.props, "rows", 1.0f));
    rows = std::max(1, rows);

    const float cell_h =
        rows > 0 ? std::max(0.0f, (inner_h - spacing_y * (rows - 1)) / rows)
                 : inner_h;
    const auto cell_constraints = ConstraintsF{inner_w, cell_h};

    std::vector<float> row_heights(static_cast<std::size_t>(rows), cell_h);
    const std::size_t child_count = node.children.size();
    const std::size_t cols =
        static_cast<std::size_t>((child_count + static_cast<std::size_t>(rows) - 1) /
                                 static_cast<std::size_t>(rows));

    std::vector<float> col_widths(cols, 0.0f);
    for (std::size_t c = 0; c < cols; ++c) {
      float wmax = 0.0f;
      for (int r = 0; r < rows; ++r) {
        const std::size_t idx = c * static_cast<std::size_t>(rows) + static_cast<std::size_t>(r);
        if (idx >= child_count) {
          continue;
        }
        const auto cs = measure_node(node.children[idx], cell_constraints);
        wmax = std::max(wmax, cs.w);
        row_heights[static_cast<std::size_t>(r)] =
            std::max(row_heights[static_cast<std::size_t>(r)], cs.h);
      }
      col_widths[c] = wmax;
    }

    float y = inner_y;
    for (int r = 0; r < rows; ++r) {
      float x = inner_x;
      for (std::size_t c = 0; c < cols; ++c) {
        const std::size_t idx = c * static_cast<std::size_t>(rows) + static_cast<std::size_t>(r);
        if (idx >= child_count) {
          continue;
        }
        RectF child_frame{x, y, col_widths[c], row_heights[static_cast<std::size_t>(r)]};
        out.children.push_back(layout_node(node.children[idx], child_frame));
        x += col_widths[c] + spacing_x;
      }
      y += row_heights[static_cast<std::size_t>(r)];
      if (r + 1 < rows) {
        y += spacing_y;
      }
    }

    return true;
  }

  int columns = static_cast<int>(prop_as_float(node.props, "columns", 2.0f));
  columns = std::max(1, columns);

  const float cell_w =
      columns > 0 ? std::max(0.0f, (inner_w - spacing_x * (columns - 1)) / columns)
                  : inner_w;
  const auto cell_constraints = ConstraintsF{cell_w, inner_h};

  const std::size_t rows =
      (node.children.size() + static_cast<std::size_t>(columns) - 1) /
      static_cast<std::size_t>(columns);
  std::vector<float> row_heights(rows, 0.0f);
  for (std::size_t i = 0; i < node.children.size(); ++i) {
    const auto cs = measure_node(node.children[i], cell_constraints);
    const std::size_t r = i / static_cast<std::size_t>(columns);
    row_heights[r] = std::max(row_heights[r], cs.h);
  }

  float y = inner_y;
  for (std::size_t r = 0; r < rows; ++r) {
    float x = inner_x;
    for (int c = 0; c < columns; ++c) {
      const std::size_t idx = r * static_cast<std::size_t>(columns) + static_cast<std::size_t>(c);
      if (idx >= node.children.size()) {
        break;
      }
      RectF child_frame{x, y, cell_w, row_heights[r]};
      out.children.push_back(layout_node(node.children[idx], child_frame));
      x += cell_w + spacing_x;
    }
    y += row_heights[r];
    if (r + 1 < rows) {
      y += spacing_y;
    }
  }

  return true;
}

} // namespace duorou::ui

