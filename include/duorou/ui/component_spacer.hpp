#pragma once

#include <duorou/ui/base_render.hpp>

namespace duorou::ui {

inline ViewNode Spacer(double min_length = 0.0) {
  auto b = view("Spacer");
  b.prop("min_length", min_length);
  return std::move(b).build();
}

inline bool measure_leaf_spacer(const ViewNode &node, ConstraintsF constraints,
                                SizeF &out) {
  if (node.type != "Spacer") {
    return false;
  }
  out = apply_explicit_size(node, constraints, SizeF{0.0f, 0.0f});
  return true;
}

} // namespace duorou::ui

