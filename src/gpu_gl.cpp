#include <duorou/ui/runtime.hpp>

#if !defined(_WIN32) && !defined(__linux__)
int main() { return 0; }
#else

#include <GLFW/glfw3.h>

#if !defined(GL_CLAMP_TO_EDGE)
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#include <algorithm>
#include <cmath>
#include <cstddef>
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

#if defined(_WIN32)
#define DUOROU_GL_APIENTRY __stdcall
#else
#define DUOROU_GL_APIENTRY
#endif

#if !defined(GL_ARRAY_BUFFER)
#define GL_ARRAY_BUFFER 0x8892
#endif
#if !defined(GL_STREAM_DRAW)
#define GL_STREAM_DRAW 0x88E0
#endif
#if !defined(GL_FRAGMENT_SHADER)
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#if !defined(GL_VERTEX_SHADER)
#define GL_VERTEX_SHADER 0x8B31
#endif
#if !defined(GL_COMPILE_STATUS)
#define GL_COMPILE_STATUS 0x8B81
#endif
#if !defined(GL_LINK_STATUS)
#define GL_LINK_STATUS 0x8B82
#endif
#if !defined(GL_INFO_LOG_LENGTH)
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#if !defined(GL_TEXTURE0)
#define GL_TEXTURE0 0x84C0
#endif
#if !defined(GL_TEXTURE_WRAP_S)
#define GL_TEXTURE_WRAP_S 0x2802
#endif
#if !defined(GL_TEXTURE_WRAP_T)
#define GL_TEXTURE_WRAP_T 0x2803
#endif
#if !defined(GL_TEXTURE_MIN_FILTER)
#define GL_TEXTURE_MIN_FILTER 0x2801
#endif
#if !defined(GL_TEXTURE_MAG_FILTER)
#define GL_TEXTURE_MAG_FILTER 0x2800
#endif
#if !defined(GL_ACTIVE_TEXTURE)
#define GL_ACTIVE_TEXTURE 0x84E0
#endif

struct GLProcs {
  using PFNGLGENBUFFERSPROC = void(DUOROU_GL_APIENTRY *)(GLsizei, GLuint *);
  using PFNGLBINDBUFFERPROC = void(DUOROU_GL_APIENTRY *)(GLenum, GLuint);
  using PFNGLBUFFERDATAPROC =
      void(DUOROU_GL_APIENTRY *)(GLenum, std::ptrdiff_t, const void *, GLenum);
  using PFNGLBUFFERSUBDATAPROC =
      void(DUOROU_GL_APIENTRY *)(GLenum, std::ptrdiff_t, std::ptrdiff_t,
                                const void *);
  using PFNGLDELETEBUFFERSPROC = void(DUOROU_GL_APIENTRY *)(GLsizei,
                                                           const GLuint *);

  using PFNGLCREATESHADERPROC = GLuint(DUOROU_GL_APIENTRY *)(GLenum);
  using PFNGLSHADERSOURCEPROC =
      void(DUOROU_GL_APIENTRY *)(GLuint, GLsizei, const char *const *,
                                const GLint *);
  using PFNGLCOMPILESHADERPROC = void(DUOROU_GL_APIENTRY *)(GLuint);
  using PFNGLGETSHADERIVPROC = void(DUOROU_GL_APIENTRY *)(GLuint, GLenum,
                                                          GLint *);
  using PFNGLGETSHADERINFOLOGPROC =
      void(DUOROU_GL_APIENTRY *)(GLuint, GLsizei, GLsizei *, char *);
  using PFNGLDELETESHADERPROC = void(DUOROU_GL_APIENTRY *)(GLuint);

  using PFNGLCREATEPROGRAMPROC = GLuint(DUOROU_GL_APIENTRY *)();
  using PFNGLATTACHSHADERPROC = void(DUOROU_GL_APIENTRY *)(GLuint, GLuint);
  using PFNGLLINKPROGRAMPROC = void(DUOROU_GL_APIENTRY *)(GLuint);
  using PFNGLGETPROGRAMIVPROC = void(DUOROU_GL_APIENTRY *)(GLuint, GLenum,
                                                           GLint *);
  using PFNGLGETPROGRAMINFOLOGPROC =
      void(DUOROU_GL_APIENTRY *)(GLuint, GLsizei, GLsizei *, char *);
  using PFNGLUSEPROGRAMPROC = void(DUOROU_GL_APIENTRY *)(GLuint);
  using PFNGLDELETEPROGRAMPROC = void(DUOROU_GL_APIENTRY *)(GLuint);

  using PFNGLGETUNIFORMLOCATIONPROC =
      GLint(DUOROU_GL_APIENTRY *)(GLuint, const char *);
  using PFNGLUNIFORMMATRIX4FVPROC =
      void(DUOROU_GL_APIENTRY *)(GLint, GLsizei, GLboolean, const GLfloat *);
  using PFNGLUNIFORM1IPROC = void(DUOROU_GL_APIENTRY *)(GLint, GLint);

  using PFNGLGETATTRIBLOCATIONPROC =
      GLint(DUOROU_GL_APIENTRY *)(GLuint, const char *);
  using PFNGLENABLEVERTEXATTRIBARRAYPROC =
      void(DUOROU_GL_APIENTRY *)(GLuint);
  using PFNGLDISABLEVERTEXATTRIBARRAYPROC =
      void(DUOROU_GL_APIENTRY *)(GLuint);
  using PFNGLVERTEXATTRIBPOINTERPROC =
      void(DUOROU_GL_APIENTRY *)(GLuint, GLint, GLenum, GLboolean, GLsizei,
                                const void *);

  using PFNGLACTIVETEXTUREPROC = void(DUOROU_GL_APIENTRY *)(GLenum);

  using PFNGLGENVERTEXARRAYSPROC = void(DUOROU_GL_APIENTRY *)(GLsizei, GLuint *);
  using PFNGLBINDVERTEXARRAYPROC = void(DUOROU_GL_APIENTRY *)(GLuint);
  using PFNGLDELETEVERTEXARRAYSPROC = void(DUOROU_GL_APIENTRY *)(GLsizei,
                                                                const GLuint *);

  PFNGLGENBUFFERSPROC GenBuffers{};
  PFNGLBINDBUFFERPROC BindBuffer{};
  PFNGLBUFFERDATAPROC BufferData{};
  PFNGLBUFFERSUBDATAPROC BufferSubData{};
  PFNGLDELETEBUFFERSPROC DeleteBuffers{};

  PFNGLCREATESHADERPROC CreateShader{};
  PFNGLSHADERSOURCEPROC ShaderSource{};
  PFNGLCOMPILESHADERPROC CompileShader{};
  PFNGLGETSHADERIVPROC GetShaderiv{};
  PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog{};
  PFNGLDELETESHADERPROC DeleteShader{};

  PFNGLCREATEPROGRAMPROC CreateProgram{};
  PFNGLATTACHSHADERPROC AttachShader{};
  PFNGLLINKPROGRAMPROC LinkProgram{};
  PFNGLGETPROGRAMIVPROC GetProgramiv{};
  PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog{};
  PFNGLUSEPROGRAMPROC UseProgram{};
  PFNGLDELETEPROGRAMPROC DeleteProgram{};

  PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation{};
  PFNGLUNIFORMMATRIX4FVPROC UniformMatrix4fv{};
  PFNGLUNIFORM1IPROC Uniform1i{};

  PFNGLGETATTRIBLOCATIONPROC GetAttribLocation{};
  PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray{};
  PFNGLDISABLEVERTEXATTRIBARRAYPROC DisableVertexAttribArray{};
  PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer{};

  PFNGLACTIVETEXTUREPROC ActiveTexture{};

  PFNGLGENVERTEXARRAYSPROC GenVertexArrays{};
  PFNGLBINDVERTEXARRAYPROC BindVertexArray{};
  PFNGLDELETEVERTEXARRAYSPROC DeleteVertexArrays{};
  bool has_vao{};
};

template <class Fn>
static bool duorou_gl_load_fn(Fn &out, const char *name) {
  out = reinterpret_cast<Fn>(glfwGetProcAddress(name));
  return out != nullptr;
}

static bool duorou_gl_load(GLProcs &gl) {
  bool ok = true;
  ok = ok && duorou_gl_load_fn(gl.GenBuffers, "glGenBuffers");
  ok = ok && duorou_gl_load_fn(gl.BindBuffer, "glBindBuffer");
  ok = ok && duorou_gl_load_fn(gl.BufferData, "glBufferData");
  ok = ok && duorou_gl_load_fn(gl.BufferSubData, "glBufferSubData");
  ok = ok && duorou_gl_load_fn(gl.DeleteBuffers, "glDeleteBuffers");

  ok = ok && duorou_gl_load_fn(gl.CreateShader, "glCreateShader");
  ok = ok && duorou_gl_load_fn(gl.ShaderSource, "glShaderSource");
  ok = ok && duorou_gl_load_fn(gl.CompileShader, "glCompileShader");
  ok = ok && duorou_gl_load_fn(gl.GetShaderiv, "glGetShaderiv");
  ok = ok && duorou_gl_load_fn(gl.GetShaderInfoLog, "glGetShaderInfoLog");
  ok = ok && duorou_gl_load_fn(gl.DeleteShader, "glDeleteShader");

  ok = ok && duorou_gl_load_fn(gl.CreateProgram, "glCreateProgram");
  ok = ok && duorou_gl_load_fn(gl.AttachShader, "glAttachShader");
  ok = ok && duorou_gl_load_fn(gl.LinkProgram, "glLinkProgram");
  ok = ok && duorou_gl_load_fn(gl.GetProgramiv, "glGetProgramiv");
  ok = ok && duorou_gl_load_fn(gl.GetProgramInfoLog, "glGetProgramInfoLog");
  ok = ok && duorou_gl_load_fn(gl.UseProgram, "glUseProgram");
  ok = ok && duorou_gl_load_fn(gl.DeleteProgram, "glDeleteProgram");

  ok = ok && duorou_gl_load_fn(gl.GetUniformLocation, "glGetUniformLocation");
  ok = ok && duorou_gl_load_fn(gl.UniformMatrix4fv, "glUniformMatrix4fv");
  ok = ok && duorou_gl_load_fn(gl.Uniform1i, "glUniform1i");

  ok = ok && duorou_gl_load_fn(gl.GetAttribLocation, "glGetAttribLocation");
  ok = ok &&
       duorou_gl_load_fn(gl.EnableVertexAttribArray, "glEnableVertexAttribArray");
  ok = ok && duorou_gl_load_fn(gl.DisableVertexAttribArray,
                               "glDisableVertexAttribArray");
  ok = ok && duorou_gl_load_fn(gl.VertexAttribPointer, "glVertexAttribPointer");

  ok = ok && duorou_gl_load_fn(gl.ActiveTexture, "glActiveTexture");

  gl.has_vao = duorou_gl_load_fn(gl.GenVertexArrays, "glGenVertexArrays") &&
               duorou_gl_load_fn(gl.BindVertexArray, "glBindVertexArray") &&
               duorou_gl_load_fn(gl.DeleteVertexArrays, "glDeleteVertexArrays");

  return ok;
}

static void duorou_ortho_px(int w, int h, float out16[16]) {
  const float l = 0.0f;
  const float r = static_cast<float>(w);
  const float t = 0.0f;
  const float b = static_cast<float>(h);

  const float rl = r - l;
  const float tb = t - b;

  const float m00 = 2.0f / rl;
  const float m11 = 2.0f / tb;
  const float m30 = -(r + l) / rl;
  const float m31 = -(t + b) / tb;

  out16[0] = m00;
  out16[1] = 0.0f;
  out16[2] = 0.0f;
  out16[3] = 0.0f;

  out16[4] = 0.0f;
  out16[5] = m11;
  out16[6] = 0.0f;
  out16[7] = 0.0f;

  out16[8] = 0.0f;
  out16[9] = 0.0f;
  out16[10] = -1.0f;
  out16[11] = 0.0f;

  out16[12] = m30;
  out16[13] = m31;
  out16[14] = 0.0f;
  out16[15] = 1.0f;
}

static GLuint duorou_compile_shader(GLProcs &gl, GLenum type,
                                   const char *src) {
  const GLuint sh = gl.CreateShader(type);
  if (sh == 0) {
    return 0;
  }
  gl.ShaderSource(sh, 1, &src, nullptr);
  gl.CompileShader(sh);
  GLint ok = 0;
  gl.GetShaderiv(sh, GL_COMPILE_STATUS, &ok);
  if (ok) {
    return sh;
  }
  gl.DeleteShader(sh);
  return 0;
}

static GLuint duorou_link_program(GLProcs &gl, GLuint vs, GLuint fs) {
  const GLuint prog = gl.CreateProgram();
  if (prog == 0) {
    return 0;
  }
  gl.AttachShader(prog, vs);
  gl.AttachShader(prog, fs);
  gl.LinkProgram(prog);
  GLint ok = 0;
  gl.GetProgramiv(prog, GL_LINK_STATUS, &ok);
  if (ok) {
    return prog;
  }
  gl.DeleteProgram(prog);
  return 0;
}

static GLuint duorou_make_demo_rgba_texture() {
  constexpr int w = 64;
  constexpr int h = 64;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(w) *
                                       static_cast<std::size_t>(h) * 4,
                                   0);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const int i = (y * w + x) * 4;
      const bool chk = ((x / 8) ^ (y / 8)) & 1;
      const std::uint8_t r =
          static_cast<std::uint8_t>(chk ? 240 : 40);
      const std::uint8_t g = static_cast<std::uint8_t>(x * 4);
      const std::uint8_t b = static_cast<std::uint8_t>(y * 4);
      pixels[static_cast<std::size_t>(i) + 0] = r;
      pixels[static_cast<std::size_t>(i) + 1] = g;
      pixels[static_cast<std::size_t>(i) + 2] = b;
      pixels[static_cast<std::size_t>(i) + 3] = 255;
    }
  }

  GLuint tex = 0;
  glGenTextures(1, &tex);
  if (tex == 0) {
    return 0;
  }
  glBindTexture(GL_TEXTURE_2D, tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               pixels.data());
  return tex;
}

struct GLTextQuad {
  float x0{};
  float y0{};
  float x1{};
  float y1{};
  float u0{};
  float v0{};
  float u1{};
  float v1{};
  GLuint texture{};
};

struct GLTextEntry {
  int w{};
  int h{};
  std::vector<GLTextQuad> quads;
  std::vector<float> caret_x;
};

static std::string duorou_readable_font_path() {
  const char *candidates[] = {
#if defined(_WIN32)
      "C:/Windows/Fonts/msyh.ttc",
      "C:/Windows/Fonts/msyh.ttf",
      "C:/Windows/Fonts/msyhbd.ttc",
      "C:/Windows/Fonts/simsun.ttc",
      "C:/Windows/Fonts/simhei.ttf",
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
    for (auto &p : pages_) {
      if (p.texture != 0) {
        glDeleteTextures(1, &p.texture);
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
    if (!build_entry(text, font_px, e)) {
      return nullptr;
    }

    auto [it, _] = cache_.emplace(key, e);
    return &it->second;
  }

private:
  struct AtlasPage {
    GLuint texture{};
    int w{};
    int h{};
    int pen_x{1};
    int pen_y{1};
    int row_h{};
  };

  struct CachedGlyph {
    GLuint texture{};
    int advance{};
    int bitmap_left{};
    int bitmap_top{};
    int w{};
    int h{};
    float u0{};
    float v0{};
    float u1{};
    float v1{};
  };

  static std::string make_key(std::string_view text, float font_px) {
    const auto scaled = static_cast<int>(std::lround(font_px * 100.0f));
    std::string k;
    k.reserve(16 + text.size());
    k.append(std::to_string(scaled));
    k.push_back(':');
    k.append(text.data(), text.size());
    return k;
  }

  static std::uint64_t make_glyph_key(std::uint32_t glyph_index, int px) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(px)) << 32) |
           static_cast<std::uint64_t>(glyph_index);
  }

  bool alloc_in_page(AtlasPage &p, int gw, int gh, int &out_x, int &out_y) {
    if (gw <= 0 || gh <= 0) {
      out_x = 0;
      out_y = 0;
      return true;
    }

    const int pad = 1;
    if (p.pen_x + gw + pad > p.w) {
      p.pen_x = pad;
      p.pen_y += p.row_h + pad;
      p.row_h = 0;
    }
    if (p.pen_y + gh + pad > p.h) {
      return false;
    }

    out_x = p.pen_x;
    out_y = p.pen_y;
    p.pen_x += gw + pad;
    p.row_h = std::max(p.row_h, gh);
    return true;
  }

  AtlasPage *ensure_page(int gw, int gh, int &out_x, int &out_y) {
    for (auto &p : pages_) {
      if (alloc_in_page(p, gw, gh, out_x, out_y)) {
        return &p;
      }
    }

    AtlasPage p{};
    p.w = 1024;
    p.h = 1024;
    glGenTextures(1, &p.texture);
    if (p.texture == 0) {
      return nullptr;
    }

    glBindTexture(GL_TEXTURE_2D, p.texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    std::vector<std::uint8_t> zeros(
        static_cast<std::size_t>(p.w) * static_cast<std::size_t>(p.h), 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, p.w, p.h, 0, GL_ALPHA,
                 GL_UNSIGNED_BYTE, zeros.data());

    pages_.push_back(p);
    if (!alloc_in_page(pages_.back(), gw, gh, out_x, out_y)) {
      return nullptr;
    }
    return &pages_.back();
  }

  const CachedGlyph *get_glyph(std::uint32_t glyph_index, int px) {
#if !(defined(DUOROU_HAS_FREETYPE) && DUOROU_HAS_FREETYPE)
    (void)glyph_index;
    (void)px;
    return nullptr;
#else
    if (!ensure_face()) {
      return nullptr;
    }

    px = std::max(1, px);
    if (px != last_px_) {
      if (FT_Set_Pixel_Sizes(face_, 0, static_cast<FT_UInt>(px)) != 0) {
        return nullptr;
      }
      last_px_ = px;
    }

    const auto key = make_glyph_key(glyph_index, px);
    if (const auto it = glyphs_.find(key); it != glyphs_.end()) {
      return &it->second;
    }

    if (FT_Load_Glyph(face_, static_cast<FT_UInt>(glyph_index),
                      FT_LOAD_DEFAULT) != 0) {
      return nullptr;
    }
    if (FT_Render_Glyph(face_->glyph, FT_RENDER_MODE_NORMAL) != 0) {
      return nullptr;
    }

    const int gw = static_cast<int>(face_->glyph->bitmap.width);
    const int gh = static_cast<int>(face_->glyph->bitmap.rows);

    int atlas_x = 0;
    int atlas_y = 0;
    AtlasPage *page = ensure_page(gw, gh, atlas_x, atlas_y);
    if (!page) {
      return nullptr;
    }

    if (gw > 0 && gh > 0) {
      std::vector<std::uint8_t> buf(static_cast<std::size_t>(gw) *
                                        static_cast<std::size_t>(gh),
                                    0);
      for (int y = 0; y < gh; ++y) {
        const auto *src =
            reinterpret_cast<const std::uint8_t *>(face_->glyph->bitmap.buffer) +
            static_cast<std::size_t>(y) *
                static_cast<std::size_t>(face_->glyph->bitmap.pitch);
        auto *dst = buf.data() + static_cast<std::size_t>(y) *
                                     static_cast<std::size_t>(gw);
        std::memcpy(dst, src, static_cast<std::size_t>(gw));
      }

      glBindTexture(GL_TEXTURE_2D, page->texture);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexSubImage2D(GL_TEXTURE_2D, 0, atlas_x, atlas_y, gw, gh, GL_ALPHA,
                      GL_UNSIGNED_BYTE, buf.data());
    }

    CachedGlyph cg{};
    cg.texture = page->texture;
    cg.advance = static_cast<int>(face_->glyph->advance.x >> 6);
    cg.bitmap_left = face_->glyph->bitmap_left;
    cg.bitmap_top = face_->glyph->bitmap_top;
    cg.w = gw;
    cg.h = gh;
    cg.u0 = page->w > 0 ? static_cast<float>(atlas_x) / page->w : 0.0f;
    cg.v0 = page->h > 0 ? static_cast<float>(atlas_y) / page->h : 0.0f;
    cg.u1 = page->w > 0 ? static_cast<float>(atlas_x + gw) / page->w : 0.0f;
    cg.v1 = page->h > 0 ? static_cast<float>(atlas_y + gh) / page->h : 0.0f;

    auto [it, _] = glyphs_.emplace(key, cg);
    return &it->second;
#endif
  }

  bool build_entry(std::string_view text, float font_px, GLTextEntry &out) {
#if !(defined(DUOROU_HAS_FREETYPE) && DUOROU_HAS_FREETYPE)
    (void)text;
    (void)font_px;
    (void)out;
    return false;
#else
    if (!ensure_face()) {
      return false;
    }

    const int px = std::max(1, static_cast<int>(std::lround(font_px)));

    struct ShapedChar {
      std::optional<CachedGlyph> glyph;
      int advance{};
    };

    std::vector<ShapedChar> shaped;
    shaped.reserve(text.size());

    int pen_x = 0;
    int max_top = 0;
    int max_bottom = 0;

    std::size_t i = 0;
    while (i < text.size()) {
      std::uint32_t cp = 0;
      if (!duorou_utf8_next(text, i, cp)) {
        break;
      }

      auto gi = static_cast<std::uint32_t>(FT_Get_Char_Index(face_, (FT_ULong)cp));
      if (gi == 0) {
        gi = static_cast<std::uint32_t>(FT_Get_Char_Index(face_, (FT_ULong)'?'));
      }
      const auto *g = gi != 0 ? get_glyph(gi, px) : nullptr;
      if (!g) {
        const int adv = std::max(1, px / 2);
        shaped.push_back(ShapedChar{std::nullopt, adv});
        pen_x += adv;
        continue;
      }
      const int adv = std::max(0, g->advance);
      shaped.push_back(ShapedChar{*g, adv});

      max_top = std::max(max_top, g->bitmap_top);
      const int bottom = g->h - g->bitmap_top;
      max_bottom = std::max(max_bottom, bottom);

      pen_x += adv;
    }

    if (shaped.empty()) {
      return false;
    }

    const int pad = 2;
    out.w = std::max(1, pen_x + pad * 2);
    out.h = std::max(1, max_top + max_bottom + pad * 2);
    out.quads.clear();
    out.caret_x.clear();
    out.caret_x.reserve(shaped.size() + 1);

    const int baseline = pad + max_top;
    int x = pad;
    out.caret_x.push_back(static_cast<float>(x));
    for (const auto &ch : shaped) {
      if (ch.glyph) {
        const auto &g = *ch.glyph;
        const int dst_x0 = x + g.bitmap_left;
        const int dst_y0 = baseline - g.bitmap_top;
        if (g.w > 0 && g.h > 0) {
          GLTextQuad q{};
          q.x0 = static_cast<float>(dst_x0);
          q.y0 = static_cast<float>(dst_y0);
          q.x1 = static_cast<float>(dst_x0 + g.w);
          q.y1 = static_cast<float>(dst_y0 + g.h);
          q.u0 = g.u0;
          q.v0 = g.v0;
          q.u1 = g.u1;
          q.v1 = g.v1;
          q.texture = g.texture;
          out.quads.push_back(q);
        }
      }
      x += ch.advance;
      out.caret_x.push_back(static_cast<float>(x));
    }

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

    FT_Face base{};
    if (FT_New_Face(ft_, font_path_.c_str(), 0, &base) != 0) {
      return false;
    }

    auto try_prepare = [&](FT_Face f) {
      if (!f) {
        return false;
      }
      if (FT_Select_Charmap(f, FT_ENCODING_UNICODE) != 0) {
        if (f->num_charmaps > 0 && f->charmaps) {
          if (FT_Set_Charmap(f, f->charmaps[0]) != 0) {
            return false;
          }
        } else {
          return false;
        }
      }
      return true;
    };

    if (!try_prepare(base)) {
      FT_Done_Face(base);
      return false;
    }

    const auto test_cp = static_cast<FT_ULong>(0x4E2Du);
    const auto base_gi = FT_Get_Char_Index(base, test_cp);

    if (base_gi != 0 || base->num_faces <= 1) {
      face_ = base;
      return true;
    }

    const FT_Long nfaces = base->num_faces;
    FT_Done_Face(base);

    for (FT_Long idx = 1; idx < nfaces; ++idx) {
      FT_Face f{};
      if (FT_New_Face(ft_, font_path_.c_str(), idx, &f) != 0) {
        continue;
      }
      if (!try_prepare(f)) {
        FT_Done_Face(f);
        continue;
      }
      if (FT_Get_Char_Index(f, test_cp) != 0) {
        face_ = f;
        return true;
      }
      FT_Done_Face(f);
    }

    if (FT_New_Face(ft_, font_path_.c_str(), 0, &face_) != 0) {
      face_ = nullptr;
      return false;
    }
    if (!try_prepare(face_)) {
      FT_Done_Face(face_);
      face_ = nullptr;
      return false;
    }
    return true;
  }

  FT_Library ft_{};
  FT_Face face_{};
  std::string font_path_{};
  int last_px_{};
#endif

  std::unordered_map<std::string, GLTextEntry> cache_;
  std::unordered_map<std::uint64_t, CachedGlyph> glyphs_;
  std::vector<AtlasPage> pages_;
};

struct GLRenderer final {
  GLProcs *gl{};
  int vw{};
  int vh{};

  GLuint program{};
  GLint u_mvp{-1};
  GLint u_tex{-1};
  GLint u_tex_mode{-1};
  GLint a_pos{-1};
  GLint a_uv{-1};
  GLint a_color{-1};

  GLuint vbo{};
  GLuint vao{};
  bool has_vao{};

  GLuint bound_tex{};
  int use_tex{};

  ~GLRenderer() {
    if (gl) {
      if (vbo != 0) {
        gl->DeleteBuffers(1, &vbo);
      }
      if (has_vao && vao != 0) {
        gl->DeleteVertexArrays(1, &vao);
      }
      if (program != 0) {
        gl->DeleteProgram(program);
      }
    }
  }

  bool init(GLProcs &p) {
    gl = &p;
    has_vao = gl->has_vao;

    const char *vs_src =
        "#version 120\n"
        "attribute vec2 aPos;\n"
        "attribute vec2 aUV;\n"
        "attribute vec4 aColor;\n"
        "uniform mat4 uMVP;\n"
        "varying vec2 vUV;\n"
        "varying vec4 vColor;\n"
        "void main() {\n"
        "  vUV = aUV;\n"
        "  vColor = aColor;\n"
        "  gl_Position = uMVP * vec4(aPos, 0.0, 1.0);\n"
        "}\n";

    const char *fs_src =
        "#version 120\n"
        "uniform sampler2D uTex;\n"
        "uniform int uTexMode;\n"
        "varying vec2 vUV;\n"
        "varying vec4 vColor;\n"
        "void main() {\n"
        "  if (uTexMode == 0) {\n"
        "    gl_FragColor = vColor;\n"
        "  } else if (uTexMode == 1) {\n"
        "    float a = texture2D(uTex, vUV).a;\n"
        "    gl_FragColor = vec4(vColor.rgb, vColor.a * a);\n"
        "  } else {\n"
        "    vec4 t = texture2D(uTex, vUV);\n"
        "    gl_FragColor = vec4(vColor.rgb * t.rgb, vColor.a * t.a);\n"
        "  }\n"
        "}\n";

    const GLuint vs = duorou_compile_shader(*gl, GL_VERTEX_SHADER, vs_src);
    const GLuint fs = duorou_compile_shader(*gl, GL_FRAGMENT_SHADER, fs_src);
    if (vs == 0 || fs == 0) {
      if (vs != 0) {
        gl->DeleteShader(vs);
      }
      if (fs != 0) {
        gl->DeleteShader(fs);
      }
      return false;
    }

    program = duorou_link_program(*gl, vs, fs);
    gl->DeleteShader(vs);
    gl->DeleteShader(fs);
    if (program == 0) {
      return false;
    }

    u_mvp = gl->GetUniformLocation(program, "uMVP");
    u_tex = gl->GetUniformLocation(program, "uTex");
    u_tex_mode = gl->GetUniformLocation(program, "uTexMode");
    a_pos = gl->GetAttribLocation(program, "aPos");
    a_uv = gl->GetAttribLocation(program, "aUV");
    a_color = gl->GetAttribLocation(program, "aColor");
    if (u_mvp < 0 || u_tex < 0 || u_tex_mode < 0 || a_pos < 0 || a_uv < 0 ||
        a_color < 0) {
      return false;
    }

    gl->GenBuffers(1, &vbo);
    if (vbo == 0) {
      return false;
    }

    if (has_vao) {
      gl->GenVertexArrays(1, &vao);
      if (vao == 0) {
        has_vao = false;
      }
    }

    if (has_vao) {
      gl->BindVertexArray(vao);
      gl->BindBuffer(GL_ARRAY_BUFFER, vbo);
      setup_attribs();
      gl->BindVertexArray(0);
    }

    return true;
  }

  void begin_frame(int w, int h) {
    vw = w;
    vh = h;
    glViewport(0, 0, w, h);

    gl->UseProgram(program);
    float mvp[16]{};
    duorou_ortho_px(w, h, mvp);
    gl->UniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp);
    gl->ActiveTexture(GL_TEXTURE0);
    gl->Uniform1i(u_tex, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);

    bound_tex = 0;
    use_tex = 0;
    gl->Uniform1i(u_tex_mode, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    apply_scissor(RectF{0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)});

    if (has_vao) {
      gl->BindVertexArray(vao);
    } else {
      gl->BindBuffer(GL_ARRAY_BUFFER, vbo);
      setup_attribs();
    }
  }

  void end_frame() {
    if (has_vao) {
      gl->BindVertexArray(0);
    } else {
      gl->DisableVertexAttribArray(static_cast<GLuint>(a_pos));
      gl->DisableVertexAttribArray(static_cast<GLuint>(a_uv));
      gl->DisableVertexAttribArray(static_cast<GLuint>(a_color));
      gl->BindBuffer(GL_ARRAY_BUFFER, 0);
    }
    glDisable(GL_SCISSOR_TEST);
    gl->UseProgram(0);
  }

  void draw_tree(const RenderTree &tree) {
    if (!gl) {
      return;
    }
    if (tree.vertices.empty() || tree.batches.empty()) {
      return;
    }

    gl->BindBuffer(GL_ARRAY_BUFFER, vbo);
    const auto bytes =
        static_cast<std::ptrdiff_t>(tree.vertices.size() * sizeof(RenderVertex));
    gl->BufferData(GL_ARRAY_BUFFER, bytes, tree.vertices.data(), GL_STREAM_DRAW);

    RectF last_scissor{};
    bool has_last_scissor = false;

    GLuint last_tex = bound_tex;
    int last_use_tex = use_tex;

    for (const auto &b : tree.batches) {
      if (b.count == 0) {
        continue;
      }

      if (!has_last_scissor || b.scissor.x != last_scissor.x ||
          b.scissor.y != last_scissor.y || b.scissor.w != last_scissor.w ||
          b.scissor.h != last_scissor.h) {
        apply_scissor(b.scissor);
        last_scissor = b.scissor;
        has_last_scissor = true;
      }

      if (b.pipeline == RenderPipeline::Color) {
        if (last_use_tex != 0) {
          gl->Uniform1i(u_tex_mode, 0);
          last_use_tex = 0;
        }
        if (last_tex != 0) {
          glBindTexture(GL_TEXTURE_2D, 0);
          last_tex = 0;
        }
      } else if (b.pipeline == RenderPipeline::Text) {
        const auto tex = static_cast<GLuint>(b.texture);
        if (last_use_tex != 1) {
          gl->Uniform1i(u_tex_mode, 1);
          last_use_tex = 1;
        }
        if (tex != last_tex) {
          glBindTexture(GL_TEXTURE_2D, tex);
          last_tex = tex;
        }
      } else {
        const auto tex = static_cast<GLuint>(b.texture);
        if (last_use_tex != 2) {
          gl->Uniform1i(u_tex_mode, 2);
          last_use_tex = 2;
        }
        if (tex != last_tex) {
          glBindTexture(GL_TEXTURE_2D, tex);
          last_tex = tex;
        }
      }

      glDrawArrays(GL_TRIANGLES, static_cast<GLint>(b.first),
                   static_cast<GLsizei>(b.count));
    }

    use_tex = last_use_tex;
    bound_tex = last_tex;
  }

private:
  void apply_scissor(RectF r) {
    int x0 = static_cast<int>(std::floor(r.x));
    int y0 = static_cast<int>(std::floor(r.y));
    int x1 = static_cast<int>(std::ceil(r.x + r.w));
    int y1 = static_cast<int>(std::ceil(r.y + r.h));

    x0 = std::max(0, std::min(x0, vw));
    y0 = std::max(0, std::min(y0, vh));
    x1 = std::max(0, std::min(x1, vw));
    y1 = std::max(0, std::min(y1, vh));

    const int w = std::max(0, x1 - x0);
    const int h = std::max(0, y1 - y0);
    const int sc_y = vh - (y0 + h);
    glScissor(x0, sc_y, w, h);
  }

  void setup_attribs() {
    gl->EnableVertexAttribArray(static_cast<GLuint>(a_pos));
    gl->EnableVertexAttribArray(static_cast<GLuint>(a_uv));
    gl->EnableVertexAttribArray(static_cast<GLuint>(a_color));

    gl->VertexAttribPointer(static_cast<GLuint>(a_pos), 2, GL_FLOAT, GL_FALSE,
                            static_cast<GLsizei>(sizeof(RenderVertex)),
                            reinterpret_cast<const void *>(
                                offsetof(RenderVertex, x)));
    gl->VertexAttribPointer(static_cast<GLuint>(a_uv), 2, GL_FLOAT, GL_FALSE,
                            static_cast<GLsizei>(sizeof(RenderVertex)),
                            reinterpret_cast<const void *>(
                                offsetof(RenderVertex, u)));
    gl->VertexAttribPointer(
        static_cast<GLuint>(a_color), 4, GL_UNSIGNED_BYTE, GL_TRUE,
        static_cast<GLsizei>(sizeof(RenderVertex)),
        reinterpret_cast<const void *>(offsetof(RenderVertex, rgba)));
  }
};

struct InputCtx {
  ViewInstance *app{};
  int pointer_id{1};
};

static void utf8_append(std::string &out, std::uint32_t cp) {
  if (cp <= 0x7F) {
    out.push_back(static_cast<char>(cp));
    return;
  }
  if (cp <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    return;
  }
  if (cp <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    return;
  }
  out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
  out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
  out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
  out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
}

static void utf8_pop_back(std::string &s) {
  if (s.empty()) {
    return;
  }
  std::size_t i = s.size() - 1;
  while (i > 0) {
    const auto b = static_cast<std::uint8_t>(s[i]);
    if ((b & 0xC0) != 0x80) {
      break;
    }
    --i;
  }
  s.erase(i);
}

static std::int64_t utf8_count(std::string_view s) {
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

static std::size_t utf8_byte_offset_from_char(std::string_view s,
                                              std::int64_t char_index) {
  if (char_index <= 0) {
    return 0;
  }
  std::int64_t n = 0;
  std::size_t i = 0;
  while (i < s.size() && n < char_index) {
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
  return i;
}

static void utf8_erase_prev_char(std::string &s, std::int64_t &caret) {
  const auto len = ::utf8_count(s);
  caret = std::max<std::int64_t>(0, std::min(caret, len));
  if (caret <= 0) {
    return;
  }
  const auto end = ::utf8_byte_offset_from_char(s, caret);
  const auto start = ::utf8_byte_offset_from_char(s, caret - 1);
  if (start <= end && end <= s.size()) {
    s.erase(start, end - start);
    --caret;
  }
}

static void utf8_insert_at_char(std::string &s, std::int64_t &caret,
                                std::string_view insert) {
  const auto len = ::utf8_count(s);
  caret = std::max<std::int64_t>(0, std::min(caret, len));
  const auto pos = ::utf8_byte_offset_from_char(s, caret);
  s.insert(pos, insert);
  caret += ::utf8_count(insert);
}

static void utf8_erase_at_char(std::string &s, std::int64_t caret) {
  const auto len = ::utf8_count(s);
  caret = std::max<std::int64_t>(0, std::min(caret, len));
  if (caret >= len) {
    return;
  }
  const auto start = ::utf8_byte_offset_from_char(s, caret);
  const auto end = ::utf8_byte_offset_from_char(s, caret + 1);
  if (start <= end && end <= s.size()) {
    s.erase(start, end - start);
  }
}

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

static void key_cb(GLFWwindow *win, int key, int scancode, int action,
                   int mods) {
  auto *ctx = static_cast<InputCtx *>(glfwGetWindowUserPointer(win));
  if (!ctx || !ctx->app) {
    return;
  }
  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    ctx->app->dispatch_key_down(key, scancode, mods);
  } else if (action == GLFW_RELEASE) {
    ctx->app->dispatch_key_up(key, scancode, mods);
  }
}

static void char_cb(GLFWwindow *win, unsigned int codepoint) {
  auto *ctx = static_cast<InputCtx *>(glfwGetWindowUserPointer(win));
  if (!ctx || !ctx->app) {
    return;
  }
  std::string text;
  utf8_append(text, static_cast<std::uint32_t>(codepoint));
  if (!text.empty()) {
    ctx->app->dispatch_text_input(std::move(text));
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

  GLProcs gl{};
  if (!duorou_gl_load(gl)) {
    glfwDestroyWindow(win);
    glfwTerminate();
    return 1;
  }

  GLRenderer renderer;
  if (!renderer.init(gl)) {
    glfwDestroyWindow(win);
    glfwTerminate();
    return 1;
  }

  GLuint demo_tex = duorou_make_demo_rgba_texture();
  const TextureHandle demo_tex_handle = static_cast<TextureHandle>(demo_tex);

  auto count = state<std::int64_t>(0);
  auto pressed = state<bool>(false);
  auto checked = state<bool>(true);
  auto slider = state<double>(0.35);
  auto field = state<std::string>(std::string{});
  auto field_focused = state<bool>(false);
  auto field_caret = state<std::int64_t>(0);
  auto progress = state<double>(0.25);
  auto stepper_value = state<double>(3.0);
  auto secure_value = state<std::string>(std::string{});
  auto secure_focused = state<bool>(false);
  auto secure_caret = state<std::int64_t>(0);
  auto editor_value = state<std::string>(std::string{"Multi-line text editor"});
  auto editor_focused = state<bool>(false);
  auto editor_caret = state<std::int64_t>(0);
  auto bind_field_value =
      state<std::string>(std::string{"Drag to select (Binding TextField)"});
  auto bind_secure_value = state<std::string>(std::string{"secret"});
  auto bind_editor_value = state<std::string>(
      std::string{"Drag to select (Binding TextEditor)\nSecond line\nThird line"});

  GLTextCache text_cache;

  ViewInstance app{[&]() {
    auto slider_set_from_pointer = [slider]() mutable {
      auto r = target_frame();
      if (!r || !(r->w > 0.0f)) {
        return;
      }
      const float t = (pointer_x() - r->x) / r->w;
      const double v = static_cast<double>(std::min(1.0f, std::max(0.0f, t)));
      slider.set(v);
    };

    return view("Column")
        .prop("padding", 24)
        .prop("spacing", 12)
        .prop("cross_align", "start")
        .children({
            view("Box")
                .prop("padding", 12)
                .prop("bg", 0xFF202020)
                .prop("border", 0xFF3A3A3A)
                .prop("border_width", 1.0)
                .children({
                    view("Text")
                        .prop("value", "duorou: basic components")
                        .prop("font_size", 18.0)
                        .build(),
                })
                .build(),

            view("Box")
                .prop("padding", 12)
                .prop("bg", 0xFF202020)
                .prop("border", 0xFF3A3A3A)
                .prop("border_width", 1.0)
                .children({
                    view("Row")
                        .prop("spacing", 12)
                        .prop("cross_align", "center")
                        .children({
                            view("Image")
                                .prop("texture",
                                      static_cast<std::int64_t>(demo_tex_handle))
                                .prop("width", 128.0)
                                .prop("height", 128.0)
                                .build(),
                            view("Column")
                                .prop("spacing", 6)
                                .prop("cross_align", "start")
                                .children({
                                    view("Text")
                                        .prop("value", "Image: RGBA texture")
                                        .prop("font_size", 16.0)
                                        .build(),
                                    view("Text")
                                        .prop("value",
                                              "Sampling: RenderPipeline::Image")
                                        .prop("font_size", 12.0)
                                        .prop("color", 0xFFB0B0B0)
                                        .build(),
                                })
                                .build(),
                        })
                        .build(),
                })
                .build(),

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

            view("Divider").prop("thickness", 1.0).prop("color", 0xFF3A3A3A).build(),

            view("Checkbox")
                .key("cb")
                .prop("label", "Enable feature")
                .prop("checked", checked.get())
                .event("pointer_up", on_pointer_up([checked]() mutable {
                         checked.set(!checked.get());
                       }))
                .build(),

            view("Text")
                .prop("value",
                      std::string{"Slider: "} +
                          std::to_string(static_cast<int>(slider.get() * 100.0)))
                .build(),
            view("Slider")
                .key("sl")
                .prop("value", slider.get())
                .prop("min", 0.0)
                .prop("max", 1.0)
                .event("pointer_down", on_pointer_down(
                                         [slider_set_from_pointer]() mutable {
                                           slider_set_from_pointer();
                                           capture_pointer();
                                         }))
                .event("pointer_move", on_pointer_move(
                                         [slider_set_from_pointer]() mutable {
                                           slider_set_from_pointer();
                                         }))
                .event("pointer_up", on_pointer_up(
                                       [slider_set_from_pointer]() mutable {
                                         slider_set_from_pointer();
                                         release_pointer();
                                       }))
                .build(),

            view("Text")
                .prop("value",
                      std::string{"ProgressView: "} +
                          std::to_string(static_cast<int>(progress.get() * 100.0)) +
                          "%")
                .build(),
            view("Row")
                .prop("spacing", 10.0)
                .prop("cross_align", "center")
                .children({
                    view("ProgressView")
                        .prop("value", progress.get())
                        .prop("width", 220.0)
                        .prop("height", 10.0)
                        .build(),
                    view("Button")
                        .prop("title", "+10%")
                        .event("pointer_up", on_pointer_up([progress]() mutable {
                                 progress.set(std::min(1.0, progress.get() + 0.1));
                               }))
                        .build(),
                })
                .build(),

            view("Text")
                .prop("value", std::string{"Stepper: "} +
                                   std::to_string(static_cast<int>(stepper_value.get())))
                .build(),
            view("Stepper")
                .prop("value", stepper_value.get())
                .prop("width", 160.0)
                .event("pointer_up", on_pointer_up([stepper_value]() mutable {
                         auto r = target_frame();
                         if (!r || !(r->w > 0.0f)) {
                           return;
                         }
                         const float local_x = pointer_x() - r->x;
                         const bool inc = local_x > r->w * 0.5f;
                         const double next =
                             stepper_value.get() + (inc ? 1.0 : -1.0);
                         stepper_value.set(std::max(0.0, next));
                       }))
                .build(),

            view("Text")
                .prop("value", std::string{"TextField: "} + field.get())
                .build(),
            view("TextField")
                .key("tf")
                .prop("value", field.get())
                .prop("caret", field_caret.get())
                .prop("placeholder", "Type here (click to focus)")
                .prop("focused", field_focused.get())
                .event("focus", on_focus([field_focused, field, field_caret]() mutable {
                         field_focused.set(true);
                         field_caret.set(::utf8_count(field.get()));
                       }))
                .event("blur", on_blur([field_focused]() mutable {
                         field_focused.set(false);
                       }))
                .event("pointer_down", on_pointer_down([field, field_caret, &text_cache]() mutable {
                         auto r = target_frame();
                         if (!r) {
                           return;
                         }
                         const float padding = 10.0f;
                         const float font_px = 16.0f;
                         const float local_x = pointer_x() - (r->x + padding);
                         const float target_w = std::max(0.0f, r->w - padding * 2.0f);
                         const float target_h = std::max(0.0f, r->h);

                         const auto text = field.get();
                         const auto len = ::utf8_count(text);
                         if (len <= 0) {
                           field_caret.set(0);
                           return;
                         }

                         const auto *entry = text_cache.get(text, font_px);
                         if (!entry || entry->caret_x.empty() || !(entry->w > 0) ||
                             !(entry->h > 0)) {
                           const float char_w = font_px * 0.5f;
                           auto pos = static_cast<std::int64_t>(
                               std::round(char_w > 0.0f ? (local_x / char_w)
                                                        : 0.0f));
                           pos = std::max<std::int64_t>(0, std::min(pos, len));
                           field_caret.set(pos);
                           return;
                         }

                         const float scale =
                             std::min(target_w / static_cast<float>(entry->w),
                                      target_h / static_cast<float>(entry->h));
                         if (!(scale > 0.0f)) {
                           field_caret.set(len);
                           return;
                         }

                         const float lx = local_x / scale;
                         const auto &cx = entry->caret_x;
                         auto it = std::lower_bound(cx.begin(), cx.end(), lx);
                         auto idx = static_cast<std::int64_t>(it - cx.begin());
                         const auto last =
                             static_cast<std::int64_t>(cx.size()) - 1;
                         idx = std::max<std::int64_t>(0, std::min(idx, last));
                         field_caret.set(idx);
                       }))
                .event("key_down", on_key_down([field, field_caret]() mutable {
                         auto caret = field_caret.get();
                         auto s = field.get();
                         const auto len = ::utf8_count(s);
                         caret = std::max<std::int64_t>(0, std::min(caret, len));

                         if (key_code() == GLFW_KEY_LEFT) {
                           caret = std::max<std::int64_t>(0, caret - 1);
                         } else if (key_code() == GLFW_KEY_RIGHT) {
                           caret = std::min<std::int64_t>(len, caret + 1);
                         } else if (key_code() == GLFW_KEY_HOME) {
                           caret = 0;
                         } else if (key_code() == GLFW_KEY_END) {
                           caret = len;
                         } else if (key_code() == GLFW_KEY_BACKSPACE) {
                           ::utf8_erase_prev_char(s, caret);
                           field.set(std::move(s));
                         } else if (key_code() == GLFW_KEY_DELETE) {
                           ::utf8_erase_at_char(s, caret);
                           field.set(std::move(s));
                         }

                         field_caret.set(caret);
                       }))
                .event("text_input", on_text_input([field, field_caret]() mutable {
                         auto caret = field_caret.get();
                         auto s = field.get();
                         ::utf8_insert_at_char(s, caret, text_input());
                         field.set(std::move(s));
                         field_caret.set(caret);
                       }))
                .build(),

            view("Text")
                .prop("value", std::string{"SecureField: "} + secure_value.get())
                .build(),
            view("TextField")
                .key("sf")
                .prop("secure", true)
                .prop("value", secure_value.get())
                .prop("caret", secure_caret.get())
                .prop("placeholder", "Password")
                .prop("focused", secure_focused.get())
                .event("focus", on_focus([secure_focused, secure_value, secure_caret]() mutable {
                         secure_focused.set(true);
                         secure_caret.set(::utf8_count(secure_value.get()));
                       }))
                .event("blur", on_blur([secure_focused]() mutable {
                         secure_focused.set(false);
                       }))
                .event("pointer_down", on_pointer_down([secure_value, secure_caret, &text_cache]() mutable {
                         auto r = target_frame();
                         if (!r) {
                           return;
                         }
                         const float padding = 10.0f;
                         const float font_px = 16.0f;
                         const float local_x = pointer_x() - (r->x + padding);
                         const float target_w = std::max(0.0f, r->w - padding * 2.0f);
                         const float target_h = std::max(0.0f, r->h);

                         const auto raw = secure_value.get();
                         const auto len = ::utf8_count(raw);
                         if (len <= 0) {
                           secure_caret.set(0);
                           return;
                         }

                         std::string masked;
                         masked.assign(static_cast<std::size_t>(len), '*');
                         const auto *entry = text_cache.get(masked, font_px);
                         if (!entry || entry->caret_x.empty() || !(entry->w > 0) ||
                             !(entry->h > 0)) {
                           const float char_w = font_px * 0.5f;
                           auto pos = static_cast<std::int64_t>(
                               std::round(char_w > 0.0f ? (local_x / char_w)
                                                        : 0.0f));
                           pos = std::max<std::int64_t>(0, std::min(pos, len));
                           secure_caret.set(pos);
                           return;
                         }

                         const float scale =
                             std::min(target_w / static_cast<float>(entry->w),
                                      target_h / static_cast<float>(entry->h));
                         if (!(scale > 0.0f)) {
                           secure_caret.set(len);
                           return;
                         }

                         const float lx = local_x / scale;
                         const auto &cx = entry->caret_x;
                         auto it = std::lower_bound(cx.begin(), cx.end(), lx);
                         auto idx = static_cast<std::int64_t>(it - cx.begin());
                         const auto last =
                             static_cast<std::int64_t>(cx.size()) - 1;
                         idx = std::max<std::int64_t>(0, std::min(idx, last));
                         secure_caret.set(idx);
                       }))
                .event("key_down", on_key_down([secure_value, secure_caret]() mutable {
                         auto caret = secure_caret.get();
                         auto s = secure_value.get();
                         const auto len = ::utf8_count(s);
                         caret = std::max<std::int64_t>(0, std::min(caret, len));

                         if (key_code() == GLFW_KEY_LEFT) {
                           caret = std::max<std::int64_t>(0, caret - 1);
                         } else if (key_code() == GLFW_KEY_RIGHT) {
                           caret = std::min<std::int64_t>(len, caret + 1);
                         } else if (key_code() == GLFW_KEY_HOME) {
                           caret = 0;
                         } else if (key_code() == GLFW_KEY_END) {
                           caret = len;
                         } else if (key_code() == GLFW_KEY_BACKSPACE) {
                           ::utf8_erase_prev_char(s, caret);
                           secure_value.set(std::move(s));
                         } else if (key_code() == GLFW_KEY_DELETE) {
                           ::utf8_erase_at_char(s, caret);
                           secure_value.set(std::move(s));
                         }

                         secure_caret.set(caret);
                       }))
                .event("text_input", on_text_input([secure_value, secure_caret]() mutable {
                         auto caret = secure_caret.get();
                         auto s = secure_value.get();
                         ::utf8_insert_at_char(s, caret, text_input());
                         secure_value.set(std::move(s));
                         secure_caret.set(caret);
                       }))
                .build(),

            view("Text")
                .prop("value", "TextEditor:")
                .build(),
            view("TextEditor")
                .key("te")
                .prop("value", editor_value.get())
                .prop("focused", editor_focused.get())
                .prop("caret", editor_caret.get())
                .prop("width", 360.0)
                .prop("height", 110.0)
                .event("focus", on_focus([editor_focused, editor_value, editor_caret]() mutable {
                         editor_focused.set(true);
                         editor_caret.set(::utf8_count(editor_value.get()));
                       }))
                .event("blur", on_blur([editor_focused]() mutable {
                         editor_focused.set(false);
                       }))
                .event("pointer_down", on_pointer_down([editor_value, editor_caret, &text_cache]() mutable {
                         auto r = target_frame();
                         if (!r) {
                           return;
                         }
                         const float padding = 10.0f;
                         const float font_px = 16.0f;
                         const float line_h = font_px * 1.2f;

                         const float local_x = pointer_x() - (r->x + padding);
                         const float local_y = pointer_y() - (r->y + padding);
                         const std::int64_t row = static_cast<std::int64_t>(
                             std::max(0.0f, std::floor(line_h > 0.0f ? (local_y / line_h)
                                                                  : 0.0f)));
                         const float target_w = std::max(0.0f, r->w - padding * 2.0f);
                         const float target_h = std::max(0.0f, line_h);

                         const auto text = editor_value.get();
                         struct LineInfo {
                           std::string_view text;
                           std::int64_t start{};
                           std::int64_t len{};
                         };
                         std::vector<LineInfo> lines;
                         lines.reserve(8);
                         {
                           std::size_t start_b = 0;
                           std::int64_t start_c = 0;
                           for (std::size_t i = 0; i <= text.size(); ++i) {
                             if (i == text.size() || text[i] == '\n') {
                                auto sv = std::string_view{text}.substr(start_b, i - start_b);
                               const auto len_c = ::utf8_count(sv);
                               lines.push_back(LineInfo{sv, start_c, len_c});
                               start_b = i + 1;
                               start_c += len_c + 1;
                              }
                            }
                            if (lines.empty()) {
                              lines.push_back(LineInfo{std::string_view{}, 0, 0});
                            }
                          }

                         const auto idx =
                             static_cast<std::size_t>(std::min<std::int64_t>(
                                 row, static_cast<std::int64_t>(lines.size() - 1)));
                         const auto &line = lines[idx];
                         std::int64_t col = 0;
                         if (!line.text.empty()) {
                           const auto *entry = text_cache.get(line.text, font_px);
                           if (entry && !entry->caret_x.empty() && entry->w > 0 &&
                               entry->h > 0) {
                             const float scale = std::min(
                                 target_w / static_cast<float>(entry->w),
                                 target_h / static_cast<float>(entry->h));
                             if (scale > 0.0f) {
                               const float lx = local_x / scale;
                               const auto &cx = entry->caret_x;
                               auto it = std::lower_bound(cx.begin(), cx.end(), lx);
                               col = static_cast<std::int64_t>(it - cx.begin());
                               const auto last =
                                   static_cast<std::int64_t>(cx.size()) - 1;
                               col = std::max<std::int64_t>(0, std::min(col, last));
                             }
                           } else {
                             const float char_w = font_px * 0.5f;
                             col = static_cast<std::int64_t>(std::max(
                                 0.0f, std::round(char_w > 0.0f ? (local_x / char_w)
                                                               : 0.0f)));
                           }
                         }
                         col = std::max<std::int64_t>(0, std::min(col, line.len));
                         const auto pos = line.start + col;
                         editor_caret.set(pos);
                       }))
                .event("key_down", on_key_down([editor_value, editor_caret]() mutable {
                         auto caret = editor_caret.get();
                         auto s = editor_value.get();

                         struct LineInfo {
                           std::int64_t start{};
                           std::int64_t len{};
                         };
                         std::vector<LineInfo> lines;
                         lines.reserve(8);
                         {
                           std::size_t start_b = 0;
                           std::int64_t start_c = 0;
                           for (std::size_t i = 0; i <= s.size(); ++i) {
                             if (i == s.size() || s[i] == '\n') {
                               auto sv =
                                   std::string_view{s}.substr(start_b, i - start_b);
                              const auto len_c = ::utf8_count(sv);
                              lines.push_back(LineInfo{start_c, len_c});
                              start_b = i + 1;
                              start_c += len_c + 1;
                             }
                           }
                           if (lines.empty()) {
                             lines.push_back(LineInfo{0, 0});
                           }
                         }

                         const auto total_len = ::utf8_count(s);
                         caret = std::max<std::int64_t>(0, std::min(caret, total_len));

                         std::size_t line_idx = 0;
                         for (std::size_t i = 0; i < lines.size(); ++i) {
                           if (caret <= lines[i].start + lines[i].len) {
                             line_idx = i;
                             break;
                           }
                           line_idx = i;
                         }
                         const std::int64_t col = caret - lines[line_idx].start;

                         if (key_code() == GLFW_KEY_LEFT) {
                           caret = std::max<std::int64_t>(0, caret - 1);
                         } else if (key_code() == GLFW_KEY_RIGHT) {
                           caret = std::min<std::int64_t>(total_len, caret + 1);
                         } else if (key_code() == GLFW_KEY_HOME) {
                           caret = lines[line_idx].start;
                         } else if (key_code() == GLFW_KEY_END) {
                           caret = lines[line_idx].start + lines[line_idx].len;
                         } else if (key_code() == GLFW_KEY_UP) {
                           if (line_idx > 0) {
                             const auto prev = lines[line_idx - 1];
                             caret = prev.start + std::min(col, prev.len);
                           }
                         } else if (key_code() == GLFW_KEY_DOWN) {
                           if (line_idx + 1 < lines.size()) {
                             const auto next = lines[line_idx + 1];
                             caret = next.start + std::min(col, next.len);
                           }
                         } else if (key_code() == GLFW_KEY_BACKSPACE) {
                           ::utf8_erase_prev_char(s, caret);
                           editor_value.set(std::move(s));
                         } else if (key_code() == GLFW_KEY_DELETE) {
                           ::utf8_erase_at_char(s, caret);
                           editor_value.set(std::move(s));
                         } else if (key_code() == GLFW_KEY_ENTER ||
                                    key_code() == GLFW_KEY_KP_ENTER) {
                           ::utf8_insert_at_char(s, caret, "\n");
                           editor_value.set(std::move(s));
                         }

                         editor_caret.set(caret);
                       }))
                .event("text_input", on_text_input([editor_value, editor_caret]() mutable {
                         auto caret = editor_caret.get();
                         auto s = editor_value.get();
                         ::utf8_insert_at_char(s, caret, text_input());
                         editor_value.set(std::move(s));
                         editor_caret.set(caret);
                       }))
                .build(),

            view("Divider").prop("thickness", 1.0).prop("color", 0xFF3A3A3A).build(),

            view("Text")
                .prop("value", "TextField (Binding, drag to select):")
                .build(),
            view("TextField")
                .key("tf_bind")
                .prop("binding", duorou::ui::bind(bind_field_value))
                .prop("placeholder", "Type here")
                .build(),

            view("Text").prop("value", "SecureField (Binding):").build(),
            view("TextField")
                .key("sf_bind")
                .prop("secure", true)
                .prop("binding", duorou::ui::bind(bind_secure_value))
                .prop("placeholder", "Password")
                .build(),

            view("Text")
                .prop("value", "TextEditor (Binding, drag to select):")
                .build(),
            view("TextEditor")
                .key("te_bind")
                .prop("binding", duorou::ui::bind(bind_editor_value))
                .prop("width", 360.0)
                .prop("height", 110.0)
                .build(),

            view("Divider").prop("thickness", 1.0).prop("color", 0xFF3A3A3A).build(),

            view("Box")
                .prop("padding", 12)
                .prop("bg", 0xFF202020)
                .prop("border", 0xFF3A3A3A)
                .prop("border_width", 1.0)
                .children({
                    view("Column")
                        .prop("spacing", 8)
                        .prop("cross_align", "start")
                        .children({
                            view("Text")
                                .prop("value", "ScrollView/List demo (drag to scroll)")
                                .prop("font_size", 16.0)
                                .build(),
                            view("ScrollView")
                                .key("demo_scroll")
                                .prop("clip", true)
                                .prop("height", 220.0)
                                .children({
                                    view("Column")
                                        .prop("spacing", 0.0)
                                        .prop("cross_align", "stretch")
                                        .children([&](auto &c) {
                                          for (int i = 0; i < 60; ++i) {
                                            const bool alt = (i % 2) == 0;
                                            c.add(view("Box")
                                                      .prop("padding", 10.0)
                                                      .prop("bg", alt ? 0xFF262626
                                                                      : 0xFF1E1E1E)
                                                      .children({
                                                          view("Text")
                                                              .prop("value",
                                                                    std::string{"Row "} +
                                                                        std::to_string(i))
                                                              .prop("color", 0xFFE0E0E0)
                                                              .build(),
                                                      })
                                                      .build());
                                          }
                                        })
                                        .build(),
                                })
                                .build(),
                        })
                        .build(),
                })
                .build(),
        })
        .build();
  }};

  InputCtx input;
  input.app = &app;
  glfwSetWindowUserPointer(win, &input);
  glfwSetCursorPosCallback(win, cursor_pos_cb);
  glfwSetMouseButtonCallback(win, mouse_button_cb);
  glfwSetKeyCallback(win, key_cb);
  glfwSetCharCallback(win, char_cb);

  {
    struct GLTextProvider final : TextProvider {
      GLTextCache *cache{};

      bool layout_text(std::string_view text, float font_px,
                       TextLayout &out) override {
        out.quads.clear();
        out.caret_x.clear();
        if (!cache) {
          return false;
        }
        const auto *e = cache->get(text, font_px);
        if (!e) {
          return false;
        }
        out.w = static_cast<float>(e->w);
        out.h = static_cast<float>(e->h);
        out.quads.reserve(e->quads.size());
        for (const auto &q : e->quads) {
          TextQuad tq;
          tq.x0 = q.x0;
          tq.y0 = q.y0;
          tq.x1 = q.x1;
          tq.y1 = q.y1;
          tq.u0 = q.u0;
          tq.v0 = q.v0;
          tq.u1 = q.u1;
          tq.v1 = q.v1;
          tq.texture = static_cast<TextureHandle>(q.texture);
          out.quads.push_back(tq);
        }
        out.caret_x = e->caret_x;
        return true;
      }
    };

    GLTextProvider text;
    text.cache = &text_cache;

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

      glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      renderer.begin_frame(fbw, fbh);
      const auto tree = build_render_tree(
          app.render_ops(), SizeF{static_cast<float>(fbw), static_cast<float>(fbh)},
          text);
      renderer.draw_tree(tree);
      renderer.end_frame();

      glfwSwapBuffers(win);
    }
  }

  if (demo_tex != 0) {
    glDeleteTextures(1, &demo_tex);
  }

  glfwDestroyWindow(win);
  glfwTerminate();
  return 0;
}

#endif
