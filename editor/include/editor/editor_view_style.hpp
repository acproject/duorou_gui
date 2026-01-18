#pragma once

#include <duorou/ui/runtime.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace duorou::ui::editor {

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

} // namespace duorou::ui::editor

