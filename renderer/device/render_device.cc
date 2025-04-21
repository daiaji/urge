// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/device/render_device.h"

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
#if defined(OS_WIN)
  if (driver_type == DriverType::UNDEFINED)
    driver_type = DriverType::D3D11;

  native_window.hWnd = SDL_GetPointerProperty(
      window_properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#endif

  Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device;
  Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context;
  Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapchain;

  // Initialize driver descriptor
  Diligent::EngineCreateInfo engine_create_info;
  Diligent::SwapChainDesc swap_chain_desc;
  Diligent::FullScreenModeDesc fullscreen_mode_desc;

  // Initialize specific graphics api
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
  } else if (driver_type == DriverType::VULKAN) {
#if ENGINE_DLL
    auto GetEngineFactoryVk = Diligent::LoadGraphicsEngineVk();
#endif
    auto* factory = GetEngineFactoryVk();

    Diligent::EngineVkCreateInfo vk_create_info(engine_create_info);
    factory->CreateDeviceAndContextsVk(vk_create_info, &device, &context);
    factory->CreateSwapChainVk(device, context, swap_chain_desc, native_window,
                               &swapchain);
  } else if (driver_type == DriverType::D3D11) {
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
  } else if (driver_type == DriverType::D3D12) {
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

  // Initialize graphics pipelines
  std::unique_ptr<PipelineSet> pipelines_set =
      std::make_unique<PipelineSet>(device, Diligent::TEX_FORMAT_RGBA8_UNORM);

  // Initialize generic quad index buffer
  std::unique_ptr<QuadIndexCache> quad_index_cache =
      QuadIndexCache::Make(device);
  quad_index_cache->Allocate(1 << 10);

  // etc
  printf("[Renderer] Device: %s\n",
         GetRenderDeviceTypeString(device->GetDeviceInfo().Type));
  printf("[Renderer] Adapter: %s\n", device->GetAdapterInfo().Description);
  printf("[Renderer] MaxTexture Size: %d\n",
         device->GetAdapterInfo().Texture.MaxTexture2DDimension);

  // Wait for creating
  context->WaitForIdle();

  // Create new instance
  return std::unique_ptr<RenderDevice>(
      new RenderDevice(window_target, device, context, swapchain,
                       std::move(pipelines_set), std::move(quad_index_cache)));
}

RenderDevice::RenderDevice(
    base::WeakPtr<ui::Widget> window,
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device,
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context,
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapchain,
    std::unique_ptr<PipelineSet> pipelines,
    std::unique_ptr<QuadIndexCache> quad_index)
    : window_(std::move(window)),
      device_(device),
      context_(context),
      swapchain_(swapchain),
      pipelines_(std::move(pipelines)),
      quad_index_(std::move(quad_index)) {}

RenderDevice::~RenderDevice() = default;

}  // namespace renderer
