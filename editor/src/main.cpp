#include <duorou/ui/runtime.hpp>

#include <cstdint>
#include <iostream>
#include <string>

using namespace duorou::ui;

static ViewNode panel(std::string title, ViewNode content, float width) {
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

static ViewNode tree_panel() {
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

static ViewNode preview_panel(float width, float height) {
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
                      .prop("value", "Preview (placeholder)")
                      .prop("font_size", 16.0)
                      .prop("color", 0xFFE0E0E0)
                      .build(),
                  view("Row")
                      .prop("spacing", 10.0)
                      .prop("cross_align", "center")
                      .children({
                          view("Button").prop("title", "Button").build(),
                          view("Text").prop("value", "Text").build(),
                          view("TextField")
                              .prop("value", "")
                              .prop("placeholder", "Input")
                              .prop("width", 220.0)
                              .build(),
                      })
                      .build(),
              })
              .build(),
      })
      .build();
}

static ViewNode style_panel() {
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
                                  view("Text").prop("value", "#202020").prop("color", 0xFFB0B0B0).build(),
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

int main() {
  constexpr float viewport_w = 1200.0f;
  constexpr float viewport_h = 720.0f;
  constexpr float left_w = 260.0f;
  constexpr float right_w = 360.0f;
  constexpr float spacing = 12.0f;
  constexpr float padding = 12.0f;
  const float center_w =
      std::max(320.0f, viewport_w - left_w - right_w - spacing * 2.0f - padding * 2.0f);

  ViewInstance editor{[&]() {
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
                                       .prop("value", "duorou Editor")
                                       .prop("font_size", 14.0)
                                       .build(),
                                   view("Spacer").build(),
                                   view("Button").prop("title", "Light").build(),
                                   view("Button").prop("title", "Dark").build(),
                               })
                               .build(),
                       })
                       .build();

    auto body = view("Row")
                    .prop("spacing", spacing)
                    .prop("cross_align", "stretch")
                    .children({
                        panel("Component Tree", tree_panel(), left_w),
                        panel("Live Preview", preview_panel(center_w, viewport_h - 80.0f), center_w),
                        panel("Style", style_panel(), right_w),
                    })
                    .build();

    return view("Column")
        .prop("padding", padding)
        .prop("spacing", 12.0)
        .prop("cross_align", "stretch")
        .children({std::move(top_bar), std::move(body)})
        .build();
  }};

  editor.set_viewport(SizeF{viewport_w, viewport_h});
  editor.update();

  std::cout << "Editor tree:\n";
  dump_tree(std::cout, editor.tree());
  std::cout << "Layout:\n";
  dump_layout(std::cout, editor.layout());
  std::cout << "ASCII render:\n";
  render_ascii(std::cout, editor.render_ops(), SizeF{viewport_w, viewport_h}, 120, 36);
  return 0;
}

