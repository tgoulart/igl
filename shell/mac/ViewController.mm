/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#import "ViewController.h"

#include <Carbon/Carbon.h>

#import "GLView.h"
#import "MetalView.h"
#import "VulkanView.h"
// @fb-only

#import <igl/Common.h>
#import <igl/IGL.h>
#if IGL_BACKEND_METAL
#import <igl/metal/HWDevice.h>
#import <igl/metal/Texture.h>
#import <igl/metal/macos/Device.h>
#endif
#if IGL_BACKEND_OPENGL
#import <igl/opengl/macos/Context.h>
#import <igl/opengl/macos/Device.h>
#import <igl/opengl/macos/HWDevice.h>
#endif
#include <shell/shared/platform/mac/PlatformMac.h>
#include <shell/shared/renderSession/AppParams.h>
#include <shell/shared/renderSession/DefaultSession.h>
#include <shell/shared/renderSession/ShellParams.h>
// @fb-only
// @fb-only
// @fb-only
// @fb-only
// @fb-only
#if IGL_BACKEND_VULKAN
#import <igl/vulkan/Device.h>
#import <igl/vulkan/HWDevice.h>
#import <igl/vulkan/VulkanContext.h>
#endif
#import <math.h>
#import <simd/simd.h>

using namespace igl;

@interface ViewController () {
  igl::BackendType backendType_;
  igl::shell::ShellParams shellParams_;
  CGRect frame_;
  CVDisplayLinkRef displayLink_; // For OpenGL only
  id<CAMetalDrawable> currentDrawable_;
  id<MTLTexture> depthStencilTexture_;
  std::shared_ptr<igl::shell::Platform> shellPlatform_;
  std::unique_ptr<igl::shell::RenderSession> session_;
  bool preferLatestVersion_;
  int majorVersion_;
  int minorVersion_;
  float kMouseSpeed_;
}
@end

@implementation ViewController

///--------------------------------------
/// MARK: - Init
///--------------------------------------

- (instancetype)initWithFrame:(CGRect)frame
                  backendType:(igl::BackendType)backendType
          preferLatestVersion:(bool)preferLatestVersion {
  self = [super initWithNibName:nil bundle:nil];
  if (!self)
    return self;

  backendType_ = backendType;
  shellParams_ = igl::shell::ShellParams();
  frame.size.width = shellParams_.viewportSize.x;
  frame.size.height = shellParams_.viewportSize.y;
  frame_ = frame;
  kMouseSpeed_ = 0.0005;
  currentDrawable_ = nil;
  depthStencilTexture_ = nil;
  preferLatestVersion_ = preferLatestVersion;

  return self;
}

- (instancetype)initWithFrame:(CGRect)frame
                  backendType:(igl::BackendType)backendType
                 majorVersion:(int)majorVersion
                 minorVersion:(int)minorVersion {
  majorVersion_ = majorVersion;
  minorVersion_ = minorVersion;
  return [self initWithFrame:frame backendType:backendType preferLatestVersion:false];
}

- (void)initModule {
}

- (void)teardown {
  session_ = nullptr;
  shellPlatform_ = nullptr;
}

- (void)render {
  if (session_ == nullptr) {
    return;
  }

  shellParams_.viewportSize = glm::vec2(self.view.frame.size.width, self.view.frame.size.height);
  shellParams_.viewportScale = self.view.window.backingScaleFactor;
  session_->setShellParams(shellParams_);
  // process user input
  shellPlatform_->getInputDispatcher().processEvents();

  // surface textures
  igl::SurfaceTextures surfaceTextures = igl::SurfaceTextures{
      [self createTextureFromNativeDrawable], [self createTextureFromNativeDepth]};
  IGL_ASSERT(surfaceTextures.color != nullptr && surfaceTextures.depth != nullptr);
  const auto& dims = surfaceTextures.color->getDimensions();
  shellParams_.nativeSurfaceDimensions = glm::ivec2{dims.width, dims.height};

  // update retina scale
  float pixelsPerPoint = shellParams_.nativeSurfaceDimensions.x / shellParams_.viewportSize.x;
  session_->setPixelsPerPoint(pixelsPerPoint);
// @fb-only
  // @fb-only
  IGL_ASSERT(fabs(shellParams_.nativeSurfaceDimensions.y / shellParams_.viewportSize.y -
                  pixelsPerPoint) < FLT_EPSILON);
// @fb-only
  // draw
  session_->update(std::move(surfaceTextures));
  if (session_->appParams().exitRequested)
    [[NSApplication sharedApplication] terminate:nil];
}

- (void)loadView {
  // We don't care about the device type, just
  // return something that works
  HWDeviceQueryDesc queryDesc(HWDeviceType::Unknown);

  switch (backendType_) {
#if IGL_BACKEND_METAL
  case igl::BackendType::Metal: {
    auto hwDevices = metal::HWDevice().queryDevices(queryDesc, nullptr);
    auto device = metal::HWDevice().create(hwDevices[0], nullptr);

    auto d = static_cast<igl::metal::macos::Device*>(device.get())->get();

    auto metalView = [[MetalView alloc] initWithFrame:frame_ device:d];
    metalView.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;

    metalView.delegate = self;

    metalView.colorPixelFormat =
        metal::Texture::textureFormatToMTLPixelFormat(shellParams_.defaultColorFramebufferFormat);
    // !!!WARNING must be called after setting the colorPixelFormat WARNING!!!
    //
    // Disables OS Level Color Management to achieve parity with OpenGL
    // Without this, the OS will try to "color convert" the resulting framebuffer
    // to the monitor's color profile which is fine but doesn't seem to be
    // supported under OpenGL which results in discrepancies between Metal
    // and OpenGL. This feature is equivalent to using MoltenVK colorSpace
    // VK_COLOR_SPACE_PASS_THROUGH_EXT
    // Must be called after set colorPixelFormat since it resets the colorspace
    metalView.colorspace = nil;

    metalView.framebufferOnly = NO;
    [metalView setViewController:self];
    self.view = metalView;
    shellPlatform_ = std::make_shared<igl::shell::PlatformMac>(std::move(device));
    break;
  }
#endif

#if IGL_BACKEND_OPENGL
  case igl::BackendType::OpenGL: {
    NSOpenGLPixelFormat* pixelFormat;
    if (preferLatestVersion_) {
      static NSOpenGLPixelFormatAttribute attributes[] = {
          NSOpenGLPFADoubleBuffer,
          NSOpenGLPFAAllowOfflineRenderers,
          NSOpenGLPFAMultisample,
          1,
          NSOpenGLPFASampleBuffers,
          1,
          NSOpenGLPFASamples,
          4,
          NSOpenGLPFAColorSize,
          32,
          NSOpenGLPFADepthSize,
          24,
          NSOpenGLPFAOpenGLProfile,
          NSOpenGLProfileVersion4_1Core,
          0,
      };
      pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
      IGL_ASSERT_MSG(pixelFormat, "Requested attributes not supported");
    } else {
      if (majorVersion_ >= 4 && minorVersion_ >= 1) {
        static NSOpenGLPixelFormatAttribute attributes[] = {
            NSOpenGLPFADoubleBuffer,
            NSOpenGLPFAAllowOfflineRenderers,
            NSOpenGLPFAMultisample,
            1,
            NSOpenGLPFASampleBuffers,
            1,
            NSOpenGLPFASamples,
            4,
            NSOpenGLPFAColorSize,
            32,
            NSOpenGLPFADepthSize,
            24,
            NSOpenGLPFAOpenGLProfile,
            NSOpenGLProfileVersion4_1Core,
            0,
        };
        pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
      } else if (majorVersion_ >= 3 && minorVersion_ >= 2) {
        static NSOpenGLPixelFormatAttribute attributes[] = {
            NSOpenGLPFADoubleBuffer,
            NSOpenGLPFAAllowOfflineRenderers,
            NSOpenGLPFAMultisample,
            1,
            NSOpenGLPFASampleBuffers,
            1,
            NSOpenGLPFASamples,
            4,
            NSOpenGLPFAColorSize,
            32,
            NSOpenGLPFADepthSize,
            24,
            NSOpenGLPFAOpenGLProfile,
            NSOpenGLProfileVersion3_2Core,
            0,
        };
        pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
      } else {
        static NSOpenGLPixelFormatAttribute attributes[] = {
            NSOpenGLPFADoubleBuffer,
            NSOpenGLPFAAllowOfflineRenderers,
            NSOpenGLPFAMultisample,
            1,
            NSOpenGLPFASampleBuffers,
            1,
            NSOpenGLPFASamples,
            4,
            NSOpenGLPFAColorSize,
            32,
            NSOpenGLPFADepthSize,
            24,
            NSOpenGLPFAOpenGLProfile,
            NSOpenGLProfileVersionLegacy,
            0,
        };
        pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
      }
    }
    auto openGLView = [[GLView alloc] initWithFrame:frame_ pixelFormat:pixelFormat];
    igl::Result result;
    auto context = igl::opengl::macos::Context::createContext(openGLView.openGLContext, &result);
    IGL_ASSERT(result.isOk());
    shellPlatform_ = std::make_shared<igl::shell::PlatformMac>(
        opengl::macos::HWDevice().createWithContext(std::move(context), nullptr));
    self.view = openGLView;
    break;
  }
#endif

#if IGL_BACKEND_VULKAN
  case igl::BackendType::Vulkan: {
    auto vulkanView = [[VulkanView alloc] initWithFrame:frame_];

    self.view = vulkanView;

    // vulkanView.wantsLayer =
    // YES; // Back the view with a layer created by the makeBackingLayer method.

    const auto properties =
        igl::TextureFormatProperties::fromTextureFormat(shellParams_.defaultColorFramebufferFormat);

    igl::vulkan::VulkanContextConfig vulkanContextConfig;
    vulkanContextConfig.terminateOnValidationError = true;
    vulkanContextConfig.enhancedShaderDebugging = false;
    vulkanContextConfig.swapChainColorSpace = properties.isSRGB() ? igl::ColorSpace::SRGB_NONLINEAR
                                                                  : igl::ColorSpace::SRGB_LINEAR;

    auto context =
        igl::vulkan::HWDevice::createContext(vulkanContextConfig, (__bridge void*)vulkanView);
    auto devices = igl::vulkan::HWDevice::queryDevices(
        *context.get(), igl::HWDeviceQueryDesc(igl::HWDeviceType::DiscreteGpu), nullptr);
    if (devices.empty()) {
      devices = igl::vulkan::HWDevice::queryDevices(
          *context.get(), igl::HWDeviceQueryDesc(igl::HWDeviceType::IntegratedGpu), nullptr);
    }
    auto device = igl::vulkan::HWDevice::create(std::move(context), devices[0], 0, 0);

    shellPlatform_ = std::make_shared<igl::shell::PlatformMac>(std::move(device));
    [vulkanView prepareVulkan:shellPlatform_];
    break;
  }
#endif

// @fb-only
  // @fb-only
    // @fb-only

    // @fb-only
        // @fb-only

    // @fb-only
    // @fb-only
        // @fb-only
        // @fb-only
        // @fb-only
        // @fb-only

    // @fb-only

    // @fb-only

    // @fb-only
    // @fb-only
  // @fb-only
// @fb-only

  default: {
    IGL_ASSERT_NOT_IMPLEMENTED();
    break;
  }
  }

  session_ = igl::shell::createDefaultRenderSession(shellPlatform_);
  IGL_ASSERT_MSG(session_, "createDefaultRenderSession() must return a valid session");
  // Get initial native surface dimensions
  shellParams_.nativeSurfaceDimensions = glm::ivec2(2048, 1536);
  session_->initialize();
}

- (void)viewDidAppear {
  if ([self.view isKindOfClass:[MetalView class]]) {
    MetalView* v = (MetalView*)self.view;
    v.paused = NO;
  } else if ([self.view isKindOfClass:[GLView class]]) {
    GLView* v = (GLView*)self.view;
    [v startTimer];
  }
}

- (void)viewWillDisappear {
  if ([self.view isKindOfClass:[MetalView class]]) {
    MetalView* v = (MetalView*)self.view;
    v.paused = YES;
  } else if ([self.view isKindOfClass:[GLView class]]) {
    GLView* v = (GLView*)self.view;
    [v stopTimer];
  }
}

- (void)drawInMTKView:(nonnull MTKView*)view {
  @autoreleasepool {
    currentDrawable_ = view.currentDrawable;
    depthStencilTexture_ = view.depthStencilTexture;
    [self render];
    currentDrawable_ = nil;
  }
}

- (void)mtkView:(nonnull MTKView*)view drawableSizeWillChange:(CGSize)size {
  frame_.size = size;
}

- (void)mtkView:(nonnull MTKView*)view drawableSizeDidChange:(CGSize)size {
}

- (std::shared_ptr<igl::ITexture>)createTextureFromNativeDrawable {
  switch (backendType_) {
#if IGL_BACKEND_METAL
  case igl::BackendType::Metal: {
    auto& device = shellPlatform_->getDevice();
    auto platformDevice = device.getPlatformDevice<igl::metal::PlatformDevice>();
    IGL_ASSERT(platformDevice);
    IGL_ASSERT(currentDrawable_ != nil);
    auto texture = platformDevice->createTextureFromNativeDrawable(currentDrawable_, nullptr);
    return texture;
  }
#endif

#if IGL_BACKEND_OPENGL
  case igl::BackendType::OpenGL: {
    auto& device = shellPlatform_->getDevice();
    auto platformDevice = device.getPlatformDevice<igl::opengl::macos::PlatformDevice>();
    IGL_ASSERT(platformDevice);
    auto texture = platformDevice->createTextureFromNativeDrawable(nullptr);
    return texture;
  }
#endif

#if IGL_BACKEND_VULKAN
  case igl::BackendType::Vulkan: {
    auto& device = shellPlatform_->getDevice();
    auto platformDevice = device.getPlatformDevice<igl::vulkan::PlatformDevice>();
    IGL_ASSERT(platformDevice);
    auto texture = platformDevice->createTextureFromNativeDrawable(nullptr);
    return texture;
  }
#endif

// @fb-only
  // @fb-only
    // @fb-only
    // @fb-only
    // @fb-only
    // @fb-only
    // @fb-only
  // @fb-only
// @fb-only

  default: {
    IGL_ASSERT_NOT_IMPLEMENTED();
    return nullptr;
  }
  }
}

- (std::shared_ptr<igl::ITexture>)createTextureFromNativeDepth {
  switch (backendType_) {
#if IGL_BACKEND_METAL
  case igl::BackendType::Metal: {
    auto& device = shellPlatform_->getDevice();
    auto platformDevice = device.getPlatformDevice<igl::metal::PlatformDevice>();
    IGL_ASSERT(platformDevice);
    auto texture = platformDevice->createTextureFromNativeDepth(depthStencilTexture_, nullptr);
    return texture;
  }
#endif

#if IGL_BACKEND_OPENGL
  case igl::BackendType::OpenGL: {
    auto& device = shellPlatform_->getDevice();
    auto platformDevice = device.getPlatformDevice<igl::opengl::macos::PlatformDevice>();
    IGL_ASSERT(platformDevice);
    auto texture = platformDevice->createTextureFromNativeDepth(nullptr);
    return texture;
  }
#endif

#if IGL_BACKEND_VULKAN
  case igl::BackendType::Vulkan: {
    auto& device = static_cast<igl::vulkan::Device&>(shellPlatform_->getDevice());
    auto extents = device.getVulkanContext().getSwapchainExtent();
    auto platformDevice =
        shellPlatform_->getDevice().getPlatformDevice<igl::vulkan::PlatformDevice>();

    IGL_ASSERT(platformDevice);
    auto texture =
        platformDevice->createTextureFromNativeDepth(extents.width, extents.height, nullptr);
    return texture;
  }
#endif

// @fb-only
  // @fb-only
    // @fb-only
        // @fb-only
        // @fb-only
        // @fb-only
  // @fb-only
// @fb-only

  default: {
    IGL_ASSERT_NOT_IMPLEMENTED();
    return nullptr;
  }
  }
}

- (BOOL)acceptsFirstResponder {
  return YES;
}

- (NSPoint)locationForEvent:(NSEvent*)event {
  NSRect contentRect = self.view.frame;
  NSPoint pos = [event locationInWindow];
  return NSMakePoint(pos.x, contentRect.size.height - pos.y);
}

// See https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_osx.mm
static std::optional<igl::shell::Key> convertToShellKey(int key_code) {
  switch (key_code) {
    // Control
  case kVK_Escape:
    return igl::shell::Key::Escape;
  case kVK_Shift:
    return igl::shell::Key::LeftShift;
  case kVK_Control:
    return igl::shell::Key::LeftCtrl;
  case kVK_Option:
    return igl::shell::Key::LeftAlt;
  case kVK_Command:
    return igl::shell::Key::LeftSuper;
  case kVK_RightShift:
    return igl::shell::Key::RightShift;
  case kVK_RightControl:
    return igl::shell::Key::RightCtrl;
  case kVK_RightOption:
    return igl::shell::Key::RightAlt;
  case kVK_RightCommand:
    return igl::shell::Key::RightSuper;

    // Navigation
  case kVK_LeftArrow:
    return igl::shell::Key::LeftArrow;
  case kVK_RightArrow:
    return igl::shell::Key::RightArrow;
  case kVK_DownArrow:
    return igl::shell::Key::DownArrow;
  case kVK_UpArrow:
    return igl::shell::Key::UpArrow;
  case kVK_PageUp:
    return igl::shell::Key::PageUp;
  case kVK_PageDown:
    return igl::shell::Key::PageDown;
  case kVK_Home:
    return igl::shell::Key::Home;
  case kVK_End:
    return igl::shell::Key::End;
  case kVK_Help:
    return igl::shell::Key::Insert;
  case kVK_ForwardDelete:
    return igl::shell::Key::Delete;

    // Typing
  case kVK_Space:
    return igl::shell::Key::Space;
  case kVK_Return:
    return igl::shell::Key::Enter;
  case kVK_Delete:
    return igl::shell::Key::Backspace;
  case kVK_Tab:
    return igl::shell::Key::Tab;
  case kVK_CapsLock:
    return igl::shell::Key::CapsLock;

    // Characters
  case kVK_ANSI_A:
    return igl::shell::Key::A;
  case kVK_ANSI_B:
    return igl::shell::Key::B;
  case kVK_ANSI_C:
    return igl::shell::Key::C;
  case kVK_ANSI_D:
    return igl::shell::Key::D;
  case kVK_ANSI_E:
    return igl::shell::Key::E;
  case kVK_ANSI_F:
    return igl::shell::Key::F;
  case kVK_ANSI_G:
    return igl::shell::Key::G;
  case kVK_ANSI_H:
    return igl::shell::Key::H;
  case kVK_ANSI_I:
    return igl::shell::Key::I;
  case kVK_ANSI_J:
    return igl::shell::Key::J;
  case kVK_ANSI_K:
    return igl::shell::Key::K;
  case kVK_ANSI_L:
    return igl::shell::Key::L;
  case kVK_ANSI_M:
    return igl::shell::Key::M;
  case kVK_ANSI_N:
    return igl::shell::Key::N;
  case kVK_ANSI_O:
    return igl::shell::Key::O;
  case kVK_ANSI_P:
    return igl::shell::Key::P;
  case kVK_ANSI_Q:
    return igl::shell::Key::Q;
  case kVK_ANSI_R:
    return igl::shell::Key::R;
  case kVK_ANSI_S:
    return igl::shell::Key::S;
  case kVK_ANSI_T:
    return igl::shell::Key::T;
  case kVK_ANSI_U:
    return igl::shell::Key::U;
  case kVK_ANSI_V:
    return igl::shell::Key::V;
  case kVK_ANSI_W:
    return igl::shell::Key::W;
  case kVK_ANSI_Y:
    return igl::shell::Key::Y;
  case kVK_ANSI_X:
    return igl::shell::Key::X;
  case kVK_ANSI_Z:
    return igl::shell::Key::Z;

  // Digits
  case kVK_ANSI_0:
    return igl::shell::Key::Zero;
  case kVK_ANSI_1:
    return igl::shell::Key::One;
  case kVK_ANSI_2:
    return igl::shell::Key::Two;
  case kVK_ANSI_3:
    return igl::shell::Key::Three;
  case kVK_ANSI_4:
    return igl::shell::Key::Four;
  case kVK_ANSI_5:
    return igl::shell::Key::Five;
  case kVK_ANSI_6:
    return igl::shell::Key::Six;
  case kVK_ANSI_7:
    return igl::shell::Key::Seven;
  case kVK_ANSI_8:
    return igl::shell::Key::Eight;
  case kVK_ANSI_9:
    return igl::shell::Key::Nine;

    // Punctuation
  case kVK_ANSI_Grave:
    return igl::shell::Key::GraveAccent;
  case kVK_ANSI_Minus:
    return igl::shell::Key::Minus;
  case kVK_ANSI_Equal:
    return igl::shell::Key::Equal;
  case kVK_ANSI_RightBracket:
    return igl::shell::Key::RightBracket;
  case kVK_ANSI_LeftBracket:
    return igl::shell::Key::LeftBracket;
  case kVK_ANSI_Backslash:
    return igl::shell::Key::Backslash;
  case kVK_ANSI_Semicolon:
    return igl::shell::Key::Semicolon;
  case kVK_ANSI_Quote:
    return igl::shell::Key::Apostrophe;
  case kVK_ANSI_Comma:
    return igl::shell::Key::Comma;
  case kVK_ANSI_Period:
    return igl::shell::Key::Period;
  case kVK_ANSI_Slash:
    return igl::shell::Key::Slash;

    // Function
  case kVK_F1:
    return igl::shell::Key::F1;
  case kVK_F2:
    return igl::shell::Key::F2;
  case kVK_F3:
    return igl::shell::Key::F3;
  case kVK_F4:
    return igl::shell::Key::F4;
  case kVK_F5:
    return igl::shell::Key::F5;
  case kVK_F6:
    return igl::shell::Key::F6;
  case kVK_F7:
    return igl::shell::Key::F7;
  case kVK_F8:
    return igl::shell::Key::F8;
  case kVK_F9:
    return igl::shell::Key::F9;
  case kVK_F10:
    return igl::shell::Key::F10;
  case kVK_F11:
    return igl::shell::Key::F11;
  case kVK_F12:
    return igl::shell::Key::F12;

  // Keypad
  case kVK_ANSI_KeypadClear:
    return igl::shell::Key::NumLock;
  case kVK_ANSI_Keypad0:
    return igl::shell::Key::Keypad0;
  case kVK_ANSI_Keypad1:
    return igl::shell::Key::Keypad1;
  case kVK_ANSI_Keypad2:
    return igl::shell::Key::Keypad2;
  case kVK_ANSI_Keypad3:
    return igl::shell::Key::Keypad3;
  case kVK_ANSI_Keypad4:
    return igl::shell::Key::Keypad4;
  case kVK_ANSI_Keypad5:
    return igl::shell::Key::Keypad5;
  case kVK_ANSI_Keypad6:
    return igl::shell::Key::Keypad6;
  case kVK_ANSI_Keypad7:
    return igl::shell::Key::Keypad7;
  case kVK_ANSI_Keypad8:
    return igl::shell::Key::Keypad8;
  case kVK_ANSI_Keypad9:
    return igl::shell::Key::Keypad9;
  case kVK_ANSI_KeypadDecimal:
    return igl::shell::Key::KeypadDecimal;
  case kVK_ANSI_KeypadDivide:
    return igl::shell::Key::KeypadDivide;
  case kVK_ANSI_KeypadMultiply:
    return igl::shell::Key::KeypadMultiply;
  case kVK_ANSI_KeypadMinus:
    return igl::shell::Key::KeypadSubtract;
  case kVK_ANSI_KeypadPlus:
    return igl::shell::Key::KeypadAdd;
  case kVK_ANSI_KeypadEnter:
    return igl::shell::Key::KeypadEnter;
  case kVK_ANSI_KeypadEquals:
    return igl::shell::Key::KeypadEqual;

  // Esoteric
  case 0x6E:
    return igl::shell::Key::Menu;

  default:
    return {};
  }
}

- (void)keyUp:(NSEvent*)event {
  auto shellKey = convertToShellKey(event.keyCode);
  if (!shellKey.has_value()) {
    return;
  }
  shellPlatform_->getInputDispatcher().queueEvent(igl::shell::KeyEvent(false, shellKey.value()));
}

- (void)keyDown:(NSEvent*)event {
  auto shellKey = convertToShellKey(event.keyCode);
  if (!shellKey.has_value()) {
    return;
  }
  shellPlatform_->getInputDispatcher().queueEvent(igl::shell::KeyEvent(true, shellKey.value()));
}

- (void)mouseDown:(NSEvent*)event {
  NSPoint curPoint = [self locationForEvent:event];
  shellPlatform_->getInputDispatcher().queueEvent(
      igl::shell::MouseButtonEvent(igl::shell::MouseButton::Left, true, curPoint.x, curPoint.y));
}

- (void)rightMouseDown:(NSEvent*)event {
  NSPoint curPoint = [self locationForEvent:event];
  shellPlatform_->getInputDispatcher().queueEvent(
      igl::shell::MouseButtonEvent(igl::shell::MouseButton::Right, true, curPoint.x, curPoint.y));
}

- (void)otherMouseDown:(NSEvent*)event {
  NSPoint curPoint = [self locationForEvent:event];
  shellPlatform_->getInputDispatcher().queueEvent(
      igl::shell::MouseButtonEvent(igl::shell::MouseButton::Middle, true, curPoint.x, curPoint.y));
}

- (void)mouseUp:(NSEvent*)event {
  NSPoint curPoint = [self locationForEvent:event];
  shellPlatform_->getInputDispatcher().queueEvent(
      igl::shell::MouseButtonEvent(igl::shell::MouseButton::Left, false, curPoint.x, curPoint.y));
}

- (void)rightMouseUp:(NSEvent*)event {
  NSPoint curPoint = [self locationForEvent:event];
  shellPlatform_->getInputDispatcher().queueEvent(
      igl::shell::MouseButtonEvent(igl::shell::MouseButton::Right, false, curPoint.x, curPoint.y));
}

- (void)otherMouseUp:(NSEvent*)event {
  NSPoint curPoint = [self locationForEvent:event];
  shellPlatform_->getInputDispatcher().queueEvent(
      igl::shell::MouseButtonEvent(igl::shell::MouseButton::Middle, false, curPoint.x, curPoint.y));
}

- (void)mouseMoved:(NSEvent*)event {
  NSPoint curPoint = [self locationForEvent:event];
  shellPlatform_->getInputDispatcher().queueEvent(
      igl::shell::MouseMotionEvent(curPoint.x, curPoint.y, event.deltaX, event.deltaY));
}

- (void)mouseDragged:(NSEvent*)event {
  NSPoint curPoint = [self locationForEvent:event];
  shellPlatform_->getInputDispatcher().queueEvent(
      igl::shell::MouseMotionEvent(curPoint.x, curPoint.y, event.deltaX, event.deltaY));
}

- (void)rightMouseDragged:(NSEvent*)event {
  NSPoint curPoint = [self locationForEvent:event];
  shellPlatform_->getInputDispatcher().queueEvent(
      igl::shell::MouseMotionEvent(curPoint.x, curPoint.y, event.deltaX, event.deltaY));
}

- (void)otherMouseDragged:(NSEvent*)event {
  NSPoint curPoint = [self locationForEvent:event];
  shellPlatform_->getInputDispatcher().queueEvent(
      igl::shell::MouseMotionEvent(curPoint.x, curPoint.y, event.deltaX, event.deltaY));
}

- (void)scrollWheel:(NSEvent*)event {
  shellPlatform_->getInputDispatcher().queueEvent(igl::shell::MouseWheelEvent(
      event.scrollingDeltaX * kMouseSpeed_, event.scrollingDeltaY * kMouseSpeed_));
}

- (CGRect)frame {
  return frame_;
}

@end
