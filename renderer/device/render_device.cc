// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/device/render_device.h"

#include "SDL3/SDL_hints.h"
#include "SDL3/SDL_loadso.h"
#include "SDL3/SDL_video.h"
#include "magic_enum/magic_enum.hpp"

#include "Graphics/GraphicsAccessories/interface/GraphicsAccessories.hpp"
#if GL_SUPPORTED || GLES_SUPPORTED
#include "Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h"
#endif  //! GL_SUPPORTED || GLES_SUPPORTED
#if VULKAN_SUPPORTED
#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#endif  // !VULKAN_SUPPORTED
#if D3D11_SUPPORTED
#include "Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h"
#endif  //! D3D11_SUPPORTED
#if D3D12_SUPPORTED
#include "Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h"
#endif  // !D3D12_SUPPORTED
#if WEBGPU_SUPPORTED
#include "Graphics/GraphicsEngineWebGPU/interface/EngineFactoryWebGPU.h"
#endif  //! WEBGPU_SUPPORTED
#include "Primitives/interface/DebugOutput.h"
#include <third_party/DiligentCore/Platforms/interface/NativeWindow.h>

#if PLATFORM_WEB
#include <emscripten/html5_webgpu.h>
#endif

#include "base/debug/logging.h"
#include "ui/widget/widget.h"

#if defined(OS_ANDROID)
#include "Graphics/GraphicsEngineOpenGL/interface/RenderDeviceGLES.h"
#elif defined(OS_MACOSX)
#include <objc/objc.h>
#include <objc/message.h>
#include <objc/runtime.h>
#endif

namespace renderer {

//--------------------------------------------------------------------------------------
// Internal Helper Functions
//--------------------------------------------------------------------------------------

namespace {

void DILIGENT_CALL_TYPE
DebugMessageOutputFunc(Diligent::DEBUG_MESSAGE_SEVERITY Severity,
                       const Diligent::Char* Message,
                       const Diligent::Char* Function,
                       const Diligent::Char* File,
                       int Line) {
  if (Function)
    LOG(INFO) << "[Renderer] Function " << Function << ":";

  switch (Severity) {
    default:
    case Diligent::DEBUG_MESSAGE_SEVERITY_INFO:
      LOG(INFO) << "[Renderer] " << Message;
      break;
    case Diligent::DEBUG_MESSAGE_SEVERITY_WARNING:
      LOG(WARNING) << "[Renderer] " << Message;
      break;
    case Diligent::DEBUG_MESSAGE_SEVERITY_ERROR:
      LOG(ERROR) << "[Renderer] " << Message;
      break;
    case Diligent::DEBUG_MESSAGE_SEVERITY_FATAL_ERROR:
      LOG(FATAL) << "[Renderer] " << Message;
      break;
  }
}

}  // namespace

RenderDevice::CreateDeviceResult RenderDevice::Create(
    base::WeakPtr<ui::Widget> window_target,
    DriverType driver_type,
    bool validation) {
  // Setup debugging output
  Diligent::SetDebugMessageCallback(DebugMessageOutputFunc);

  // Setup native window
  Diligent::NativeWindow native_window;
  SDL_PropertiesID window_properties =
      SDL_GetWindowProperties(window_target->AsSDLWindow());

  // Setup specific platform window handle
  SDL_GLContext glcontext = nullptr;
#if defined(OS_WIN)
  native_window.hWnd = SDL_GetPointerProperty(
      window_properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif defined(OS_LINUX)
  // X11 native window properties (Wayland windows won't have these)
  void* xdisplay = SDL_GetPointerProperty(
      window_properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
  int64_t xwindow = SDL_GetNumberProperty(window_properties,
                                          SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);

  // Get XCB connection from Xlib for Vulkan (X11 only)
  void* xcb_connection = nullptr;
  if (xdisplay) {
    SDL_SharedObject* xlib_xcb_library = SDL_LoadObject("libX11-xcb.so");
    if (!xlib_xcb_library)
      xlib_xcb_library = SDL_LoadObject("libX11-xcb.so.1");

    if (xlib_xcb_library) {
      using XGetXCBConnection = void* (*)(void*);
      auto xgetxcb_func = (XGetXCBConnection)SDL_LoadFunction(
          xlib_xcb_library, "XGetXCBConnection");
      if (xgetxcb_func)
        xcb_connection = xgetxcb_func(xdisplay);
    }

  }

  // Setup native window
  native_window.WindowId = static_cast<uint32_t>(xwindow);
  native_window.pDisplay = xdisplay;
  native_window.pXCBConnection = xcb_connection;

  // Diligent Engine requires X11 on Linux
  if (!xdisplay) {
    LOG(ERROR) << "[Renderer] X11 display not available, "
               << "Diligent Engine requires X11 on Linux. "
               << "Make sure X11/XWayland is running.";
    return CreateDeviceResult(nullptr, nullptr);
  }
#elif defined(OS_ANDROID)
  native_window.pAWindow = SDL_GetPointerProperty(
      window_properties, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);
#elif defined(OS_EMSCRIPTEN)
  native_window.pCanvasId = SDL_GetStringProperty(
      window_properties, SDL_PROP_WINDOW_EMSCRIPTEN_CANVAS_ID_STRING,
      "#canvas");
#elif defined(OS_MACOSX)
  glcontext = SDL_GL_CreateContext(window_target->AsSDLWindow());
  if (glcontext == nullptr) {
    LOG(ERROR) << "[Renderer] SDL_GL_CreateContext failed: " << SDL_GetError();
    return CreateDeviceResult(nullptr, nullptr);
  }
  if (!SDL_GL_MakeCurrent(window_target->AsSDLWindow(), glcontext)) {
    LOG(ERROR) << "[Renderer] SDL_GL_MakeCurrent failed: " << SDL_GetError();
    SDL_GL_DestroyContext(glcontext);
    return CreateDeviceResult(nullptr, nullptr);
  }

  void* ns_window = SDL_GetPointerProperty(
      window_properties, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
  native_window.pNSView = nullptr;
  if (ns_window) {
    using ContentViewGetter = void* (*)(void*, SEL);
    auto get_content_view =
        reinterpret_cast<ContentViewGetter>(objc_msgSend);
    native_window.pNSView =
        get_content_view(ns_window, sel_registerName("contentView"));
  }
#else
#error "Unsupport Platform"
#endif

// Backend priority per platform
#if defined(OS_WIN)
  static const DriverType kBackendPriority[] = {
      DriverType::D3D12,
      DriverType::VULKAN,
      DriverType::D3D11,
      DriverType::OPENGL,
  };
#elif defined(OS_LINUX)
  static const DriverType kBackendPriority[] = {
      DriverType::VULKAN,
      DriverType::OPENGL,
  };
#elif defined(OS_ANDROID)
  static const DriverType kBackendPriority[] = {
      DriverType::OPENGL,
      DriverType::VULKAN,
  };
#elif defined(OS_EMSCRIPTEN)
  static const DriverType kBackendPriority[] = {
      DriverType::WEBGPU,
      DriverType::OPENGL,
  };
#elif defined(OS_MACOSX)
  static const DriverType kBackendPriority[] = {
      DriverType::OPENGL,
  };
#else
#error "Unsupport Platform"
#endif

  // If the user-specified backend is not in the priority list, use the first priority
  bool user_backend_valid = false;
  for (auto p : kBackendPriority) {
    if (driver_type == p) {
      user_backend_valid = true;
      break;
    }
  }
  if (!user_backend_valid)
    driver_type = kBackendPriority[0];

  Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device;
  Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context;
  Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapchain;

  // Initialize driver descriptor
  Diligent::EngineCreateInfo engine_create_info;
  Diligent::SwapChainDesc swap_chain_desc;
#if D3D11_SUPPORTED || D3D12_SUPPORTED
  Diligent::FullScreenModeDesc fullscreen_mode_desc;
#endif

  // Setup renderer create info
  engine_create_info.EnableValidation = validation;
  if (engine_create_info.EnableValidation)
    LOG(INFO) << "[Renderer] Enable renderer validation.";

  // Requested features
  if (driver_type != DriverType::OPENGL)
    engine_create_info.Features.SeparablePrograms =
        Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
  engine_create_info.Features.ComputeShaders =
      Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
  engine_create_info.Features.ShaderFloat16 =
      Diligent::DEVICE_FEATURE_STATE_OPTIONAL;

  // Setup primary swapchain
  swap_chain_desc.ColorBufferFormat = Diligent::TEX_FORMAT_RGBA8_UNORM;
  swap_chain_desc.PreTransform = Diligent::SURFACE_TRANSFORM_OPTIMAL;
  swap_chain_desc.IsPrimary = Diligent::True;

  // Create device and fallback when error
  // Build effective priority: user's choice first, then platform defaults
  DriverType priority[8];
  size_t priority_size = 0;
  for (auto p : kBackendPriority)
    if (driver_type == p) {
      priority[priority_size++] = driver_type;
      break;
    }
  for (auto p : kBackendPriority)
    if (driver_type != p)
      priority[priority_size++] = p;

  for (size_t i = 0; i < priority_size; ++i) {
    driver_type = priority[i];
#if GL_SUPPORTED || GLES_SUPPORTED
    if (driver_type == DriverType::OPENGL) {
#if defined(OS_LINUX)
      if (!glcontext) {
        glcontext = SDL_GL_CreateContext(window_target->AsSDLWindow());
        if (!glcontext) {
          LOG(ERROR) << "[Renderer] SDL_GL_CreateContext failed: "
                     << SDL_GetError();
          continue;
        }
      }
      if (!SDL_GL_MakeCurrent(window_target->AsSDLWindow(), glcontext)) {
        LOG(ERROR) << "[Renderer] SDL_GL_MakeCurrent failed: "
                   << SDL_GetError();
        SDL_GL_DestroyContext(glcontext);
        glcontext = nullptr;
        continue;
      }
#endif
#if ENGINE_DLL
      auto GetEngineFactoryOpenGL = Diligent::LoadGraphicsEngineOpenGL();
#else
      using Diligent::GetEngineFactoryOpenGL;
#endif
      auto* factory = GetEngineFactoryOpenGL();

      Diligent::EngineGLCreateInfo gl_create_info(engine_create_info);
      gl_create_info.Window = native_window;

      factory->CreateDeviceAndSwapChainGL(gl_create_info, &device, &context,
                                          swap_chain_desc, &swapchain);
    }
#endif  // OPENGL_SUPPORT
#if VULKAN_SUPPORTED
    if (driver_type == DriverType::VULKAN) {
#if ENGINE_DLL
      auto GetEngineFactoryVk = Diligent::LoadGraphicsEngineVk();
#else
      using Diligent::GetEngineFactoryVk;
#endif
      auto* factory = GetEngineFactoryVk();

      Diligent::EngineVkCreateInfo vk_create_info(engine_create_info);
      vk_create_info.FeaturesVk.DynamicRendering =
          Diligent::DEVICE_FEATURE_STATE_OPTIONAL;

      factory->CreateDeviceAndContextsVk(vk_create_info, &device, &context);
      factory->CreateSwapChainVk(device, context, swap_chain_desc,
                                 native_window, &swapchain);
    }
#endif  // VULKAN_SUPPORT
#if D3D11_SUPPORTED
    if (driver_type == DriverType::D3D11) {
#if ENGINE_DLL
      auto GetEngineFactoryD3D11 = Diligent::LoadGraphicsEngineD3D11();
#else
      using Diligent::GetEngineFactoryD3D11;
#endif
      auto* pFactory = GetEngineFactoryD3D11();

      Diligent::EngineD3D11CreateInfo d3d11_create_info(engine_create_info);
      pFactory->CreateDeviceAndContextsD3D11(d3d11_create_info, &device,
                                             &context);
      pFactory->CreateSwapChainD3D11(device, context, swap_chain_desc,
                                     fullscreen_mode_desc, native_window,
                                     &swapchain);
    }
#endif  // D3D11_SUPPORT
#if D3D12_SUPPORTED
    if (driver_type == DriverType::D3D12) {
#if ENGINE_DLL
      auto GetEngineFactoryD3D12 = Diligent::LoadGraphicsEngineD3D12();
#else
      using Diligent::GetEngineFactoryD3D12;
#endif
      auto* pFactoryD3D12 = GetEngineFactoryD3D12();

      Diligent::EngineD3D12CreateInfo d3d12_create_info(engine_create_info);
      pFactoryD3D12->CreateDeviceAndContextsD3D12(d3d12_create_info, &device,
                                                  &context);
      pFactoryD3D12->CreateSwapChainD3D12(device, context, swap_chain_desc,
                                          fullscreen_mode_desc, native_window,
                                          &swapchain);
    }
#endif  // D3D12_SUPPORT
#if WEBGPU_SUPPORTED
    if (driver_type == DriverType::WEBGPU) {
#if ENGINE_DLL
      auto GetEngineFactoryWebGPU = Diligent::LoadGraphicsEngineWebGPU();
#else
      using Diligent::GetEngineFactoryWebGPU;
#endif
      auto* pFactoryWebGPU = GetEngineFactoryWebGPU();

      Diligent::EngineWebGPUCreateInfo webgpu_create_info(engine_create_info);
      webgpu_create_info.Features.AsyncShaderCompilation =
          Diligent::DEVICE_FEATURE_STATE_DISABLED;

#if PLATFORM_WEB
      WGPUInstance wgpuInstance = wgpuCreateInstance(nullptr);
      WGPUDevice wgpuDevice = emscripten_webgpu_get_device();
      pFactoryWebGPU->AttachToWebGPUDevice(wgpuInstance, nullptr, wgpuDevice,
                                           webgpu_create_info, &device,
                                           &context);
#else
      pFactoryWebGPU->CreateDeviceAndContextsWebGPU(webgpu_create_info, &device,
                                                    &context);
#endif

      pFactoryWebGPU->CreateSwapChainWebGPU(device, context, swap_chain_desc,
                                            native_window, &swapchain);
    }
#endif  // WEBGPU_SUPPORTED

    if (device && context && swapchain) {
      // Success
      break;
    }
  }

  if (!device || !context || !swapchain) {
    LOG(ERROR) << "[Renderer] Failed to create renderer.";
    return CreateDeviceResult(nullptr, nullptr);
  }

  // etc
  const auto& device_info = device->GetDeviceInfo();
  const auto& adapter_info = device->GetAdapterInfo();
  const int32_t max_texture_size =
      static_cast<int32_t>(adapter_info.Texture.MaxTexture2DDimension);

  LOG(INFO) << "[Renderer] DeviceType: " +
                   std::string(GetRenderDeviceTypeString(device_info.Type)) +
                   " (version "
            << device_info.APIVersion.Major << "."
            << device_info.APIVersion.Minor << ")";
  LOG(INFO) << "[Renderer] Adapter: " << adapter_info.Description;
  LOG(INFO) << "[Renderer] MaxTexture Size: " << max_texture_size;

  // Global render device
  std::unique_ptr<RenderDevice> render_device(
      new RenderDevice(max_texture_size, window_target, swap_chain_desc, device,
                       swapchain, glcontext));

  return std::make_tuple(std::move(render_device), std::move(context));
}

RenderDevice::RenderDevice(
    int32_t max_texture_size,
    base::WeakPtr<ui::Widget> window,
    const Diligent::SwapChainDesc& swapchain_desc,
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device,
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapchain,
    SDL_GLContext gl_context)
    : window_(std::move(window)),
      swapchain_desc_(swapchain_desc),
      max_texture_size_(max_texture_size),
      device_(device),
      swapchain_(swapchain),
      device_type_(device_->GetDeviceInfo().Type),
      gl_context_(gl_context) {}

RenderDevice::~RenderDevice() {
  if (gl_context_)
    SDL_GL_DestroyContext(gl_context_);
}

void RenderDevice::SuspendContext() {
#if defined(OS_ANDROID)
  switch (device_type_) {
    case Diligent::RENDER_DEVICE_TYPE_GLES: {
      Diligent::RefCntAutoPtr<Diligent::IRenderDeviceGLES> es_device(
          device_, Diligent::IID_RenderDeviceGLES);
      es_device->Suspend();
    } break;
#if VULKAN_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_VULKAN:
      swapchain_.Release();
      break;
#endif  // VULKAN_SUPPORTED
    default:
      break;
  }
#endif  // OS_ANDROID
}

int32_t RenderDevice::ResumeContext(
    Diligent::IDeviceContext* immediate_context) {
#if defined(OS_ANDROID)
  SDL_PropertiesID window_properties =
      SDL_GetWindowProperties(window_->AsSDLWindow());
  void* android_native_window = SDL_GetPointerProperty(
      window_properties, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);

  switch (device_type_) {
    case Diligent::RENDER_DEVICE_TYPE_GLES: {
      Diligent::RefCntAutoPtr<Diligent::IRenderDeviceGLES> es_device(
          device_, Diligent::IID_RenderDeviceGLES);
      return es_device->Resume(
          static_cast<ANativeWindow*>(android_native_window));
    }
#if VULKAN_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_VULKAN: {
      device_->IdleGPU();

      Diligent::NativeWindow native_window;
      native_window.pAWindow = android_native_window;

#if ENGINE_DLL
      auto GetEngineFactoryVk = Diligent::LoadGraphicsEngineVk();
#else
      using Diligent::GetEngineFactoryVk;
#endif
      auto* factory = GetEngineFactoryVk();
      factory->CreateSwapChainVk(device_, immediate_context, swapchain_desc_,
                                 native_window, &swapchain_);

      return swapchain_ ? EGL_SUCCESS : EGL_NOT_INITIALIZED;
    }
#endif  // VULKAN_SUPPORTED
    default:
      break;
  }

  return EGL_NOT_INITIALIZED;
#else
  return 0;
#endif  // OS_ANDROID
}

}  // namespace renderer
