#pragma once

#include <duorou/ui/base_render.hpp>

#include <algorithm>
#include <optional>
#include <string_view>
#include <string>
#include <vector>

namespace duorou::ui {

inline ViewNode TextField(std::string value, std::string placeholder = {}) {
  auto b = view("TextField");
  b.prop("value", std::move(value));
  if (!placeholder.empty()) {
    b.prop("placeholder", std::move(placeholder));
  }
  return std::move(b).build();
}

inline ViewNode TextField(BindingId binding, std::string placeholder = {}) {
  auto b = view("TextField");
  b.prop("binding", binding);
  if (!placeholder.empty()) {
    b.prop("placeholder", std::move(placeholder));
  }
  return std::move(b).build();
}

inline ViewNode SecureField(std::string value, std::string placeholder = {}) {
  auto b = view("TextField");
  b.prop("secure", true);
  b.prop("value", std::move(value));
  if (!placeholder.empty()) {
    b.prop("placeholder", std::move(placeholder));
  }
  return std::move(b).build();
}

inline ViewNode SecureField(BindingId binding, std::string placeholder = {}) {
  auto b = view("TextField");
  b.prop("secure", true);
  b.prop("binding", binding);
  if (!placeholder.empty()) {
    b.prop("placeholder", std::move(placeholder));
  }
  return std::move(b).build();
}

inline ViewNode TextEditor(std::string value) {
  auto b = view("TextEditor");
  b.prop("value", std::move(value));
  return std::move(b).build();
}

inline ViewNode TextEditor(BindingId binding) {
  auto b = view("TextEditor");
  b.prop("binding", binding);
  return std::move(b).build();
}

inline bool measure_leaf_textfield(const ViewNode &node, ConstraintsF constraints,
                                   SizeF &out) {
  if (node.type != "TextField") {
    return false;
  }
  const auto font_size = prop_as_float(node.props, "font_size", 16.0f);
  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto char_w = font_size * 0.5f;
  const auto line_h = font_size * 1.2f;
  const auto value = prop_as_string(node.props, "value", "");
  const auto placeholder = prop_as_string(node.props, "placeholder", "");
  const auto text_len = std::max<std::size_t>(value.size(), placeholder.size());
  const auto min_w = prop_as_float(node.props, "min_width", 140.0f);
  const auto w =
      std::max(min_w,
               static_cast<float>(text_len) * char_w + padding * 2.0f + 24.0f);
  const auto h = std::max(28.0f, line_h + 12.0f) + padding * 2.0f;
  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  return true;
}

inline bool measure_leaf_texteditor(const ViewNode &node, ConstraintsF constraints,
                                    SizeF &out) {
  if (node.type != "TextEditor") {
    return false;
  }
  const auto padding = prop_as_float(node.props, "padding", 0.0f);
  const auto default_w = prop_as_float(node.props, "default_width", 240.0f);
  const auto default_h = prop_as_float(node.props, "default_height", 120.0f);
  const auto w = default_w + padding * 2.0f;
  const auto h = default_h + padding * 2.0f;
  out = apply_explicit_size(
      node, constraints,
      SizeF{clampf(w, 0.0f, constraints.max_w), clampf(h, 0.0f, constraints.max_h)});
  return true;
}

inline std::string mask_text(std::string_view s) {
  std::string out;
  out.assign(s.size(), '*');
  return out;
}

inline std::optional<std::int64_t> prop_as_i64_opt(const Props &props,
                                                   const std::string &key) {
  const auto *pv = find_prop(props, key);
  if (!pv) {
    return std::nullopt;
  }
  if (const auto *i = std::get_if<std::int64_t>(pv)) {
    return *i;
  }
  if (const auto *d = std::get_if<double>(pv)) {
    return static_cast<std::int64_t>(*d);
  }
  return std::nullopt;
}

inline bool emit_render_ops_textfield(const ViewNode &v, const LayoutNode &l,
                                     std::vector<RenderOp> &out) {
  if (v.type != "TextField") {
    return false;
  }
  const auto padding = prop_as_float(v.props, "padding", 10.0f);
  const auto font_px = prop_as_float(v.props, "font_size", 16.0f);
  const auto focused = prop_as_bool(v.props, "focused", false);
  const auto value = prop_as_string(v.props, "value", "");
  const auto placeholder = prop_as_string(v.props, "placeholder", "");
  const auto secure = prop_as_bool(v.props, "secure", false);
  const auto caret = prop_as_i64_opt(v.props, "caret");
  const auto sel_start = prop_as_i64_opt(v.props, "sel_start");
  const auto sel_end = prop_as_i64_opt(v.props, "sel_end");
  out.push_back(DrawRect{l.frame, focused ? ColorU8{55, 55, 55, 255}
                                         : ColorU8{45, 45, 45, 255}});
  const RectF tr{l.frame.x + padding, l.frame.y,
                 std::max(0.0f, l.frame.w - padding * 2.0f), l.frame.h};
  if (value.empty()) {
    out.push_back(
        DrawText{tr, placeholder, ColorU8{160, 160, 160, 255}, font_px, 0.0f, 0.5f});
  } else {
    DrawText t{tr, secure ? mask_text(value) : value, ColorU8{235, 235, 235, 255},
               font_px, 0.0f, 0.5f};
    if (focused) {
      if (caret) {
        t.caret_pos = std::max<std::int64_t>(0, *caret);
      } else {
        t.caret_end = true;
      }
    }
    if (sel_start && sel_end) {
      t.sel_start = *sel_start;
      t.sel_end = *sel_end;
    }
    out.push_back(std::move(t));
  }
  if (focused && value.empty()) {
    DrawText t{tr, std::string{}, ColorU8{235, 235, 235, 255}, font_px, 0.0f, 0.5f};
    if (caret) {
      t.caret_pos = std::max<std::int64_t>(0, *caret);
    } else {
      t.caret_end = true;
    }
    if (sel_start && sel_end) {
      t.sel_start = *sel_start;
      t.sel_end = *sel_end;
    }
    out.push_back(std::move(t));
  }
  return true;
}

inline bool emit_render_ops_texteditor(const ViewNode &v, const LayoutNode &l,
                                       std::vector<RenderOp> &out) {
  if (v.type != "TextEditor") {
    return false;
  }
  const auto padding = prop_as_float(v.props, "padding", 10.0f);
  const auto font_px = prop_as_float(v.props, "font_size", 16.0f);
  const auto focused = prop_as_bool(v.props, "focused", false);
  const auto value = prop_as_string(v.props, "value", "");
  const auto secure = prop_as_bool(v.props, "secure", false);
  const auto caret = prop_as_i64_opt(v.props, "caret");
  const auto sel_start = prop_as_i64_opt(v.props, "sel_start");
  const auto sel_end = prop_as_i64_opt(v.props, "sel_end");

  out.push_back(DrawRect{l.frame, focused ? ColorU8{55, 55, 55, 255}
                                         : ColorU8{45, 45, 45, 255}});
  const RectF tr{l.frame.x + padding, l.frame.y + padding,
                 std::max(0.0f, l.frame.w - padding * 2.0f),
                 std::max(0.0f, l.frame.h - padding * 2.0f)};

  const std::string text = secure ? mask_text(value) : value;
  auto utf8_len = [](std::string_view s) -> std::int64_t {
    std::int64_t n = 0;
    for (std::size_t i = 0; i < s.size();) {
      const auto b = static_cast<std::uint8_t>(s[i]);
      std::size_t adv = 1;
      if ((b & 0x80) == 0x00) {
        adv = 1;
      } else if ((b & 0xE0) == 0xC0) {
        adv = 2;
      } else if ((b & 0xF0) == 0xE0) {
        adv = 3;
      } else if ((b & 0xF8) == 0xF0) {
        adv = 4;
      }
      if (i + adv > s.size()) {
        adv = 1;
      }
      i += adv;
      ++n;
    }
    return n;
  };

  struct LineInfo {
    std::string_view text;
    std::int64_t start_char{};
    std::int64_t len_char{};
  };

  std::vector<LineInfo> lines;
  lines.reserve(8);
  {
    std::size_t start_b = 0;
    std::int64_t start_c = 0;
    for (std::size_t i = 0; i <= text.size(); ++i) {
      if (i == text.size() || text[i] == '\n') {
        auto sv = std::string_view{text}.substr(start_b, i - start_b);
        const auto len_c = utf8_len(sv);
        lines.push_back(LineInfo{sv, start_c, len_c});
        start_b = i + 1;
        start_c += len_c + 1;
      }
    }
    if (lines.empty()) {
      lines.push_back(LineInfo{std::string_view{}, 0, 0});
    }
  }

  const float line_h = font_px * 1.2f;
  const std::size_t max_lines =
      line_h > 0.0f ? static_cast<std::size_t>(tr.h / line_h) + 1u : lines.size();
  const std::size_t n = std::min(lines.size(), max_lines);

  std::int64_t caret_char = -1;
  if (focused && caret) {
    caret_char = std::max<std::int64_t>(0, *caret);
  }
  std::size_t caret_line = static_cast<std::size_t>(-1);
  std::int64_t caret_in_line = -1;
  if (caret_char >= 0) {
    for (std::size_t i = 0; i < n; ++i) {
      const auto start = lines[i].start_char;
      const auto end = start + lines[i].len_char;
      if (caret_char <= end) {
        caret_line = i;
        caret_in_line = caret_char - start;
        break;
      }
    }
    if (caret_line == static_cast<std::size_t>(-1) && n > 0) {
      caret_line = n - 1;
      caret_in_line = lines[caret_line].len_char;
    }
  }

  for (std::size_t i = 0; i < n; ++i) {
    const RectF lr{tr.x, tr.y + static_cast<float>(i) * line_h, tr.w, line_h};
    DrawText t{lr, std::string{lines[i].text}, ColorU8{235, 235, 235, 255}, font_px,
               0.0f, 0.0f};
    if (caret_char >= 0 && i == caret_line) {
      t.caret_pos = std::max<std::int64_t>(0, caret_in_line);
    } else {
      t.caret_end = focused && !caret && (i + 1 == n);
    }
    if (sel_start && sel_end) {
      const auto a = std::min(*sel_start, *sel_end);
      const auto b = std::max(*sel_start, *sel_end);
      const auto line_a = lines[i].start_char;
      const auto line_b = lines[i].start_char + lines[i].len_char;
      const auto lo = std::max<std::int64_t>(a, line_a);
      const auto hi = std::min<std::int64_t>(b, line_b);
      if (hi > lo) {
        t.sel_start = lo - line_a;
        t.sel_end = hi - line_a;
      }
    }
    out.push_back(std::move(t));
  }
  return true;
}

} // namespace duorou::ui
