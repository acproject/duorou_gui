#pragma once

#include <duorou/ui/layout.hpp>
#include <duorou/ui/node.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
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
  float align_x{0.5f};
  float align_y{0.5f};
  bool caret_end{};
  ColorU8 caret_color{220, 220, 220, 255};
  float caret_w{1.0f};
  float caret_h_factor{1.1f};
};

struct PushClip {
  RectF rect;
};

struct PopClip {};

using RenderOp = std::variant<PushClip, PopClip, DrawRect, DrawText>;

struct Renderer {
  virtual ~Renderer() = default;
  virtual void push_clip(const PushClip &c) = 0;
  virtual void pop_clip(const PopClip &c) = 0;
  virtual void draw_rect(const DrawRect &r) = 0;
  virtual void draw_text(const DrawText &t) = 0;
};

inline void render_with(Renderer &renderer, const std::vector<RenderOp> &ops) {
  for (const auto &op : ops) {
    std::visit(
        [&](const auto &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, PushClip>) {
            renderer.push_clip(v);
          } else if constexpr (std::is_same_v<T, PopClip>) {
            renderer.pop_clip(v);
          } else if constexpr (std::is_same_v<T, DrawRect>) {
            renderer.draw_rect(v);
          } else if constexpr (std::is_same_v<T, DrawText>) {
            renderer.draw_text(v);
          }
        },
        op);
  }
}

using TextureHandle = std::uint64_t;

struct RenderVertex {
  float x{};
  float y{};
  float u{};
  float v{};
  std::uint32_t rgba{};
};

enum class RenderPipeline : std::uint8_t {
  Color = 0,
  Text = 1,
};

struct RenderBatch {
  RenderPipeline pipeline{RenderPipeline::Color};
  TextureHandle texture{};
  RectF scissor{};
  std::size_t first{};
  std::size_t count{};
};

struct RenderTree {
  SizeF viewport{};
  std::vector<RenderVertex> vertices{};
  std::vector<RenderBatch> batches{};
};

struct TextQuad {
  float x0{};
  float y0{};
  float x1{};
  float y1{};
  float u0{};
  float v0{};
  float u1{};
  float v1{};
  TextureHandle texture{};
};

struct TextLayout {
  float w{};
  float h{};
  std::vector<TextQuad> quads{};
};

struct TextProvider {
  virtual ~TextProvider() = default;
  virtual bool layout_text(std::string_view text, float font_px,
                           TextLayout &out) = 0;
};

inline std::uint32_t pack_rgba(ColorU8 c) {
  return static_cast<std::uint32_t>(c.r) |
         (static_cast<std::uint32_t>(c.g) << 8) |
         (static_cast<std::uint32_t>(c.b) << 16) |
         (static_cast<std::uint32_t>(c.a) << 24);
}

inline RectF intersect_rect(RectF a, RectF b) {
  const float x0 = std::max(a.x, b.x);
  const float y0 = std::max(a.y, b.y);
  const float x1 = std::min(a.x + a.w, b.x + b.w);
  const float y1 = std::min(a.y + a.h, b.y + b.h);
  const float w = std::max(0.0f, x1 - x0);
  const float h = std::max(0.0f, y1 - y0);
  return RectF{x0, y0, w, h};
}

inline RenderTree build_render_tree(const std::vector<RenderOp> &ops,
                                    SizeF viewport, TextProvider &text) {
  RenderTree tree;
  tree.viewport = viewport;
  tree.vertices.reserve(4096);
  tree.batches.reserve(256);

  auto rect_eq = [&](RectF a, RectF b) {
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
  };

  auto ensure_batch = [&](RenderPipeline pipeline, TextureHandle texture,
                          RectF scissor) -> RenderBatch & {
    if (tree.batches.empty() ||
        tree.batches.back().pipeline != pipeline ||
        tree.batches.back().texture != texture ||
        !rect_eq(tree.batches.back().scissor, scissor)) {
      RenderBatch b;
      b.pipeline = pipeline;
      b.texture = texture;
      b.scissor = scissor;
      b.first = tree.vertices.size();
      b.count = 0;
      tree.batches.push_back(b);
    }
    return tree.batches.back();
  };

  auto push_quad = [&](float x0, float y0, float x1, float y1, float u0, float v0,
                       float u1, float v1, std::uint32_t rgba) {
    tree.vertices.push_back(RenderVertex{x0, y0, u0, v0, rgba});
    tree.vertices.push_back(RenderVertex{x1, y0, u1, v0, rgba});
    tree.vertices.push_back(RenderVertex{x0, y1, u0, v1, rgba});
    tree.vertices.push_back(RenderVertex{x0, y1, u0, v1, rgba});
    tree.vertices.push_back(RenderVertex{x1, y0, u1, v0, rgba});
    tree.vertices.push_back(RenderVertex{x1, y1, u1, v1, rgba});
  };

  std::vector<RectF> clip_stack;
  clip_stack.reserve(32);
  clip_stack.push_back(RectF{0.0f, 0.0f, viewport.w, viewport.h});

  auto current_clip = [&]() -> RectF {
    if (clip_stack.empty()) {
      return RectF{0.0f, 0.0f, viewport.w, viewport.h};
    }
    return clip_stack.back();
  };

  for (const auto &op : ops) {
    std::visit(
        [&](const auto &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, PushClip>) {
            const auto top = current_clip();
            clip_stack.push_back(intersect_rect(top, v.rect));
          } else if constexpr (std::is_same_v<T, PopClip>) {
            if (!clip_stack.empty()) {
              clip_stack.pop_back();
            }
            if (clip_stack.empty()) {
              clip_stack.push_back(RectF{0.0f, 0.0f, viewport.w, viewport.h});
            }
          } else if constexpr (std::is_same_v<T, DrawRect>) {
            auto &b = ensure_batch(RenderPipeline::Color, 0, current_clip());
            push_quad(v.rect.x, v.rect.y, v.rect.x + v.rect.w, v.rect.y + v.rect.h,
                      0.0f, 0.0f, 0.0f, 0.0f, pack_rgba(v.fill));
            b.count += 6;
          } else if constexpr (std::is_same_v<T, DrawText>) {
            const auto sc = current_clip();
            if (!v.text.empty()) {
              TextLayout layout;
              if (!text.layout_text(v.text, v.font_px, layout)) {
                return;
              }
              if (!(layout.w > 0.0f) || !(layout.h > 0.0f) ||
                  layout.quads.empty()) {
                return;
              }

              const float target_w = v.rect.w;
              const float target_h = v.rect.h;
              const float scale =
                  std::min(target_w / layout.w, target_h / layout.h);
              if (!(scale > 0.0f)) {
                return;
              }

              const float draw_w = layout.w * scale;
              const float draw_h = layout.h * scale;
              const float ox = v.rect.x + (v.rect.w - draw_w) * v.align_x;
              const float oy = v.rect.y + (v.rect.h - draw_h) * v.align_y;

              const auto col = pack_rgba(v.color);
              for (const auto &q : layout.quads) {
                auto &b = ensure_batch(RenderPipeline::Text, q.texture, sc);
                const float x0 = ox + q.x0 * scale;
                const float y0 = oy + q.y0 * scale;
                const float x1 = ox + q.x1 * scale;
                const float y1 = oy + q.y1 * scale;
                push_quad(x0, y0, x1, y1, q.u0, q.v0, q.u1, q.v1, col);
                b.count += 6;
              }

              if (v.caret_end) {
                const float caret_x = ox + draw_w;
                const float caret_h =
                    std::min(v.rect.h, v.font_px * v.caret_h_factor * scale);
                const float caret_y = oy + (draw_h - caret_h) * 0.5f;
                auto &b = ensure_batch(RenderPipeline::Color, 0, sc);
                push_quad(caret_x, caret_y, caret_x + v.caret_w,
                          caret_y + caret_h, 0.0f, 0.0f, 0.0f, 0.0f,
                          pack_rgba(v.caret_color));
                b.count += 6;
              }
            } else if (v.caret_end) {
              const float scale = 1.0f;
              const float caret_h =
                  std::min(v.rect.h, v.font_px * v.caret_h_factor * scale);
              const float caret_y =
                  v.rect.y + (v.rect.h - caret_h) * v.align_y;
              const float caret_x = v.rect.x;
              auto &b = ensure_batch(RenderPipeline::Color, 0, sc);
              push_quad(caret_x, caret_y, caret_x + v.caret_w, caret_y + caret_h,
                        0.0f, 0.0f, 0.0f, 0.0f, pack_rgba(v.caret_color));
              b.count += 6;
            }
          }
        },
        op);
  }

  return tree;
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

inline std::uint8_t clamp_u8(int v) {
  if (v < 0) {
    return 0;
  }
  if (v > 255) {
    return 255;
  }
  return static_cast<std::uint8_t>(v);
}

inline ColorU8 color_from_u32(std::uint32_t rgba) {
  ColorU8 c;
  c.r = static_cast<std::uint8_t>(rgba & 0xFFu);
  c.g = static_cast<std::uint8_t>((rgba >> 8) & 0xFFu);
  c.b = static_cast<std::uint8_t>((rgba >> 16) & 0xFFu);
  c.a = static_cast<std::uint8_t>((rgba >> 24) & 0xFFu);
  return c;
}

inline int hex_nibble(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  if (ch >= 'a' && ch <= 'f') {
    return 10 + (ch - 'a');
  }
  if (ch >= 'A' && ch <= 'F') {
    return 10 + (ch - 'A');
  }
  return -1;
}

inline bool parse_hex_byte(std::string_view s, std::size_t i, std::uint8_t &out) {
  if (i + 1 >= s.size()) {
    return false;
  }
  const int hi = hex_nibble(s[i]);
  const int lo = hex_nibble(s[i + 1]);
  if (hi < 0 || lo < 0) {
    return false;
  }
  out = static_cast<std::uint8_t>((hi << 4) | lo);
  return true;
}

inline std::optional<ColorU8> parse_color(std::string_view s) {
  if (s.size() == 7 && s[0] == '#') {
    std::uint8_t r{}, g{}, b{};
    if (!parse_hex_byte(s, 1, r) || !parse_hex_byte(s, 3, g) ||
        !parse_hex_byte(s, 5, b)) {
      return std::nullopt;
    }
    return ColorU8{r, g, b, 255};
  }
  if (s.size() == 9 && s[0] == '#') {
    std::uint8_t r{}, g{}, b{}, a{};
    if (!parse_hex_byte(s, 1, r) || !parse_hex_byte(s, 3, g) ||
        !parse_hex_byte(s, 5, b) || !parse_hex_byte(s, 7, a)) {
      return std::nullopt;
    }
    return ColorU8{r, g, b, a};
  }
  return std::nullopt;
}

inline ColorU8 prop_as_color(const Props &props, const std::string &key,
                             ColorU8 fallback) {
  const auto *pv = find_prop(props, key);
  if (!pv) {
    return fallback;
  }
  if (const auto *i = std::get_if<std::int64_t>(pv)) {
    const auto u = static_cast<std::uint32_t>(
        static_cast<std::uint64_t>(*i) & 0xFFFFFFFFull);
    if (u <= 0xFFFFFFu) {
      return ColorU8{static_cast<std::uint8_t>(u & 0xFFu),
                     static_cast<std::uint8_t>((u >> 8) & 0xFFu),
                     static_cast<std::uint8_t>((u >> 16) & 0xFFu), 255};
    }
    return color_from_u32(u);
  }
  if (const auto *d = std::get_if<double>(pv)) {
    const auto u = static_cast<std::uint32_t>(static_cast<std::uint64_t>(*d));
    if (u <= 0xFFFFFFu) {
      return ColorU8{static_cast<std::uint8_t>(u & 0xFFu),
                     static_cast<std::uint8_t>((u >> 8) & 0xFFu),
                     static_cast<std::uint8_t>((u >> 16) & 0xFFu), 255};
    }
    return color_from_u32(u);
  }
  if (const auto *s = std::get_if<std::string>(pv)) {
    if (const auto c = parse_color(*s)) {
      return *c;
    }
  }
  return fallback;
}

inline void build_render_ops(const ViewNode &v, const LayoutNode &l,
                             std::vector<RenderOp> &out) {
  const bool clip = prop_as_bool(v.props, "clip", false);
  if (clip) {
    out.push_back(PushClip{l.frame});
  }

  if (v.type == "Box") {
    if (find_prop(v.props, "bg")) {
      const auto bg = prop_as_color(v.props, "bg", ColorU8{0, 0, 0, 0});
      out.push_back(DrawRect{l.frame, bg});
    }
    const auto bw = prop_as_float(v.props, "border_width", 0.0f);
    if (bw > 0.0f && find_prop(v.props, "border")) {
      const auto bc = prop_as_color(v.props, "border", ColorU8{80, 80, 80, 255});
      const auto t = bw;
      out.push_back(DrawRect{RectF{l.frame.x, l.frame.y, l.frame.w, t}, bc});
      out.push_back(DrawRect{RectF{l.frame.x, l.frame.y + l.frame.h - t, l.frame.w, t}, bc});
      out.push_back(DrawRect{RectF{l.frame.x, l.frame.y, t, l.frame.h}, bc});
      out.push_back(DrawRect{RectF{l.frame.x + l.frame.w - t, l.frame.y, t, l.frame.h}, bc});
    }
  } else if (v.type == "Divider") {
    const auto col = prop_as_color(v.props, "color", ColorU8{60, 60, 60, 255});
    out.push_back(DrawRect{l.frame, col});
  } else if (v.type == "Checkbox") {
    const auto padding = prop_as_float(v.props, "padding", 0.0f);
    const auto gap = prop_as_float(v.props, "gap", 8.0f);
    const auto font_px = prop_as_float(v.props, "font_size", 16.0f);
    const auto box = std::max(12.0f, std::min(l.frame.h - padding * 2.0f, font_px * 1.0f));
    const float bx = l.frame.x + padding;
    const float by = l.frame.y + (l.frame.h - box) * 0.5f;
    const RectF outer{bx, by, box, box};
    out.push_back(DrawRect{outer, ColorU8{40, 40, 40, 255}});
    if (box >= 2.0f) {
      out.push_back(DrawRect{RectF{bx + 1.0f, by + 1.0f, box - 2.0f, box - 2.0f},
                             ColorU8{120, 120, 120, 255}});
    }
    const auto checked = prop_as_bool(v.props, "checked", false);
    if (checked && box >= 6.0f) {
      out.push_back(DrawRect{RectF{bx + 3.0f, by + 3.0f, box - 6.0f, box - 6.0f},
                             ColorU8{30, 200, 120, 255}});
    }
    const auto label = prop_as_string(v.props, "label", "");
    const RectF tr{bx + box + gap, l.frame.y, std::max(0.0f, l.frame.w - (box + gap + padding)), l.frame.h};
    out.push_back(DrawText{tr, label, ColorU8{230, 230, 230, 255}, font_px, 0.0f, 0.5f});
  } else if (v.type == "Slider") {
    const auto padding = prop_as_float(v.props, "padding", 0.0f);
    const auto min_v = prop_as_float(v.props, "min", 0.0f);
    const auto max_v = prop_as_float(v.props, "max", 1.0f);
    const auto v0 = prop_as_float(v.props, "value", 0.0f);
    const auto thumb = prop_as_float(v.props, "thumb_size", 14.0f);
    const auto track_h = prop_as_float(v.props, "track_height", 4.0f);
    const auto denom = (max_v - min_v);
    const auto t = denom != 0.0f ? clampf((v0 - min_v) / denom, 0.0f, 1.0f) : 0.0f;
    const float x0 = l.frame.x + padding;
    const float x1 = l.frame.x + l.frame.w - padding;
    const float w = std::max(0.0f, x1 - x0);
    const float cy = l.frame.y + l.frame.h * 0.5f;
    const RectF track{x0, cy - track_h * 0.5f, w, track_h};
    out.push_back(DrawRect{track, ColorU8{60, 60, 60, 255}});
    out.push_back(DrawRect{RectF{track.x, track.y, track.w * t, track.h}, ColorU8{80, 140, 255, 255}});
    const float thumb_x = x0 + (w - thumb) * t;
    const RectF knob{thumb_x, cy - thumb * 0.5f, thumb, thumb};
    out.push_back(DrawRect{knob, ColorU8{200, 200, 200, 255}});
  } else if (v.type == "TextField") {
    const auto padding = prop_as_float(v.props, "padding", 10.0f);
    const auto font_px = prop_as_float(v.props, "font_size", 16.0f);
    const auto focused = prop_as_bool(v.props, "focused", false);
    const auto value = prop_as_string(v.props, "value", "");
    const auto placeholder = prop_as_string(v.props, "placeholder", "");
    out.push_back(DrawRect{l.frame, focused ? ColorU8{55, 55, 55, 255}
                                           : ColorU8{45, 45, 45, 255}});
    const RectF tr{l.frame.x + padding, l.frame.y, std::max(0.0f, l.frame.w - padding * 2.0f), l.frame.h};
    if (value.empty()) {
      out.push_back(DrawText{tr, placeholder, ColorU8{160, 160, 160, 255},
                             font_px, 0.0f, 0.5f});
    } else {
      DrawText t{tr, value, ColorU8{235, 235, 235, 255}, font_px, 0.0f, 0.5f};
      t.caret_end = focused;
      out.push_back(std::move(t));
    }
    if (focused && value.empty()) {
      DrawText t{tr, std::string{}, ColorU8{235, 235, 235, 255}, font_px, 0.0f,
                 0.5f};
      t.caret_end = true;
      out.push_back(std::move(t));
    }
  }

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

  if (clip) {
    out.push_back(PopClip{});
  }
}

inline std::vector<RenderOp> build_render_ops(const ViewNode &root,
                                              const LayoutNode &layout_root) {
  std::vector<RenderOp> out;
  out.push_back(PushClip{layout_root.frame});
  build_render_ops(root, layout_root, out);
  out.push_back(PopClip{});
  return out;
}

inline void dump_render_ops(std::ostream &os,
                            const std::vector<RenderOp> &ops) {
  for (const auto &op : ops) {
    std::visit(
        [&](const auto &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, PushClip>) {
            os << "PushClip [" << v.rect.x << "," << v.rect.y << " " << v.rect.w
               << "x" << v.rect.h << "]\n";
          } else if constexpr (std::is_same_v<T, PopClip>) {
            os << "PopClip\n";
          } else if constexpr (std::is_same_v<T, DrawRect>) {
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

  auto intersect = [&](RectF a, RectF b) {
    const float x0 = std::max(a.x, b.x);
    const float y0 = std::max(a.y, b.y);
    const float x1 = std::min(a.x + a.w, b.x + b.w);
    const float y1 = std::min(a.y + a.h, b.y + b.h);
    const float w = std::max(0.0f, x1 - x0);
    const float h = std::max(0.0f, y1 - y0);
    return RectF{x0, y0, w, h};
  };

  std::vector<RectF> clip_stack;
  clip_stack.push_back(RectF{0.0f, 0.0f, static_cast<float>(cols),
                             static_cast<float>(rows)});

  for (const auto &op : ops) {
    std::visit(
        [&](const auto &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, PushClip>) {
            const auto mapped = map_rect(v.rect);
            const auto top = clip_stack.empty() ? mapped : clip_stack.back();
            clip_stack.push_back(intersect(top, mapped));
          } else if constexpr (std::is_same_v<T, PopClip>) {
            if (clip_stack.size() > 1) {
              clip_stack.pop_back();
            }
          } else if constexpr (std::is_same_v<T, DrawRect>) {
            const auto c = clip_stack.empty() ? map_rect(v.rect)
                                              : intersect(clip_stack.back(),
                                                          map_rect(v.rect));
            draw_rect_ascii(surf, c, '#');
          } else if constexpr (std::is_same_v<T, DrawText>) {
            const auto c = clip_stack.empty() ? map_rect(v.rect)
                                              : intersect(clip_stack.back(),
                                                          map_rect(v.rect));
            const float w = std::max(0.0f, c.w);
            const float h = std::max(0.0f, c.h);
            const float tw = static_cast<float>(v.text.size());
            const float th = 1.0f;
            const float ox = c.x + std::max(0.0f, (w - tw)) * v.align_x;
            const float oy = c.y + std::max(0.0f, (h - th)) * v.align_y;
            draw_text_ascii(surf, RectF{ox, oy, c.w, c.h}, v.text);
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
