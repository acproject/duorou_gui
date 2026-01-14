#pragma once

#include <ostream>
#include <string>

#include <duorou/ui/base_node.hpp>

#include <duorou/ui/component_box.hpp>
#include <duorou/ui/component_button.hpp>
#include <duorou/ui/component_canvas.hpp>
#include <duorou/ui/component_checkbox.hpp>
#include <duorou/ui/component_column.hpp>
#include <duorou/ui/component_divider.hpp>
#include <duorou/ui/component_image.hpp>
#include <duorou/ui/component_row.hpp>
#include <duorou/ui/component_scrollview.hpp>
#include <duorou/ui/component_slider.hpp>
#include <duorou/ui/component_spacer.hpp>
#include <duorou/ui/component_text.hpp>
#include <duorou/ui/component_textfield.hpp>

namespace duorou::ui {

inline ViewNode Group(std::initializer_list<ViewNode> children) {
  auto b = view("Group");
  b.children(children);
  return std::move(b).build();
}

inline ViewNode List(std::initializer_list<ViewNode> children) {
  auto content = view("Column");
  content.prop("spacing", 0.0);
  content.prop("cross_align", "stretch");
  content.children(children);

  auto b = view("ScrollView");
  b.prop("clip", true);
  b.children({std::move(content).build()});
  return std::move(b).build();
}

inline ViewNode Form(std::initializer_list<ViewNode> children) {
  auto content = view("Column");
  content.prop("padding", 12.0);
  content.prop("spacing", 10.0);
  content.prop("cross_align", "stretch");
  content.children(children);

  auto b = view("ScrollView");
  b.prop("clip", true);
  b.children({std::move(content).build()});
  return std::move(b).build();
}

inline ViewNode Canvas(std::string key, CanvasDrawFn draw,
                       double default_width = 200.0,
                       double default_height = 200.0) {
  auto b = view("Canvas");
  b.prop("default_width", default_width);
  b.prop("default_height", default_height);
  auto node = std::move(b).build();

  if (key.empty()) {
    key = std::string{"canvas:"} + std::to_string(node.id);
  }
  node.key = key;

  const auto id_u64 = canvas_hash64(key) & 0x7FFFFFFFFFFFFFFFull;
  const auto id_i64 = static_cast<std::int64_t>(id_u64);
  node.props.insert_or_assign("canvas_id", PropValue{id_i64});
  register_canvas_draw(static_cast<std::uint64_t>(id_u64), std::move(draw));
  return node;
}

inline ViewNode Section(std::string header, std::initializer_list<ViewNode> children) {
  auto content = view("Column");
  content.prop("spacing", 8.0);
  content.prop("cross_align", "stretch");
  content.children([&](auto &c) {
    if (!header.empty()) {
      c.add(view("Text")
                .prop("value", header)
                .prop("font_size", 12.0)
                .prop("color", 0xFFB0B0B0)
                .build());
    }
    for (auto &ch : children) {
      c.add(std::move(ch));
    }
  });

  auto b = view("Box");
  b.prop("padding", 12.0);
  b.prop("bg", 0xFF202020);
  b.prop("border", 0xFF3A3A3A);
  b.prop("border_width", 1.0);
  b.children({std::move(content).build()});
  return std::move(b).build();
}

inline void dump_tree(std::ostream &os, const ViewNode &node,
                      int indent_spaces = 0) {
  for (int i = 0; i < indent_spaces; ++i) {
    os.put(' ');
  }

  os << node.type << "#" << node.id;

  if (!node.props.empty()) {
    os << " {";
    bool first = true;
    for (const auto &kv : node.props) {
      if (!std::exchange(first, false)) {
        os << ", ";
      }
      os << kv.first << ": ";
      std::visit([&](const auto &v) { os << v; }, kv.second);
    }
    os << "}";
  }

  os << "\n";

  for (const auto &child : node.children) {
    dump_tree(os, child, indent_spaces + 2);
  }
}

} // namespace duorou::ui
