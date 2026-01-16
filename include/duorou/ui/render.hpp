#pragma once

#include <duorou/ui/base_render.hpp>
#include <duorou/ui/layout.hpp>
#include <duorou/ui/node.hpp>

#include <algorithm>
#include <cmath>
#include <ostream>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace duorou::ui {

inline std::uint8_t apply_opacity_u8(std::uint8_t a, float opacity) {
  const float x = static_cast<float>(a) * std::max(0.0f, std::min(1.0f, opacity));
  const int ai = static_cast<int>(std::lround(x));
  return clamp_u8(ai);
}

inline void apply_opacity(RenderOp &op, float opacity) {
  if (opacity >= 1.0f) {
    return;
  }
  std::visit(
      [&](auto &v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, DrawRect>) {
          v.fill.a = apply_opacity_u8(v.fill.a, opacity);
        } else if constexpr (std::is_same_v<T, DrawText>) {
          v.color.a = apply_opacity_u8(v.color.a, opacity);
          v.caret_color.a = apply_opacity_u8(v.caret_color.a, opacity);
          v.sel_color.a = apply_opacity_u8(v.sel_color.a, opacity);
        } else if constexpr (std::is_same_v<T, DrawImage>) {
          v.tint.a = apply_opacity_u8(v.tint.a, opacity);
        }
      },
      op);
}

inline RectF apply_offset(RectF r, float ox, float oy) {
  r.x += ox;
  r.y += oy;
  return r;
}

inline RectF apply_scale_about(RectF r, float ox, float oy, float s) {
  r.x = ox + (r.x - ox) * s;
  r.y = oy + (r.y - oy) * s;
  r.w *= s;
  r.h *= s;
  return r;
}

inline void apply_scale_about(RenderOp &op, float ox, float oy, float s) {
  if (s == 1.0f) {
    return;
  }
  std::visit(
      [&](auto &v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, PushClip>) {
          v.rect = apply_scale_about(v.rect, ox, oy, s);
        } else if constexpr (std::is_same_v<T, DrawRect>) {
          v.rect = apply_scale_about(v.rect, ox, oy, s);
        } else if constexpr (std::is_same_v<T, DrawText>) {
          v.rect = apply_scale_about(v.rect, ox, oy, s);
          v.font_px *= s;
          v.caret_w *= s;
        } else if constexpr (std::is_same_v<T, DrawImage>) {
          v.rect = apply_scale_about(v.rect, ox, oy, s);
        }
      },
      op);
}

inline void build_render_ops(const ViewNode &v, const LayoutNode &l,
                             float parent_opacity, float parent_ox,
                             float parent_oy, std::vector<RenderOp> &out) {
  const bool clip = prop_as_bool(v.props, "clip", false);
  const float opacity =
      parent_opacity * prop_as_float(v.props, "opacity", 1.0f);
  const float ox = parent_ox + prop_as_float(v.props, "render_offset_x", 0.0f);
  const float oy = parent_oy + prop_as_float(v.props, "render_offset_y", 0.0f);
  const float render_scale = prop_as_float(v.props, "render_scale", 1.0f);

  const RectF frame = apply_offset(l.frame, ox, oy);

  const auto start_all = out.size();
  if (clip) {
    out.push_back(PushClip{frame});
  }

  const auto start0 = out.size();
  emit_render_ops_box(v, l, out);
  emit_render_ops_divider(v, l, out);
  emit_render_ops_checkbox(v, l, out);
  emit_render_ops_slider(v, l, out);
  emit_render_ops_progressview(v, l, out);
  emit_render_ops_textfield(v, l, out);
  emit_render_ops_texteditor(v, l, out);
  emit_render_ops_button(v, l, out);
  emit_render_ops_stepper(v, l, out);
  emit_render_ops_image(v, l, out);
  emit_render_ops_canvas(v, l, out);
  emit_render_ops_text(v, l, out);
  for (std::size_t i = start0; i < out.size(); ++i) {
    apply_opacity(out[i], opacity);
    std::visit(
        [&](auto &op) {
          using T = std::decay_t<decltype(op)>;
          if constexpr (std::is_same_v<T, PushClip>) {
            op.rect = apply_offset(op.rect, ox, oy);
          } else if constexpr (std::is_same_v<T, DrawRect>) {
            op.rect = apply_offset(op.rect, ox, oy);
          } else if constexpr (std::is_same_v<T, DrawText>) {
            op.rect = apply_offset(op.rect, ox, oy);
          } else if constexpr (std::is_same_v<T, DrawImage>) {
            op.rect = apply_offset(op.rect, ox, oy);
          }
        },
        out[i]);
  }

  const auto n = std::min(v.children.size(), l.children.size());
  for (std::size_t i = 0; i < n; ++i) {
    build_render_ops(v.children[i], l.children[i], opacity, ox, oy, out);
  }

  const auto start1 = out.size();
  emit_render_ops_scrollview(v, l, out);
  for (std::size_t i = start1; i < out.size(); ++i) {
    apply_opacity(out[i], opacity);
    std::visit(
        [&](auto &op) {
          using T = std::decay_t<decltype(op)>;
          if constexpr (std::is_same_v<T, DrawRect>) {
            op.rect = apply_offset(op.rect, ox, oy);
          } else if constexpr (std::is_same_v<T, DrawText>) {
            op.rect = apply_offset(op.rect, ox, oy);
          } else if constexpr (std::is_same_v<T, DrawImage>) {
            op.rect = apply_offset(op.rect, ox, oy);
          }
        },
        out[i]);
  }

  if (clip) {
    out.push_back(PopClip{});
  }

  if (render_scale != 1.0f) {
    for (std::size_t i = start_all; i < out.size(); ++i) {
      apply_scale_about(out[i], frame.x, frame.y, render_scale);
    }
  }
}

inline std::vector<RenderOp> build_render_ops(const ViewNode &root,
                                              const LayoutNode &layout_root) {
  std::vector<RenderOp> out;
  out.push_back(PushClip{layout_root.frame});
  build_render_ops(root, layout_root, 1.0f, 0.0f, 0.0f, out);
  out.push_back(PopClip{});
  return out;
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
          } else if constexpr (std::is_same_v<T, DrawImage>) {
            const auto c = clip_stack.empty() ? map_rect(v.rect)
                                              : intersect(clip_stack.back(),
                                                          map_rect(v.rect));
            draw_rect_ascii(surf, c, '@');
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
