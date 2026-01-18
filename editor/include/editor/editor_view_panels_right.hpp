#pragma once

#include <editor/editor_view_history.hpp>
#include <editor/editor_view_tree_ops.hpp>

#include <duorou/ui/runtime.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace duorou::ui::editor {

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

inline ViewNode props_panel(StateHandle<ViewNode> design_root,
                            StateHandle<std::string> selected_key,
                            StateHandle<std::vector<ViewNode>> history,
                            StateHandle<std::int64_t> history_idx) {
  enum class Kind { Str, Num, Color, Bool };
  struct Item {
    const char *label;
    const char *key;
    Kind kind;
  };

  static const Item items[] = {
      {"title", "title", Kind::Str},
      {"value", "value", Kind::Str},
      {"placeholder", "placeholder", Kind::Str},
      {"variant", "variant", Kind::Str},
      {"cross_align", "cross_align", Kind::Str},
      {"padding", "padding", Kind::Num},
      {"spacing", "spacing", Kind::Num},
      {"width", "width", Kind::Num},
      {"height", "height", Kind::Num},
      {"font_size", "font_size", Kind::Num},
      {"opacity", "opacity", Kind::Num},
      {"bg", "bg", Kind::Color},
      {"border", "border", Kind::Color},
      {"border_width", "border_width", Kind::Num},
      {"clip", "clip", Kind::Bool},
  };

  auto props_last_key = local_state<std::string>("editor:props_last_key", "");
  auto selected = selected_key.get();
  const auto root = design_root.get();
  const auto *sel_node = find_node_by_key(root, selected);

  std::vector<StateHandle<std::string>> field_states;
  field_states.reserve(std::size(items));
  for (const auto &it : items) {
    field_states.push_back(
        local_state<std::string>(std::string{"editor:prop:"} + it.key, ""));
  }

  auto stringify = [&](const PropValue &v, Kind k) -> std::string {
    if (const auto *s = std::get_if<std::string>(&v)) {
      return *s;
    }
    if (const auto *b = std::get_if<bool>(&v)) {
      return *b ? "true" : "false";
    }
    if (const auto *d = std::get_if<double>(&v)) {
      char buf[64]{};
      std::snprintf(buf, sizeof(buf), "%.3f", *d);
      return std::string{buf};
    }
    if (const auto *i = std::get_if<std::int64_t>(&v)) {
      if (k == Kind::Color) {
        char buf[64]{};
        std::snprintf(buf, sizeof(buf), "0x%llX",
                      static_cast<unsigned long long>(*i));
        return std::string{buf};
      }
      return std::to_string(static_cast<long long>(*i));
    }
    return {};
  };

  if (props_last_key.get() != selected) {
    for (std::size_t i = 0; i < std::size(items); ++i) {
      const auto &it = items[i];
      std::string v;
      if (sel_node) {
        if (const auto prop_it = sel_node->props.find(std::string{it.key});
            prop_it != sel_node->props.end()) {
          v = stringify(prop_it->second, it.kind);
        }
      }
      field_states[i].set(std::move(v));
    }
    props_last_key.set(selected);
  }

  const auto idx = history_idx.get();
  const auto hist = history.get();
  const bool can_undo = idx > 0;
  const bool can_redo = idx >= 0 && (idx + 1) < static_cast<std::int64_t>(hist.size());
  const bool can_delete = sel_node && selected != "design:root";

  auto apply = [design_root, selected_key, selected,
                history, history_idx, field_states]() mutable {
    auto root = design_root.get();
    if (root.type.empty()) {
      return;
    }
    auto *n = find_node_by_key_mut(root, selected);
    if (!n) {
      return;
    }

    auto parse_bool = [&](std::string_view s) -> std::optional<bool> {
      if (s == "true" || s == "1") {
        return true;
      }
      if (s == "false" || s == "0") {
        return false;
      }
      return std::nullopt;
    };

    auto trim = [&](std::string &s) -> void {
      while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.erase(s.begin());
      }
      while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
      }
    };

    for (std::size_t i = 0; i < std::size(items); ++i) {
      const auto &it = items[i];
      auto s = field_states[i].get();
      trim(s);
      if (s.empty()) {
        n->props.erase(std::string{it.key});
        continue;
      }
      if (it.kind == Kind::Str) {
        n->props.insert_or_assign(std::string{it.key}, PropValue{std::move(s)});
        continue;
      }
      if (it.kind == Kind::Bool) {
        if (auto b = parse_bool(s)) {
          n->props.insert_or_assign(std::string{it.key}, PropValue{*b});
        }
        continue;
      }
      if (it.kind == Kind::Color) {
        const char *begin = s.c_str();
        int base = 10;
        if (s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
          begin += 2;
          base = 16;
        } else if (!s.empty() && s[0] == '#') {
          begin += 1;
          base = 16;
        }
        char *end = nullptr;
        const auto v = std::strtoll(begin, &end, base);
        if (end && end != begin) {
          n->props.insert_or_assign(std::string{it.key}, PropValue{static_cast<std::int64_t>(v)});
        }
        continue;
      }

      char *end = nullptr;
      const auto v = std::strtod(s.c_str(), &end);
      if (end && end != s.c_str()) {
        n->props.insert_or_assign(std::string{it.key}, PropValue{static_cast<double>(v)});
      }
    }

    push_history(history, history_idx, root);
    design_root.set(std::move(root));
    selected_key.set(selected);
  };

  auto undo = [history, history_idx, design_root](bool redo) mutable {
    auto idx = history_idx.get();
    auto h = history.get();
    if (h.empty() || idx < 0 || idx >= static_cast<std::int64_t>(h.size())) {
      return;
    }
    const auto next = redo ? (idx + 1) : (idx - 1);
    if (next < 0 || next >= static_cast<std::int64_t>(h.size())) {
      return;
    }
    history_idx.set(next);
    design_root.set(h[static_cast<std::size_t>(next)]);
  };

  auto del = [design_root, selected_key, history, history_idx, selected]() mutable {
    if (selected.empty() || selected == "design:root") {
      return;
    }
    auto root = design_root.get();
    if (root.type.empty()) {
      return;
    }
    if (!take_node_by_key_mut(root, selected)) {
      return;
    }
    push_history(history, history_idx, root);
    design_root.set(std::move(root));
    selected_key.set("design:root");
  };

  return view("ScrollView")
      .prop("clip", true)
      .prop("default_width", 360.0)
      .prop("default_height", 600.0)
      .children({
          view("Column")
              .prop("spacing", 10.0)
              .prop("cross_align", "stretch")
              .children([&](auto &c) {
                c.add(view("Text")
                          .prop("value", "Props")
                          .prop("font_size", 12.0)
                          .prop("color", 0xFFB0B0B0)
                          .build());

                std::string header = selected.empty() ? std::string{"(none)"} : selected;
                if (sel_node) {
                  header = sel_node->type + "  " + header;
                }
                c.add(view("Text")
                          .prop("value", header)
                          .prop("font_size", 13.0)
                          .prop("color", 0xFFE0E0E0)
                          .build());

                c.add(view("Row")
                          .prop("spacing", 8.0)
                          .prop("cross_align", "center")
                          .children({
                              view("Button")
                                  .prop("title", "Undo")
                                  .prop("disabled", !can_undo)
                                  .event("pointer_up", on_pointer_up([undo]() mutable { undo(false); }))
                                  .build(),
                              view("Button")
                                  .prop("title", "Redo")
                                  .prop("disabled", !can_redo)
                                  .event("pointer_up", on_pointer_up([undo]() mutable { undo(true); }))
                                  .build(),
                              view("Spacer").build(),
                              view("Button")
                                  .prop("title", "Apply")
                                  .prop("variant", "primary")
                                  .event("pointer_up", on_pointer_up([apply]() mutable { apply(); }))
                                  .build(),
                              view("Button")
                                  .prop("title", "Delete")
                                  .prop("disabled", !can_delete)
                                  .event("pointer_up", on_pointer_up([del]() mutable { del(); }))
                                  .build(),
                          })
                          .build());

                c.add(view("Divider").prop("thickness", 1.0).prop("color", 0xFF2A2A2A).build());

                for (std::size_t i = 0; i < std::size(items); ++i) {
                  const auto &it = items[i];
                  c.add(view("Text")
                            .prop("value", it.label)
                            .prop("font_size", 12.0)
                            .prop("color", 0xFFB0B0B0)
                            .build());
                  c.add(TextField(field_states[i], std::string{"editor:prop_field:"} + it.key,
                                  std::string{"("} + it.key + ")"));
                }
              })
              .build(),
      })
      .build();
}

} // namespace duorou::ui::editor

