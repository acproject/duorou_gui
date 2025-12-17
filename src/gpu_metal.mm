#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreText/CoreText.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include <duorou/ui/runtime.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

using namespace duorou::ui;

static NSString *duorou_msl_source() {
  return @"#include <metal_stdlib>\n"
          "using namespace metal;\n"
          "struct VertexColor { float2 pos; float4 color; };\n"
          "struct VSColorOut { float4 pos [[position]]; float4 color; };\n"
          "vertex VSColorOut duorou_vertex_color(const device VertexColor* vtx "
          "[[buffer(0)]], uint vid [[vertex_id]]) {\n"
          "  VSColorOut o;\n"
          "  o.pos = float4(vtx[vid].pos, 0.0, 1.0);\n"
          "  o.color = vtx[vid].color;\n"
          "  return o;\n"
          "}\n"
          "fragment float4 duorou_fragment_color(VSColorOut in [[stage_in]]) "
          "{\n"
          "  return in.color;\n"
          "}\n"
          "struct VertexTex { float2 pos; float2 uv; float4 color; };\n"
          "struct VSTexOut { float4 pos [[position]]; float2 uv; float4 color; "
          "};\n"
          "vertex VSTexOut duorou_vertex_tex(const device VertexTex* vtx "
          "[[buffer(0)]], uint vid [[vertex_id]]) {\n"
          "  VSTexOut o;\n"
          "  o.pos = float4(vtx[vid].pos, 0.0, 1.0);\n"
          "  o.uv = vtx[vid].uv;\n"
          "  o.color = vtx[vid].color;\n"
          "  return o;\n"
          "}\n"
          "fragment float4 duorou_fragment_tex(VSTexOut in [[stage_in]], "
          "texture2d<float> tex [[texture(0)]], sampler s [[sampler(0)]]) {\n"
          "  float a = tex.sample(s, in.uv).a;\n"
          "  float ao = a * in.color.a;\n"
          "  return float4(in.color.rgb * ao, ao);\n"
          "}\n";
}

typedef struct {
  float x;
  float y;
  float _pad0;
  float _pad1;
  float r;
  float g;
  float b;
  float a;
} VertexColor;

typedef struct {
  float x;
  float y;
  float u;
  float v;
  float r;
  float g;
  float b;
  float a;
} VertexTex;

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

  id<MTLFunction> vtxColor = [lib newFunctionWithName:@"duorou_vertex_color"];
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

  id<MTLFunction> vtxTex = [lib newFunctionWithName:@"duorou_vertex_tex"];
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

  std::vector<VertexColor> colorVertices;
  struct TextBatch {
    std::vector<VertexTex> vertices;
    id<MTLTexture> texture;
  };
  std::vector<TextBatch> textBatches;

  const auto &ops = self.instance->render_ops();
  for (const auto &op : ops) {
    std::visit(
        [&](const auto &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, DrawRect>) {
            float x = v.rect.x;
            float y = v.rect.y;
            float w = v.rect.w;
            float h = v.rect.h;

            float x0 = (x / vw) * 2.0f - 1.0f;
            float y0 = 1.0f - (y / vh) * 2.0f;
            float x1 = ((x + w) / vw) * 2.0f - 1.0f;
            float y1 = 1.0f - ((y + h) / vh) * 2.0f;

            float r = v.fill.r / 255.0f;
            float g = v.fill.g / 255.0f;
            float b = v.fill.b / 255.0f;
            float a = v.fill.a / 255.0f;

            VertexColor v0{x0, y0, 0.0f, 0.0f, r, g, b, a};
            VertexColor v1{x1, y0, 0.0f, 0.0f, r, g, b, a};
            VertexColor v2{x0, y1, 0.0f, 0.0f, r, g, b, a};
            VertexColor v3{x1, y1, 0.0f, 0.0f, r, g, b, a};

            colorVertices.push_back(v0);
            colorVertices.push_back(v1);
            colorVertices.push_back(v2);
            colorVertices.push_back(v2);
            colorVertices.push_back(v1);
            colorVertices.push_back(v3);
          } else if constexpr (std::is_same_v<T, DrawText>) {
            if (v.text.empty()) {
              return;
            }

            NSString *key =
                [[NSString alloc] initWithBytes:v.text.data()
                                         length:v.text.size()
                                       encoding:NSUTF8StringEncoding];
            if (!key) {
              return;
            }

            NSValue *sizeVal = [self.textSizeCache objectForKey:key];
            id<MTLTexture> tex = [self.textCache objectForKey:key];
            CGSize size;
            if (!tex || !sizeVal) {
              size = CGSizeZero;
              tex = duorou_make_text_texture(view.device, key, 16.0, &size);
              if (!tex || size.width <= 0.0 || size.height <= 0.0) {
                return;
              }
              [self.textCache setObject:tex forKey:key];
              [self.textSizeCache setObject:[NSValue valueWithSize:size]
                                     forKey:key];
            } else {
              size = [sizeVal sizeValue];
            }

            float tex_w = static_cast<float>(size.width);
            float tex_h = static_cast<float>(size.height);
            if (tex_w <= 0.0f || tex_h <= 0.0f) {
              return;
            }

            float target_w = v.rect.w;
            float target_h = v.rect.h;
            float scale = std::min(target_w / tex_w, target_h / tex_h);
            if (scale <= 0.0f) {
              return;
            }

            float draw_w = tex_w * scale;
            float draw_h = tex_h * scale;
            float x = v.rect.x + (v.rect.w - draw_w) * 0.5f;
            float y = v.rect.y + (v.rect.h - draw_h) * 0.5f;

            float x0 = (x / vw) * 2.0f - 1.0f;
            float y0 = 1.0f - (y / vh) * 2.0f;
            float x1 = ((x + draw_w) / vw) * 2.0f - 1.0f;
            float y1 = 1.0f - ((y + draw_h) / vh) * 2.0f;

            float r = v.color.r / 255.0f;
            float g = v.color.g / 255.0f;
            float b = v.color.b / 255.0f;
            float a = v.color.a / 255.0f;

            TextBatch batch;
            batch.texture = tex;
            batch.vertices.reserve(6);

            batch.vertices.push_back(VertexTex{x0, y0, 0.0f, 0.0f, r, g, b, a});
            batch.vertices.push_back(VertexTex{x1, y0, 1.0f, 0.0f, r, g, b, a});
            batch.vertices.push_back(VertexTex{x0, y1, 0.0f, 1.0f, r, g, b, a});
            batch.vertices.push_back(VertexTex{x0, y1, 0.0f, 1.0f, r, g, b, a});
            batch.vertices.push_back(VertexTex{x1, y0, 1.0f, 0.0f, r, g, b, a});
            batch.vertices.push_back(VertexTex{x1, y1, 1.0f, 1.0f, r, g, b, a});

            textBatches.push_back(std::move(batch));
          }
        },
        op);
  }

  id<MTLCommandBuffer> cmd = [self.queue commandBuffer];
  id<MTLRenderCommandEncoder> enc =
      [cmd renderCommandEncoderWithDescriptor:pass];
  [enc setCullMode:MTLCullModeNone];
  [enc setFrontFacingWinding:MTLWindingClockwise];

  const CGSize ds = view.drawableSize;
  MTLViewport vp;
  vp.originX = 0.0;
  vp.originY = 0.0;
  vp.width = std::max(1.0, (double)ds.width);
  vp.height = std::max(1.0, (double)ds.height);
  vp.znear = 0.0;
  vp.zfar = 1.0;
  [enc setViewport:vp];

  MTLScissorRect sc;
  sc.x = 0;
  sc.y = 0;
  sc.width = (NSUInteger)std::max(1.0, (double)ds.width);
  sc.height = (NSUInteger)std::max(1.0, (double)ds.height);
  [enc setScissorRect:sc];
  if (!colorVertices.empty()) {
    [enc setRenderPipelineState:self.pipelineColor];
    [enc setVertexBytes:colorVertices.data()
                 length:colorVertices.size() * sizeof(VertexColor)
                atIndex:0];
    [enc drawPrimitives:MTLPrimitiveTypeTriangle
            vertexStart:0
            vertexCount:(NSUInteger)colorVertices.size()];
  }

  if (!textBatches.empty() && self.pipelineText && self.sampler) {
    [enc setRenderPipelineState:self.pipelineText];
    [enc setFragmentSamplerState:self.sampler atIndex:0];
    for (const auto &batch : textBatches) {
      if (!batch.texture) {
        continue;
      }
      if (batch.vertices.empty()) {
        continue;
      }
      [enc setFragmentTexture:batch.texture atIndex:0];
      [enc setVertexBytes:batch.vertices.data()
                   length:batch.vertices.size() * sizeof(VertexTex)
                  atIndex:0];
      [enc drawPrimitives:MTLPrimitiveTypeTriangle
              vertexStart:0
              vertexCount:(NSUInteger)batch.vertices.size()];
    }
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

static std::unique_ptr<ViewInstance> make_instance() {
  auto count = state<std::int64_t>(0);
  auto pressed = state<bool>(false);

  auto fn = [count, pressed]() {
    return Column({
        view("Text")
            .prop("value", std::string("Count: ") + std::to_string(count.get()))
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
    });
  };

  return std::make_unique<ViewInstance>(std::move(fn));
}

int main(int argc, const char *argv[]) {
  (void)argc;
  (void)argv;

  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    DuorouAppDelegate *delegate = [DuorouAppDelegate new];
    [NSApp setDelegate:delegate];

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
      return 1;
    }

    auto instance = make_instance();

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
