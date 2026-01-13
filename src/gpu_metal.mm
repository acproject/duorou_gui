#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreText/CoreText.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include <duorou/ui/runtime.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "terminal_view.hpp"

using namespace duorou::ui;

static NSString *duorou_msl_source() {
  return @"#include <metal_stdlib>\n"
          "using namespace metal;\n"
          "struct Vertex { packed_float2 pos; packed_float2 uv; uint color; };\n"
          "static inline float4 unpack_color(uint c) {\n"
          "  float r = float(c & 255u) / 255.0;\n"
          "  float g = float((c >> 8) & 255u) / 255.0;\n"
          "  float b = float((c >> 16) & 255u) / 255.0;\n"
          "  float a = float((c >> 24) & 255u) / 255.0;\n"
          "  return float4(r, g, b, a);\n"
          "}\n"
          "struct VSOut { float4 pos [[position]]; float2 uv; float4 color; };\n"
          "vertex VSOut duorou_vertex(const device Vertex* vtx [[buffer(0)]],\n"
          "                           constant float2& viewport [[buffer(1)]],\n"
          "                           uint vid [[vertex_id]]) {\n"
          "  VSOut o;\n"
          "  float2 p = float2(vtx[vid].pos);\n"
          "  float vw = max(1.0, viewport.x);\n"
          "  float vh = max(1.0, viewport.y);\n"
          "  float x = (p.x / vw) * 2.0 - 1.0;\n"
          "  float y = 1.0 - (p.y / vh) * 2.0;\n"
          "  o.pos = float4(x, y, 0.0, 1.0);\n"
          "  o.uv = float2(vtx[vid].uv);\n"
          "  o.color = unpack_color(vtx[vid].color);\n"
          "  return o;\n"
          "}\n"
          "fragment float4 duorou_fragment_color(VSOut in [[stage_in]]) {\n"
          "  return in.color;\n"
          "}\n"
          "fragment float4 duorou_fragment_tex(VSOut in [[stage_in]],\n"
          "                                  texture2d<float> tex [[texture(0)]],\n"
          "                                  sampler s [[sampler(0)]]) {\n"
          "  float a = tex.sample(s, in.uv).a;\n"
          "  float ao = a * in.color.a;\n"
          "  return float4(in.color.rgb * ao, ao);\n"
          "}\n"
          "fragment float4 duorou_fragment_image(VSOut in [[stage_in]],\n"
          "                                  texture2d<float> tex [[texture(0)]],\n"
          "                                  sampler s [[sampler(0)]]) {\n"
          "  float4 t = tex.sample(s, in.uv);\n"
          "  return float4(in.color.rgb * t.rgb, in.color.a * t.a);\n"
          "}\n";
}

static id<MTLTexture> duorou_make_text_texture(id<MTLDevice> device,
                                               NSString *text, CGFloat font_px,
                                               CGSize *out_size_px) {
  if (!text || text.length == 0) {
    if (out_size_px) {
      *out_size_px = CGSizeMake(0, 0);
    }
    return nil;
  }

  CTFontRef font = CTFontCreateWithName(CFSTR("SF Pro"), font_px, NULL);
  if (!font) {
    font = CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, font_px, NULL);
  }

  NSDictionary *attrs = @{
    (__bridge id)kCTFontAttributeName : (__bridge id)font,
  };

  NSAttributedString *as = [[NSAttributedString alloc] initWithString:text
                                                           attributes:attrs];
  CTLineRef line =
      CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)as);

  CGFloat ascent = 0.0;
  CGFloat descent = 0.0;
  CGFloat leading = 0.0;
  const double width =
      CTLineGetTypographicBounds(line, &ascent, &descent, &leading);

  const std::size_t pad = 2;
  const std::size_t w = std::max<std::size_t>(
      1, static_cast<std::size_t>(std::ceil(width)) + pad * 2);
  const std::size_t h = std::max<std::size_t>(
      1, static_cast<std::size_t>(std::ceil(ascent + descent)) + pad * 2);

  const std::size_t bytes_per_row = w * 4;
  std::vector<std::uint8_t> pixels(bytes_per_row * h, 0);

  CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
  CGContextRef ctx = CGBitmapContextCreate(
      pixels.data(), w, h, 8, bytes_per_row, cs,
      kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
  CGColorSpaceRelease(cs);

  if (!ctx) {
    CFRelease(line);
    CFRelease(font);
    if (out_size_px) {
      *out_size_px = CGSizeMake(0, 0);
    }
    return nil;
  }

  CGContextSetRGBFillColor(ctx, 1.0, 1.0, 1.0, 1.0);
  CGContextSetTextPosition(ctx, (CGFloat)pad, (CGFloat)pad + descent);
  CTLineDraw(line, ctx);

  CGContextRelease(ctx);
  CFRelease(line);
  CFRelease(font);

  MTLTextureDescriptor *td = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                   width:(NSUInteger)w
                                  height:(NSUInteger)h
                               mipmapped:NO];
  td.usage = MTLTextureUsageShaderRead;
  id<MTLTexture> tex = [device newTextureWithDescriptor:td];
  if (!tex) {
    if (out_size_px) {
      *out_size_px = CGSizeMake(0, 0);
    }
    return nil;
  }

  [tex replaceRegion:MTLRegionMake2D(0, 0, w, h)
         mipmapLevel:0
           withBytes:pixels.data()
         bytesPerRow:bytes_per_row];

  if (out_size_px) {
    *out_size_px = CGSizeMake((CGFloat)w, (CGFloat)h);
  }
  return tex;
}

@interface DuorouMetalView : MTKView <MTKViewDelegate>
@property(nonatomic) ViewInstance *instance;
@property(nonatomic, strong) id<MTLCommandQueue> queue;
@property(nonatomic, strong) id<MTLRenderPipelineState> pipelineColor;
@property(nonatomic, strong) id<MTLRenderPipelineState> pipelineText;
@property(nonatomic, strong) id<MTLRenderPipelineState> pipelineImage;
@property(nonatomic, strong) id<MTLSamplerState> sampler;
@property(nonatomic, strong)
    NSMutableDictionary<NSString *, id<MTLTexture>> *textCache;
@property(nonatomic, strong)
    NSMutableDictionary<NSString *, NSValue *> *textSizeCache;
@end

@interface DuorouAppDelegate
    : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end

@implementation DuorouAppDelegate

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:
    (NSApplication *)sender {
  (void)sender;
  return YES;
}

- (void)windowWillClose:(NSNotification *)notification {
  (void)notification;
  [NSApp terminate:nil];
}

@end

@implementation DuorouMetalView

- (instancetype)initWithFrame:(NSRect)frameRect
                       device:(id<MTLDevice>)device
                     instance:(ViewInstance *)instance {
  self = [super initWithFrame:frameRect device:device];
  if (!self) {
    return nil;
  }

  self.instance = instance;
  self.delegate = self;
  self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
  self.clearColor = MTLClearColorMake(0.1, 0.1, 0.1, 1.0);
  self.preferredFramesPerSecond = 60;
  self.paused = NO;
  self.enableSetNeedsDisplay = NO;

  self.queue = [device newCommandQueue];

  NSError *err = nil;
  id<MTLLibrary> lib = [device newLibraryWithSource:duorou_msl_source()
                                            options:nil
                                              error:&err];
  if (!lib) {
    NSLog(@"Failed to compile MSL: %@", err);
    return self;
  }

  id<MTLFunction> vtxColor = [lib newFunctionWithName:@"duorou_vertex"];
  id<MTLFunction> fragColor =
      [lib newFunctionWithName:@"duorou_fragment_color"];

  MTLRenderPipelineDescriptor *descColor =
      [[MTLRenderPipelineDescriptor alloc] init];
  descColor.vertexFunction = vtxColor;
  descColor.fragmentFunction = fragColor;
  descColor.colorAttachments[0].pixelFormat = self.colorPixelFormat;

  self.pipelineColor = [device newRenderPipelineStateWithDescriptor:descColor
                                                              error:&err];
  if (!self.pipelineColor) {
    NSLog(@"Failed to create color pipeline: %@", err);
  }

  id<MTLFunction> vtxTex = [lib newFunctionWithName:@"duorou_vertex"];
  id<MTLFunction> fragTex = [lib newFunctionWithName:@"duorou_fragment_tex"];

  MTLRenderPipelineDescriptor *descTex =
      [[MTLRenderPipelineDescriptor alloc] init];
  descTex.vertexFunction = vtxTex;
  descTex.fragmentFunction = fragTex;
  descTex.colorAttachments[0].pixelFormat = self.colorPixelFormat;

  self.pipelineText = [device newRenderPipelineStateWithDescriptor:descTex
                                                             error:&err];
  if (!self.pipelineText) {
    NSLog(@"Failed to create text pipeline: %@", err);
  }

  id<MTLFunction> vtxImg = [lib newFunctionWithName:@"duorou_vertex"];
  id<MTLFunction> fragImg = [lib newFunctionWithName:@"duorou_fragment_image"];

  MTLRenderPipelineDescriptor *descImg =
      [[MTLRenderPipelineDescriptor alloc] init];
  descImg.vertexFunction = vtxImg;
  descImg.fragmentFunction = fragImg;
  descImg.colorAttachments[0].pixelFormat = self.colorPixelFormat;

  self.pipelineImage = [device newRenderPipelineStateWithDescriptor:descImg
                                                             error:&err];
  if (!self.pipelineImage) {
    NSLog(@"Failed to create image pipeline: %@", err);
  }

  MTLSamplerDescriptor *sd = [[MTLSamplerDescriptor alloc] init];
  sd.minFilter = MTLSamplerMinMagFilterLinear;
  sd.magFilter = MTLSamplerMinMagFilterLinear;
  sd.sAddressMode = MTLSamplerAddressModeClampToEdge;
  sd.tAddressMode = MTLSamplerAddressModeClampToEdge;
  self.sampler = [device newSamplerStateWithDescriptor:sd];

  self.textCache = [NSMutableDictionary dictionary];
  self.textSizeCache = [NSMutableDictionary dictionary];

  SizeF vp;
  vp.w = static_cast<float>(self.bounds.size.width);
  vp.h = static_cast<float>(self.bounds.size.height);
  self.instance->set_viewport(vp);

  return self;
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
  (void)size;
  SizeF vp;
  vp.w = static_cast<float>(view.bounds.size.width);
  vp.h = static_cast<float>(view.bounds.size.height);
  self.instance->set_viewport(vp);
}

- (void)drawInMTKView:(MTKView *)view {
  if (!self.pipelineColor) {
    return;
  }

  id<CAMetalDrawable> drawable = view.currentDrawable;
  MTLRenderPassDescriptor *pass = view.currentRenderPassDescriptor;
  if (!drawable || !pass) {
    return;
  }

  self.instance->update();

  const auto viewport = self.instance->layout().frame;
  const float vw = viewport.w > 0 ? viewport.w : 1.0f;
  const float vh = viewport.h > 0 ? viewport.h : 1.0f;

  const CGSize ds = view.drawableSize;
  const float sx =
      view.bounds.size.width > 0.0 ? (float)ds.width / (float)view.bounds.size.width
                                   : 1.0f;
  const float sy =
      view.bounds.size.height > 0.0
          ? (float)ds.height / (float)view.bounds.size.height
          : 1.0f;
  struct MetalTextProvider final : TextProvider {
    DuorouMetalView *owner{};
    MTKView *mtkView{};

    bool layout_text(std::string_view text, float font_px,
                     TextLayout &out) override {
      out.quads.clear();
      if (!owner || !mtkView) {
        return false;
      }
      if (text.empty()) {
        return false;
      }

      NSString *nsText = [[NSString alloc] initWithBytes:text.data()
                                                  length:text.size()
                                                encoding:NSUTF8StringEncoding];
      if (!nsText) {
        return false;
      }

      NSString *cacheKey =
          [NSString stringWithFormat:@"%.2f:%@", (double)font_px, nsText];

      NSValue *sizeVal = [owner.textSizeCache objectForKey:cacheKey];
      id<MTLTexture> tex = [owner.textCache objectForKey:cacheKey];
      CGSize size;
      if (!tex || !sizeVal) {
        size = CGSizeZero;
        tex = duorou_make_text_texture(mtkView.device, nsText, font_px, &size);
        if (!tex || size.width <= 0.0 || size.height <= 0.0) {
          return false;
        }
        [owner.textCache setObject:tex forKey:cacheKey];
        [owner.textSizeCache setObject:[NSValue valueWithSize:size]
                                forKey:cacheKey];
      } else {
        size = [sizeVal sizeValue];
      }

      out.w = static_cast<float>(size.width);
      out.h = static_cast<float>(size.height);
      if (!(out.w > 0.0f) || !(out.h > 0.0f)) {
        return false;
      }

      TextQuad q;
      q.x0 = 0.0f;
      q.y0 = 0.0f;
      q.x1 = out.w;
      q.y1 = out.h;
      q.u0 = 0.0f;
      q.v0 = 0.0f;
      q.u1 = 1.0f;
      q.v1 = 1.0f;
      q.texture = static_cast<TextureHandle>(
          (std::uint64_t)(std::uintptr_t)(__bridge void *)tex);
      out.quads.push_back(q);
      return true;
    }
  };

  MetalTextProvider text;
  text.owner = self;
  text.mtkView = view;

  const auto tree = build_render_tree(self.instance->render_ops(),
                                      SizeF{viewport.w, viewport.h}, text);

  id<MTLCommandBuffer> cmd = [self.queue commandBuffer];
  id<MTLRenderCommandEncoder> enc =
      [cmd renderCommandEncoderWithDescriptor:pass];
  [enc setCullMode:MTLCullModeNone];
  [enc setFrontFacingWinding:MTLWindingClockwise];
  MTLViewport vp;
  vp.originX = 0.0;
  vp.originY = 0.0;
  vp.width = std::max(1.0, (double)ds.width);
  vp.height = std::max(1.0, (double)ds.height);
  vp.znear = 0.0;
  vp.zfar = 1.0;
  [enc setViewport:vp];

  auto to_scissor = [&](RectF r) {
    float x0f = std::floor(r.x * sx);
    float y0f = std::floor(r.y * sy);
    float x1f = std::ceil((r.x + r.w) * sx);
    float y1f = std::ceil((r.y + r.h) * sy);

    const int maxw = (int)std::max(1.0, (double)ds.width);
    const int maxh = (int)std::max(1.0, (double)ds.height);

    int x0 = std::max(0, std::min((int)x0f, maxw));
    int y0 = std::max(0, std::min((int)y0f, maxh));
    int x1 = std::max(0, std::min((int)x1f, maxw));
    int y1 = std::max(0, std::min((int)y1f, maxh));

    const int w = std::max(0, x1 - x0);
    const int h = std::max(0, y1 - y0);

    MTLScissorRect sc;
    sc.x = (NSUInteger)x0;
    sc.y = (NSUInteger)y0;
    sc.width = (NSUInteger)w;
    sc.height = (NSUInteger)h;
    return sc;
  };

  struct MetalViewport {
    float w{};
    float h{};
  };
  MetalViewport vpu;
  vpu.w = vw;
  vpu.h = vh;

  if (!tree.vertices.empty()) {
    [enc setVertexBytes:tree.vertices.data()
                 length:tree.vertices.size() * sizeof(RenderVertex)
                atIndex:0];
    [enc setVertexBytes:&vpu length:sizeof(vpu) atIndex:1];
  }

  RenderPipeline cur = RenderPipeline::Color;
  bool has_pipeline = false;

  for (const auto &b : tree.batches) {
    if (b.count == 0) {
      continue;
    }
    if (tree.vertices.empty()) {
      break;
    }

    if (!has_pipeline || b.pipeline != cur) {
      cur = b.pipeline;
      has_pipeline = true;
      if (cur == RenderPipeline::Color) {
        [enc setRenderPipelineState:self.pipelineColor];
      } else if (cur == RenderPipeline::Text) {
        if (!self.pipelineText || !self.sampler) {
          continue;
        }
        [enc setRenderPipelineState:self.pipelineText];
        [enc setFragmentSamplerState:self.sampler atIndex:0];
      } else {
        if (!self.pipelineImage || !self.sampler) {
          continue;
        }
        [enc setRenderPipelineState:self.pipelineImage];
        [enc setFragmentSamplerState:self.sampler atIndex:0];
      }
    }

    [enc setScissorRect:to_scissor(b.scissor)];

    if (cur == RenderPipeline::Text || cur == RenderPipeline::Image) {
      if (b.texture == 0) {
        continue;
      }
      id<MTLTexture> tex =
          (__bridge id<MTLTexture>)(void *)(std::uintptr_t)b.texture;
      if (!tex) {
        continue;
      }
      [enc setFragmentTexture:tex atIndex:0];
    }

    [enc drawPrimitives:MTLPrimitiveTypeTriangle
            vertexStart:(NSUInteger)b.first
            vertexCount:(NSUInteger)b.count];
  }
  [enc endEncoding];
  [cmd presentDrawable:drawable];
  [cmd commit];
}

- (void)mouseDown:(NSEvent *)event {
  NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
  float x = static_cast<float>(p.x);
  float y = static_cast<float>(self.bounds.size.height - p.y);
  self.instance->dispatch_pointer_down(0, x, y);
}

- (void)mouseDragged:(NSEvent *)event {
  NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
  float x = static_cast<float>(p.x);
  float y = static_cast<float>(self.bounds.size.height - p.y);
  self.instance->dispatch_pointer_move(0, x, y);
}

- (void)mouseUp:(NSEvent *)event {
  NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
  float x = static_cast<float>(p.x);
  float y = static_cast<float>(self.bounds.size.height - p.y);
  self.instance->dispatch_pointer_up(0, x, y);
}

@end

static std::unique_ptr<ViewInstance> make_instance(bool use_terminal) {
  if (use_terminal) {
    auto fn = []() { return duorou::ui::examples::terminal_view(); };
    return std::make_unique<ViewInstance>(std::move(fn));
  }

  auto count = state<std::int64_t>(0);
  auto pressed = state<bool>(false);

  auto fn = [count, pressed]() {
    return view("Column")
        .prop("padding", 24)
        .prop("spacing", 12)
        .prop("cross_align", "start")
        .children({
            view("Text")
                .prop("value",
                      std::string("Count: ") + std::to_string(count.get()))
                .build(),
            view("Button")
                .key("inc")
                .prop("title", "Inc")
                .prop("pressed", pressed.get())
                .event("pointer_down", on_pointer_down([pressed]() mutable {
                         pressed.set(true);
                         capture_pointer();
                       }))
                .event("pointer_up", on_pointer_up([count, pressed]() mutable {
                         pressed.set(false);
                         release_pointer();
                         count.set(count.get() + 1);
                       }))
                .build(),
        })
        .build();
  };

  return std::make_unique<ViewInstance>(std::move(fn));
}

int main(int argc, const char *argv[]) {
  bool use_terminal = false;
  for (int i = 1; i < argc; ++i) {
    const char *arg = argv ? argv[i] : nullptr;
    if (!arg) {
      continue;
    }
    if (std::strcmp(arg, "--terminal") == 0 || std::strcmp(arg, "terminal") == 0) {
      use_terminal = true;
    }
  }

  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    DuorouAppDelegate *delegate = [DuorouAppDelegate new];
    [NSApp setDelegate:delegate];

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
      return 1;
    }

    auto instance = make_instance(use_terminal);

    NSRect frame = NSMakeRect(0, 0, 640, 480);
    NSWindowStyleMask style = NSWindowStyleMaskTitled |
                              NSWindowStyleMaskClosable |
                              NSWindowStyleMaskResizable;

    NSWindow *window =
        [[NSWindow alloc] initWithContentRect:frame
                                    styleMask:style
                                      backing:NSBackingStoreBuffered
                                        defer:NO];
    window.delegate = delegate;

    DuorouMetalView *view =
        [[DuorouMetalView alloc] initWithFrame:frame
                                        device:device
                                      instance:instance.get()];

    [window setContentView:view];
    [window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];

    [NSApp run];
  }

  return 0;
}
