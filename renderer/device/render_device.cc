// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/device/render_device.h"

#include "SDL3/SDL_hints.h"
#include "SDL3/SDL_loadso.h"
#include "SDL3/SDL_video.h"
#include "magic_enum/magic_enum.hpp"

#include "Graphics/GraphicsAccessories/interface/GraphicsAccessories.hpp"
#include "Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h"
#include "Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h"
#include "Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h"
#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#include "Primitives/interface/DebugOutput.h"

#include "base/debug/logging.h"
#include "ui/widget/widget.h"

#if defined(OS_LINUX)
using PFN_XGetXCBConnection = void* (*)(void*);

#ifdef SDL_PLATFORM_OPENBSD
#define DEFAULT_VULKAN "libvulkan.so"
#define DEFAULT_X11_XCB "libX11-xcb.so"
#else
#define DEFAULT_VULKAN "libvulkan.so.1"
#define DEFAULT_X11_XCB "libX11-xcb.so.1"
#endif

#endif  // OS_LINUX

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
  if (Severity >=
      Diligent::DEBUG_MESSAGE_SEVERITY::DEBUG_MESSAGE_SEVERITY_ERROR) {
    printf("[Renderer] Function \"%s\":\n", Function);
    printf("[Renderer] %s\n", Message);
  }
}

}  // namespace

#if !ENGINE_DLL
using Diligent::GetEngineFactoryD3D11;
using Diligent::GetEngineFactoryD3D12;
using Diligent::GetEngineFactoryOpenGL;
using Diligent::GetEngineFactoryVk;
#endif

std::unique_ptr<RenderDevice> RenderDevice::Create(
    base::WeakPtr<ui::Widget> window_target,
    DriverType driver_type) {
  // Setup debugging output
  Diligent::SetDebugMessageCallback(DebugMessageOutputFunc);

  // Setup native window
  Diligent::NativeWindow native_window;
  SDL_PropertiesID window_properties =
      SDL_GetWindowProperties(window_target->AsSDLWindow());

  // Setup specific platform window handle
  SDL_GLContext glcontext = nullptr;
#if defined(OS_WIN)
  if (driver_type == DriverType::UNDEFINED)
    driver_type = DriverType::D3D11;

  native_window.hWnd = SDL_GetPointerProperty(
      window_properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif defined(OS_LINUX)
  if (driver_type == DriverType::UNDEFINED)
    driver_type = DriverType::OPENGL;

  {
    // Xlib Display
    void* xdisplay = SDL_GetPointerProperty(
        window_properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
    int64_t xwindow = SDL_GetNumberProperty(
        window_properties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);

    glcontext = SDL_GL_CreateContext(window_target->AsSDLWindow());
    SDL_GL_MakeCurrent(window_target->AsSDLWindow(), glcontext);

    // Get XCBConnect from Xlib
    const char* xcb_library_name = SDL_GetHint(SDL_HINT_X11_XCB_LIBRARY);
    if (!xcb_library_name || !*xcb_library_name)
      xcb_library_name = DEFAULT_X11_XCB;

    SDL_SharedObject* xlib_xcb_library = SDL_LoadObject(xcb_library_name);
    PFN_XGetXCBConnection xgetxcb_func = nullptr;
    if (xlib_xcb_library)
      xgetxcb_func = (PFN_XGetXCBConnection)SDL_LoadFunction(
          xlib_xcb_library, "XGetXCBConnection");

    // Setup native window
    native_window.WindowId = static_cast<uint32_t>(xwindow);
    native_window.pDisplay = xdisplay;
    native_window.pXCBConnection =
        xgetxcb_func ? xgetxcb_func(xdisplay) : nullptr;
  }
#endif

  Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device;
  Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context;
  Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapchain;

  // Initialize driver descriptor
  Diligent::EngineCreateInfo engine_create_info;
  Diligent::SwapChainDesc swap_chain_desc;
  Diligent::FullScreenModeDesc fullscreen_mode_desc;

  // Setup requied features
  engine_create_info.Features.ComputeShaders =
      Diligent::DEVICE_FEATURE_STATE_OPTIONAL;

// Initialize specific graphics api
#if GL_SUPPORTED
  if (driver_type == DriverType::OPENGL) {
#if ENGINE_DLL
    auto GetEngineFactoryOpenGL = Diligent::LoadGraphicsEngineOpenGL();
#endif
    auto* factory = GetEngineFactoryOpenGL();

    Diligent::EngineGLCreateInfo gl_create_info(engine_create_info);
    gl_create_info.Window = native_window;
    gl_create_info.ZeroToOneNDZ = Diligent::True;
    factory->CreateDeviceAndSwapChainGL(gl_create_info, &device, &context,
                                        swap_chain_desc, &swapchain);
  }
#endif  // OPENGL_SUPPORT
#if VULKAN_SUPPORTED
  if (driver_type == DriverType::VULKAN) {
#if ENGINE_DLL
    auto GetEngineFactoryVk = Diligent::LoadGraphicsEngineVk();
#endif
    auto* factory = GetEngineFactoryVk();

    Diligent::EngineVkCreateInfo vk_create_info(engine_create_info);
    factory->CreateDeviceAndContextsVk(vk_create_info, &device, &context);
    factory->CreateSwapChainVk(device, context, swap_chain_desc, native_window,
                               &swapchain);
  }
#endif  // VULKAN_SUPPORT
#if D3D11_SUPPORTED
  if (driver_type == DriverType::D3D11) {
#if ENGINE_DLL
    auto GetEngineFactoryD3D11 = Diligent::LoadGraphicsEngineD3D11();
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

  // etc
  const auto& device_info = device->GetDeviceInfo();
  const auto& adapter_info = device->GetAdapterInfo();
  printf("[Renderer] DeviceType: %s (version %d.%d)\n",
         GetRenderDeviceTypeString(device_info.Type),
         device_info.APIVersion.Major, device_info.APIVersion.Minor);
  printf("[Renderer] Adapter: %s\n", adapter_info.Description);
  printf("[Renderer] MaxTexture Size: %d\n",
         adapter_info.Texture.MaxTexture2DDimension);
  if (device_info.Features.ComputeShaders ==
      Diligent::DEVICE_FEATURE_STATE_DISABLED)
    printf("[Renderer] Detect computeShaders is disabled.\n");

  // Initialize graphics pipelines
  std::unique_ptr<PipelineSet> pipelines_set =
      std::make_unique<PipelineSet>(device, Diligent::TEX_FORMAT_RGBA8_UNORM);

  // Initialize generic quad index buffer
  std::unique_ptr<QuadIndexCache> quad_index_cache =
      QuadIndexCache::Make(device);
  quad_index_cache->Allocate(1 << 10);

  // Wait for creating
  context->WaitForIdle();

  // Create new instance
  return std::unique_ptr<RenderDevice>(new RenderDevice(
      window_target, device, context, swapchain, std::move(pipelines_set),
      std::move(quad_index_cache), glcontext));
}

RenderDevice::RenderDevice(
    base::WeakPtr<ui::Widget> window,
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device,
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context,
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapchain,
    std::unique_ptr<PipelineSet> pipelines,
    std::unique_ptr<QuadIndexCache> quad_index,
    SDL_GLContext gl_context)
    : window_(std::move(window)),
      device_(device),
      context_(context),
      swapchain_(swapchain),
      pipelines_(std::move(pipelines)),
      quad_index_(std::move(quad_index)),
      gl_context_(gl_context) {}

RenderDevice::~RenderDevice() {
  if (gl_context_)
    SDL_GL_DestroyContext(gl_context_);
}

}  // namespace renderer
