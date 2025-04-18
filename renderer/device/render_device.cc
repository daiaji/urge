// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/device/render_device.h"

#include "SDL3/SDL_video.h"
#include "magic_enum/magic_enum.hpp"

#include "Graphics/GraphicsAccessories/interface/GraphicsAccessories.hpp"
#include "Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h"
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
      Diligent::DEBUG_MESSAGE_SEVERITY::DEBUG_MESSAGE_SEVERITY_ERROR)
    printf("[Renderer] Function: %s, Info: %s\n", Function, Message);
}

}  // namespace

using Diligent::GetEngineFactoryOpenGL;

std::unique_ptr<RenderDevice> RenderDevice::Create(
    base::WeakPtr<ui::Widget> window_target) {
  // Setup debugging output
  Diligent::SetDebugMessageCallback(DebugMessageOutputFunc);

  // Setup native window
  Diligent::NativeWindow native_window;
  SDL_PropertiesID window_properties =
      SDL_GetWindowProperties(window_target->AsSDLWindow());

  // Setup specific platform window handle
#if defined(OS_WIN)
  native_window.hWnd = SDL_GetPointerProperty(
      window_properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#endif

  Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device;
  Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context;
  Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapchain;

  // Initialize swapchain descriptor
  Diligent::SwapChainDesc swap_chain_desc;
  swap_chain_desc.IsPrimary = Diligent::True;

  // Initialize specific graphics api
  {
#if ENGINE_DLL
    auto GetEngineFactoryOpenGL = Diligent::LoadGraphicsEngineOpenGL();
#endif
    auto* factory = GetEngineFactoryOpenGL();
    Diligent::EngineGLCreateInfo gl_create_info;
    gl_create_info.Window = native_window;
    gl_create_info.ZeroToOneNDZ = Diligent::True;
    factory->CreateDeviceAndSwapChainGL(gl_create_info, &device, &context,
                                        swap_chain_desc, &swapchain);
  }

  // Initialize graphics pipelines
  std::unique_ptr<PipelineSet> pipelines_set = std::make_unique<PipelineSet>(
      device, Diligent::TEXTURE_FORMAT::TEX_FORMAT_RGBA8_UNORM_SRGB);

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
