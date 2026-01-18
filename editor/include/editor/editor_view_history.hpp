#pragma once

#include <duorou/ui/runtime.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace duorou::ui::editor {

inline ViewNode make_default_node(TextureHandle demo_tex_handle,
                                 std::string_view type, std::string key) {
  if (type == "Button") {
    return view("Button").key(std::move(key)).prop("title", "Button").build();
  }
  if (type == "Text") {
    return view("Text").key(std::move(key)).prop("value", "Text").build();
  }
  if (type == "TextField") {
    return view("TextField")
        .key(std::move(key))
        .prop("value", "")
        .prop("placeholder", "Input")
        .prop("width", 260.0)
        .build();
  }
  if (type == "Image") {
    return view("Image")
        .key(std::move(key))
        .prop("texture", static_cast<std::int64_t>(demo_tex_handle))
        .prop("width", 64.0)
        .prop("height", 64.0)
        .build();
  }
  if (type == "Row") {
    return view("Row")
        .key(std::move(key))
        .prop("spacing", 10.0)
        .prop("cross_align", "center")
        .children({})
        .build();
  }
  if (type == "Box") {
    return view("Box")
        .key(std::move(key))
        .prop("padding", 12.0)
        .prop("bg", 0xFF202020)
        .prop("border", 0xFF3A3A3A)
        .prop("border_width", 1.0)
        .children({})
        .build();
  }
  return view("Column")
      .key(std::move(key))
      .prop("spacing", 12.0)
      .prop("cross_align", "start")
      .children({})
      .build();
}

inline void push_history(StateHandle<std::vector<ViewNode>> history,
                         StateHandle<std::int64_t> history_idx,
                         const ViewNode &root) {
  auto h = history.get();
  auto idx = history_idx.get();
  if (idx >= 0 && idx < static_cast<std::int64_t>(h.size())) {
    h.resize(static_cast<std::size_t>(idx + 1));
  } else if (!h.empty() && idx != static_cast<std::int64_t>(h.size() - 1)) {
    idx = static_cast<std::int64_t>(h.size() - 1);
  }
  h.push_back(root);
  history.set(std::move(h));
  history_idx.set(static_cast<std::int64_t>(history.get().size() - 1));
}

} // namespace duorou::ui::editor

