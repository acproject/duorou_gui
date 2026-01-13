#pragma once

#include <duorou/ui/base_layout.hpp>

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

inline std::int64_t utf8_len(std::string_view s) {
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
}

struct ColorU8 {
  std::uint8_t r{};
  std::uint8_t g{};
  std::uint8_t b{};
  std::uint8_t a{255};
};

using TextureHandle = std::uint64_t;

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
  std::int64_t caret_pos{-1};
  ColorU8 caret_color{220, 220, 220, 255};
  float caret_w{1.0f};
  float caret_h_factor{1.1f};
  std::int64_t sel_start{-1};
  std::int64_t sel_end{-1};
  ColorU8 sel_color{70, 120, 210, 180};
};

struct DrawImage {
  RectF rect;
  TextureHandle texture{};
  RectF uv{0.0f, 0.0f, 1.0f, 1.0f};
  ColorU8 tint{255, 255, 255, 255};
};

struct PushClip {
  RectF rect;
};

struct PopClip {};

using RenderOp =
    std::variant<PushClip, PopClip, DrawRect, DrawText, DrawImage>;

struct Renderer {
  virtual ~Renderer() = default;
  virtual void push_clip(const PushClip &c) = 0;
  virtual void pop_clip(const PopClip &c) = 0;
  virtual void draw_rect(const DrawRect &r) = 0;
  virtual void draw_text(const DrawText &t) = 0;
  virtual void draw_image(const DrawImage &i) = 0;
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
          } else if constexpr (std::is_same_v<T, DrawImage>) {
            renderer.draw_image(v);
          }
        },
        op);
  }
}

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
  Image = 2,
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
  std::vector<float> caret_x{};
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

              if (v.sel_start >= 0 && v.sel_end >= 0 &&
                  v.sel_start != v.sel_end) {
                const auto len = utf8_len(v.text);
                const auto a = std::max<std::int64_t>(
                    0, std::min<std::int64_t>(v.sel_start, len));
                const auto b = std::max<std::int64_t>(
                    0, std::min<std::int64_t>(v.sel_end, len));
                const auto s0 = std::min(a, b);
                const auto s1 = std::max(a, b);
                float x0 = ox;
                float x1 = ox;
                if (!layout.caret_x.empty()) {
                  const auto last =
                      static_cast<std::int64_t>(layout.caret_x.size()) - 1;
                  const auto i0 =
                      std::max<std::int64_t>(0, std::min(s0, last));
                  const auto i1 =
                      std::max<std::int64_t>(0, std::min(s1, last));
                  x0 = ox + layout.caret_x[static_cast<std::size_t>(i0)] * scale;
                  x1 = ox + layout.caret_x[static_cast<std::size_t>(i1)] * scale;
                } else {
                  const float p0 =
                      len > 0 ? clampf(static_cast<float>(s0), 0.0f,
                                       static_cast<float>(len)) /
                                    static_cast<float>(len)
                              : 0.0f;
                  const float p1 =
                      len > 0 ? clampf(static_cast<float>(s1), 0.0f,
                                       static_cast<float>(len)) /
                                    static_cast<float>(len)
                              : 0.0f;
                  x0 = ox + draw_w * p0;
                  x1 = ox + draw_w * p1;
                }

                const float sel_h =
                    std::min(v.rect.h, v.font_px * v.caret_h_factor * scale);
                const float sel_y = oy + (draw_h - sel_h) * 0.5f;
                auto &bsel = ensure_batch(RenderPipeline::Color, 0, sc);
                push_quad(std::min(x0, x1), sel_y, std::max(x0, x1),
                          sel_y + sel_h, 0.0f, 0.0f, 0.0f, 0.0f,
                          pack_rgba(v.sel_color));
                bsel.count += 6;
              }

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

              if (v.caret_end || v.caret_pos >= 0) {
                float caret_x = ox + draw_w;
                if (v.caret_pos >= 0) {
                  if (!layout.caret_x.empty()) {
                    const auto last =
                        static_cast<std::int64_t>(layout.caret_x.size()) - 1;
                    const auto idx = std::max<std::int64_t>(
                        0, std::min<std::int64_t>(v.caret_pos, last));
                    caret_x = ox + layout.caret_x[static_cast<std::size_t>(idx)] * scale;
                  } else {
                    const auto len = utf8_len(v.text);
                    const auto p =
                        len > 0 ? clampf(static_cast<float>(v.caret_pos), 0.0f,
                                         static_cast<float>(len)) /
                                      static_cast<float>(len)
                                : 0.0f;
                    caret_x = ox + draw_w * p;
                  }
                }
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
            } else if (v.caret_pos >= 0) {
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
          } else if constexpr (std::is_same_v<T, DrawImage>) {
          const auto sc = current_clip();
          if (v.texture == 0) {
            return;
          }
          auto &b = ensure_batch(RenderPipeline::Image, v.texture, sc);
          const float u0 = v.uv.x;
          const float v0 = v.uv.y;
          const float u1 = v.uv.x + v.uv.w;
          const float v1 = v.uv.y + v.uv.h;
          push_quad(v.rect.x, v.rect.y, v.rect.x + v.rect.w, v.rect.y + v.rect.h,
                    u0, v0, u1, v1, pack_rgba(v.tint));
          b.count += 6;
        }
        },
        op);
  }

  return tree;
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
          } else if constexpr (std::is_same_v<T, DrawImage>) {
            os << "Image [" << v.rect.x << "," << v.rect.y << " " << v.rect.w
               << "x" << v.rect.h << "] tex=" << v.texture << "\n";
          }
        },
        op);
  }
}

} // namespace duorou::ui
