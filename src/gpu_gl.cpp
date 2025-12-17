#include <duorou/ui/runtime.hpp>

#if !defined(_WIN32) && !defined(__linux__)
int main() { return 0; }
#else

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(DUOROU_HAS_FREETYPE) && DUOROU_HAS_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

using namespace duorou::ui;

static void gl_set_ortho(int w, int h) {
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, static_cast<double>(w), static_cast<double>(h), 0.0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

struct GLTextEntry {
  GLuint texture{};
  int w{};
  int h{};
};

static std::string duorou_readable_font_path() {
  const char *candidates[] = {
#if defined(_WIN32)
      "C:/Windows/Fonts/segoeui.ttf",
      "C:/Windows/Fonts/arial.ttf",
#elif defined(__APPLE__)
      "/System/Library/Fonts/Supplemental/Arial.ttf",
      "/System/Library/Fonts/SFNS.ttf",
      "/System/Library/Fonts/Supplemental/Songti.ttc",
#else
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
      "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
      "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
      "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
#endif
  };

  for (const char *p : candidates) {
    if (!p) {
      continue;
    }
    FILE *f = std::fopen(p, "rb");
    if (f) {
      std::fclose(f);
      return std::string{p};
    }
  }
  return {};
}

static bool duorou_utf8_next(std::string_view s, std::size_t &i,
                             std::uint32_t &out_cp) {
  if (i >= s.size()) {
    return false;
  }

  const auto b0 = static_cast<std::uint8_t>(s[i]);
  if (b0 < 0x80) {
    out_cp = b0;
    ++i;
    return true;
  }

  auto need = [&](int count) {
    if (i + static_cast<std::size_t>(count) > s.size()) {
      return false;
    }
    return true;
  };

  if ((b0 & 0xE0) == 0xC0) {
    if (!need(2)) {
      return false;
    }
    const auto b1 = static_cast<std::uint8_t>(s[i + 1]);
    if ((b1 & 0xC0) != 0x80) {
      ++i;
      out_cp = 0xFFFD;
      return true;
    }
    out_cp = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
    i += 2;
    return true;
  }

  if ((b0 & 0xF0) == 0xE0) {
    if (!need(3)) {
      return false;
    }
    const auto b1 = static_cast<std::uint8_t>(s[i + 1]);
    const auto b2 = static_cast<std::uint8_t>(s[i + 2]);
    if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) {
      ++i;
      out_cp = 0xFFFD;
      return true;
    }
    out_cp = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
    i += 3;
    return true;
  }

  if ((b0 & 0xF8) == 0xF0) {
    if (!need(4)) {
      return false;
    }
    const auto b1 = static_cast<std::uint8_t>(s[i + 1]);
    const auto b2 = static_cast<std::uint8_t>(s[i + 2]);
    const auto b3 = static_cast<std::uint8_t>(s[i + 3]);
    if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) {
      ++i;
      out_cp = 0xFFFD;
      return true;
    }
    out_cp = ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) |
             (b3 & 0x3F);
    i += 4;
    return true;
  }

  ++i;
  out_cp = 0xFFFD;
  return true;
}

class GLTextCache {
public:
  GLTextCache() = default;
  GLTextCache(const GLTextCache &) = delete;
  GLTextCache &operator=(const GLTextCache &) = delete;

  ~GLTextCache() {
    for (auto &kv : cache_) {
      if (kv.second.texture != 0) {
        glDeleteTextures(1, &kv.second.texture);
      }
    }

#if defined(DUOROU_HAS_FREETYPE) && DUOROU_HAS_FREETYPE
    if (face_) {
      FT_Done_Face(face_);
    }
    if (ft_) {
      FT_Done_FreeType(ft_);
    }
#endif
  }

  const GLTextEntry *get(std::string_view text, float font_px) {
    if (text.empty()) {
      return nullptr;
    }
    const auto key = make_key(text, font_px);
    if (const auto it = cache_.find(key); it != cache_.end()) {
      return &it->second;
    }

    GLTextEntry e{};
    if (!rasterize_to_texture(text, font_px, e)) {
      return nullptr;
    }

    auto [it, _] = cache_.emplace(key, e);
    return &it->second;
  }

private:
  static std::string make_key(std::string_view text, float font_px) {
    const auto scaled = static_cast<int>(std::lround(font_px * 100.0f));
    std::string k;
    k.reserve(16 + text.size());
    k.append(std::to_string(scaled));
    k.push_back(':');
    k.append(text.data(), text.size());
    return k;
  }

  bool rasterize_to_texture(std::string_view text, float font_px,
                            GLTextEntry &out) {
#if !(defined(DUOROU_HAS_FREETYPE) && DUOROU_HAS_FREETYPE)
    (void)text;
    (void)font_px;
    (void)out;
    return false;
#else
    if (!ensure_face()) {
      return false;
    }

    const auto px = std::max(1, static_cast<int>(std::lround(font_px)));
    if (FT_Set_Pixel_Sizes(face_, 0, static_cast<FT_UInt>(px)) != 0) {
      return false;
    }

    struct Glyph {
      FT_UInt index{};
      int advance{};
      int bitmap_left{};
      int bitmap_top{};
      int w{};
      int h{};
      std::vector<std::uint8_t> buffer;
    };

    std::vector<Glyph> glyphs;
    glyphs.reserve(text.size());

    int pen_x = 0;
    int max_top = 0;
    int max_bottom = 0;

    std::size_t i = 0;
    while (i < text.size()) {
      std::uint32_t cp = 0;
      if (!duorou_utf8_next(text, i, cp)) {
        break;
      }

      const FT_UInt gi = FT_Get_Char_Index(face_, static_cast<FT_ULong>(cp));
      if (FT_Load_Glyph(face_, gi, FT_LOAD_DEFAULT) != 0) {
        continue;
      }
      if (FT_Render_Glyph(face_->glyph, FT_RENDER_MODE_NORMAL) != 0) {
        continue;
      }

      Glyph g{};
      g.index = gi;
      g.advance = static_cast<int>(face_->glyph->advance.x >> 6);
      g.bitmap_left = face_->glyph->bitmap_left;
      g.bitmap_top = face_->glyph->bitmap_top;
      g.w = static_cast<int>(face_->glyph->bitmap.width);
      g.h = static_cast<int>(face_->glyph->bitmap.rows);
      g.buffer.resize(static_cast<std::size_t>(g.w) *
                      static_cast<std::size_t>(g.h));
      for (int y = 0; y < g.h; ++y) {
        const std::uint8_t *src =
            face_->glyph->bitmap.buffer +
            static_cast<std::size_t>(y) * face_->glyph->bitmap.pitch;
        std::uint8_t *dst = g.buffer.data() + static_cast<std::size_t>(y) *
                                                  static_cast<std::size_t>(g.w);
        std::memcpy(dst, src, static_cast<std::size_t>(g.w));
      }

      max_top = std::max(max_top, g.bitmap_top);
      const int bottom = g.h - g.bitmap_top;
      max_bottom = std::max(max_bottom, bottom);

      glyphs.push_back(std::move(g));
      pen_x += glyphs.back().advance;
    }

    if (glyphs.empty()) {
      return false;
    }

    const int pad = 2;
    const int w = std::max(1, pen_x + pad * 2);
    const int h = std::max(1, max_top + max_bottom + pad * 2);
    const int baseline = pad + max_top;

    std::vector<std::uint8_t> pixels(
        static_cast<std::size_t>(w) * static_cast<std::size_t>(h), 0);

    int x = pad;
    for (const auto &g : glyphs) {
      const int dst_x0 = x + g.bitmap_left;
      const int dst_y0 = baseline - g.bitmap_top;
      for (int yy = 0; yy < g.h; ++yy) {
        const int dy = dst_y0 + yy;
        if (dy < 0 || dy >= h) {
          continue;
        }
        for (int xx = 0; xx < g.w; ++xx) {
          const int dx = dst_x0 + xx;
          if (dx < 0 || dx >= w) {
            continue;
          }
          const auto src = g.buffer[static_cast<std::size_t>(yy) *
                                        static_cast<std::size_t>(g.w) +
                                    static_cast<std::size_t>(xx)];
          auto &dst = pixels[static_cast<std::size_t>(dy) *
                                 static_cast<std::size_t>(w) +
                             static_cast<std::size_t>(dx)];
          dst = std::max(dst, src);
        }
      }
      x += g.advance;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    if (tex == 0) {
      return false;
    }

    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA,
                 GL_UNSIGNED_BYTE, pixels.data());

    out.texture = tex;
    out.w = w;
    out.h = h;
    return true;
#endif
  }

#if defined(DUOROU_HAS_FREETYPE) && DUOROU_HAS_FREETYPE
  bool ensure_face() {
    if (face_) {
      return true;
    }
    if (!ft_) {
      if (FT_Init_FreeType(&ft_) != 0) {
        ft_ = nullptr;
        return false;
      }
    }

    if (font_path_.empty()) {
      font_path_ = duorou_readable_font_path();
    }
    if (font_path_.empty()) {
      return false;
    }

    if (FT_New_Face(ft_, font_path_.c_str(), 0, &face_) != 0) {
      face_ = nullptr;
      return false;
    }
    return true;
  }

  FT_Library ft_{};
  FT_Face face_{};
  std::string font_path_{};
#endif

  std::unordered_map<std::string, GLTextEntry> cache_;
};

struct GLRenderer final : Renderer {
  int vw{};
  int vh{};
  GLTextCache *text_cache{};

  void draw_rect(const DrawRect &r) override {
    (void)vw;
    (void)vh;
    glDisable(GL_TEXTURE_2D);
    glColor4ub(r.fill.r, r.fill.g, r.fill.b, r.fill.a);
    const float x0 = r.rect.x;
    const float y0 = r.rect.y;
    const float x1 = r.rect.x + r.rect.w;
    const float y1 = r.rect.y + r.rect.h;
    glBegin(GL_TRIANGLES);
    glVertex2f(x0, y0);
    glVertex2f(x1, y0);
    glVertex2f(x0, y1);
    glVertex2f(x0, y1);
    glVertex2f(x1, y0);
    glVertex2f(x1, y1);
    glEnd();
  }

  void draw_text(const DrawText &t) override {
    if (!text_cache) {
      return;
    }
    const auto *entry = text_cache->get(t.text, t.font_px);
    if (!entry || entry->texture == 0 || entry->w <= 0 || entry->h <= 0) {
      return;
    }

    const float tex_w = static_cast<float>(entry->w);
    const float tex_h = static_cast<float>(entry->h);
    const float target_w = t.rect.w;
    const float target_h = t.rect.h;
    const float scale = std::min(target_w / tex_w, target_h / tex_h);
    if (!(scale > 0.0f)) {
      return;
    }

    const float draw_w = tex_w * scale;
    const float draw_h = tex_h * scale;
    const float x = t.rect.x + (t.rect.w - draw_w) * 0.5f;
    const float y = t.rect.y + (t.rect.h - draw_h) * 0.5f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, entry->texture);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4ub(t.color.r, t.color.g, t.color.b, t.color.a);

    const float x0 = x;
    const float y0 = y;
    const float x1 = x + draw_w;
    const float y1 = y + draw_h;

    glBegin(GL_TRIANGLES);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(x0, y0);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(x1, y0);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(x0, y1);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(x0, y1);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(x1, y0);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x1, y1);
    glEnd();
  }
};

struct InputCtx {
  ViewInstance *app{};
  int pointer_id{1};
};

static void cursor_pos_cb(GLFWwindow *win, double xpos, double ypos) {
  auto *ctx = static_cast<InputCtx *>(glfwGetWindowUserPointer(win));
  if (!ctx || !ctx->app) {
    return;
  }
  int ww = 0;
  int wh = 0;
  glfwGetWindowSize(win, &ww, &wh);
  int fbw = 0;
  int fbh = 0;
  glfwGetFramebufferSize(win, &fbw, &fbh);

  const double sx =
      ww > 0 ? static_cast<double>(fbw) / static_cast<double>(ww) : 1.0;
  const double sy =
      wh > 0 ? static_cast<double>(fbh) / static_cast<double>(wh) : 1.0;

  const float x = static_cast<float>(xpos * sx);
  const float y = static_cast<float>(ypos * sy);
  ctx->app->dispatch_pointer_move(ctx->pointer_id, x, y);
}

static void mouse_button_cb(GLFWwindow *win, int button, int action, int mods) {
  (void)mods;
  if (button != GLFW_MOUSE_BUTTON_LEFT) {
    return;
  }

  auto *ctx = static_cast<InputCtx *>(glfwGetWindowUserPointer(win));
  if (!ctx || !ctx->app) {
    return;
  }

  double xpos = 0.0;
  double ypos = 0.0;
  glfwGetCursorPos(win, &xpos, &ypos);

  int ww = 0;
  int wh = 0;
  glfwGetWindowSize(win, &ww, &wh);
  int fbw = 0;
  int fbh = 0;
  glfwGetFramebufferSize(win, &fbw, &fbh);
  const double sx =
      ww > 0 ? static_cast<double>(fbw) / static_cast<double>(ww) : 1.0;
  const double sy =
      wh > 0 ? static_cast<double>(fbh) / static_cast<double>(wh) : 1.0;

  const float x = static_cast<float>(xpos * sx);
  const float y = static_cast<float>(ypos * sy);

  if (action == GLFW_PRESS) {
    ctx->app->dispatch_pointer_down(ctx->pointer_id, x, y);
  } else if (action == GLFW_RELEASE) {
    ctx->app->dispatch_pointer_up(ctx->pointer_id, x, y);
  }
}

int main() {
  if (!glfwInit()) {
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

  GLFWwindow *win =
      glfwCreateWindow(800, 600, "duorou_gpu_demo (OpenGL)", nullptr, nullptr);
  if (!win) {
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(win);
  glfwSwapInterval(1);

  auto count = state<std::int64_t>(0);
  auto pressed = state<bool>(false);

  ViewInstance app{[&]() {
    return view("Column")
        .prop("padding", 24)
        .prop("spacing", 12)
        .prop("cross_align", "start")
        .children({
            view("Text")
                .prop("value",
                      std::string{"Count: "} + std::to_string(count.get()))
                .build(),
            view("Button")
                .key("inc")
                .prop("title", "Inc")
                .prop("pressed", pressed.get())
                .event("pointer_down", on_pointer_down([&]() {
                         pressed.set(true);
                         capture_pointer();
                       }))
                .event("pointer_up", on_pointer_up([&]() {
                         pressed.set(false);
                         release_pointer();
                         count.set(count.get() + 1);
                       }))
                .build(),
        })
        .build();
  }};

  InputCtx input;
  input.app = &app;
  glfwSetWindowUserPointer(win, &input);
  glfwSetCursorPosCallback(win, cursor_pos_cb);
  glfwSetMouseButtonCallback(win, mouse_button_cb);

  {
    GLTextCache text_cache;

    int last_fbw = 0;
    int last_fbh = 0;

    while (!glfwWindowShouldClose(win)) {
      glfwPollEvents();

      int fbw = 0;
      int fbh = 0;
      glfwGetFramebufferSize(win, &fbw, &fbh);
      fbw = std::max(1, fbw);
      fbh = std::max(1, fbh);

      if (fbw != last_fbw || fbh != last_fbh) {
        app.set_viewport(
            SizeF{static_cast<float>(fbw), static_cast<float>(fbh)});
        last_fbw = fbw;
        last_fbh = fbh;
      }

      app.update();

      gl_set_ortho(fbw, fbh);
      glDisable(GL_DEPTH_TEST);
      glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      GLRenderer renderer;
      renderer.vw = fbw;
      renderer.vh = fbh;
      renderer.text_cache = &text_cache;
      render_with(renderer, app.render_ops());

      glfwSwapBuffers(win);
    }
  }

  glfwDestroyWindow(win);
  glfwTerminate();
  return 0;
}

#endif
