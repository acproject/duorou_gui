#pragma once

#include <ostream>
#include <string>

#include <duorou/ui/base_node.hpp>

#include <duorou/ui/component_box.hpp>
#include <duorou/ui/component_button.hpp>
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
