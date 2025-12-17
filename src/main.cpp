#include <iostream>
#include <string>

#include <duorou/ui/runtime.hpp>

using namespace duorou::ui;

int main() {
  auto count = state<std::int64_t>(0);
  auto pressed = state<bool>(false);

  ViewInstance app{[&]() {
    return view("Column")
        .prop("padding", 24)
        .prop("spacing", 12)
        .prop("cross_align", "start")
        .children({
            view("Text")
                .prop("value",
                      std::string{"Count: "} + std::to_string(count.get()))
                .build(),
            view("Button")
                .key("inc")
                .prop("title", "Inc")
                .prop("pressed", pressed.get())
                .event("pointer_down", on_pointer_down([&]() {
                         pressed.set(true);
                         capture_pointer();
                       }))
                .event("pointer_up", on_pointer_up([&]() {
                         pressed.set(false);
                         release_pointer();
                         count.set(count.get() + 1);
                       }))
                .build(),
        })
        .build();
  }};

  app.set_viewport(SizeF{320.0f, 240.0f});

  std::cout << "Initial tree:\n";
  dump_tree(std::cout, app.tree());
  std::cout << "Layout:\n";
  dump_layout(std::cout, app.layout());
  std::cout << "Render ops:\n";
  dump_render_ops(std::cout, app.render_ops());
  std::cout << "ASCII render:\n";
  render_ascii(std::cout, app.render_ops(), SizeF{320.0f, 240.0f}, 64, 16);

  auto dump_frame = [&]() {
    auto r = app.update();
    std::cout << "Patches:\n";
    dump_patches(std::cout, r.patches);
    std::cout << "Tree:\n";
    dump_tree(std::cout, app.tree());
    std::cout << "Layout:\n";
    dump_layout(std::cout, app.layout());
    std::cout << "Render ops:\n";
    dump_render_ops(std::cout, app.render_ops());
    std::cout << "ASCII render:\n";
    render_ascii(std::cout, app.render_ops(), SizeF{320.0f, 240.0f}, 64, 16);
  };

  const int pointer = 1;

  {
    const float x = 10.0f;
    const float y = 25.0f;
    const auto handled = app.dispatch_pointer_down(pointer, x, y);
    std::cout << "\nPointerDown (" << x << "," << y
              << ") handled=" << (handled ? "true" : "false") << "\n";
    dump_frame();
  }

  {
    const float x = 200.0f;
    const float y = 200.0f;
    const auto handled = app.dispatch_pointer_move(pointer, x, y);
    std::cout << "\nPointerMove (" << x << "," << y
              << ") handled=" << (handled ? "true" : "false") << "\n";
    dump_frame();
  }

  {
    const float x = 200.0f;
    const float y = 200.0f;
    const auto handled = app.dispatch_pointer_up(pointer, x, y);
    std::cout << "\nPointerUp (" << x << "," << y
              << ") handled=" << (handled ? "true" : "false") << "\n";
    dump_frame();
  }

  return 0;
}
