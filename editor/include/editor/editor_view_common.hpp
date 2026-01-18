#pragma once

#include <duorou/ui/runtime.hpp>

#include <string>
#include <string_view>

namespace duorou::ui::editor {

inline ViewNode panel(std::string title, ViewNode content, float width) {
  return view("Box")
      .prop("width", width)
      .prop("bg", 0xFF1B1B1B)
      .prop("border", 0xFF3A3A3A)
      .prop("border_width", 1.0)
      .children({
          view("Column")
              .prop("padding", 12.0)
              .prop("spacing", 10.0)
              .prop("cross_align", "stretch")
              .children({
                  view("Text")
                      .prop("value", std::move(title))
                      .prop("font_size", 14.0)
                      .prop("color", 0xFFE0E0E0)
                      .build(),
                  view("Divider").prop("thickness", 1.0).prop("color", 0xFF2A2A2A).build(),
                  std::move(content),
              })
              .build(),
      })
      .build();
}

inline bool starts_with(std::string_view s, std::string_view prefix) {
  return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

inline bool is_container_type(std::string_view type) {
  return type == "Column" || type == "Row" || type == "Box" || type == "Overlay" ||
         type == "Grid" || type == "ScrollView";
}

inline ViewInstance *active_instance() {
  if (!::duorou::ui::detail::active_dispatch_context) {
    return nullptr;
  }
  return ::duorou::ui::detail::active_dispatch_context->instance;
}

inline std::optional<RectF> active_layout_frame_by_key(const std::string &key) {
  if (auto *inst = active_instance()) {
    return inst->layout_frame_by_key(key);
  }
  return std::nullopt;
}

} // namespace duorou::ui::editor

