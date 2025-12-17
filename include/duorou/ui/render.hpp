#pragma once

#include <duorou/ui/layout.hpp>
#include <duorou/ui/node.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace duorou::ui {

struct ColorU8 {
  std::uint8_t r{};
  std::uint8_t g{};
  std::uint8_t b{};
  std::uint8_t a{255};
};

struct DrawRect {
  RectF rect;
  ColorU8 fill;
};

struct DrawText {
  RectF rect;
  std::string text;
  ColorU8 color;
  float font_px{16.0f};
};

using RenderOp = std::variant<DrawRect, DrawText>;

struct Renderer {
  virtual ~Renderer() = default;
  virtual void draw_rect(const DrawRect &r) = 0;
  virtual void draw_text(const DrawText &t) = 0;
};

inline void render_with(Renderer &renderer, const std::vector<RenderOp> &ops) {
  for (const auto &op : ops) {
    std::visit(
        [&](const auto &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, DrawRect>) {
            renderer.draw_rect(v);
          } else if constexpr (std::is_same_v<T, DrawText>) {
            renderer.draw_text(v);
          }
        },
        op);
  }
}

inline bool prop_as_bool(const Props &props, const std::string &key,
                         bool fallback) {
  const auto it = props.find(key);
  if (it == props.end()) {
    return fallback;
  }
  if (const auto *b = std::get_if<bool>(&it->second)) {
    return *b;
  }
  if (const auto *i = std::get_if<std::int64_t>(&it->second)) {
    return *i != 0;
  }
  if (const auto *d = std::get_if<double>(&it->second)) {
    return *d != 0.0;
  }
  return fallback;
}

inline void build_render_ops(const ViewNode &v, const LayoutNode &l,
                             std::vector<RenderOp> &out) {
  if (v.type == "Button") {
    const auto pressed = prop_as_bool(v.props, "pressed", false);
    out.push_back(DrawRect{l.frame, pressed ? ColorU8{120, 120, 120, 255}
                                            : ColorU8{80, 80, 80, 255}});
    const auto title = prop_as_string(v.props, "title", "");
    const auto font_px = prop_as_float(v.props, "font_size", 16.0f);
    out.push_back(
        DrawText{l.frame, title, ColorU8{255, 255, 255, 255}, font_px});
  } else if (v.type == "Text") {
    const auto text = prop_as_string(v.props, "value", "");
    const auto font_px = prop_as_float(v.props, "font_size", 16.0f);
    out.push_back(
        DrawText{l.frame, text, ColorU8{255, 255, 255, 255}, font_px});
  }

  const auto n = std::min(v.children.size(), l.children.size());
  for (std::size_t i = 0; i < n; ++i) {
    build_render_ops(v.children[i], l.children[i], out);
  }
}

inline std::vector<RenderOp> build_render_ops(const ViewNode &root,
                                              const LayoutNode &layout_root) {
  std::vector<RenderOp> out;
  build_render_ops(root, layout_root, out);
  return out;
}

inline void dump_render_ops(std::ostream &os,
                            const std::vector<RenderOp> &ops) {
  for (const auto &op : ops) {
    std::visit(
        [&](const auto &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, DrawRect>) {
            os << "Rect [" << v.rect.x << "," << v.rect.y << " " << v.rect.w
               << "x" << v.rect.h << "]\n";
          } else if constexpr (std::is_same_v<T, DrawText>) {
            os << "Text [" << v.rect.x << "," << v.rect.y << " " << v.rect.w
               << "x" << v.rect.h << "] '" << v.text << "'\n";
          }
        },
        op);
  }
}

struct AsciiSurface {
  int cols{};
  int rows{};
  std::vector<char> cells;

  AsciiSurface(int c, int r) : cols{c}, rows{r}, cells(c * r, ' ') {}

  void clear(char ch = ' ') { std::fill(cells.begin(), cells.end(), ch); }

  void set(int x, int y, char ch) {
    if (x < 0 || y < 0 || x >= cols || y >= rows) {
      return;
    }
    cells[static_cast<std::size_t>(y) * static_cast<std::size_t>(cols) +
          static_cast<std::size_t>(x)] = ch;
  }

  char get(int x, int y) const {
    if (x < 0 || y < 0 || x >= cols || y >= rows) {
      return ' ';
    }
    return cells[static_cast<std::size_t>(y) * static_cast<std::size_t>(cols) +
                 static_cast<std::size_t>(x)];
  }
};

inline void draw_rect_ascii(AsciiSurface &s, RectF r, char ch) {
  const auto x0 = static_cast<int>(std::floor(r.x));
  const auto y0 = static_cast<int>(std::floor(r.y));
  const auto x1 = static_cast<int>(std::ceil(r.x + r.w));
  const auto y1 = static_cast<int>(std::ceil(r.y + r.h));
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      s.set(x, y, ch);
    }
  }
}

inline void draw_text_ascii(AsciiSurface &s, RectF r, std::string_view text) {
  const auto x0 = static_cast<int>(std::floor(r.x));
  const auto y0 = static_cast<int>(std::floor(r.y));
  if (y0 < 0 || y0 >= s.rows) {
    return;
  }
  int x = x0;
  for (char ch : text) {
    if (x >= s.cols) {
      break;
    }
    if (x >= 0) {
      s.set(x, y0, ch);
    }
    ++x;
  }
}

inline void render_ascii(std::ostream &os, const std::vector<RenderOp> &ops,
                         SizeF viewport_px, int cols = 80, int rows = 24) {
  if (cols <= 0 || rows <= 0) {
    return;
  }

  AsciiSurface surf{cols, rows};
  surf.clear(' ');

  const float sx =
      viewport_px.w > 0 ? static_cast<float>(cols) / viewport_px.w : 1.0f;
  const float sy =
      viewport_px.h > 0 ? static_cast<float>(rows) / viewport_px.h : 1.0f;

  auto map_rect = [&](RectF r) {
    return RectF{r.x * sx, r.y * sy, r.w * sx, r.h * sy};
  };

  for (const auto &op : ops) {
    std::visit(
        [&](const auto &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, DrawRect>) {
            draw_rect_ascii(surf, map_rect(v.rect), '#');
          } else if constexpr (std::is_same_v<T, DrawText>) {
            draw_text_ascii(surf, map_rect(v.rect), v.text);
          }
        },
        op);
  }

  for (int y = 0; y < rows; ++y) {
    for (int x = 0; x < cols; ++x) {
      os.put(surf.get(x, y));
    }
    os.put('\n');
  }
}

} // namespace duorou::ui
