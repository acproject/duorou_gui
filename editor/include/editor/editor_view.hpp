#pragma once

#include <duorou/ui/runtime.hpp>

#include <algorithm>
#include <string>

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

inline ViewNode tree_panel() {
  return view("ScrollView")
      .prop("clip", true)
      .prop("default_width", 260.0)
      .prop("default_height", 600.0)
      .children({
          view("Column")
              .prop("spacing", 8.0)
              .prop("cross_align", "start")
              .children({
                  view("Text").prop("value", "App").prop("font_size", 13.0).build(),
                  view("Text").prop("value", "  Button").prop("font_size", 13.0).build(),
                  view("Text").prop("value", "  Text").prop("font_size", 13.0).build(),
                  view("Text").prop("value", "  Input").prop("font_size", 13.0).build(),
                  view("Text").prop("value", "  Card").prop("font_size", 13.0).build(),
                  view("Text").prop("value", "  Modal").prop("font_size", 13.0).build(),
              })
              .build(),
      })
      .build();
}

inline ViewNode preview_panel(TextureHandle demo_tex, float width, float height) {
  return view("Box")
      .prop("width", width)
      .prop("height", height)
      .prop("bg", 0xFF101010)
      .prop("border", 0xFF2A2A2A)
      .prop("border_width", 1.0)
      .children({
          view("Column")
              .prop("padding", 16.0)
              .prop("spacing", 12.0)
              .prop("cross_align", "start")
              .children({
                  view("Text")
                      .prop("value", "Preview (GPU)")
                      .prop("font_size", 16.0)
                      .prop("color", 0xFFE0E0E0)
                      .build(),
                  view("Row")
                      .prop("spacing", 12.0)
                      .prop("cross_align", "center")
                      .children({
                          view("Image")
                              .prop("texture", static_cast<std::int64_t>(demo_tex))
                              .prop("width", 64.0)
                              .prop("height", 64.0)
                              .build(),
                          view("Column")
                              .prop("spacing", 8.0)
                              .prop("cross_align", "start")
                              .children({
                                  view("Row")
                                      .prop("spacing", 10.0)
                                      .prop("cross_align", "center")
                                      .children({
                                          view("Button").prop("title", "Button").build(),
                                          view("Text").prop("value", "Text").build(),
                                      })
                                      .build(),
                                  view("TextField")
                                      .prop("value", "")
                                      .prop("placeholder", "Input")
                                      .prop("width", 260.0)
                                      .build(),
                              })
                              .build(),
                      })
                      .build(),
              })
              .build(),
      })
      .build();
}

inline ViewNode edit_panel(BindingId binding, float width, float height) {
  return view("Box")
      .prop("width", width)
      .prop("height", height)
      .prop("bg", 0xFF101010)
      .prop("border", 0xFF2A2A2A)
      .prop("border_width", 1.0)
      .children({
          view("Column")
              .prop("padding", 12.0)
              .prop("spacing", 10.0)
              .prop("cross_align", "stretch")
              .children({
                  view("Text")
                      .prop("value", "Edit")
                      .prop("font_size", 12.0)
                      .prop("color", 0xFFB0B0B0)
                      .build(),
                  view("TextEditor")
                      .key("editor_source")
                      .prop("binding", binding.raw)
                      .prop("padding", 10.0)
                      .prop("font_size", 14.0)
                      .prop("width", std::max(0.0f, width - 24.0f))
                      .prop("height", std::max(0.0f, height - 34.0f))
                      .build(),
              })
              .build(),
      })
      .build();
}

inline ViewNode style_panel() {
  return view("ScrollView")
      .prop("clip", true)
      .prop("default_width", 360.0)
      .prop("default_height", 600.0)
      .children({
          view("Column")
              .prop("spacing", 10.0)
              .prop("cross_align", "stretch")
              .children({
                  view("Text")
                      .prop("value", "Property")
                      .prop("font_size", 12.0)
                      .prop("color", 0xFFB0B0B0)
                      .build(),
                  view("Box")
                      .prop("padding", 10.0)
                      .prop("bg", 0xFF151515)
                      .prop("border", 0xFF2A2A2A)
                      .prop("border_width", 1.0)
                      .children({
                          view("Row")
                              .prop("spacing", 10.0)
                              .prop("cross_align", "center")
                              .children({
                                  view("Text").prop("value", "Button.background").build(),
                                  view("Spacer").build(),
                                  view("Text")
                                      .prop("value", "#202020")
                                      .prop("color", 0xFFB0B0B0)
                                      .build(),
                              })
                              .build(),
                      })
                      .build(),
                  view("Text")
                      .prop("value", "Source (placeholder)")
                      .prop("font_size", 12.0)
                      .prop("color", 0xFFB0B0B0)
                      .build(),
                  view("Text")
                      .prop("value", "from Button.primary.hover")
                      .prop("font_size", 12.0)
                      .prop("color", 0xFFB0B0B0)
                      .build(),
              })
              .build(),
      })
      .build();
}

inline ViewNode editor_view(TextureHandle demo_tex_handle) {
  return GeometryReader([demo_tex_handle](SizeF size) {
    const float viewport_w = std::max(320.0f, size.w);
    const float viewport_h = std::max(240.0f, size.h);
    constexpr float left_w = 260.0f;
    constexpr float right_w = 360.0f;
    constexpr float spacing = 12.0f;
    constexpr float padding = 12.0f;

    const float center_w = std::max(
        320.0f, viewport_w - left_w - right_w - spacing * 2.0f - padding * 2.0f);
    const float workspace_h = std::max(120.0f, viewport_h - 88.0f);
    const float preview_h = std::max(120.0f, workspace_h * 0.55f);
    const float edit_h = std::max(120.0f, workspace_h - preview_h - spacing);

    auto editor_source = local_state<std::string>(
        "editor:source",
        "view(\"Column\")\n"
        "  .prop(\"spacing\", 8.0)\n"
        "  .prop(\"cross_align\", \"start\")\n"
        "  .children({\n"
        "    view(\"Text\").prop(\"value\", \"Hello duorou\").build(),\n"
        "    view(\"Button\").prop(\"title\", \"Click\").build(),\n"
        "  })\n"
        "  .build();\n");

    auto top_bar = view("Box")
                       .prop("padding", 10.0)
                       .prop("bg", 0xFF161616)
                       .prop("border", 0xFF2A2A2A)
                       .prop("border_width", 1.0)
                       .children({
                           view("Row")
                               .prop("spacing", 10.0)
                               .prop("cross_align", "center")
                               .children({
                                   view("Text")
                                       .prop("value", "duorou Editor (GPU)")
                                       .prop("font_size", 14.0)
                                       .build(),
                                   view("Spacer").build(),
                                   view("Button").prop("title", "Light").build(),
                                   view("Button").prop("title", "Dark").build(),
                               })
                               .build(),
                       })
                       .build();

    auto workspace =
        view("Column")
            .prop("spacing", spacing)
            .prop("cross_align", "stretch")
            .children({
                preview_panel(demo_tex_handle, center_w, preview_h),
                edit_panel(::duorou::ui::bind(editor_source), center_w, edit_h),
            })
            .build();

    auto body =
        view("Row")
            .prop("spacing", spacing)
            .prop("cross_align", "stretch")
            .children({
                panel("Component Tree", tree_panel(), left_w),
                panel("Workspace", std::move(workspace), center_w),
                panel("Style", style_panel(), right_w),
            })
            .build();

    return view("Column")
        .prop("padding", padding)
        .prop("spacing", 12.0)
        .prop("cross_align", "stretch")
        .children({std::move(top_bar), std::move(body)})
        .build();
  });
}

} // namespace duorou::ui::editor
