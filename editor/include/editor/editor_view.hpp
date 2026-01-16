#pragma once

#include <duorou/ui/runtime.hpp>

#include <algorithm>
#include <cstdio>
#include <cctype>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

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

inline ViewNode preview_panel(TextureHandle demo_tex, float width, float height,
                              int layout_id,
                              StateHandle<std::string> selected_key) {
  auto content = [&]() -> ViewNode {
    if (layout_id == 1) {
      return view("Column")
          .prop("spacing", 12.0)
          .prop("cross_align", "start")
          .children({
              view("Text")
                  .prop("value", "Login Form")
                  .prop("font_size", 16.0)
                  .prop("color", 0xFFE0E0E0)
                  .build(),
              onTapGesture(
                  view("TextField")
                      .key("preview:form:username")
                      .prop("value", "")
                      .prop("placeholder", "Username")
                      .prop("width", 320.0)
                      .build(),
                  [selected_key]() mutable { selected_key.set("preview:form:username"); }),
              onTapGesture(
                  view("TextField")
                      .key("preview:form:password")
                      .prop("value", "")
                      .prop("placeholder", "Password")
                      .prop("width", 320.0)
                      .build(),
                  [selected_key]() mutable { selected_key.set("preview:form:password"); }),
              view("Row")
                  .prop("spacing", 10.0)
                  .prop("cross_align", "center")
                  .children({
                      onTapGesture(
                          view("Button")
                              .key("preview:form:submit")
                              .prop("title", "Sign In")
                              .prop("variant", "primary")
                              .build(),
                          [selected_key]() mutable { selected_key.set("preview:form:submit"); }),
                      onTapGesture(
                          view("Button")
                              .key("preview:form:cancel")
                              .prop("title", "Cancel")
                              .build(),
                          [selected_key]() mutable { selected_key.set("preview:form:cancel"); }),
                  })
                  .build(),
          })
          .build();
    }
    if (layout_id == 2) {
      return view("Column")
          .prop("spacing", 12.0)
          .prop("cross_align", "start")
          .children({
              view("Text")
                  .prop("value", "Grid Layout")
                  .prop("font_size", 16.0)
                  .prop("color", 0xFFE0E0E0)
                  .build(),
              view("Grid")
                  .prop("columns", 3)
                  .prop("spacing", 10.0)
                  .children({
                      onTapGesture(
                          view("Button").key("preview:grid:1").prop("title", "One").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:1"); }),
                      onTapGesture(
                          view("Button").key("preview:grid:2").prop("title", "Two").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:2"); }),
                      onTapGesture(
                          view("Button").key("preview:grid:3").prop("title", "Three").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:3"); }),
                      onTapGesture(
                          view("Button").key("preview:grid:4").prop("title", "Four").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:4"); }),
                      onTapGesture(
                          view("Button").key("preview:grid:5").prop("title", "Five").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:5"); }),
                      onTapGesture(
                          view("Button").key("preview:grid:6").prop("title", "Six").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:6"); }),
                      onTapGesture(
                          view("Button").key("preview:grid:7").prop("title", "Seven").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:7"); }),
                      onTapGesture(
                          view("Button").key("preview:grid:8").prop("title", "Eight").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:8"); }),
                      onTapGesture(
                          view("Button").key("preview:grid:9").prop("title", "Nine").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:9"); }),
                  })
                  .build(),
          })
          .build();
    }

    return view("Row")
        .prop("spacing", 12.0)
        .prop("cross_align", "center")
        .children({
            onTapGesture(
                view("Image")
                    .key("preview:image")
                    .prop("texture", static_cast<std::int64_t>(demo_tex))
                    .prop("width", 64.0)
                    .prop("height", 64.0)
                    .build(),
                [selected_key]() mutable { selected_key.set("preview:image"); }),
            view("Column")
                .prop("spacing", 8.0)
                .prop("cross_align", "start")
                .children({
                    view("Row")
                        .prop("spacing", 10.0)
                        .prop("cross_align", "center")
                        .children({
                            onTapGesture(
                                view("Button")
                                    .key("preview:button")
                                    .prop("title", "Button")
                                    .prop("variant", "primary")
                                    .build(),
                                [selected_key]() mutable { selected_key.set("preview:button"); }),
                            onTapGesture(
                                view("Text")
                                    .key("preview:text")
                                    .prop("value", "Text")
                                    .build(),
                                [selected_key]() mutable { selected_key.set("preview:text"); }),
                        })
                        .build(),
                    onTapGesture(
                        view("TextField")
                            .key("preview:input")
                            .prop("value", "")
                            .prop("placeholder", "Input")
                            .prop("width", 260.0)
                            .build(),
                        [selected_key]() mutable { selected_key.set("preview:input"); }),
                })
                .build(),
        })
        .build();
  }();

  return view("Box")
      .key("preview:panel")
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
                  std::move(content),
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

inline const ViewNode *find_node_by_key(const ViewNode &root,
                                        std::string_view key) {
  if (key.empty()) {
    return nullptr;
  }
  if (root.key == key) {
    return &root;
  }
  for (const auto &c : root.children) {
    if (const auto *r = find_node_by_key(c, key)) {
      return r;
    }
  }
  return nullptr;
}

inline std::string format_prop_value(const Props &props, std::string_view k) {
  const auto it = props.find(std::string{k});
  if (it == props.end()) {
    return {};
  }
  if (const auto *s = std::get_if<std::string>(&it->second)) {
    return *s;
  }
  if (const auto *i = std::get_if<std::int64_t>(&it->second)) {
    char buf[64]{};
    std::snprintf(buf, sizeof(buf), "0x%llX",
                  static_cast<unsigned long long>(*i));
    return std::string{buf};
  }
  if (const auto *d = std::get_if<double>(&it->second)) {
    char buf[64]{};
    std::snprintf(buf, sizeof(buf), "%.3f", *d);
    return std::string{buf};
  }
  if (const auto *b = std::get_if<bool>(&it->second)) {
    return *b ? "true" : "false";
  }
  return {};
}

inline std::string style_src_for(const Props &props, std::string_view k) {
  std::string src_key = "style_src.";
  src_key += k;
  return prop_as_string(props, src_key, "");
}

inline std::string style_chain_for(const Props &props, std::string_view k) {
  std::string chain_key = "style_chain.";
  chain_key += k;
  return prop_as_string(props, chain_key, "");
}

inline std::string style_prev_src_for(const Props &props, std::string_view k) {
  std::string src_key = "style_prev_src.";
  src_key += k;
  return prop_as_string(props, src_key, "");
}

inline std::string lower_ascii(std::string s) {
  for (auto &ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return s;
}

inline bool contains_ci(std::string_view haystack, std::string_view needle) {
  if (needle.empty()) {
    return true;
  }
  return lower_ascii(std::string{haystack}).find(lower_ascii(std::string{needle})) !=
         std::string::npos;
}

inline std::string join_str(const std::vector<std::string> &parts,
                            std::string_view sep) {
  std::string out;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) {
      out.append(sep);
    }
    out.append(parts[i]);
  }
  return out;
}

inline std::string unique_name_like(const std::vector<std::string> &names,
                                    std::string base) {
  std::unordered_set<std::string> set;
  set.reserve(names.size() * 2 + 1);
  for (const auto &n : names) {
    set.insert(n);
  }
  if (!set.contains(base)) {
    return base;
  }
  for (int i = 2; i < 10000; ++i) {
    std::string cand = base + " " + std::to_string(i);
    if (!set.contains(cand)) {
      return cand;
    }
  }
  return base + " Copy";
}

inline std::string format_theme_errors(const std::vector<StyleParseError> &errs) {
  if (errs.empty()) {
    return {};
  }
  std::string out;
  const std::size_t max_items = 8;
  const std::size_t take = std::min(max_items, errs.size());
  for (std::size_t i = 0; i < take; ++i) {
    const auto &e = errs[i];
    out += "line ";
    out += std::to_string(e.line);
    out += ":";
    out += std::to_string(e.column);
    out += " ";
    out += e.message;
    if (i + 1 < take) {
      out.push_back('\n');
    }
  }
  if (errs.size() > take) {
    out += "\n...";
  }
  return out;
}

inline std::unordered_map<std::string, PropValue>
flatten_sheet(const StyleSheetModel &sheet) {
  std::unordered_map<std::string, PropValue> out;
  out.reserve(sheet.global.size() + sheet.components.size() * 16);
  for (const auto &kv : sheet.global) {
    out.insert_or_assign("Global." + kv.first, kv.second);
  }
  for (const auto &ckv : sheet.components) {
    const auto &cname = ckv.first;
    const auto &comp = ckv.second;
    for (const auto &kv : comp.props) {
      out.insert_or_assign(cname + "." + kv.first, kv.second);
    }
    for (const auto &skv : comp.states) {
      for (const auto &kv : skv.second) {
        out.insert_or_assign(cname + "." + skv.first + "." + kv.first, kv.second);
      }
    }
    for (const auto &vkv : comp.variants) {
      const auto &vname = vkv.first;
      const auto &var = vkv.second;
      for (const auto &kv : var.props) {
        out.insert_or_assign(cname + "." + vname + "." + kv.first, kv.second);
      }
      for (const auto &skv : var.states) {
        for (const auto &kv : skv.second) {
          out.insert_or_assign(cname + "." + vname + "." + skv.first + "." + kv.first,
                               kv.second);
        }
      }
    }
  }
  return out;
}

inline std::string sheet_diff_summary(const StyleSheetModel &old_sheet,
                                      const StyleSheetModel &new_sheet) {
  const auto old_map = flatten_sheet(old_sheet);
  const auto new_map = flatten_sheet(new_sheet);

  std::size_t added = 0;
  std::size_t removed = 0;
  std::size_t changed = 0;
  std::vector<std::string> changed_keys;
  changed_keys.reserve(16);

  for (const auto &kv : new_map) {
    const auto it = old_map.find(kv.first);
    if (it == old_map.end()) {
      ++added;
      if (changed_keys.size() < 8) {
        changed_keys.push_back(std::string{"+"} + kv.first);
      }
      continue;
    }
    if (it->second != kv.second) {
      ++changed;
      if (changed_keys.size() < 8) {
        changed_keys.push_back(std::string{"~"} + kv.first);
      }
    }
  }
  for (const auto &kv : old_map) {
    if (!new_map.contains(kv.first)) {
      ++removed;
      if (changed_keys.size() < 8) {
        changed_keys.push_back(std::string{"-"} + kv.first);
      }
    }
  }

  std::string out =
      "diff: +" + std::to_string(added) + " -" + std::to_string(removed) +
      " ~" + std::to_string(changed) + " (total " +
      std::to_string(new_map.size()) + ")";
  if (!changed_keys.empty()) {
    out.push_back('\n');
    out += join_str(changed_keys, "\n");
  }
  return out;
}

inline ViewNode style_row(std::string name, std::string value,
                          std::string source) {
  return view("Box")
      .prop("padding", 10.0)
      .prop("bg", 0xFF151515)
      .prop("border", 0xFF2A2A2A)
      .prop("border_width", 1.0)
      .children({
          view("Column")
              .prop("spacing", 6.0)
              .prop("cross_align", "stretch")
              .children({
                  view("Row")
                      .prop("spacing", 10.0)
                      .prop("cross_align", "center")
                      .children({
                          view("Text").prop("value", std::move(name)).build(),
                          view("Spacer").build(),
                          view("Text")
                              .prop("value", std::move(value))
                              .prop("color", 0xFFB0B0B0)
                              .build(),
                      })
                      .build(),
                  view("Text")
                      .prop("value", std::move(source))
                      .prop("font_size", 12.0)
                      .prop("color", 0xFFB0B0B0)
                      .build(),
              })
              .build(),
      })
      .build();
}

inline ViewNode style_panel(std::string selected_key, const ViewNode *selected) {
  auto query = local_state<std::string>("editor:style_query", "");
  auto src_filter = local_state<std::string>("editor:style_src_filter", "");

  std::vector<std::string> style_keys;
  if (selected) {
    style_keys.reserve(selected->props.size());
    for (const auto &kv : selected->props) {
      const std::string_view k = kv.first;
      constexpr std::string_view prefix = "style_src.";
      if (!k.starts_with(prefix)) {
        continue;
      }
      std::string base_key{k.substr(prefix.size())};
      if (base_key.empty()) {
        continue;
      }
      style_keys.push_back(std::move(base_key));
    }
    std::sort(style_keys.begin(), style_keys.end());
    style_keys.erase(std::unique(style_keys.begin(), style_keys.end()),
                     style_keys.end());
  }

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
                          .prop("value", "Property")
                          .prop("font_size", 12.0)
                          .prop("color", 0xFFB0B0B0)
                          .build());
                c.add(view("Text")
                          .prop("value",
                                selected_key.empty()
                                    ? std::string{"Selected: (none)"}
                                    : (std::string{"Selected: "} + selected_key))
                          .prop("font_size", 12.0)
                          .prop("color", 0xFFB0B0B0)
                          .build());
                c.add(view("Text")
                          .prop("value", "Search")
                          .prop("font_size", 12.0)
                          .prop("color", 0xFFB0B0B0)
                          .build());
                c.add(TextField(query, "editor:style_query_input", "prop name"));
                c.add(TextField(src_filter, "editor:style_src_filter_input",
                                "source filter (e.g. Global / Inline / Button.primary.hover)"));

                if (!selected || style_keys.empty()) {
                  c.add(view("Text")
                            .prop("value", "Click a component to inspect")
                            .prop("font_size", 12.0)
                            .prop("color", 0xFFB0B0B0)
                            .build());
                  return;
                }

                const auto q = query.get();
                const auto sf = src_filter.get();
                for (const auto &k : style_keys) {
                  if (!q.empty() && !contains_ci(k, q)) {
                    continue;
                  }

                  std::string info;
                  std::string filter_blob;
                  {
                    const auto src = style_src_for(selected->props, k);
                    if (!src.empty()) {
                      info = "from ";
                      info += src;
                    }
                    filter_blob += src;
                  }
                  {
                    const auto chain = style_chain_for(selected->props, k);
                    if (!chain.empty()) {
                      if (!info.empty()) {
                        info.push_back('\n');
                      }
                      info += "chain: ";
                      info += chain;
                    }
                    if (!filter_blob.empty()) {
                      filter_blob.push_back('\n');
                    }
                    filter_blob += chain;
                  }
                  {
                    std::string prev_key = "style_prev.";
                    prev_key += k;
                    const auto prev = format_prop_value(selected->props, prev_key);
                    if (!prev.empty()) {
                      if (!info.empty()) {
                        info.push_back('\n');
                      }
                      info += "overridden: ";
                      info += prev;
                      const auto prev_src = style_prev_src_for(selected->props, k);
                      if (!prev_src.empty()) {
                        info += " (from ";
                        info += prev_src;
                        info.push_back(')');
                      }
                      if (!filter_blob.empty()) {
                        filter_blob.push_back('\n');
                      }
                      filter_blob += prev_src;
                    }
                  }

                  if (!sf.empty() && !contains_ci(filter_blob, sf)) {
                    continue;
                  }
                  c.add(style_row(k, format_prop_value(selected->props, k), std::move(info)));
                }
              })
              .build(),
      })
      .build();
}

inline ViewNode editor_view(TextureHandle demo_tex_handle) {
  return GeometryReader([demo_tex_handle](SizeF size) {
    auto style_mgr = StateObject<StyleManager>("editor:style_mgr");
    auto style_mgr_ptr = style_mgr.get();
    if (style_mgr_ptr && style_mgr_ptr->theme_count() == 0) {
      const char *light_toml = R"(
[Theme]
name = "Light"

[Global]
bg = 0xFFF4F4F4
color = 0xFF101010
font_size = 14

[Button]
bg = 0xFFE8E8E8
color = 0xFF101010
padding = 10
border = 0xFFCDCDCD
border_width = 1

[Button.primary]
bg = 0xFF2D6BFF
color = 0xFFFFFFFF

[Button.primary.hover]
bg = 0xFF3A7BFF

[Button.primary.active]
bg = 0xFF2458D8

[Button.loading]
opacity = 0.75

[Button.primary.loading]
opacity = 0.75

[Button.disabled]
bg = 0xFFE0E0E0
color = 0xFF8A8A8A

[Text]
color = 0xFF101010
font_size = 14

[TextField]
bg = 0xFFFFFFFF
border = 0xFFCDCDCD
border_width = 1
padding = 10
placeholder_color = 0xFF7A7A7A

[TextField.focused]
border = 0xFF2D6BFF
)";

      const char *dark_toml = R"(
[Theme]
name = "Dark"
base = "Light"

[Global]
bg = 0xFF101010
color = 0xFFE0E0E0
font_size = 14

[Button]
bg = 0xFF202020
color = 0xFFEFEFEF
padding = 10
border = 0xFF3A3A3A
border_width = 1

[Button.primary]
bg = 0xFF2D6BFF
color = 0xFFFFFFFF

[Button.primary.hover]
bg = 0xFF3A7BFF

[Button.primary.active]
bg = 0xFF2458D8

[Button.loading]
opacity = 0.75

[Button.primary.loading]
opacity = 0.75

[Button.disabled]
bg = 0xFF202020
color = 0xFF808080

[Text]
color = 0xFFE0E0E0
font_size = 14

[TextField]
bg = 0xFF151515
border = 0xFF2A2A2A
border_width = 1
padding = 10
placeholder_color = 0xFF808080

[TextField.focused]
border = 0xFF3A7BFF
)";

      const char *hc_toml = R"(
[Theme]
name = "HighContrast"

[Global]
bg = 0xFF000000
color = 0xFFFFFFFF
font_size = 14

[Button]
bg = 0xFF000000
color = 0xFFFFFFFF
padding = 10
border = 0xFFFFFFFF
border_width = 2

[Button.primary]
bg = 0xFFFFFF00
color = 0xFF000000

[Button.primary.hover]
bg = 0xFFFFD000

[Button.primary.active]
bg = 0xFFFFA000

[Button.loading]
opacity = 0.75

[Button.primary.loading]
opacity = 0.75

[Button.disabled]
bg = 0xFF000000
color = 0xFF808080

[Text]
color = 0xFFFFFFFF
font_size = 14

[TextField]
bg = 0xFF000000
border = 0xFFFFFFFF
border_width = 2
padding = 10
placeholder_color = 0xFFB0B0B0

[TextField.focused]
border = 0xFFFFFF00
)";

      style_mgr_ptr->upsert_theme(parse_theme_toml(light_toml).theme);
      style_mgr_ptr->upsert_theme(parse_theme_toml(dark_toml).theme);
      style_mgr_ptr->upsert_theme(parse_theme_toml(hc_toml).theme);
      style_mgr_ptr->set_active_theme("Dark");
    }
    if (style_mgr_ptr) {
      provide_environment_object("style.manager", style_mgr_ptr);
    }

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

    auto preview_state = local_state<std::string>("editor:preview_state", "");
    auto preview_layout = local_state<std::int64_t>("editor:preview_layout", 0);
    auto preview_zoom = local_state<double>("editor:preview_zoom", 1.0);
    auto selected_key =
        local_state<std::string>("editor:selected_key", "preview:button");
    auto preview = preview_panel(demo_tex_handle, center_w, preview_h,
                                 static_cast<int>(preview_layout.get()),
                                 selected_key);
    preview.props.insert_or_assign("render_scale", PropValue{preview_zoom.get()});
    auto zoom_root =
        view("Box")
            .key("preview:zoom_root")
            .prop("width", center_w * static_cast<float>(preview_zoom.get()))
            .prop("height", preview_h * static_cast<float>(preview_zoom.get()))
            .children({std::move(preview)})
            .build();
    preview = view("ScrollView")
                  .key("preview:scroll")
                  .prop("clip", true)
                  .prop("scroll_axis", "both")
                  .prop("width", center_w)
                  .prop("height", preview_h)
                  .prop("default_width", center_w)
                  .prop("default_height", preview_h)
                  .children({std::move(zoom_root)})
                  .build();
    {
      const auto st = preview_state.get();
      if (!st.empty()) {
        auto apply_state = [&](auto &&self, ViewNode &n) -> void {
          n.props.insert_or_assign("style_state", PropValue{st});
          for (auto &c : n.children) {
            self(self, c);
          }
        };
        apply_state(apply_state, preview);
      }
    }
    if (style_mgr_ptr) {
      style_mgr_ptr->apply_to_tree(preview);
    }
    const auto selected_key_s = selected_key.get();
    const auto *selected_node = find_node_by_key(preview, selected_key_s);
    auto style_panel_node = style_panel(selected_key_s, selected_node);

    auto theme_pop_open = local_state<bool>("editor:theme_pop_open", false);
    auto theme_pop_x = local_state<double>("editor:theme_pop_x", 0.0);
    auto theme_pop_y = local_state<double>("editor:theme_pop_y", 0.0);
    auto theme_new_name = local_state<std::string>("editor:theme_new_name", "");
    auto theme_new_base = local_state<std::string>("editor:theme_new_base", "");
    auto theme_copy_name = local_state<std::string>("editor:theme_copy_name", "");
    auto hot_theme_enabled = local_state<bool>("editor:hot_theme_enabled", false);
    auto hot_theme_path = local_state<std::string>("editor:hot_theme_path", "");
    auto hot_theme_mtime = local_state<std::int64_t>("editor:hot_theme_mtime", 0);
    auto hot_theme_status = local_state<std::string>("editor:hot_theme_status", "");
    auto hot_theme_error = local_state<std::string>("editor:hot_theme_error", "");

    auto reload_hot_theme = [style_mgr_ptr, hot_theme_path, hot_theme_mtime,
                             hot_theme_status, hot_theme_error](bool force) mutable {
      if (!style_mgr_ptr) {
        return;
      }
      const auto path = hot_theme_path.get();
      if (path.empty()) {
        return;
      }
      const auto set_error = [&](std::string v) {
        if (hot_theme_error.get() != v) {
          hot_theme_error.set(std::move(v));
        }
      };
      const auto clear_error = [&]() {
        if (!hot_theme_error.get().empty()) {
          hot_theme_error.set("");
        }
      };
      const auto set_status = [&](std::string v) {
        if (hot_theme_status.get() != v) {
          hot_theme_status.set(std::move(v));
        }
      };
      std::error_code ec;
      const auto ft = std::filesystem::last_write_time(path, ec);
      if (ec) {
        set_error("stat failed");
        return;
      }
      const auto ticks =
          static_cast<std::int64_t>(ft.time_since_epoch().count());
      if (!force && ticks == hot_theme_mtime.get()) {
        return;
      }

      auto text = ::duorou::ui::load_text_file(path);
      if (!text) {
        set_error("read failed");
        return;
      }

      auto parsed = ::duorou::ui::parse_theme_toml(*text);
      if (!parsed.errors.empty()) {
        set_error(format_theme_errors(parsed.errors));
        return;
      }

      std::string target = style_mgr_ptr->active_theme();
      if (target.empty()) {
        target = "Default";
      }
      StyleSheetModel old_sheet;
      if (const auto *old = style_mgr_ptr->theme(target)) {
        old_sheet = old->sheet;
      }
      parsed.theme.name = target;
      style_mgr_ptr->upsert_theme(parsed.theme);
      style_mgr_ptr->set_active_theme(target);
      hot_theme_mtime.set(ticks);
      clear_error();
      set_status(sheet_diff_summary(old_sheet, parsed.theme.sheet));
    };
    auto reload_hot_theme_auto = [reload_hot_theme]() mutable {
      reload_hot_theme(false);
    };
    auto reload_hot_theme_force = [reload_hot_theme]() mutable {
      reload_hot_theme(true);
    };

    std::vector<std::string> theme_names;
    std::string active_theme;
    std::string chain_str;
    std::string overrides_str;
    bool can_delete_theme = false;
    if (style_mgr_ptr) {
      theme_names = style_mgr_ptr->theme_names();
      active_theme = style_mgr_ptr->active_theme();
      can_delete_theme = style_mgr_ptr->theme_count() > 1;
      const auto chain = style_mgr_ptr->base_chain(active_theme);
      chain_str = chain.empty() ? std::string{} : join_str(chain, " -> ");

      const auto *t = style_mgr_ptr->theme(active_theme);
      if (t && !t->base.empty()) {
        const auto base_sheet = style_mgr_ptr->resolved_sheet_for(t->base);
        std::unordered_set<std::string> seen;
        std::vector<std::string> overrides;
        overrides.reserve(128);

        auto add_key = [&](std::string s) {
          if (seen.insert(s).second) {
            overrides.push_back(std::move(s));
          }
        };

        for (const auto &kv : t->sheet.global) {
          if (base_sheet.global.contains(kv.first)) {
            add_key("Global." + kv.first);
          }
        }

        for (const auto &ckv : t->sheet.components) {
          const auto it_base_comp = base_sheet.components.find(ckv.first);
          if (it_base_comp == base_sheet.components.end()) {
            continue;
          }
          for (const auto &kv : ckv.second.props) {
            if (it_base_comp->second.props.contains(kv.first)) {
              add_key(ckv.first + "." + kv.first);
            }
          }
          for (const auto &skv : ckv.second.states) {
            const auto it_bs = it_base_comp->second.states.find(skv.first);
            if (it_bs == it_base_comp->second.states.end()) {
              continue;
            }
            for (const auto &kv : skv.second) {
              if (it_bs->second.contains(kv.first)) {
                add_key(ckv.first + "." + skv.first + "." + kv.first);
              }
            }
          }
          for (const auto &vkv : ckv.second.variants) {
            const auto it_bv = it_base_comp->second.variants.find(vkv.first);
            if (it_bv == it_base_comp->second.variants.end()) {
              continue;
            }
            for (const auto &kv : vkv.second.props) {
              if (it_bv->second.props.contains(kv.first)) {
                add_key(ckv.first + "." + vkv.first + "." + kv.first);
              }
            }
            for (const auto &sv : vkv.second.states) {
              const auto it_bvs = it_bv->second.states.find(sv.first);
              if (it_bvs == it_bv->second.states.end()) {
                continue;
              }
              for (const auto &kv : sv.second) {
                if (it_bvs->second.contains(kv.first)) {
                  add_key(ckv.first + "." + vkv.first + "." + sv.first + "." +
                          kv.first);
                }
              }
            }
          }
        }

        std::sort(overrides.begin(), overrides.end());
        if (!overrides.empty()) {
          const std::size_t max_items = 24;
          std::vector<std::string> slice;
          const std::size_t take = std::min(max_items, overrides.size());
          slice.reserve(take + 1);
          for (std::size_t i = 0; i < take; ++i) {
            slice.push_back(overrides[i]);
          }
          overrides_str = "Overrides: " + std::to_string(overrides.size());
          if (!slice.empty()) {
            overrides_str += "\n";
            overrides_str += join_str(slice, "\n");
            if (overrides.size() > take) {
              overrides_str += "\n...";
            }
          }
        }
      }
    }

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
                                   view("Text")
                                       .prop("value", "Theme:")
                                       .prop("font_size", 12.0)
                                       .prop("color", 0xFFB0B0B0)
                                       .build(),
                                   view("Button")
                                       .prop("title", active_theme.empty() ? std::string{"(none)"} : active_theme)
                                       .event("pointer_up", on_pointer_up([theme_pop_open, theme_pop_x, theme_pop_y]() mutable {
                                                if (const auto r = target_frame()) {
                                                  theme_pop_x.set(r->x);
                                                  theme_pop_y.set(r->y + r->h);
                                                }
                                                theme_pop_open.set(true);
                                              }))
                                       .build(),
                               })
                               .build(),
                       })
                       .build();

    auto workspace =
        view("Column")
            .prop("spacing", spacing)
            .prop("cross_align", "stretch")
            .children({
                view("Column")
                    .prop("spacing", 10.0)
                    .prop("cross_align", "stretch")
                    .children({
                        view("Column")
                            .prop("spacing", 8.0)
                            .prop("cross_align", "stretch")
                            .children([&](auto &c) {
                              c.add(view("Row")
                                        .prop("spacing", 8.0)
                                        .prop("cross_align", "center")
                                        .children([&](auto &r) {
                                          r.add(view("Text")
                                                    .prop("value", "Hot Theme:")
                                                    .prop("font_size", 12.0)
                                                    .prop("color", 0xFFB0B0B0)
                                                    .build());
                                          auto path_field = TextField(
                                              hot_theme_path,
                                              "editor:hot_theme_path_input",
                                              "theme toml path");
                                          path_field.props.insert_or_assign(
                                              "width", PropValue{420.0});
                                          r.add(std::move(path_field));
                                          r.add(view("Button")
                                                    .prop("title", "Browse")
                                                    .event("pointer_up", on_pointer_up([hot_theme_path, hot_theme_mtime, hot_theme_status, hot_theme_error]() mutable {
                                                             if (auto p = open_file_dialog("Open Theme TOML")) {
                                                               hot_theme_path.set(*p);
                                                               hot_theme_mtime.set(0);
                                                               hot_theme_status.set("");
                                                               hot_theme_error.set("");
                                                             }
                                                           }))
                                                    .build());
                                          r.add(view("Button")
                                                    .prop("title", "Reload")
                                                    .event("pointer_up", on_pointer_up([reload_hot_theme_force]() mutable { reload_hot_theme_force(); }))
                                                    .build());
                                          r.add(view("Button")
                                                    .prop("title", hot_theme_enabled.get() ? "Watch: ON" : "Watch: OFF")
                                                    .event("pointer_up", on_pointer_up([hot_theme_enabled, hot_theme_mtime, reload_hot_theme_force]() mutable {
                                                             const bool next = !hot_theme_enabled.get();
                                                             hot_theme_enabled.set(next);
                                                             hot_theme_mtime.set(0);
                                                             if (next) {
                                                               reload_hot_theme_force();
                                                             }
                                                           }))
                                                    .build());
                                        })
                                        .build());
                              const auto err = hot_theme_error.get();
                              if (!err.empty()) {
                                c.add(view("Text")
                                          .prop("value", err)
                                          .prop("font_size", 12.0)
                                          .prop("color", 0xFFFF8080)
                                          .build());
                              } else {
                                const auto st = hot_theme_status.get();
                                if (!st.empty()) {
                                  c.add(view("Text")
                                            .prop("value", st)
                                            .prop("font_size", 12.0)
                                            .prop("color", 0xFFB0B0B0)
                                            .build());
                                }
                              }
                              if (hot_theme_enabled.get() && !hot_theme_path.get().empty()) {
                                c.add(WatchFile("editor:hot_theme_watch",
                                                hot_theme_path.get(), 250.0,
                                                [reload_hot_theme_auto]() mutable {
                                                  reload_hot_theme_auto();
                                                },
                                                false));
                              }
                            })
                            .build(),
                        view("Row")
                            .prop("spacing", 8.0)
                            .prop("cross_align", "center")
                            .children({
                                view("Text").prop("value", "Preview state:").prop("font_size", 12.0).build(),
                                view("Button")
                                    .prop("title", "Normal")
                                    .event("pointer_up", on_pointer_up([preview_state]() mutable { preview_state.set(""); }))
                                    .build(),
                                view("Button")
                                    .prop("title", "Hover")
                                    .event("pointer_up", on_pointer_up([preview_state]() mutable { preview_state.set("hover"); }))
                                    .build(),
                                view("Button")
                                    .prop("title", "Active")
                                    .event("pointer_up", on_pointer_up([preview_state]() mutable { preview_state.set("active"); }))
                                    .build(),
                                view("Button")
                                    .prop("title", "Disabled")
                                    .event("pointer_up", on_pointer_up([preview_state]() mutable { preview_state.set("disabled"); }))
                                    .build(),
                                view("Button")
                                    .prop("title", "Loading")
                                    .event("pointer_up", on_pointer_up([preview_state]() mutable { preview_state.set("loading"); }))
                                    .build(),
                                view("Spacer").build(),
                                view("Text")
                                    .prop("value", preview_state.get().empty() ? std::string{"(normal)"} : preview_state.get())
                                    .prop("font_size", 12.0)
                                    .build(),
                            })
                            .build(),
                        view("Row")
                            .prop("spacing", 8.0)
                            .prop("cross_align", "center")
                            .children({
                                view("Text").prop("value", "Layout:").prop("font_size", 12.0).build(),
                                view("Button")
                                    .prop("title", "Basic")
                                    .event("pointer_up", on_pointer_up([preview_layout, selected_key]() mutable {
                                             preview_layout.set(0);
                                             selected_key.set("preview:button");
                                           }))
                                    .build(),
                                view("Button")
                                    .prop("title", "Form")
                                    .event("pointer_up", on_pointer_up([preview_layout, selected_key]() mutable {
                                             preview_layout.set(1);
                                             selected_key.set("preview:form:submit");
                                           }))
                                    .build(),
                                view("Button")
                                    .prop("title", "Grid")
                                    .event("pointer_up", on_pointer_up([preview_layout, selected_key]() mutable {
                                             preview_layout.set(2);
                                             selected_key.set("preview:grid:1");
                                           }))
                                    .build(),
                                view("Spacer").build(),
                                view("Text").prop("value", "Zoom:").prop("font_size", 12.0).build(),
                                view("Stepper")
                                    .prop("value", preview_zoom.get())
                                    .prop("width", 140.0)
                                    .event("pointer_up", on_pointer_up([preview_zoom]() mutable {
                                             const auto r = target_frame();
                                             if (!r) {
                                               return;
                                             }
                                             const float local_x = pointer_x() - r->x;
                                             const bool inc = local_x > r->w * 0.5f;
                                             const double step = 0.1;
                                             double next = preview_zoom.get() + (inc ? step : -step);
                                             if (next < 0.5) {
                                               next = 0.5;
                                             }
                                             if (next > 2.0) {
                                               next = 2.0;
                                             }
                                             preview_zoom.set(next);
                                           }))
                                    .build(),
                                view("Text")
                                    .prop("value", std::to_string(static_cast<int>(preview_zoom.get() * 100.0)) + "%")
                                    .prop("font_size", 12.0)
                                    .build(),
                            })
                            .build(),
                        view("Box")
                            .children({
                                std::move(preview),
                            })
                            .build(),
                    })
                    .build(),
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
                panel("Style", std::move(style_panel_node), right_w),
            })
            .build();

    auto main =
        view("Column")
            .prop("padding", padding)
            .prop("spacing", 12.0)
            .prop("cross_align", "stretch")
            .children({std::move(top_bar), std::move(body)})
            .build();

    if (!theme_pop_open.get()) {
      return main;
    }

    const auto close_id =
        on_pointer_up([theme_pop_open]() mutable { theme_pop_open.set(false); });

    auto pop = Popover(
        {view("Column")
             .prop("spacing", 10.0)
             .prop("cross_align", "stretch")
             .children([&](auto &c) {
               c.add(view("Text").prop("value", "Themes").prop("font_size", 14.0).build());
               if (!active_theme.empty()) {
                 c.add(view("Text")
                           .prop("value", std::string{"Active: "} + active_theme)
                           .prop("font_size", 12.0)
                           .prop("color", 0xFFB0B0B0)
                           .build());
               }
               if (!chain_str.empty()) {
                 c.add(view("Text")
                           .prop("value", std::string{"Chain: "} + chain_str)
                           .prop("font_size", 12.0)
                           .prop("color", 0xFFB0B0B0)
                           .build());
               }
               if (!overrides_str.empty()) {
                 c.add(view("Text")
                           .prop("value", overrides_str)
                           .prop("font_size", 12.0)
                           .prop("color", 0xFFB0B0B0)
                           .build());
               }

               c.add(view("Divider").prop("thickness", 1.0).prop("color", 0xFF2A2A2A).build());

               for (const auto &n : theme_names) {
                 const bool is_active = (n == active_theme);
                 const std::string title = is_active ? ("* " + n) : n;
                 c.add(view("Button")
                           .prop("title", title)
                           .event("pointer_up",
                                  on_pointer_up([style_mgr_ptr, theme_pop_open, n]() mutable {
                                    if (style_mgr_ptr) {
                                      style_mgr_ptr->set_active_theme(n);
                                    }
                                    theme_pop_open.set(false);
                                  }))
                           .build());
               }

               c.add(view("Divider").prop("thickness", 1.0).prop("color", 0xFF2A2A2A).build());

               c.add(view("Text").prop("value", "Create").prop("font_size", 12.0).prop("color", 0xFFB0B0B0).build());
               c.add(TextField(theme_new_name, "editor:theme_new_name", "Theme name"));
               c.add(TextField(theme_new_base, "editor:theme_new_base", "Base (optional)"));
               c.add(view("Button")
                         .prop("title", "Create")
                         .event("pointer_up", on_pointer_up([style_mgr_ptr, theme_new_name, theme_new_base]() mutable {
                                  if (!style_mgr_ptr) {
                                    return;
                                  }
                                  auto names = style_mgr_ptr->theme_names();
                                  std::string name = theme_new_name.get();
                                  if (name.empty()) {
                                    name = unique_name_like(names, "Theme");
                                  } else if (std::find(names.begin(), names.end(), name) != names.end()) {
                                    name = unique_name_like(names, name);
                                  }

                                  ThemeModel t;
                                  t.name = name;
                                  std::string base = theme_new_base.get();
                                  if (base.empty()) {
                                    base = style_mgr_ptr->active_theme();
                                  }
                                  if (base == name) {
                                    base.clear();
                                  }
                                  if (!base.empty() && !style_mgr_ptr->theme(base)) {
                                    base.clear();
                                  }
                                  t.base = std::move(base);
                                  style_mgr_ptr->upsert_theme(std::move(t));
                                  style_mgr_ptr->set_active_theme(name);
                                  theme_new_name.set("");
                                  theme_new_base.set("");
                                }))
                         .build());

               c.add(view("Text").prop("value", "Copy Active").prop("font_size", 12.0).prop("color", 0xFFB0B0B0).build());
               c.add(TextField(theme_copy_name, "editor:theme_copy_name", "New theme name"));
               c.add(view("Button")
                         .prop("title", "Copy")
                         .event("pointer_up", on_pointer_up([style_mgr_ptr, theme_copy_name]() mutable {
                                  if (!style_mgr_ptr) {
                                    return;
                                  }
                                  const auto src_name = style_mgr_ptr->active_theme();
                                  const auto *src = style_mgr_ptr->theme(src_name);
                                  if (!src) {
                                    return;
                                  }
                                  auto names = style_mgr_ptr->theme_names();
                                  std::string name = theme_copy_name.get();
                                  if (name.empty()) {
                                    name = unique_name_like(names, src_name + " Copy");
                                  } else if (std::find(names.begin(), names.end(), name) != names.end()) {
                                    name = unique_name_like(names, name);
                                  }
                                  ThemeModel t = *src;
                                  t.name = name;
                                  if (t.base == name) {
                                    t.base.clear();
                                  }
                                  style_mgr_ptr->upsert_theme(std::move(t));
                                  style_mgr_ptr->set_active_theme(name);
                                  theme_copy_name.set("");
                                }))
                         .build());

               c.add(view("Divider").prop("thickness", 1.0).prop("color", 0xFF2A2A2A).build());
               c.add(view("Button")
                         .prop("title", "Delete Active")
                         .prop("disabled", !can_delete_theme)
                         .event("pointer_up", on_pointer_up([style_mgr_ptr]() mutable {
                                  if (style_mgr_ptr && style_mgr_ptr->theme_count() > 1) {
                                    const auto n = style_mgr_ptr->active_theme();
                                    style_mgr_ptr->remove_theme(n);
                                    style_mgr_ptr->set_active_theme("");
                                  }
                                }))
                         .build());
             })
             .build()},
        static_cast<float>(theme_pop_x.get()),
        static_cast<float>(theme_pop_y.get()), 0, close_id);

    return view("Overlay")
        .children([&](auto &c) {
          c.add(std::move(main));
          c.add(std::move(pop));
        })
        .build();
  });
}

} // namespace duorou::ui::editor
