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

    std::vector<CachedGlyph> glyphs;
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

      const auto gi =
          static_cast<std::uint32_t>(FT_Get_Char_Index(face_, (FT_ULong)cp));
      if (gi == 0) {
        continue;
      }
      const auto *g = get_glyph(gi, px);
      if (!g) {
        continue;
      }
      glyphs.push_back(*g);

      max_top = std::max(max_top, g->bitmap_top);
      const int bottom = g->h - g->bitmap_top;
      max_bottom = std::max(max_bottom, bottom);

      pen_x += g->advance;
    }

    if (glyphs.empty()) {
      return false;
    }

    const int pad = 2;
    out.w = std::max(1, pen_x + pad * 2);
    out.h = std::max(1, max_top + max_bottom + pad * 2);
    out.quads.clear();
    out.quads.reserve(glyphs.size());

    const int baseline = pad + max_top;
    int x = pad;
    for (const auto &g : glyphs) {
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
      x += g.advance;
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
  GLint u_use_tex{-1};
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
        "uniform int uUseTex;\n"
        "varying vec2 vUV;\n"
        "varying vec4 vColor;\n"
        "void main() {\n"
        "  float a = 1.0;\n"
        "  if (uUseTex != 0) {\n"
        "    a = texture2D(uTex, vUV).a;\n"
        "  }\n"
        "  gl_FragColor = vec4(vColor.rgb, vColor.a * a);\n"
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
    u_use_tex = gl->GetUniformLocation(program, "uUseTex");
    a_pos = gl->GetAttribLocation(program, "aPos");
    a_uv = gl->GetAttribLocation(program, "aUV");
    a_color = gl->GetAttribLocation(program, "aColor");
    if (u_mvp < 0 || u_tex < 0 || u_use_tex < 0 || a_pos < 0 || a_uv < 0 ||
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
    gl->Uniform1i(u_use_tex, 0);
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
          gl->Uniform1i(u_use_tex, 0);
          last_use_tex = 0;
        }
        if (last_tex != 0) {
          glBindTexture(GL_TEXTURE_2D, 0);
          last_tex = 0;
        }
      } else {
        const auto tex = static_cast<GLuint>(b.texture);
        if (last_use_tex != 1) {
          gl->Uniform1i(u_use_tex, 1);
          last_use_tex = 1;
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

  auto count = state<std::int64_t>(0);
  auto pressed = state<bool>(false);
  auto checked = state<bool>(true);
  auto slider = state<double>(0.35);
  auto field = state<std::string>(std::string{});
  auto field_focused = state<bool>(false);

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
                .prop("value", std::string{"TextField: "} + field.get())
                .build(),
            view("TextField")
                .key("tf")
                .prop("value", field.get())
                .prop("placeholder", "Type here (click to focus)")
                .prop("focused", field_focused.get())
                .event("focus", on_focus([field_focused]() mutable {
                         field_focused.set(true);
                       }))
                .event("blur", on_blur([field_focused]() mutable {
                         field_focused.set(false);
                       }))
                .event("key_down", on_key_down([field]() mutable {
                         if (key_code() == GLFW_KEY_BACKSPACE) {
                           auto s = field.get();
                           utf8_pop_back(s);
                           field.set(std::move(s));
                         }
                       }))
                .event("text_input", on_text_input([field]() mutable {
                         auto s = field.get();
                         s.append(text_input());
                         field.set(std::move(s));
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
  glfwSetKeyCallback(win, key_cb);
  glfwSetCharCallback(win, char_cb);

  {
    GLTextCache text_cache;
    struct GLTextProvider final : TextProvider {
      GLTextCache *cache{};

      bool layout_text(std::string_view text, float font_px,
                       TextLayout &out) override {
        out.quads.clear();
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

  glfwDestroyWindow(win);
  glfwTerminate();
  return 0;
}

#endif
