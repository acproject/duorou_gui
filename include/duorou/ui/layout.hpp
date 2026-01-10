#pragma once

#include <duorou/ui/node.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

namespace duorou::ui {

struct SizeF {
  float w{};
  float h{};
};

struct RectF {
  float x{};
  float y{};
  float w{};
  float h{};
};

struct ConstraintsF {
  float max_w{};
  float max_h{};
};

struct LayoutNode {
  NodeId id{};
  std::string key;
  std::string type;
  RectF frame;
  std::vector<LayoutNode> children;
};

inline float clampf(float v, float lo, float hi) {
  return std::min(std::max(v, lo), hi);
}

inline const PropValue *find_prop(const Props &props, const std::string &key) {
  const auto it = props.find(key);
  if (it == props.end()) {
    return nullptr;
  }
  return &it->second;
}

inline float prop_as_float(const Props &props, const std::string &key,
                           float fallback) {
  const auto *pv = find_prop(props, key);
  if (!pv) {
    return fallback;
  }

  if (const auto *d = std::get_if<double>(pv)) {
    return static_cast<float>(*d);
  }
  if (const auto *i = std::get_if<std::int64_t>(pv)) {
    return static_cast<float>(*i);
  }
  if (const auto *b = std::get_if<bool>(pv)) {
    return *b ? 1.0f : 0.0f;
  }
  return fallback;
}

inline std::string prop_as_string(const Props &props, const std::string &key,
                                  std::string fallback = {}) {
  const auto *pv = find_prop(props, key);
  if (!pv) {
    return fallback;
  }
  if (const auto *s = std::get_if<std::string>(pv)) {
    return *s;
  }
  return fallback;
}

inline SizeF measure_leaf(const ViewNode &node, ConstraintsF constraints) {
  const auto font_size = prop_as_float(node.props, "font_size", 16.0f);
  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto char_w = font_size * 0.5f;
  const auto line_h = font_size * 1.2f;

  auto apply_explicit_size = [&](SizeF s) {
    if (find_prop(node.props, "width")) {
      const auto w = prop_as_float(node.props, "width", s.w);
      s.w = clampf(w, 0.0f, constraints.max_w);
    }
    if (find_prop(node.props, "height")) {
      const auto h = prop_as_float(node.props, "height", s.h);
      s.h = clampf(h, 0.0f, constraints.max_h);
    }
    return s;
  };

  if (node.type == "Divider") {
    const auto thickness = prop_as_float(node.props, "thickness", 1.0f);
    const auto w = constraints.max_w;
    const auto h = thickness;
    return apply_explicit_size(
        SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  }

  if (node.type == "Text") {
    const auto text = prop_as_string(node.props, "value", "");
    const auto w = static_cast<float>(text.size()) * char_w + padding * 2.0f;
    const auto h = line_h + padding * 2.0f;
    return apply_explicit_size(SizeF{clampf(w, 0.0f, constraints.max_w),
                                     clampf(h, 0.0f, constraints.max_h)});
  }

  if (node.type == "Button") {
    const auto title = prop_as_string(node.props, "title", "");
    const auto w =
        static_cast<float>(title.size()) * char_w + 24.0f + padding * 2.0f;
    const auto h = std::max(28.0f, line_h + 12.0f) + padding * 2.0f;
    return apply_explicit_size(SizeF{clampf(w, 0.0f, constraints.max_w),
                                     clampf(h, 0.0f, constraints.max_h)});
  }

  if (node.type == "Checkbox") {
    const auto label = prop_as_string(node.props, "label", "");
    const auto box = std::max(12.0f, font_size * 1.0f);
    const auto gap = prop_as_float(node.props, "gap", 8.0f);
    const auto w = box + gap + static_cast<float>(label.size()) * char_w +
                   padding * 2.0f;
    const auto h = std::max(box, line_h) + padding * 2.0f;
    return apply_explicit_size(SizeF{clampf(w, 0.0f, constraints.max_w),
                                     clampf(h, 0.0f, constraints.max_h)});
  }

  if (node.type == "Slider") {
    const auto default_w = prop_as_float(node.props, "default_width", 160.0f);
    const auto track_h = prop_as_float(node.props, "track_height", 4.0f);
    const auto thumb = prop_as_float(node.props, "thumb_size", 14.0f);
    const auto w = default_w + padding * 2.0f;
    const auto h = std::max(track_h, thumb) + padding * 2.0f;
    return apply_explicit_size(SizeF{clampf(w, 0.0f, constraints.max_w),
                                     clampf(h, 0.0f, constraints.max_h)});
  }

  if (node.type == "TextField") {
    const auto value = prop_as_string(node.props, "value", "");
    const auto placeholder = prop_as_string(node.props, "placeholder", "");
    const auto text_len =
        std::max<std::size_t>(value.size(), placeholder.size());
    const auto min_w = prop_as_float(node.props, "min_width", 140.0f);
    const auto w = std::max(min_w, static_cast<float>(text_len) * char_w +
                                       padding * 2.0f + 24.0f);
    const auto h =
        std::max(28.0f, line_h + 12.0f) + padding * 2.0f;
    return apply_explicit_size(SizeF{clampf(w, 0.0f, constraints.max_w),
                                     clampf(h, 0.0f, constraints.max_h)});
  }

  return apply_explicit_size(SizeF{0.0f, 0.0f});
}

inline SizeF measure_node(const ViewNode &node, ConstraintsF constraints) {
  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto spacing = prop_as_float(node.props, "spacing", 0.0f);
  const auto inner_max_w = std::max(0.0f, constraints.max_w - padding * 2.0f);
  const auto inner_max_h = std::max(0.0f, constraints.max_h - padding * 2.0f);
  const auto inner = ConstraintsF{inner_max_w, inner_max_h};

  if (node.type == "Column") {
    float w = 0.0f;
    float h = 0.0f;
    for (std::size_t i = 0; i < node.children.size(); ++i) {
      const auto cs = measure_node(node.children[i], inner);
      w = std::max(w, cs.w);
      h += cs.h;
      if (i + 1 < node.children.size()) {
        h += spacing;
      }
    }
    w += padding * 2.0f;
    h += padding * 2.0f;
    SizeF s{clampf(w, 0.0f, constraints.max_w),
            clampf(h, 0.0f, constraints.max_h)};
    if (find_prop(node.props, "width")) {
      s.w = clampf(prop_as_float(node.props, "width", s.w), 0.0f,
                   constraints.max_w);
    }
    if (find_prop(node.props, "height")) {
      s.h = clampf(prop_as_float(node.props, "height", s.h), 0.0f,
                   constraints.max_h);
    }
    return s;
  }

  if (node.type == "Row") {
    float w = 0.0f;
    float h = 0.0f;
    for (std::size_t i = 0; i < node.children.size(); ++i) {
      const auto cs = measure_node(node.children[i], inner);
      w += cs.w;
      h = std::max(h, cs.h);
      if (i + 1 < node.children.size()) {
        w += spacing;
      }
    }
    w += padding * 2.0f;
    h += padding * 2.0f;
    SizeF s{clampf(w, 0.0f, constraints.max_w),
            clampf(h, 0.0f, constraints.max_h)};
    if (find_prop(node.props, "width")) {
      s.w = clampf(prop_as_float(node.props, "width", s.w), 0.0f,
                   constraints.max_w);
    }
    if (find_prop(node.props, "height")) {
      s.h = clampf(prop_as_float(node.props, "height", s.h), 0.0f,
                   constraints.max_h);
    }
    return s;
  }

  if (!node.children.empty()) {
    float w = 0.0f;
    float h = 0.0f;
    for (const auto &c : node.children) {
      const auto cs = measure_node(c, inner);
      w = std::max(w, cs.w);
      h = std::max(h, cs.h);
    }
    w += padding * 2.0f;
    h += padding * 2.0f;
    SizeF s{clampf(w, 0.0f, constraints.max_w),
            clampf(h, 0.0f, constraints.max_h)};
    if (find_prop(node.props, "width")) {
      s.w = clampf(prop_as_float(node.props, "width", s.w), 0.0f,
                   constraints.max_w);
    }
    if (find_prop(node.props, "height")) {
      s.h = clampf(prop_as_float(node.props, "height", s.h), 0.0f,
                   constraints.max_h);
    }
    return s;
  }

  return measure_leaf(node, constraints);
}

inline LayoutNode layout_node(const ViewNode &node, RectF frame) {
  LayoutNode out;
  out.id = node.id;
  out.key = node.key;
  out.type = node.type;
  out.frame = frame;

  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto spacing = prop_as_float(node.props, "spacing", 0.0f);

  const auto inner_x = frame.x + padding;
  const auto inner_y = frame.y + padding;
  const auto inner_w = std::max(0.0f, frame.w - padding * 2.0f);
  const auto inner_h = std::max(0.0f, frame.h - padding * 2.0f);
  const auto inner = ConstraintsF{inner_w, inner_h};

  const auto cross_align = prop_as_string(node.props, "cross_align", "stretch");

  if (node.type == "Column") {
    float cursor_y = inner_y;
    for (std::size_t i = 0; i < node.children.size(); ++i) {
      const auto cs = measure_node(node.children[i], inner);
      float child_w = inner_w;
      float child_x = inner_x;
      if (cross_align != "stretch") {
        child_w = cs.w;
        if (cross_align == "center") {
          child_x = inner_x + (inner_w - child_w) * 0.5f;
        } else if (cross_align == "end") {
          child_x = inner_x + (inner_w - child_w);
        }
      }
      RectF child_frame{child_x, cursor_y, child_w, cs.h};
      out.children.push_back(layout_node(node.children[i], child_frame));
      cursor_y += cs.h;
      if (i + 1 < node.children.size()) {
        cursor_y += spacing;
      }
    }
    return out;
  }

  if (node.type == "Row") {
    float cursor_x = inner_x;
    for (std::size_t i = 0; i < node.children.size(); ++i) {
      const auto cs = measure_node(node.children[i], inner);
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
      RectF child_frame{cursor_x, child_y, cs.w, child_h};
      out.children.push_back(layout_node(node.children[i], child_frame));
      cursor_x += cs.w;
      if (i + 1 < node.children.size()) {
        cursor_x += spacing;
      }
    }
    return out;
  }

  if (!node.children.empty()) {
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
