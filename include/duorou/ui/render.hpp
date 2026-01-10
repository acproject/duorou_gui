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
            if (v.text.empty()) {
              return;
            }
            TextLayout layout;
            if (!text.layout_text(v.text, v.font_px, layout)) {
              return;
            }
            if (!(layout.w > 0.0f) || !(layout.h > 0.0f) || layout.quads.empty()) {
              return;
            }

            const float target_w = v.rect.w;
            const float target_h = v.rect.h;
            const float scale = std::min(target_w / layout.w, target_h / layout.h);
            if (!(scale > 0.0f)) {
              return;
            }

            const float draw_w = layout.w * scale;
            const float draw_h = layout.h * scale;
            const float ox = v.rect.x + (v.rect.w - draw_w) * 0.5f;
            const float oy = v.rect.y + (v.rect.h - draw_h) * 0.5f;

            const auto col = pack_rgba(v.color);
            const auto sc = current_clip();
            for (const auto &q : layout.quads) {
              auto &b = ensure_batch(RenderPipeline::Text, q.texture, sc);
              const float x0 = ox + q.x0 * scale;
              const float y0 = oy + q.y0 * scale;
              const float x1 = ox + q.x1 * scale;
              const float y1 = oy + q.y1 * scale;
              push_quad(x0, y0, x1, y1, q.u0, q.v0, q.u1, q.v1, col);
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

inline void build_render_ops(const ViewNode &v, const LayoutNode &l,
                             std::vector<RenderOp> &out) {
  const bool clip = prop_as_bool(v.props, "clip", false);
  if (clip) {
    out.push_back(PushClip{l.frame});
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
            draw_text_ascii(surf, c, v.text);
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
