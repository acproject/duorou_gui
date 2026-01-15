#pragma once

#include <duorou/ui/base_node.hpp>
#include <duorou/ui/base_layout.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace duorou::ui {

namespace detail {

struct StyleRule {
  std::string type;
  std::string cls;
  std::string key;
  std::unordered_map<std::string, PropValue> decls;
  std::int32_t specificity{};
  std::int32_t order{};
};

inline std::string_view trim_ws(std::string_view s) {
  std::size_t i = 0;
  while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) {
    ++i;
  }
  std::size_t j = s.size();
  while (j > i && (s[j - 1] == ' ' || s[j - 1] == '\t' || s[j - 1] == '\r' || s[j - 1] == '\n')) {
    --j;
  }
  return s.substr(i, j - i);
}

inline std::vector<std::string> split_ws(std::string_view s) {
  std::vector<std::string> out;
  std::size_t i = 0;
  while (i < s.size()) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) {
      ++i;
    }
    const std::size_t start = i;
    while (i < s.size() && !(s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) {
      ++i;
    }
    if (start < i) {
      out.emplace_back(s.substr(start, i - start));
    }
  }
  return out;
}

inline std::string unescape_toml_string(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (std::size_t i = 0; i < s.size(); ++i) {
    const char c = s[i];
    if (c != '\\' || i + 1 >= s.size()) {
      out.push_back(c);
      continue;
    }
    const char n = s[++i];
    if (n == 'n') {
      out.push_back('\n');
    } else if (n == 't') {
      out.push_back('\t');
    } else if (n == 'r') {
      out.push_back('\r');
    } else if (n == '\\') {
      out.push_back('\\');
    } else if (n == '"') {
      out.push_back('"');
    } else {
      out.push_back(n);
    }
  }
  return out;
}

inline std::optional<std::string> parse_toml_table_name(std::string_view line) {
  line = trim_ws(line);
  if (line.size() < 3 || line.front() != '[' || line.back() != ']') {
    return std::nullopt;
  }
  auto inner = trim_ws(line.substr(1, line.size() - 2));
  if (inner.size() >= 2 &&
      ((inner.front() == '"' && inner.back() == '"') ||
       (inner.front() == '\'' && inner.back() == '\''))) {
    inner = inner.substr(1, inner.size() - 2);
    return unescape_toml_string(inner);
  }
  return std::string{inner};
}

inline std::pair<std::string, std::string> parse_toml_kv(std::string_view line) {
  const auto pos = line.find('=');
  if (pos == std::string_view::npos) {
    return {};
  }
  auto k = trim_ws(line.substr(0, pos));
  auto v = trim_ws(line.substr(pos + 1));
  if (k.size() >= 2 &&
      ((k.front() == '"' && k.back() == '"') || (k.front() == '\'' && k.back() == '\''))) {
    k = k.substr(1, k.size() - 2);
  }
  return {std::string{k}, std::string{v}};
}

inline PropValue parse_toml_value(std::string_view v) {
  v = trim_ws(v);
  if (v.size() >= 2 &&
      ((v.front() == '"' && v.back() == '"') || (v.front() == '\'' && v.back() == '\''))) {
    return PropValue{unescape_toml_string(v.substr(1, v.size() - 2))};
  }
  if (v == "true") {
    return PropValue{true};
  }
  if (v == "false") {
    return PropValue{false};
  }
  if (v.size() > 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) {
    try {
      const auto u = static_cast<std::uint64_t>(std::stoull(std::string{v}, nullptr, 16));
      return PropValue{static_cast<std::int64_t>(u)};
    } catch (...) {
      return PropValue{std::string{v}};
    }
  }
  if (v.find('.') != std::string_view::npos) {
    try {
      return PropValue{std::stod(std::string{v})};
    } catch (...) {
      return PropValue{std::string{v}};
    }
  }
  try {
    return PropValue{static_cast<std::int64_t>(std::stoll(std::string{v}))};
  } catch (...) {
    return PropValue{std::string{v}};
  }
}

inline StyleRule parse_style_selector(std::string_view selector) {
  selector = trim_ws(selector);
  StyleRule rule{};
  rule.specificity = 0;
  if (selector.empty()) {
    return rule;
  }

  if (selector.front() == '#') {
    rule.key = std::string{selector.substr(1)};
    rule.specificity = 100;
    return rule;
  }
  if (selector.front() == '.') {
    rule.cls = std::string{selector.substr(1)};
    rule.specificity = 10;
    return rule;
  }

  const auto dot = selector.find('.');
  if (dot == std::string_view::npos) {
    rule.type = std::string{selector};
    rule.specificity = 1;
    return rule;
  }

  rule.type = std::string{selector.substr(0, dot)};
  rule.cls = std::string{selector.substr(dot + 1)};
  rule.specificity = 11;
  return rule;
}

inline std::vector<StyleRule> parse_stylesheet_toml(std::string_view toml) {
  std::vector<StyleRule> rules;
  std::optional<std::size_t> active;
  std::int32_t order = 0;

  std::size_t pos = 0;
  while (pos <= toml.size()) {
    const auto next = toml.find('\n', pos);
    auto line = (next == std::string_view::npos) ? toml.substr(pos) : toml.substr(pos, next - pos);
    pos = (next == std::string_view::npos) ? toml.size() + 1 : next + 1;

    const auto hash = line.find('#');
    if (hash != std::string_view::npos) {
      line = line.substr(0, hash);
    }
    line = trim_ws(line);
    if (line.empty()) {
      continue;
    }

    if (auto t = parse_toml_table_name(line)) {
      StyleRule r = parse_style_selector(*t);
      r.order = order++;
      rules.push_back(std::move(r));
      active = rules.size() - 1;
      continue;
    }

    if (!active) {
      continue;
    }

    const auto kv = parse_toml_kv(line);
    if (kv.first.empty()) {
      continue;
    }
    rules[*active].decls.insert_or_assign(kv.first, parse_toml_value(kv.second));
  }

  return rules;
}

inline void apply_styles_to_tree(ViewNode &root, const std::vector<StyleRule> &rules) {
  auto apply_node = [&](auto &&self, ViewNode &node) -> void {
    struct StyledValue {
      PropValue value;
      std::int32_t specificity{};
      std::int32_t order{};
    };
    std::unordered_map<std::string, StyledValue> styled;

    const auto classes = split_ws(prop_as_string(node.props, "class", ""));
    const auto has_class = [&](const std::string &c) -> bool {
      for (const auto &it : classes) {
        if (it == c) {
          return true;
        }
      }
      return false;
    };

    for (const auto &r : rules) {
      if (!r.key.empty() && node.key != r.key) {
        continue;
      }
      if (!r.type.empty() && node.type != r.type) {
        continue;
      }
      if (!r.cls.empty() && !has_class(r.cls)) {
        continue;
      }
      for (const auto &kv : r.decls) {
        const auto it = styled.find(kv.first);
        if (it == styled.end() ||
            r.specificity > it->second.specificity ||
            (r.specificity == it->second.specificity && r.order >= it->second.order)) {
          styled.insert_or_assign(kv.first,
                                  StyledValue{kv.second, r.specificity, r.order});
        }
      }
    }

    for (const auto &kv : styled) {
      if (!node.props.contains(kv.first)) {
        node.props.insert_or_assign(kv.first, kv.second.value);
      }
    }

    for (auto &ch : node.children) {
      self(self, ch);
    }
  };

  apply_node(apply_node, root);
}

} // namespace detail

enum class StylePropType : std::uint8_t {
  Unknown = 0,
  Color = 1,
  Number = 2,
  Bool = 3,
  String = 4,
};

inline StylePropType style_prop_type(std::string_view key) {
  if (key == "bg" || key == "border" || key == "color" || key == "tint" ||
      key == "track" || key == "fill" || key == "scrollbar_track" ||
      key == "scrollbar_thumb") {
    return StylePropType::Color;
  }
  if (key == "width" || key == "height" || key == "min_width" ||
      key == "min_length" || key == "padding" || key == "spacing" ||
      key == "border_width" || key == "font_size" || key == "opacity" ||
      key == "render_offset_x" || key == "render_offset_y" || key == "gap" ||
      key == "default_width" || key == "default_height" || key == "thumb_size" ||
      key == "track_height" || key == "value" || key == "thickness") {
    return StylePropType::Number;
  }
  if (key == "clip" || key == "secure" || key == "disabled" || key == "hover" ||
      key == "active" || key == "focused") {
    return StylePropType::Bool;
  }
  return StylePropType::String;
}

struct StyleVariantModel {
  Props props;
  std::unordered_map<std::string, Props> states;
};

struct StyleComponentModel {
  Props props;
  std::unordered_map<std::string, StyleVariantModel> variants;
  std::unordered_map<std::string, Props> states;
};

struct StyleSheetModel {
  Props global;
  std::unordered_map<std::string, StyleComponentModel> components;
};

struct ThemeModel {
  std::string name;
  std::string base;
  StyleSheetModel sheet;
};

struct ThemeRegistry {
  std::unordered_map<std::string, ThemeModel> themes;
};

inline void merge_props(Props &dst, const Props &src) {
  for (const auto &kv : src) {
    dst.insert_or_assign(kv.first, kv.second);
  }
}

inline StyleSheetModel resolve_theme_sheet(const ThemeRegistry &reg, std::string_view name) {
  StyleSheetModel out;
  std::unordered_set<std::string> visiting;

  auto apply_theme = [&](auto &&self, std::string_view theme_name) -> void {
    if (theme_name.empty()) {
      return;
    }
    const std::string key{theme_name};
    if (visiting.contains(key)) {
      return;
    }
    const auto it = reg.themes.find(key);
    if (it == reg.themes.end()) {
      return;
    }

    visiting.insert(key);
    if (!it->second.base.empty()) {
      self(self, it->second.base);
    }

    merge_props(out.global, it->second.sheet.global);
    for (const auto &ckv : it->second.sheet.components) {
      auto &dst_comp = out.components[ckv.first];
      merge_props(dst_comp.props, ckv.second.props);

      for (const auto &skv : ckv.second.states) {
        merge_props(dst_comp.states[skv.first], skv.second);
      }

      for (const auto &vkv : ckv.second.variants) {
        auto &dst_var = dst_comp.variants[vkv.first];
        merge_props(dst_var.props, vkv.second.props);
        for (const auto &sv : vkv.second.states) {
          merge_props(dst_var.states[sv.first], sv.second);
        }
      }
    }
    visiting.erase(key);
  };

  apply_theme(apply_theme, name);
  return out;
}

inline Props resolve_style_props(const StyleSheetModel &sheet, std::string_view component,
                                std::string_view variant, std::string_view state) {
  Props out;
  merge_props(out, sheet.global);

  const auto comp_it = sheet.components.find(std::string{component});
  if (comp_it == sheet.components.end()) {
    return out;
  }

  merge_props(out, comp_it->second.props);

  if (!variant.empty()) {
    const auto var_it = comp_it->second.variants.find(std::string{variant});
    if (var_it != comp_it->second.variants.end()) {
      merge_props(out, var_it->second.props);
    }
  }

  if (!state.empty()) {
    const auto st_it = comp_it->second.states.find(std::string{state});
    if (st_it != comp_it->second.states.end()) {
      merge_props(out, st_it->second);
    }
    if (!variant.empty()) {
      const auto var_it = comp_it->second.variants.find(std::string{variant});
      if (var_it != comp_it->second.variants.end()) {
        const auto vst_it = var_it->second.states.find(std::string{state});
        if (vst_it != var_it->second.states.end()) {
          merge_props(out, vst_it->second);
        }
      }
    }
  }

  return out;
}

struct StyleParseError {
  std::int32_t line{};
  std::int32_t column{};
  std::string message;
};

struct ParseThemeResult {
  ThemeModel theme;
  std::vector<StyleParseError> errors;
};

inline bool style_is_known_state(std::string_view s) {
  return s == "hover" || s == "active" || s == "disabled" || s == "focused";
}

inline std::vector<std::string> split_dot(std::string_view s) {
  std::vector<std::string> out;
  std::size_t pos = 0;
  while (pos <= s.size()) {
    const auto next = s.find('.', pos);
    const auto part = (next == std::string_view::npos) ? s.substr(pos) : s.substr(pos, next - pos);
    out.emplace_back(part);
    if (next == std::string_view::npos) {
      break;
    }
    pos = next + 1;
  }
  return out;
}

inline ParseThemeResult parse_theme_toml(std::string_view toml) {
  ParseThemeResult out;
  std::vector<std::string> table;

  const auto add_error = [&](std::int32_t line, std::int32_t col, std::string msg) {
    out.errors.push_back(StyleParseError{line, col, std::move(msg)});
  };

  std::size_t pos = 0;
  std::int32_t line_no = 0;
  while (pos <= toml.size()) {
    ++line_no;
    const auto next = toml.find('\n', pos);
    auto line = (next == std::string_view::npos) ? toml.substr(pos) : toml.substr(pos, next - pos);
    pos = (next == std::string_view::npos) ? toml.size() + 1 : next + 1;

    const auto hash = line.find('#');
    if (hash != std::string_view::npos) {
      line = line.substr(0, hash);
    }
    line = detail::trim_ws(line);
    if (line.empty()) {
      continue;
    }

    if (auto t = detail::parse_toml_table_name(line)) {
      table = split_dot(*t);
      continue;
    }

    if (table.empty()) {
      add_error(line_no, 1, "key/value outside any table");
      continue;
    }

    const auto kv = detail::parse_toml_kv(line);
    if (kv.first.empty()) {
      add_error(line_no, 1, "invalid key/value");
      continue;
    }

    const auto value = detail::parse_toml_value(kv.second);

    const auto table0 = table[0];
    if (table0 == "Theme") {
      if (kv.first == "name") {
        if (const auto *s = std::get_if<std::string>(&value)) {
          out.theme.name = *s;
        } else {
          add_error(line_no, 1, "Theme.name must be string");
        }
      } else if (kv.first == "base") {
        if (const auto *s = std::get_if<std::string>(&value)) {
          out.theme.base = *s;
        } else {
          add_error(line_no, 1, "Theme.base must be string");
        }
      }
      continue;
    }

    if (table0 == "Global") {
      out.theme.sheet.global.insert_or_assign(kv.first, value);
      continue;
    }

    const std::string component = table0;
    auto &comp = out.theme.sheet.components[component];
    if (table.size() == 1) {
      comp.props.insert_or_assign(kv.first, value);
      continue;
    }

    if (table.size() == 2) {
      const auto &seg = table[1];
      if (style_is_known_state(seg)) {
        comp.states[seg].insert_or_assign(kv.first, value);
      } else {
        comp.variants[seg].props.insert_or_assign(kv.first, value);
      }
      continue;
    }

    if (table.size() == 3) {
      const auto &variant = table[1];
      const auto &state = table[2];
      if (!style_is_known_state(state)) {
        add_error(line_no, 1, "unknown state name: " + state);
      }
      comp.variants[variant].states[state].insert_or_assign(kv.first, value);
      continue;
    }

    add_error(line_no, 1, "table nesting too deep");
  }

  if (out.theme.name.empty()) {
    out.theme.name = "Default";
  }

  return out;
}

namespace detail {

struct JsonValue {
  using Object = std::unordered_map<std::string, JsonValue>;
  using Array = std::vector<JsonValue>;
  std::variant<std::nullptr_t, bool, double, std::string, Object, Array> v;
};

struct JsonParser {
  std::string_view s;
  std::size_t i{};
  std::int32_t line{1};
  std::int32_t col{1};
  std::vector<StyleParseError> *errors{};

  void add_error(std::string msg) {
    if (errors) {
      errors->push_back(StyleParseError{line, col, std::move(msg)});
    }
  }

  bool eof() const { return i >= s.size(); }

  char peek() const { return eof() ? '\0' : s[i]; }

  char get() {
    if (eof()) {
      return '\0';
    }
    const char c = s[i++];
    if (c == '\n') {
      ++line;
      col = 1;
    } else {
      ++col;
    }
    return c;
  }

  void skip_ws() {
    for (;;) {
      const char c = peek();
      if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
        get();
        continue;
      }
      break;
    }
  }

  bool consume(char c) {
    skip_ws();
    if (peek() != c) {
      return false;
    }
    get();
    return true;
  }

  std::optional<std::string> parse_string() {
    skip_ws();
    if (peek() != '"') {
      add_error("expected string");
      return std::nullopt;
    }
    get();
    std::string out;
    while (!eof()) {
      const char c = get();
      if (c == '"') {
        return out;
      }
      if (c == '\\') {
        if (eof()) {
          break;
        }
        const char e = get();
        if (e == '"' || e == '\\' || e == '/') {
          out.push_back(e);
        } else if (e == 'b') {
          out.push_back('\b');
        } else if (e == 'f') {
          out.push_back('\f');
        } else if (e == 'n') {
          out.push_back('\n');
        } else if (e == 'r') {
          out.push_back('\r');
        } else if (e == 't') {
          out.push_back('\t');
        } else {
          add_error("unsupported escape");
          out.push_back(e);
        }
        continue;
      }
      out.push_back(c);
    }
    add_error("unterminated string");
    return std::nullopt;
  }

  std::optional<double> parse_number() {
    skip_ws();
    const std::size_t start = i;
    if (peek() == '-') {
      get();
    }
    bool any = false;
    while (peek() >= '0' && peek() <= '9') {
      any = true;
      get();
    }
    if (peek() == '.') {
      get();
      while (peek() >= '0' && peek() <= '9') {
        any = true;
        get();
      }
    }
    if (peek() == 'e' || peek() == 'E') {
      get();
      if (peek() == '+' || peek() == '-') {
        get();
      }
      while (peek() >= '0' && peek() <= '9') {
        any = true;
        get();
      }
    }
    if (!any) {
      add_error("expected number");
      return std::nullopt;
    }
    try {
      return std::stod(std::string{s.substr(start, i - start)});
    } catch (...) {
      add_error("invalid number");
      return std::nullopt;
    }
  }

  std::optional<JsonValue> parse_value() {
    skip_ws();
    const char c = peek();
    if (c == '{') {
      return parse_object();
    }
    if (c == '[') {
      return parse_array();
    }
    if (c == '"') {
      auto str = parse_string();
      if (!str) {
        return std::nullopt;
      }
      return JsonValue{std::move(*str)};
    }
    if (c == '-' || (c >= '0' && c <= '9')) {
      auto num = parse_number();
      if (!num) {
        return std::nullopt;
      }
      return JsonValue{*num};
    }
    if (s.substr(i, 4) == "true") {
      i += 4;
      col += 4;
      return JsonValue{true};
    }
    if (s.substr(i, 5) == "false") {
      i += 5;
      col += 5;
      return JsonValue{false};
    }
    if (s.substr(i, 4) == "null") {
      i += 4;
      col += 4;
      return JsonValue{nullptr};
    }
    add_error("unexpected token");
    return std::nullopt;
  }

  std::optional<JsonValue> parse_object() {
    if (!consume('{')) {
      add_error("expected '{'");
      return std::nullopt;
    }
    JsonValue::Object obj;
    skip_ws();
    if (consume('}')) {
      return JsonValue{std::move(obj)};
    }
    for (;;) {
      auto k = parse_string();
      if (!k) {
        return std::nullopt;
      }
      if (!consume(':')) {
        add_error("expected ':'");
        return std::nullopt;
      }
      auto v = parse_value();
      if (!v) {
        return std::nullopt;
      }
      obj.insert_or_assign(*k, std::move(*v));
      skip_ws();
      if (consume('}')) {
        return JsonValue{std::move(obj)};
      }
      if (!consume(',')) {
        add_error("expected ',' or '}'");
        return std::nullopt;
      }
    }
  }

  std::optional<JsonValue> parse_array() {
    if (!consume('[')) {
      add_error("expected '['");
      return std::nullopt;
    }
    JsonValue::Array arr;
    skip_ws();
    if (consume(']')) {
      return JsonValue{std::move(arr)};
    }
    for (;;) {
      auto v = parse_value();
      if (!v) {
        return std::nullopt;
      }
      arr.push_back(std::move(*v));
      skip_ws();
      if (consume(']')) {
        return JsonValue{std::move(arr)};
      }
      if (!consume(',')) {
        add_error("expected ',' or ']'");
        return std::nullopt;
      }
    }
  }
};

inline std::optional<std::int64_t> parse_hex_i64(std::string_view s) {
  if (s.size() <= 2) {
    return std::nullopt;
  }
  if (!(s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))) {
    return std::nullopt;
  }
  try {
    const auto u = static_cast<std::uint64_t>(std::stoull(std::string{s}, nullptr, 16));
    return static_cast<std::int64_t>(u);
  } catch (...) {
    return std::nullopt;
  }
}

inline std::optional<PropValue> json_to_prop_value(std::string_view key, const JsonValue &v) {
  if (const auto *b = std::get_if<bool>(&v.v)) {
    return PropValue{*b};
  }
  if (const auto *d = std::get_if<double>(&v.v)) {
    const double iv = std::round(*d);
    if (std::abs(*d - iv) < 1e-9 && iv >= static_cast<double>(std::numeric_limits<std::int64_t>::min()) &&
        iv <= static_cast<double>(std::numeric_limits<std::int64_t>::max())) {
      return PropValue{static_cast<std::int64_t>(iv)};
    }
    return PropValue{*d};
  }
  if (const auto *s = std::get_if<std::string>(&v.v)) {
    if (auto hex = parse_hex_i64(*s)) {
      return PropValue{*hex};
    }
    if (style_prop_type(key) == StylePropType::Color) {
      if (auto hex2 = parse_hex_i64(*s)) {
        return PropValue{*hex2};
      }
    }
    return PropValue{*s};
  }
  if (std::holds_alternative<std::nullptr_t>(v.v)) {
    return std::nullopt;
  }
  return std::nullopt;
}

inline const JsonValue::Object *json_as_object(const JsonValue &v) {
  return std::get_if<JsonValue::Object>(&v.v);
}

inline const JsonValue *json_get(const JsonValue::Object &o, const char *k0, const char *k1 = nullptr) {
  if (const auto it = o.find(k0); it != o.end()) {
    return &it->second;
  }
  if (k1) {
    if (const auto it = o.find(k1); it != o.end()) {
      return &it->second;
    }
  }
  return nullptr;
}

inline void json_fill_props(Props &dst, const JsonValue::Object &o) {
  for (const auto &kv : o) {
    if (auto pv = json_to_prop_value(kv.first, kv.second)) {
      dst.insert_or_assign(kv.first, *pv);
    }
  }
}

} // namespace detail

inline ParseThemeResult parse_theme_json(std::string_view json) {
  ParseThemeResult out;
  detail::JsonParser p;
  p.s = json;
  p.errors = &out.errors;
  auto root = p.parse_value();
  if (!root) {
    if (out.theme.name.empty()) {
      out.theme.name = "Default";
    }
    return out;
  }
  auto *root_obj = detail::json_as_object(*root);
  if (!root_obj) {
    out.errors.push_back(StyleParseError{1, 1, "root must be object"});
    if (out.theme.name.empty()) {
      out.theme.name = "Default";
    }
    return out;
  }

  if (const auto *theme_v = detail::json_get(*root_obj, "theme", "Theme")) {
    if (const auto *theme_o = detail::json_as_object(*theme_v)) {
      if (const auto *name_v = detail::json_get(*theme_o, "name", "Name")) {
        if (const auto *s = std::get_if<std::string>(&name_v->v)) {
          out.theme.name = *s;
        }
      }
      if (const auto *base_v = detail::json_get(*theme_o, "base", "Base")) {
        if (const auto *s = std::get_if<std::string>(&base_v->v)) {
          out.theme.base = *s;
        }
      }
    }
  }

  if (const auto *global_v = detail::json_get(*root_obj, "global", "Global")) {
    if (const auto *global_o = detail::json_as_object(*global_v)) {
      detail::json_fill_props(out.theme.sheet.global, *global_o);
    }
  }

  if (const auto *comps_v = detail::json_get(*root_obj, "components", "Components")) {
    if (const auto *comps_o = detail::json_as_object(*comps_v)) {
      for (const auto &ckv : *comps_o) {
        const auto *comp_o = detail::json_as_object(ckv.second);
        if (!comp_o) {
          continue;
        }
        auto &comp = out.theme.sheet.components[ckv.first];
        if (const auto *props_v = detail::json_get(*comp_o, "props", "Props")) {
          if (const auto *props_o = detail::json_as_object(*props_v)) {
            detail::json_fill_props(comp.props, *props_o);
          }
        }
        if (const auto *states_v = detail::json_get(*comp_o, "states", "States")) {
          if (const auto *states_o = detail::json_as_object(*states_v)) {
            for (const auto &skv : *states_o) {
              if (const auto *so = detail::json_as_object(skv.second)) {
                detail::json_fill_props(comp.states[skv.first], *so);
              }
            }
          }
        }
        if (const auto *vars_v = detail::json_get(*comp_o, "variants", "Variants")) {
          if (const auto *vars_o = detail::json_as_object(*vars_v)) {
            for (const auto &vkv : *vars_o) {
              if (const auto *vo = detail::json_as_object(vkv.second)) {
                auto &var = comp.variants[vkv.first];
                if (const auto *vprops_v = detail::json_get(*vo, "props", "Props")) {
                  if (const auto *vprops_o = detail::json_as_object(*vprops_v)) {
                    detail::json_fill_props(var.props, *vprops_o);
                  }
                }
                if (const auto *vstates_v = detail::json_get(*vo, "states", "States")) {
                  if (const auto *vstates_o = detail::json_as_object(*vstates_v)) {
                    for (const auto &sv : *vstates_o) {
                      if (const auto *svo = detail::json_as_object(sv.second)) {
                        detail::json_fill_props(var.states[sv.first], *svo);
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  if (out.theme.name.empty()) {
    out.theme.name = "Default";
  }
  return out;
}

} // namespace duorou::ui
