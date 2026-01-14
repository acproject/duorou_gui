#pragma once

#include <duorou/ui/runtime.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace duorou::ui::examples {

inline std::string trim_ascii(std::string s) {
  auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };
  while (!s.empty() && is_space(static_cast<unsigned char>(s.front()))) {
    s.erase(s.begin());
  }
  while (!s.empty() && is_space(static_cast<unsigned char>(s.back()))) {
    s.pop_back();
  }
  return s;
}

inline std::vector<std::string> split_ws(std::string_view s) {
  std::vector<std::string> out;
  std::string cur;
  for (char ch : s) {
    if (std::isspace(static_cast<unsigned char>(ch))) {
      if (!cur.empty()) {
        out.push_back(std::move(cur));
        cur.clear();
      }
      continue;
    }
    cur.push_back(ch);
  }
  if (!cur.empty()) {
    out.push_back(std::move(cur));
  }
  return out;
}

inline void push_line(StateHandle<std::vector<std::string>> lines,
                      std::string line, std::size_t limit) {
  auto v = lines.get();
  v.push_back(std::move(line));
  if (v.size() > limit) {
    v.erase(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(v.size() - limit));
  }
  lines.set(std::move(v));
}

inline ViewNode terminal_view() {
  auto lines = local_state<std::vector<std::string>>(
      "terminal:lines",
      std::vector<std::string>{
          "duorou terminal",
          "Commands: help, echo <...>, clear, date, history",
          "Tips: drag in input to select text; drag log to scroll",
      });
  auto history = local_state<std::vector<std::string>>("terminal:history", {});
  auto hist_idx = local_state<std::int64_t>("terminal:hist_idx", -1);
  auto draft = local_state<std::string>("terminal:draft", std::string{});

  auto follow = local_state<bool>("terminal:follow", true);

  auto input = local_state<std::string>("terminal:input", std::string{});
  auto prompt = local_state<std::string>("terminal:prompt", std::string{"$ "});

  auto run_submit = [=]() mutable {
    auto cmd_raw = trim_ascii(input.get());
    if (cmd_raw.empty()) {
      return;
    }

    push_line(lines, prompt.get() + cmd_raw, 400);

    auto hist = history.get();
    if (hist.empty() || hist.back() != cmd_raw) {
      hist.push_back(cmd_raw);
      if (hist.size() > 200) {
        hist.erase(hist.begin(), hist.begin() + static_cast<std::ptrdiff_t>(hist.size() - 200));
      }
      history.set(std::move(hist));
    }
    hist_idx.set(static_cast<std::int64_t>(history.get().size()));
    draft.set(std::string{});

    auto parts = split_ws(cmd_raw);
    if (parts.empty()) {
      input.set(std::string{});
      return;
    }

    const auto &cmd = parts[0];
    if (cmd == "help") {
      push_line(lines, "help: show this message", 400);
      push_line(lines, "echo <text>: print text", 400);
      push_line(lines, "clear: clear log", 400);
      push_line(lines, "date: show unix milliseconds", 400);
      push_line(lines, "history: show recent commands", 400);
    } else if (cmd == "echo") {
      std::string rest;
      if (cmd_raw.size() > 4) {
        rest = std::string{cmd_raw.substr(4)};
        rest = trim_ascii(std::move(rest));
      }
      push_line(lines, rest, 400);
    } else if (cmd == "clear") {
      lines.set(std::vector<std::string>{});
    } else if (cmd == "date") {
      const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
      push_line(lines, std::to_string(static_cast<long long>(ms)), 400);
    } else if (cmd == "history") {
      const auto hist2 = history.get();
      if (hist2.empty()) {
        push_line(lines, "(empty)", 400);
      } else {
        for (std::size_t i = 0; i < hist2.size(); ++i) {
          push_line(lines, std::to_string(i) + ": " + hist2[i], 400);
        }
      }
    } else {
      push_line(lines, "unknown command: " + cmd, 400);
    }

    input.set(std::string{});
  };

  auto on_key = [=]() mutable {
    const int k = key_code();
    if (k == KEY_ENTER || k == KEY_KP_ENTER) {
      run_submit();
      return;
    }

    if (k == KEY_UP || k == KEY_DOWN) {
      auto hist = history.get();
      if (hist.empty()) {
        return;
      }

      auto idx = hist_idx.get();
      if (idx < 0) {
        idx = static_cast<std::int64_t>(hist.size());
      }

      if (k == KEY_UP) {
        if (idx == static_cast<std::int64_t>(hist.size())) {
          draft.set(input.get());
        }
        idx = std::max<std::int64_t>(0, idx - 1);
        hist_idx.set(idx);
        input.set(hist[static_cast<std::size_t>(idx)]);
      } else {
        idx = std::min<std::int64_t>(static_cast<std::int64_t>(hist.size()), idx + 1);
        hist_idx.set(idx);
        if (idx == static_cast<std::int64_t>(hist.size())) {
          input.set(draft.get());
        } else {
          input.set(hist[static_cast<std::size_t>(idx)]);
        }
      }
    }
  };

  auto b = view("Column");
  b.prop("padding", 18);
  b.prop("spacing", 10);
  b.prop("cross_align", "stretch");
  b.children([&](auto &c) {
    c.add(view("Row")
              .prop("spacing", 10)
              .prop("cross_align", "center")
              .children({
                  view("Text")
                      .prop("value", "duorou terminal")
                      .prop("font_size", 18.0)
                      .build(),
                  view("Spacer").build(),
                  view("Checkbox")
                      .key("follow")
                      .prop("label", "Follow")
                      .prop("checked", follow.get())
                      .event("pointer_up", on_pointer_up([follow]() mutable {
                               follow.set(!follow.get());
                             }))
                      .build(),
                  view("Button")
                      .prop("title", "Clear")
                      .event("pointer_up", on_pointer_up([lines]() mutable {
                               lines.set(std::vector<std::string>{});
                             }))
                      .build(),
              })
              .build());

    auto sv = view("ScrollView");
    sv.key("terminal_scroll");
    sv.prop("clip", true);
    sv.prop("padding", 12);
    sv.prop("height", 420.0);
    sv.prop("bg", 0xFF141414);
    sv.prop("border", 0xFF2A2A2A);
    sv.prop("border_width", 1.0);
    if (follow.get()) {
      sv.prop("scroll_y", 1000000000.0);
    }

    sv.children([&](auto &sv_children) {
      auto col = view("Column");
      col.prop("spacing", 4.0);
      col.prop("cross_align", "start");
      col.children([&](auto &cc) {
        const auto v = lines.get();
        for (const auto &line : v) {
          cc.add(view("Text")
                     .prop("value", line)
                     .prop("font_size", 14.0)
                     .prop("color", 0xFFB0B0B0)
                     .build());
        }
      });
      sv_children.add(std::move(col).build());
    });

    c.add(std::move(sv).build());

    c.add(view("Row")
              .prop("spacing", 10.0)
              .prop("cross_align", "center")
              .children({
                  view("Text")
                      .prop("value", prompt.get())
                      .prop("font_size", 16.0)
                      .prop("color", 0xFFEAEAEA)
                      .build(),
                  view("TextField")
                      .key("terminal_input")
                      .prop("binding", duorou::ui::bind(input))
                      .prop("placeholder", "Type a command and press Enter")
                      .prop("min_width", 520.0)
                      .event("key_up", on_key_up(on_key))
                      .build(),
                  view("Button")
                      .prop("title", "Run")
                      .event("pointer_up", on_pointer_up([run_submit]() mutable {
                               run_submit();
                             }))
                      .build(),
              })
              .build());
  });

  return std::move(b).build();
}

inline ViewNode navigation_container_view() {
  auto tab = local_state<std::int64_t>("nav:tab", 0);
  auto route = local_state<std::int64_t>("nav:route", 0);
  auto detail_id = local_state<std::int64_t>("nav:detail_id", 0);

  auto set_tab = [=](std::int64_t v) mutable { tab.set(v); };
  auto push_detail = [=](std::int64_t id) mutable {
    detail_id.set(id);
    route.set(1);
  };
  auto pop = [=]() mutable { route.set(0); };

  auto top_bar = [=]() mutable {
    if (route.get() == 0) {
      return view("Row")
          .prop("spacing", 10.0)
          .prop("cross_align", "center")
          .children({
              view("Text")
                  .prop("value", "Navigation & Containers")
                  .prop("font_size", 18.0)
                  .build(),
              view("Spacer").build(),
              view("Button")
                  .prop("title", "Tab: Home")
                  .event("pointer_up", on_pointer_up([=]() mutable { set_tab(0); }))
                  .build(),
              view("Button")
                  .prop("title", "Tab: Settings")
                  .event("pointer_up", on_pointer_up([=]() mutable { set_tab(1); }))
                  .build(),
          })
          .build();
    }

    return view("Row")
        .prop("spacing", 10.0)
        .prop("cross_align", "center")
        .children({
            view("Button")
                .prop("title", "Back")
                .event("pointer_up", on_pointer_up([=]() mutable { pop(); }))
                .build(),
            view("Text")
                .prop("value",
                      std::string{"Detail #"} + std::to_string(detail_id.get()))
                .prop("font_size", 18.0)
                .build(),
            view("Spacer").build(),
            view("Text")
                .prop("value", std::string{"(Tab "} + std::to_string(tab.get()) +
                                   ")")
                .prop("font_size", 12.0)
                .prop("color", 0xFFB0B0B0)
                .build(),
        })
        .build();
  };

  auto root = view("Column");
  root.prop("padding", 18.0);
  root.prop("spacing", 12.0);
  root.prop("cross_align", "stretch");
  root.children([&](auto &c) {
    c.add(top_bar());

    auto sv = view("ScrollView");
    sv.key("nav_scroll");
    sv.prop("clip", true);
    sv.prop("padding", 10.0);
    sv.prop("height", 520.0);
    sv.prop("bg", 0xFF141414);
    sv.prop("border", 0xFF2A2A2A);
    sv.prop("border_width", 1.0);

    auto content = view("Column");
    content.prop("spacing", 12.0);
    content.prop("cross_align", "stretch");

    std::vector<ViewNode> nodes;
    nodes.reserve(6);

    nodes.push_back(duorou::ui::Group({
        duorou::ui::Section(
            "Container: Group + Section",
            {
                view("Text")
                    .prop("value",
                          "Group is logical; Section is a titled container.")
                    .prop("font_size", 14.0)
                    .prop("color", 0xFFB0B0B0)
                    .build(),
            }),
    }));

    if (tab.get() == 0) {
      if (route.get() == 0) {
        nodes.push_back(duorou::ui::Section(
            "Navigation (模拟)",
            {
                view("Text")
                    .prop("value",
                          "Click buttons to switch views (simulate NavigationLink).")
                    .prop("font_size", 14.0)
                    .prop("color", 0xFFB0B0B0)
                    .build(),
                view("Button")
                    .prop("title", "Open Detail 1")
                    .event("pointer_up",
                           on_pointer_up([=]() mutable { push_detail(1); }))
                    .build(),
                view("Button")
                    .prop("title", "Open Detail 2")
                    .event("pointer_up",
                           on_pointer_up([=]() mutable { push_detail(2); }))
                    .build(),
            }));
      } else {
        nodes.push_back(duorou::ui::Section(
            "Detail",
            {
                view("Text")
                    .prop("value",
                          std::string{"Detail page, detail_id="} +
                              std::to_string(detail_id.get()))
                    .prop("font_size", 14.0)
                    .prop("color", 0xFFEAEAEA)
                    .build(),
                view("Text")
                    .prop("value", "More content can go here; it can scroll.")
                    .prop("font_size", 14.0)
                    .prop("color", 0xFFB0B0B0)
                    .build(),
            }));
      }
    } else {
      nodes.push_back(duorou::ui::Section(
          "Tab: Settings (模拟 TabView)",
          {
              view("Text")
                  .prop("value",
                        "Switch content by tab state (simulate TabView).")
                  .prop("font_size", 14.0)
                  .prop("color", 0xFFB0B0B0)
                  .build(),
              view("Checkbox")
                  .key("nav_setting_a")
                  .prop("label", "Enable A")
                  .prop("checked", true)
                  .build(),
              view("Checkbox")
                  .key("nav_setting_b")
                  .prop("label", "Enable B")
                  .prop("checked", false)
                  .build(),
          }));
    }

    nodes.push_back(duorou::ui::Section(
        "Layout (模拟 SplitView)",
        {
            view("Row")
                .prop("spacing", 10.0)
                .prop("cross_align", "stretch")
                .children({
                    view("Box")
                        .prop("padding", 10.0)
                        .prop("bg", 0xFF1E1E1E)
                        .prop("border", 0xFF2A2A2A)
                        .prop("border_width", 1.0)
                        .children({
                            view("Text")
                                .prop("value", "Sidebar")
                                .prop("font_size", 14.0)
                                .prop("color", 0xFFEAEAEA)
                                .build(),
                        })
                        .build(),
                    view("Box")
                        .prop("padding", 10.0)
                        .prop("bg", 0xFF1E1E1E)
                        .prop("border", 0xFF2A2A2A)
                        .prop("border_width", 1.0)
                        .children({
                            view("Text")
                                .prop("value", "Detail Pane")
                                .prop("font_size", 14.0)
                                .prop("color", 0xFFEAEAEA)
                                .build(),
                        })
                        .build(),
                })
                .build(),
        }));

    content.children([&](auto &cc) {
      for (auto &n : nodes) {
        cc.add(std::move(n));
      }
    });

    sv.children({std::move(content).build()});

    c.add(std::move(sv).build());
  });

  return std::move(root).build();
}

} // namespace duorou::ui::examples
