// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/device/render_device.h"

#include "base/debug/logging.h"
#include "ui/widget/widget.h"

#include "SDL3/SDL_video.h"

namespace renderer {

//--------------------------------------------------------------------------------------
// Internal Helper Functions
//--------------------------------------------------------------------------------------

namespace {

/* WGPU global scoped instance */
wgpu::Instance g_wgpu_instance = nullptr;

/**
 * Callback that gets triggered when the WGPU device is lost.
 *
 * @param device  The WGPU device instance that triggered this callback.
 * @param reason  The enumerated reason why the device was lost.
 * @param message Additional diagnostic message from the GPU stack or driver.
 */
void OnDeviceLost(const wgpu::Device& /*device*/,
                  wgpu::DeviceLostReason reason,
                  wgpu::StringView message) {
  std::string reason_str;
  switch (reason) {
    case wgpu::DeviceLostReason::Unknown:
      reason_str = "Unknown";
      break;
    case wgpu::DeviceLostReason::Destroyed:
      reason_str = "Destroyed";
      break;
    case wgpu::DeviceLostReason::InstanceDropped:
      reason_str = "InstanceDropped";
      break;
    case wgpu::DeviceLostReason::FailedCreation:
      reason_str = "FailedCreation";
      break;
    // You may want to handle additional enumerations if WGPU adds more
    default:
      reason_str = "Unrecognized Reason";
      break;
  }

  LOG(INFO) << "[Renderer] Device lost - " << reason_str << ": "
            << std::string_view(message);
}

/**
 * Callback for uncaptured WGPU errors such as validation, OOM, or unknown
 * errors.
 *
 * @param device  The WGPU device instance that triggered this callback.
 * @param error_type  The enumerated type of error.
 * @param message     The error's diagnostic message.
 */
void OnUncapturedError(const wgpu::Device& /*device*/,
                       wgpu::ErrorType error_type,
                       wgpu::StringView message) {
  const char* type_str = nullptr;
  switch (error_type) {
    case wgpu::ErrorType::Validation:
      type_str = "Validation";
      break;
    case wgpu::ErrorType::OutOfMemory:
      type_str = "OutOfMemory";
      break;
    case wgpu::ErrorType::Internal:
      type_str = "Internal";
      break;
    case wgpu::ErrorType::Unknown:
      type_str = "Unknown";
      break;
    default:
      return;
  }

  LOG(INFO) << "[Renderer] " << type_str
            << " error: " << std::string_view(message);
}

}  // namespace

//--------------------------------------------------------------------------------------
// Public Static Methods
//--------------------------------------------------------------------------------------

/**
 * Factory function to create a RenderDevice instance.
 *
 * @param window_target     A weak pointer to the UI widget that represents our
 * main window.
 * @param required_backend  An optional WGPU backend requirement (e.g., Vulkan,
 * Metal, etc.).
 *
 * @return A unique_ptr to the RenderDevice or nullptr on failure.
 */
std::unique_ptr<RenderDevice> RenderDevice::Create(
    base::WeakPtr<ui::Widget> window_target,
    wgpu::BackendType required_backend /* = wgpu::BackendType::Undefined */) {
  // 1) Create a WGPU instance if needed
  if (!g_wgpu_instance) {
    wgpu::InstanceDescriptor instance_desc;
    instance_desc.capabilities.timedWaitAnyEnable = true;
    g_wgpu_instance = wgpu::CreateInstance(&instance_desc);
    if (!g_wgpu_instance) {
      LOG(ERROR) << "[Renderer] Failed to create WGPU instance.";
      return nullptr;
    }
  }

  // 2) Request an adapter that is "compatibilityMode" = true (for broad
  // compatibility)
  wgpu::RequestAdapterOptions adapter_options;
  adapter_options.backendType = required_backend;

  wgpu::Adapter adapter;
  // We wait synchronously with WaitAny(...) for an adapter to be provided
  g_wgpu_instance.WaitAny(
      g_wgpu_instance.RequestAdapter(
          &adapter_options, wgpu::CallbackMode::WaitAnyOnly,
          [&](wgpu::RequestAdapterStatus status, wgpu::Adapter result_adapter,
              wgpu::StringView message) {
            if (status != wgpu::RequestAdapterStatus::Success) {
              LOG(ERROR) << "[Renderer] Failed to get an adapter: "
                         << std::string_view(message);
              return;
            }
            adapter = std::move(result_adapter);
          }),
      /*timeout=*/UINT64_MAX);

  if (!adapter) {
    LOG(ERROR) << "[Renderer] No valid WGPU adapter available.";
    return nullptr;
  }

  // 3) Log some adapter info
  wgpu::AdapterInfo info;
  adapter.GetInfo(&info);
  LOG(INFO) << "[Renderer] Using adapter device: "
            << std::string_view(info.device);
  LOG(INFO) << "[Renderer] Using adapter description: "
            << std::string_view(info.description);

  // 4) Request the actual WGPU device
  wgpu::DeviceDescriptor device_desc = {};
  device_desc.SetUncapturedErrorCallback(OnUncapturedError);
  device_desc.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous,
                                    OnDeviceLost);

  wgpu::Device device;
  g_wgpu_instance.WaitAny(
      adapter.RequestDevice(
          &device_desc, wgpu::CallbackMode::WaitAnyOnly,
          [&](wgpu::RequestDeviceStatus status, wgpu::Device result_device,
              wgpu::StringView message) {
            if (status != wgpu::RequestDeviceStatus::Success) {
              LOG(ERROR) << "[Renderer] Failed to create a WGPU device: "
                         << std::string_view(message);
              return;
            }
            device = std::move(result_device);
          }),
      UINT64_MAX);

  if (!device) {
    LOG(ERROR) << "[Renderer] No valid WGPU device created.";
    return nullptr;
  }

  // 5) Create a WGPU surface from the native window
  SDL_PropertiesID window_properties =
      SDL_GetWindowProperties(window_target->AsSDLWindow());

  wgpu::SurfaceDescriptor surface_desc;
  surface_desc.label = nullptr;

#if defined(OS_WIN)
  wgpu::SurfaceSourceWindowsHWND hwnd_desc;
  hwnd_desc.hinstance = ::GetModuleHandle(nullptr);
  hwnd_desc.hwnd = SDL_GetPointerProperty(
      window_properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
  surface_desc.nextInChain = &hwnd_desc;
#elif defined(OS_LINUX)
  // Not implemented: e.g. wgpu::SurfaceSourceXcb ...
  LOG(ERROR) << "Linux platform not implemented for surface creation. "
             << "Please fill in the wgpu::SurfaceSource* details for your "
                "environment.";
  return nullptr;
#else
#error "Unsupported platform for surface creation."
#endif

  wgpu::Surface surface = g_wgpu_instance.CreateSurface(&surface_desc);
  if (!surface) {
    LOG(ERROR) << "[Renderer] Surface creation failed.";
    return nullptr;
  }

  // 6) Retrieve surface capabilities and configure
  wgpu::SurfaceCapabilities caps;
  surface.GetCapabilities(adapter, &caps);
  if (!caps.formatCount) {
    LOG(ERROR) << "[Renderer] Surface has no supported formats.";
    return nullptr;
  }

  wgpu::SurfaceConfiguration config;
  config.device = device;
  config.format = caps.formats[0];
  config.width = window_target->GetSize().x;
  config.height = window_target->GetSize().y;

  // Additional config fields can be set here (e.g., usage, presentMode, etc.)
  surface.Configure(&config);

  // 7) Create pipelines instance
  std::unique_ptr<PipelineSet> pipelines_set =
      std::make_unique<PipelineSet>(device, wgpu::TextureFormat::RGBA8Unorm);

  // 8) Success: create and return our RenderDevice instance
  return std::unique_ptr<RenderDevice>(
      new RenderDevice(window_target, adapter, device, device.GetQueue(),
                       surface, config.format, std::move(pipelines_set)));
}

wgpu::Instance* RenderDevice::GetGPUInstance() {
  return &g_wgpu_instance;
}

RenderDevice::RenderDevice(base::WeakPtr<ui::Widget> window,
                           const wgpu::Adapter& adapter,
                           const wgpu::Device& device,
                           const wgpu::Queue& queue,
                           const wgpu::Surface& surface,
                           wgpu::TextureFormat surface_format,
                           std::unique_ptr<PipelineSet> pipelines)
    : window_(std::move(window)),
      adapter_(adapter),
      device_(device),
      queue_(queue),
      surface_(surface),
      surface_format_(surface_format),
      pipelines_(std::move(pipelines)) {}

RenderDevice::~RenderDevice() {
  g_wgpu_instance = nullptr;
}

}  // namespace renderer
