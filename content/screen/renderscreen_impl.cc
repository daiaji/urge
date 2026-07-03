// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/screen/renderscreen_impl.h"

#include <unordered_map>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include "magic_enum/magic_enum.hpp"

#include "Common/interface/BasicMath.hpp"
#include "Graphics/GraphicsAccessories/interface/GraphicsAccessories.hpp"
#if defined(OS_ANDROID)
#include "Graphics/GraphicsEngineOpenGL/interface/RenderDeviceGLES.h"
#endif

#include "content/canvas/canvas_scheduler.h"
#include "content/common/rect_impl.h"
#include "content/context/execution_context.h"
#include "content/gpu/buffer_impl.h"
#include "content/gpu/device_context_impl.h"
#include "content/gpu/render_device_impl.h"
#include "content/profile/command_ids.h"
#include "renderer/utils/texture_utils.h"

namespace content {

///////////////////////////////////////////////////////////////////////////////
// RenderScreenImpl Implement

RenderScreenImpl::RenderScreenImpl(ExecutionContext* execution_context,
                                   uint32_t frame_rate)
    : EngineObject(execution_context),
      limiter_(frame_rate),
      frozen_(false),
      brightness_(255),
      frame_count_(0),
      frame_rate_(frame_rate > 0 ? frame_rate : 60),
      unlimited_fps_(frame_rate == 0) {
  // Setup render device on render thread if possible
  GPUCreateGraphicsHostInternal();

  // Initialize fps limiter
  limiter_.Reset();
}

RenderScreenImpl::~RenderScreenImpl() = default;

void RenderScreenImpl::PresentScreenBuffer(
    Diligent::ImGuiDiligentRenderer* gui_renderer) {
  // Present to screen surface
  GPUPresentScreenBufferInternal(context()->primary_render_context,
                                 gui_renderer);
}

void RenderScreenImpl::ResetFPSCounter() {
  limiter_.Reset();
}

void RenderScreenImpl::CreateButtonGUISettings() {
  const auto& adapter_info = (*context()->render_device)->GetAdapterInfo();
  const auto& device_info = (*context()->render_device)->GetDeviceInfo();
  auto& settings_profile = *context()->engine_profile;

  if (ImGui::CollapsingHeader(
          context()
              ->i18n_profile->GetI18NString(IDS_SETTINGS_GRAPHICS, "Graphics")
              .c_str())) {
    ImGui::TextWrapped(
        "%s: %s (%d.%d)",
        context()
            ->i18n_profile->GetI18NString(IDS_GRAPHICS_BACKEND, "Backend")
            .c_str(),
        Diligent::GetRenderDeviceTypeString(device_info.Type),
        device_info.APIVersion.Major, device_info.APIVersion.Minor);
    ImGui::Separator();
    ImGui::TextWrapped(
        "%s: %s (PCI: %X)",
        context()
            ->i18n_profile->GetI18NString(IDS_GRAPHICS_RENDERER, "Renderer")
            .c_str(),
        adapter_info.Description, adapter_info.DeviceId);
    ImGui::Separator();
    ImGui::TextWrapped(
        "%s: %s (PCI: %X)",
        context()
            ->i18n_profile->GetI18NString(IDS_GRAPHICS_VENDOR, "Vendor")
            .c_str(),
        magic_enum::enum_name(adapter_info.Vendor).data() +
            sizeof("ADAPTER_VENDOR"),
        adapter_info.VendorId);
    ImGui::Separator();

    // VSync
    bool vsync_enabled = settings_profile.vsync != 0;
    ImGui::Checkbox(
        context()
            ->i18n_profile->GetI18NString(IDS_GRAPHICS_VSYNC, "VSync")
            .c_str(),
        &vsync_enabled);
    settings_profile.vsync = vsync_enabled ? 1 : 0;
    context()->engine_profile->MarkDirty();

    // Frame Rate
    static int32_t frame_rate_tmp = static_cast<int32_t>(frame_rate_);
    frame_rate_tmp = unlimited_fps_ ? 0 : static_cast<int32_t>(frame_rate_);
    ImGui::SliderInt(
        context()
            ->i18n_profile->GetI18NString(IDS_GRAPHICS_FRAME_RATE, "Frame Rate")
            .c_str(),
        &frame_rate_tmp, 0, 360);
    if (frame_rate_tmp == 0 && !unlimited_fps_) {
      unlimited_fps_ = true;
      limiter_.SetDisabled(true);
    } else if (frame_rate_tmp > 0) {
      unlimited_fps_ = false;
      frame_rate_ = static_cast<uint32_t>(frame_rate_tmp);
      limiter_.SetDisabled(false);
      limiter_.SetFrameRate(frame_rate_);
    }

    // Keep Ratio
    if (ImGui::Checkbox(
            context()
                ->i18n_profile->GetI18NString(IDS_GRAPHICS_KEEP_RATIO, "Keep Ratio")
                .c_str(),
            &settings_profile.keep_ratio))
      context()->engine_profile->MarkDirty();

    // Integer Scaling (requires keep_ratio)
    ImGui::BeginDisabled(!settings_profile.keep_ratio);
    if (ImGui::Checkbox("Integer Scaling",
                        &settings_profile.integer_scaling))
      context()->engine_profile->MarkDirty();
    ImGui::EndDisabled();

    // Skip Frame
    if (ImGui::Checkbox(
            context()
                ->i18n_profile->GetI18NString(IDS_GRAPHICS_SKIP_FRAME, "Skip Frame")
                .c_str(),
            &settings_profile.allow_skip_frame))
      context()->engine_profile->MarkDirty();

    // Fullscreen
    bool is_fullscreen = context()->window->IsFullscreen(),
         last_fullscreen = is_fullscreen;
    ImGui::Checkbox(
        context()
            ->i18n_profile->GetI18NString(IDS_GRAPHICS_FULLSCREEN, "Fullscreen")
            .c_str(),
        &is_fullscreen);
    if (last_fullscreen != is_fullscreen)
      context()->window->SetFullscreen(is_fullscreen);

    // Background running
    if (ImGui::Checkbox(context()
                            ->i18n_profile
                            ->GetI18NString(IDS_GRAPHICS_BACKGROUND_RUNNING,
                                            "Background Running")
                            .c_str(),
                        &settings_profile.background_running))
      context()->engine_profile->MarkDirty();

    // Scaling algorithm
    ImGui::Separator();
    ImGui::Text("Scaling Algorithm");
    const char* scaling_items[] = {"Bilinear", "Nearest",
                                   "Lanczos3", "Bicubic",
                                   "Anime4K", "Anime4K+Sobel",
                                   "Anime4K Mode A"};
    if (ImGui::Combo("##scaling_mode", &settings_profile.scaling_mode,
                 scaling_items, IM_ARRAYSIZE(scaling_items)))
      context()->engine_profile->MarkDirty();

    if (settings_profile.scaling_mode == 5) {
      ImGui::SliderFloat("##sobel_strength",
                         &settings_profile.scaling_sobel_strength,
                         0.0f, 2.0f, "Sobel:%.1f");
      if (ImGui::IsItemDeactivatedAfterEdit())
        context()->engine_profile->MarkDirty();
      ImGui::SliderFloat("##warp_strength",
                         &settings_profile.scaling_warp_strength,
                         0.0f, 1.0f, "Warp:%.2f");
      if (ImGui::IsItemDeactivatedAfterEdit())
        context()->engine_profile->MarkDirty();
      ImGui::SliderFloat("##darken_strength",
                         &settings_profile.scaling_darken_strength,
                         0.0f, 2.0f, "Dark:%.1f");
      if (ImGui::IsItemDeactivatedAfterEdit())
        context()->engine_profile->MarkDirty();
    }

    if (settings_profile.scaling_mode == 6) {
      ImGui::TextDisabled("Clamp->Restore CNN->Upscale CNN->DoG");
      ImGui::SameLine();
      ImGui::TextDisabled("(20-pass)");

      if (ImGui::Checkbox("Auto-fit Window", &settings_profile.mode_a_auto_fit))
        context()->engine_profile->MarkDirty();

      if (settings_profile.mode_a_auto_fit) {
        ImGui::SameLine();
        if (ImGui::Button("Apply##autofit")) {
          auto native = context()->resolution;
          SDL_Window* win = context()->window->AsSDLWindow();
          int display = SDL_GetDisplayForWindow(win);
          SDL_Rect bounds;
          SDL_GetDisplayBounds(display, &bounds);
          // Mode A outputs at 2x native; use largest even multiplier that fits
          int max_factor = std::min(bounds.w / native.x, bounds.h / native.y);
          if (max_factor < 2) max_factor = 2;
          // Round down to nearest even number
          int factor = (max_factor / 2) * 2;
          int w = native.x * factor;
          int h = native.y * factor;
          SDL_SetWindowSize(win, w, h);
          int cx = bounds.x + (bounds.w - w) / 2;
          int cy = bounds.y + (bounds.h - h) / 2;
          SDL_SetWindowPosition(win, cx, cy);
        }
      }
    }

    // CAS sharpening
    if (ImGui::Checkbox("CAS Sharpen", &settings_profile.cas_enabled))
      context()->engine_profile->MarkDirty();
    if (settings_profile.cas_enabled) {
      ImGui::SliderFloat("##cas_sharpness", &settings_profile.cas_sharpness,
                         0.0f, 1.0f, "%.2f");
      if (ImGui::IsItemDeactivatedAfterEdit())
        context()->engine_profile->MarkDirty();
    }

    ImGui::Separator();
    if (ImGui::Button("Reset##settings")) {
      context()->engine_profile->ResetRendererDefaults();
      frame_rate_ = context()->engine_profile->frame_rate;
      unlimited_fps_ = frame_rate_ == 0;
      limiter_.SetDisabled(unlimited_fps_);
      if (!unlimited_fps_)
        limiter_.SetFrameRate(frame_rate_);
      context()->engine_profile->SaveConfigure();
    }
  }

  if (!ImGui::IsAnyItemActive())
    context()->engine_profile->SaveIfDirty();
}

void RenderScreenImpl::Update(ExceptionState& exception_state) {
  if (rebuild_buffers_pending_) {
    GPUResetScreenBufferInternal();
    rebuild_buffers_pending_ = false;
  }

  const bool frozen_render = frozen_;
  const bool need_skip_frame = limiter_.RequireFrameSkip() &&
                               context()->engine_profile->allow_skip_frame;

  if (!frozen_render && !need_skip_frame) {
    // Render a frame and push into render queue
    // This function only encodes the render commands
    RenderFrameInternal(gpu_.screen_buffer, gpu_.screen_depth_stencil);
  }

  if (need_skip_frame)
    limiter_.Reset();

  // Process frame delay
  // This calling will yield to event coroutine and present
  FrameProcessInternal(gpu_.screen_buffer);
}

void RenderScreenImpl::Wait(uint32_t duration,
                            ExceptionState& exception_state) {
  for (uint32_t i = 0; i < duration; ++i)
    Update(exception_state);
}

void RenderScreenImpl::FadeOut(uint32_t duration,
                               ExceptionState& exception_state) {
  duration = std::max(duration, 1u);

  float current_brightness = static_cast<float>(brightness_);
  for (uint32_t i = 0; i < duration; ++i) {
    brightness_ = current_brightness -
                  current_brightness * (i / static_cast<float>(duration));
    if (frozen_) {
      FrameProcessInternal(gpu_.frozen_buffer);
    } else {
      Update(exception_state);
    }
  }

  // Set final brightness
  brightness_ = 0;
  Update(exception_state);
}

void RenderScreenImpl::FadeIn(uint32_t duration,
                              ExceptionState& exception_state) {
  duration = std::max(duration, 1u);

  float current_brightness = static_cast<float>(brightness_);
  float diff = 255.0f - current_brightness;
  for (uint32_t i = 0; i < duration; ++i) {
    brightness_ =
        current_brightness + diff * (i / static_cast<float>(duration));

    if (frozen_) {
      FrameProcessInternal(gpu_.frozen_buffer);
    } else {
      Update(exception_state);
    }
  }

  // Set final brightness
  brightness_ = 255;
  Update(exception_state);
}

void RenderScreenImpl::Freeze(ExceptionState& exception_state) {
  if (!frozen_) {
    // Get frozen scene snapshot for transition
    RenderFrameInternal(gpu_.frozen_buffer, gpu_.frozen_depth_stencil);

    // Set forzen flag for blocking frame update
    frozen_ = true;
  }
}

void RenderScreenImpl::Transition(ExceptionState& exception_state) {
  Transition(10, std::string(), 40, exception_state);
}

void RenderScreenImpl::Transition(uint32_t duration,
                                  ExceptionState& exception_state) {
  Transition(duration, std::string(), 40, exception_state);
}

void RenderScreenImpl::Transition(uint32_t duration,
                                  const std::string& filename,
                                  ExceptionState& exception_state) {
  Transition(duration, filename, 40, exception_state);
}

void RenderScreenImpl::Transition(uint32_t duration,
                                  const std::string& filename,
                                  uint32_t vague,
                                  ExceptionState& exception_state) {
  scoped_refptr<CanvasImpl> transition_mapping = nullptr;
  if (!filename.empty())
    transition_mapping =
        CanvasImpl::Create(context(), filename, exception_state);
  if (exception_state.HadException())
    return;

  TransitionWithBitmap(duration, transition_mapping, vague, exception_state);
}

void RenderScreenImpl::TransitionWithBitmap(uint32_t duration,
                                            scoped_refptr<Bitmap> bitmap,
                                            uint32_t vague,
                                            ExceptionState& exception_state) {
  if (!frozen_)
    return;

  if (duration > 0) {
    // Reset brightness
    brightness_ = 255;
    // Transition attribute
    const float vague_norm = std::clamp<uint32_t>(vague, 1, 256) / 256.0f;

    // Derive transition mapping if available
    scoped_refptr<CanvasImpl> mapping_bitmap = CanvasImpl::FromBitmap(bitmap);
    BitmapTexture* texture_agent =
        Disposable::IsValid(mapping_bitmap.get()) ? **mapping_bitmap : nullptr;
    Diligent::ITextureView* transition_mapping =
        texture_agent ? texture_agent->shader_resource_view.RawPtr() : nullptr;

    // Get current scene snapshot for transition
    auto* render_context = context()->primary_render_context;
    RenderFrameInternal(gpu_.transition_buffer, gpu_.transition_depth_stencil);

    // Transition render loop
    for (uint32_t i = 0; i < duration; ++i) {
      // Norm transition progress
      float progress = i * (1.0f / duration);

      // Render per transition frame
      if (transition_mapping)
        GPURenderVagueTransitionFrameInternal(
            render_context, transition_mapping, progress, vague_norm);
      else
        GPURenderAlphaTransitionFrameInternal(render_context, progress);

      // Present to screen
      FrameProcessInternal(gpu_.screen_buffer);
    }
  } else {
    // Update frame as common
    Update(exception_state);
  }

  // Transition process complete
  frozen_ = false;
}

scoped_refptr<Bitmap> RenderScreenImpl::SnapToBitmap(
    ExceptionState& exception_state) {
  scoped_refptr<CanvasImpl> target =
      CanvasImpl::Create(context(), context()->resolution, exception_state);
  BitmapTexture* texture_agent =
      Disposable::IsValid(target.get()) ? **target : nullptr;

  if (texture_agent) {
    // Invalidate render target bitmap cache
    target->InvalidateSurfaceCache();

    // Execute rendering
    RenderFrameInternal(texture_agent->data, texture_agent->depth);
  }

  return target;
}

void RenderScreenImpl::FrameReset(ExceptionState& exception_state) {
  limiter_.Reset();
}

uint32_t RenderScreenImpl::Width(ExceptionState& exception_state) {
  return context()->resolution.x;
}

uint32_t RenderScreenImpl::Height(ExceptionState& exception_state) {
  return context()->resolution.y;
}

void RenderScreenImpl::ResizeScreen(uint32_t width,
                                    uint32_t height,
                                    ExceptionState& exception_state) {
  context()->resolution = base::Vec2i(width, height);
  GPUResetScreenBufferInternal();

  if (context()->engine_profile->fixed_aspect_ratio) {
    float ratio = static_cast<float>(width) / static_cast<float>(height);
    SDL_SetWindowAspectRatio(context()->window->AsSDLWindow(), ratio, ratio);
  }
}

void RenderScreenImpl::Reset(ExceptionState& exception_state) {
  /* Reset freeze */
  frozen_ = false;

  /* Reset attribute */
  frame_rate_ = context()->engine_profile->api_version ==
                        ContentProfile::APIVersion::RGSS1
                    ? 40
                    : 60;
  unlimited_fps_ = false;
  brightness_ = 255;
  FrameReset(exception_state);
}

void RenderScreenImpl::MoveWindow(int32_t x,
                                  int32_t y,
                                  int32_t width,
                                  int32_t height,
                                  ExceptionState& exception_state) {
  auto* window = context()->window->AsSDLWindow();
  SDL_SetWindowSize(window, width, height);
  SDL_SetWindowPosition(window, x, y);
}

void RenderScreenImpl::Center(ExceptionState& exception_state) {
  auto* window = context()->window->AsSDLWindow();
  SDL_DisplayID display = SDL_GetDisplayForWindow(window);
  SDL_Rect display_bounds;
  SDL_GetDisplayBounds(display, &display_bounds);
  int win_w, win_h;
  SDL_GetWindowSize(window, &win_w, &win_h);
  int cx = display_bounds.x + (display_bounds.w - win_w) / 2;
  int cy = display_bounds.y + (display_bounds.h - win_h) / 2;
  SDL_SetWindowPosition(window, cx, cy);
}

scoped_refptr<Rect> RenderScreenImpl::GetWindowRect(
    ExceptionState& exception_state) {
  auto* window = context()->window->AsSDLWindow();
  base::Rect window_rect;
  SDL_GetWindowPosition(window, &window_rect.x, &window_rect.y);
  SDL_GetWindowSize(window, &window_rect.width, &window_rect.height);
  return base::MakeRefCounted<RectImpl>(window_rect);
}

uint32_t RenderScreenImpl::GetDisplayID(ExceptionState& exception_state) {
  return SDL_GetDisplayForWindow(context()->window->AsSDLWindow());
}

void RenderScreenImpl::SetWindowIcon(scoped_refptr<Bitmap> icon,
                                     ExceptionState& exception_state) {
  scoped_refptr<CanvasImpl> data = CanvasImpl::FromBitmap(icon);
  SDL_Surface* icon_surface = data->RequireMemorySurface();
  SDL_SetWindowIcon(context()->window->AsSDLWindow(), icon_surface);
}

int32_t RenderScreenImpl::GetMaxTextureSize(ExceptionState& exception_state) {
  return context()->render_device->MaxTextureSize();
}

scoped_refptr<GPURenderDevice> RenderScreenImpl::GetRenderDevice(
    ExceptionState& exception_state) {
  return base::MakeRefCounted<RenderDeviceImpl>(context(),
                                                **context()->render_device);
}

scoped_refptr<GPUDeviceContext> RenderScreenImpl::GetImmediateContext(
    ExceptionState& exception_state) {
  return base::MakeRefCounted<DeviceContextImpl>(
      context(), context()->primary_render_context);
}

scoped_refptr<GPUBuffer> RenderScreenImpl::GetGenericQuadIndexBuffer(
    uint32_t draw_quad_count,
    ExceptionState& exception_state) {
  auto& quad_index = context()->render.quad_index;
  quad_index->Allocate(draw_quad_count);
  return base::MakeRefCounted<BufferImpl>(context(), **quad_index);
}

GPU::ValueType RenderScreenImpl::GetInternalIndexType(
    ExceptionState& exception_state) {
  auto& quad_index = context()->render.quad_index;
  return static_cast<GPU::ValueType>(quad_index->GetIndexType());
}

void RenderScreenImpl::SyncToRefreshRate(int32_t refresh_rate) {
  if (refresh_rate > 0) {
    unlimited_fps_ = false;
    frame_rate_ = static_cast<uint32_t>(refresh_rate);
    context()->engine_profile->vsync = 1;
    limiter_.SetDisabled(true);
  }
}

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    FrameRate,
    uint32_t,
    RenderScreenImpl,
    { return frame_rate_; },
    {
      if (value == 0) {
        unlimited_fps_ = true;
        limiter_.SetDisabled(true);
      } else {
        unlimited_fps_ = false;
        frame_rate_ = value;
        limiter_.SetDisabled(false);
        limiter_.SetFrameRate(frame_rate_);
      }
    });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    FrameCount,
    uint32_t,
    RenderScreenImpl,
    { return frame_count_; },
    { frame_count_ = value; });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    Brightness,
    uint32_t,
    RenderScreenImpl,
    { return brightness_; },
    { brightness_ = std::clamp<uint32_t>(value, 0, 255); });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    VSync,
    uint32_t,
    RenderScreenImpl,
    { return context()->engine_profile->vsync; },
    {
      context()->engine_profile->vsync = std::clamp<uint32_t>(value, 0, 255);
    });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    Fullscreen,
    bool,
    RenderScreenImpl,
    { return context()->window->IsFullscreen(); },
    { context()->window->SetFullscreen(value); });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    Skipframe,
    bool,
    RenderScreenImpl,
    { return context()->engine_profile->allow_skip_frame; },
    { context()->engine_profile->allow_skip_frame = value; });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    KeepRatio,
    bool,
    RenderScreenImpl,
    { return context()->engine_profile->keep_ratio; },
    { context()->engine_profile->keep_ratio = value; });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    IntegerScaling,
    bool,
    RenderScreenImpl,
    { return context()->engine_profile->integer_scaling; },
    { context()->engine_profile->integer_scaling = value; });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    SmoothScaling,
    int32_t,
    RenderScreenImpl,
    { return context()->engine_profile->smooth_scaling; },
    { context()->engine_profile->smooth_scaling = value; });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    SmoothScalingDown,
    int32_t,
    RenderScreenImpl,
    { return context()->engine_profile->smooth_scaling_down; },
    { context()->engine_profile->smooth_scaling_down = value; });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    ScalingMode,
    int32_t,
    RenderScreenImpl,
    { return context()->engine_profile->scaling_mode; },
    { context()->engine_profile->scaling_mode = value; });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    BackgroundRunning,
    bool,
    RenderScreenImpl,
    { return context()->engine_profile->background_running; },
    { context()->engine_profile->background_running = value; });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    Ox,
    int32_t,
    RenderScreenImpl,
    { return origin_.x; },
    {
      origin_.x = value;
      GPUUpdateScreenWorldInternal();
    });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    Oy,
    int32_t,
    RenderScreenImpl,
    { return origin_.y; },
    {
      origin_.y = value;
      GPUUpdateScreenWorldInternal();
    });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    WindowTitle,
    std::string,
    RenderScreenImpl,
    { return SDL_GetWindowTitle(context()->window->AsSDLWindow()); },
    { SDL_SetWindowTitle(context()->window->AsSDLWindow(), value.c_str()); });

void RenderScreenImpl::FrameProcessInternal(
    Diligent::ITexture* present_target) {
  // Increase frame render count
  ++frame_count_;

  // Determine wait delay time
  limiter_.Delay();

  // Only run scaling pipeline if the upscale shader compiled successfully
  bool scaling_avail = context()->render.pipeline_states->upscale.RawPtr() != nullptr;

  // Recreate upscale buffer to match window size (if needed)
  if (scaling_avail)
    GPURecreateUpscaleBufferInternal();

  // Ensure CAS target exists before present_target selection
  if (scaling_avail && context()->engine_profile->cas_enabled)
    GPURecreateSharpenedBufferInternal();

  // Run scaling pass if upscale buffer is active
  bool scaling_done = false;
  if (scaling_avail && gpu_.upscale_buffer && present_target == gpu_.screen_buffer)
    scaling_done = GPUScalingPassInternal(context()->primary_render_context);
  if (scaling_done) {
    present_target = gpu_.upscale_buffer;
    if (context()->engine_profile->cas_enabled && gpu_.sharpened_buffer)
      present_target = gpu_.sharpened_buffer;
  }

  // Tick callback
  frame_tick_handler_.Run(present_target);
}

void RenderScreenImpl::RenderFrameInternal(Diligent::ITexture* render_target,
                                           Diligent::ITexture* depth_stencil) {
  // Submit pending canvas commands
  context()->canvas_scheduler->SubmitPendingPaintCommands();

  // Setup viewport
  controller_.CurrentViewport().bound = context()->resolution;
  controller_.CurrentViewport().origin = origin_;

  // Prepare for rendering context
  DrawableNode::RenderControllerParams controller_params;
  controller_params.context = context()->primary_render_context;
  controller_params.screen_buffer = render_target;
  controller_params.screen_depth_stencil = depth_stencil;
  controller_params.screen_size = context()->resolution;

  // 1) Execute pre-composite handler
  controller_.BroadCastNotification(DrawableNode::BEFORE_RENDER,
                                    &controller_params);

  // 1.5) Update sprite batch data
  context()->sprite_batcher->SubmitBatchDataAndResetCache(
      context()->render.quad_index.get(), controller_params.context);

  // 2) Setup renderpass
  GPUFrameBeginRenderPassInternal(controller_params.context, render_target,
                                  depth_stencil);

  // 3) Notify render a frame
  ScissorStack scissor_stack(controller_params.context, context()->resolution);
  controller_params.root_world = gpu_.root_transform;
  controller_params.world_binding = gpu_.world_transform;
  controller_params.scissors = &scissor_stack;
  controller_.BroadCastNotification(DrawableNode::ON_RENDERING,
                                    &controller_params);

  // 4) End render pass and process after-render effect
  GPUFrameEndRenderPassInternal(controller_params.context);
}

void RenderScreenImpl::GPUCreateGraphicsHostInternal() {
  // Create generic quads batch
  gpu_.transition_quads = renderer::QuadBatch::Make(**context()->render_device);
  gpu_.effect_quads = renderer::QuadBatch::Make(**context()->render_device);
  gpu_.scaling_quads = renderer::QuadBatch::Make(**context()->render_device);

  // Create generic shader binding
  gpu_.transition_binding_alpha =
      context()->render.pipeline_loader->alphatrans.CreateBinding();
  gpu_.transition_binding_vague =
      context()->render.pipeline_loader->mappedtrans.CreateBinding();
  gpu_.effect_binding =
      context()->render.pipeline_loader->color.CreateBinding();
  gpu_.upscale_binding =
      context()->render.pipeline_loader->upscale.CreateBinding();
  gpu_.anime4k_enhance_binding =
      context()->render.pipeline_loader->anime4k_enhance.CreateBinding();
  gpu_.cas_binding =
      context()->render.pipeline_loader->cas.CreateBinding();
  gpu_.mode_a_binding =
      context()->render.pipeline_loader->anime4k_clamp_hl_pass0.CreateBinding();
  gpu_.mode_a_restore_merge_binding =
      context()->render.pipeline_loader->anime4k_restore_pass7.CreateBinding();
  gpu_.mode_a_upscale_merge_binding =
      context()->render.pipeline_loader->anime4k_upscale_pass7.CreateBinding();
  gpu_.mode_a_upscale_d2s_binding =
      context()->render.pipeline_loader->anime4k_upscale_pass8.CreateBinding();

  // Create upscale params buffer (dynamic, no initial data)
  Diligent::CreateUniformBuffer(
      **context()->render_device,
      sizeof(renderer::Binding_Upscale::ScalingParams),
      "screen.upscale.params", &gpu_.upscale_params_buffer,
      Diligent::USAGE_DYNAMIC, Diligent::BIND_UNIFORM_BUFFER,
      Diligent::CPU_ACCESS_WRITE, nullptr);

  // Create screen buffer
  GPUResetScreenBufferInternal();
}

void RenderScreenImpl::GPUUpdateScreenWorldInternal() {
  renderer::WorldTransform world_transform;
  renderer::MakeProjectionMatrix(world_transform.projection,
                                 context()->resolution.Recast<float>());
  renderer::MakeTransformMatrix(world_transform.transform,
                                base::Vec2(-origin_.x, -origin_.y));

  gpu_.world_transform.Release();
  Diligent::CreateUniformBuffer(
      **context()->render_device, sizeof(world_transform),
      "graphics.world.transform", &gpu_.world_transform,
      Diligent::USAGE_IMMUTABLE, Diligent::BIND_UNIFORM_BUFFER,
      Diligent::CPU_ACCESS_NONE, &world_transform);
}

void RenderScreenImpl::GPUResetScreenBufferInternal() {
  // Reset original buffer
  gpu_.screen_buffer.Release();
  gpu_.screen_depth_stencil.Release();
  gpu_.frozen_buffer.Release();
  gpu_.frozen_depth_stencil.Release();
  gpu_.transition_buffer.Release();
  gpu_.transition_depth_stencil.Release();

  // Color attachment
  renderer::CreateTexture2D(
      **context()->render_device, &gpu_.screen_buffer, "screen.main.buffer",
      context()->resolution, Diligent::USAGE_DEFAULT,
      Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE);
  renderer::CreateTexture2D(
      **context()->render_device, &gpu_.frozen_buffer, "screen.frozen.buffer",
      context()->resolution, Diligent::USAGE_DEFAULT,
      Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE);
  renderer::CreateTexture2D(
      **context()->render_device, &gpu_.transition_buffer,
      "screen.transition.buffer", context()->resolution,
      Diligent::USAGE_DEFAULT,
      Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE);

  // Depth stencil
  renderer::CreateTexture2D(
      **context()->render_device, &gpu_.screen_depth_stencil,
      "screen.main.depth_stencil", context()->resolution,
      Diligent::USAGE_DEFAULT, Diligent::BIND_DEPTH_STENCIL,
      Diligent::CPU_ACCESS_NONE, Diligent::TEX_FORMAT_D24_UNORM_S8_UINT);
  renderer::CreateTexture2D(
      **context()->render_device, &gpu_.frozen_depth_stencil,
      "screen.frozen.depth_stencil", context()->resolution,
      Diligent::USAGE_DEFAULT, Diligent::BIND_DEPTH_STENCIL,
      Diligent::CPU_ACCESS_NONE, Diligent::TEX_FORMAT_D24_UNORM_S8_UINT);
  renderer::CreateTexture2D(
      **context()->render_device, &gpu_.transition_depth_stencil,
      "screen.transition.depth_stencil", context()->resolution,
      Diligent::USAGE_DEFAULT, Diligent::BIND_DEPTH_STENCIL,
      Diligent::CPU_ACCESS_NONE, Diligent::TEX_FORMAT_D24_UNORM_S8_UINT);

  // Root transform
  renderer::WorldTransform world_transform;
  renderer::MakeProjectionMatrix(world_transform.projection,
                                 context()->resolution.Recast<float>());
  renderer::MakeIdentityMatrix(world_transform.transform);

  gpu_.root_transform.Release();
  Diligent::CreateUniformBuffer(
      **context()->render_device, sizeof(world_transform),
      "graphics.root.transform", &gpu_.root_transform,
      Diligent::USAGE_IMMUTABLE, Diligent::BIND_UNIFORM_BUFFER,
      Diligent::CPU_ACCESS_NONE, &world_transform);

  // Offset transform
  GPUUpdateScreenWorldInternal();
}

void RenderScreenImpl::GPURecreateUpscaleBufferInternal() {
  auto* swapchain = context()->render_device->GetSwapChain();
  if (!swapchain)
    return;
  base::Vec2i window_size(swapchain->GetDesc().Width,
                          swapchain->GetDesc().Height);

  if (gpu_.upscale_buffer) {
    const auto& desc = gpu_.upscale_buffer->GetDesc();
    if (desc.Width == static_cast<uint32_t>(window_size.x) &&
        desc.Height == static_cast<uint32_t>(window_size.y))
      return;
  }

  gpu_.upscale_buffer.Release();
  renderer::CreateTexture2D(
      **context()->render_device, &gpu_.upscale_buffer,
      "screen.upscale.buffer", window_size, Diligent::USAGE_DEFAULT,
      Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE);
}

void RenderScreenImpl::GPURecreateAnime4KTargetsInternal() {
  GPURecreateAnime4KTargetsInternal(context()->resolution);
}

void RenderScreenImpl::GPURecreateAnime4KTargetsInternal(
    const base::Vec2i& size) {
  auto res = size;

  auto recreate = [&](RRefPtr<Diligent::ITexture>& tex, const char* name,
                      Diligent::TEXTURE_FORMAT fmt) {
    if (tex) {
      const auto& d = tex->GetDesc();
      if (d.Width == static_cast<uint32_t>(res.x) &&
          d.Height == static_cast<uint32_t>(res.y) && d.Format == fmt)
        return;
    }
    tex.Release();
    renderer::CreateTexture2D(**context()->render_device, &tex, name, res,
                              Diligent::USAGE_DEFAULT,
                              Diligent::BIND_RENDER_TARGET |
                                  Diligent::BIND_SHADER_RESOURCE,
                              Diligent::CPU_ACCESS_NONE, fmt);
  };

  recreate(gpu_.enhanced_tex, "anime4k.enhanced",
           Diligent::TEX_FORMAT_RGBA8_UNORM);
}

void RenderScreenImpl::GPURecreateSharpenedBufferInternal() {
  auto* swapchain = context()->render_device->GetSwapChain();
  if (!swapchain)
    return;
  base::Vec2i window_size(swapchain->GetDesc().Width,
                          swapchain->GetDesc().Height);

  if (gpu_.sharpened_buffer) {
    const auto& desc = gpu_.sharpened_buffer->GetDesc();
    if (desc.Width == static_cast<uint32_t>(window_size.x) &&
        desc.Height == static_cast<uint32_t>(window_size.y))
      return;
  }

  gpu_.sharpened_buffer.Release();
  renderer::CreateTexture2D(
      **context()->render_device, &gpu_.sharpened_buffer,
      "screen.cas.buffer", window_size, Diligent::USAGE_DEFAULT,
      Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE);
}

void RenderScreenImpl::GPURecreateModeATargetsInternal() {
  auto res = context()->resolution;  // 640×480

  auto recreate = [&](RRefPtr<Diligent::ITexture>& tex, const char* name,
                      base::Vec2i size, Diligent::TEXTURE_FORMAT fmt) {
    if (tex) {
      const auto& d = tex->GetDesc();
      if (d.Width == static_cast<uint32_t>(size.x) &&
          d.Height == static_cast<uint32_t>(size.y) && d.Format == fmt)
        return;
    }
    tex.Release();
    renderer::CreateTexture2D(**context()->render_device, &tex, name, size,
                              Diligent::USAGE_DEFAULT,
                              Diligent::BIND_RENDER_TARGET |
                                  Diligent::BIND_SHADER_RESOURCE,
                              Diligent::CPU_ACCESS_NONE, fmt);
  };

  recreate(gpu_.mode_a_tex0, "anime4k_a.tex0", res,
           Diligent::TEX_FORMAT_RGBA8_UNORM);
  recreate(gpu_.mode_a_tex1, "anime4k_a.tex1", res,
           Diligent::TEX_FORMAT_RGBA8_UNORM);
  // 2x upscale target
  recreate(gpu_.mode_a_tex2, "anime4k_a.tex2", res * 2,
           Diligent::TEX_FORMAT_RGBA8_UNORM);
  recreate(gpu_.mode_a_tex3, "anime4k_a.tex3", res * 2,
           Diligent::TEX_FORMAT_RGBA8_UNORM);

  // Restore CNN intermediate layers (7 textures for merge pass)
  auto& rt = gpu_.mode_a_restore_tex;
  if (rt.size() != 7) {
    rt.resize(7);
    for (int i = 0; i < 7; i++) {
      char name[32];
      std::snprintf(name, sizeof(name), "anime4k_a.restore_%d", i);
      rt[i].Release();
      renderer::CreateTexture2D(**context()->render_device, &rt[i], name,
                                res, Diligent::USAGE_DEFAULT,
                                Diligent::BIND_RENDER_TARGET |
                                    Diligent::BIND_SHADER_RESOURCE,
                                Diligent::CPU_ACCESS_NONE,
                                Diligent::TEX_FORMAT_RGBA8_UNORM);
    }
  }
}

// Forward declarations for static helpers defined below
static void SetAnime4KScissorAndViewport(Diligent::IDeviceContext* ctx,
                                         const base::Vec2i& size);
static void DrawScalingQuad(renderer::QuadBatch& quads,
                            Diligent::IDeviceContext* ctx,
                            Diligent::IBuffer* quad_index,
                            Diligent::VALUE_TYPE index_type);
static void WriteScalingParams(Diligent::IDeviceContext* ctx,
                               Diligent::IBuffer* buf,
                               const renderer::Binding_Upscale::ScalingParams& params);

void RenderScreenImpl::GPURunModeAPassesInternal(
    Diligent::IDeviceContext* render_context) {
  auto& states = *context()->render.pipeline_states;

  // Check all PSOs used across all phases
  auto check_pso = [](Diligent::IPipelineState* pso) { return pso != nullptr; };
  if (!check_pso(states.anime4k_clamp_hl_pass0) ||
      !check_pso(states.anime4k_clamp_hl_pass1) ||
      !check_pso(states.anime4k_clamp_hl_pass2) ||
      !check_pso(states.anime4k_restore_pass0) ||
      !check_pso(states.anime4k_restore_pass1) ||
      !check_pso(states.anime4k_restore_pass2) ||
      !check_pso(states.anime4k_restore_pass3) ||
      !check_pso(states.anime4k_restore_pass4) ||
      !check_pso(states.anime4k_restore_pass5) ||
      !check_pso(states.anime4k_restore_pass6) ||
      !check_pso(states.anime4k_restore_pass7) ||
      !check_pso(states.anime4k_upscale_pass0) ||
      !check_pso(states.anime4k_upscale_pass1) ||
      !check_pso(states.anime4k_upscale_pass2) ||
      !check_pso(states.anime4k_upscale_pass3) ||
      !check_pso(states.anime4k_upscale_pass4) ||
      !check_pso(states.anime4k_upscale_pass5) ||
      !check_pso(states.anime4k_upscale_pass6) ||
      !check_pso(states.anime4k_upscale_pass7) ||
      !check_pso(states.anime4k_upscale_pass8) ||
      !check_pso(states.anime4k_enhance))
    return;

  // Check bindings
  if (!gpu_.mode_a_binding.u_texture || !gpu_.mode_a_binding.u_params ||
      !gpu_.mode_a_restore_merge_binding.u_texture ||
      !gpu_.mode_a_restore_merge_binding.u_texture1 ||
      !gpu_.mode_a_upscale_merge_binding.u_texture1 ||
      !gpu_.mode_a_upscale_merge_binding.u_params ||
      !gpu_.mode_a_upscale_d2s_binding.u_texture ||
      !gpu_.mode_a_upscale_d2s_binding.u_params ||
      !gpu_.anime4k_enhance_binding.u_texture ||
      !gpu_.anime4k_enhance_binding.u_params)
    return;

  // Check screen buffer and params buffer (not covered by PSO/binding checks)
  if (!gpu_.screen_buffer || !gpu_.upscale_params_buffer)
    return;

  GPURecreateModeATargetsInternal();
  if (!gpu_.mode_a_tex0 || !gpu_.mode_a_tex1 || !gpu_.mode_a_tex2 ||
      !gpu_.mode_a_tex3)
    return;

  // Check restore intermediate textures
  auto& rt_check = gpu_.mode_a_restore_tex;
  if (rt_check.size() < 7)
    return;
  for (auto& tex : rt_check) {
    if (!tex)
      return;
  }


  auto* profile = context()->engine_profile;
  auto native = context()->resolution;
  auto upscaled = native * 2;
  auto* quad_index = **context()->render.quad_index;
  auto index_type = context()->render.quad_index->GetIndexType();

  auto& loader = *context()->render.pipeline_loader;
  // states already declared above

  // Helper: set viewport for given size
  auto set_viewport = [&](const base::Vec2i& size) {
    SetAnime4KScissorAndViewport(render_context, size);
  };

  // Helper: get RTV from texture
  auto get_rtv = [](RRefPtr<Diligent::ITexture>& tex) {
    return tex->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
  };
  auto get_srv = [](RRefPtr<Diligent::ITexture>& tex) {
    return tex->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
  };

  // Helper: clear and set RT, draw a single-texture pass
  auto run_pass = [&](Diligent::IPipelineState* pso,
                       RRefPtr<Diligent::ITexture>& out_tex,
                       RRefPtr<Diligent::ITexture>& in_tex,
                       const base::Vec2i& viewport_size,
                       bool clear = false) {
    auto* rtv = get_rtv(out_tex);
    render_context->SetRenderTargets(
        1, &rtv, nullptr,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    if (clear) {
      float clr[] = {0, 0, 0, 0};
      render_context->ClearRenderTarget(
          rtv, clr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
    set_viewport(viewport_size);

    render_context->SetPipelineState(pso);
    gpu_.mode_a_binding.u_texture->Set(get_srv(in_tex));
    gpu_.mode_a_binding.u_params->Set(gpu_.upscale_params_buffer);

    render_context->CommitShaderResources(
        *gpu_.mode_a_binding,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    DrawScalingQuad(gpu_.scaling_quads, render_context, quad_index, index_type);
  };

  // Helper: write scaling params
  auto write_params = [&](const renderer::Binding_Upscale::ScalingParams& p) {
    WriteScalingParams(render_context, gpu_.upscale_params_buffer.RawPtr(), p);
  };

  // ── Chain setup ──
  // tex0, tex1: 640×480 ping-pong
  // screen_buffer: original game render
  // tex2: 1280×960 upscale target
  auto& tex0 = gpu_.mode_a_tex0;
  auto& tex1 = gpu_.mode_a_tex1;
  auto& tex2 = gpu_.mode_a_tex2;
  auto& screen = gpu_.screen_buffer;

  // Scaling params for native (1:1)
  renderer::Binding_Upscale::ScalingParams native_params;
  native_params.input_size = native.Recast<float>();
  native_params.output_size = native.Recast<float>();
  native_params.input_pt = base::Vec2(1.0f / native.x, 1.0f / native.y);
  native_params.output_pt = base::Vec2(1.0f / native.x, 1.0f / native.y);
  native_params.mode = 0;
  native_params.bicubic_c = profile->scaling_darken_strength;

  // Scaling params for 2x upscale
  renderer::Binding_Upscale::ScalingParams upscale_params;
  upscale_params.input_size = native.Recast<float>();
  upscale_params.output_size = upscaled.Recast<float>();
  upscale_params.input_pt = base::Vec2(1.0f / native.x, 1.0f / native.y);
  upscale_params.output_pt = base::Vec2(1.0f / upscaled.x, 1.0f / upscaled.y);
  upscale_params.mode = 0;

  write_params(native_params);

  // ════════════════════════════════════════════
  // Phase 1: Clamp_Highlights (3 passes × 640×480)
  // ════════════════════════════════════════════
  // Pass 0: horiz stats → tex0 (read from screen)
  run_pass(states.anime4k_clamp_hl_pass0, tex0, screen, native, true);
  // Pass 1: vert stats → tex1 (single texture, reads from tex0)
  run_pass(states.anime4k_clamp_hl_pass1, tex1, tex0, native);

  // Pass 2: clamp → tex0 (u_Texture = STATSMAX from tex1, u_Texture1 = HOOKED from screen)
  {
    auto* rtv = get_rtv(tex0);
    render_context->SetRenderTargets(
        1, &rtv, nullptr,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    set_viewport(native);
    render_context->SetPipelineState(states.anime4k_clamp_hl_pass2);
    gpu_.mode_a_binding.u_texture->Set(get_srv(tex1));
    gpu_.mode_a_binding.u_params->Set(gpu_.upscale_params_buffer);
    auto* u_tex1_hooked = (*gpu_.mode_a_binding)
        ->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "u_Texture1");
    if (u_tex1_hooked)
      u_tex1_hooked->Set(get_srv(screen));
    render_context->CommitShaderResources(
        *gpu_.mode_a_binding,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    DrawScalingQuad(gpu_.scaling_quads, render_context, quad_index, index_type);
  }

  // ════════════════════════════════════════════
  // Phase 2: Restore_CNN (8 passes × 640×480)
  // ════════════════════════════════════════════
  // Each conv pass writes to a unique intermediate texture (restore_tex[0..5])
  // so that the merge pass (pass7) can read from all 7 intermediate layers.
  // Input: tex0 (clamped output), output: tex0

  // Ensure intermediate textures exist
  auto& rt = gpu_.mode_a_restore_tex;
  if (rt.size() < 7)
    GPURecreateModeATargetsInternal();

  // Pass 0: Conv-4x3x3x3 → restore_tex[0] (conv2d_tf)
  run_pass(states.anime4k_restore_pass0, rt[0], tex0, native);

  // Passes 1-6: Conv-4x3x3x8 → restore_tex[1..6]
  for (int pi = 0; pi < 6; pi++) {
    Diligent::IPipelineState* pso = nullptr;
    switch (pi) {
      case 0: pso = states.anime4k_restore_pass1; break;
      case 1: pso = states.anime4k_restore_pass2; break;
      case 2: pso = states.anime4k_restore_pass3; break;
      case 3: pso = states.anime4k_restore_pass4; break;
      case 4: pso = states.anime4k_restore_pass5; break;
      case 5: pso = states.anime4k_restore_pass6; break;
    }
    auto& prev = (pi == 0) ? rt[0] : rt[pi];
    run_pass(pso, rt[pi + 1], prev, native);
  }

  // Pass 7: Merge - reads 8 textures (screen + 7 intermediate layers)
  // u_Texture = screen (residual), u_Texture1..7 = rt[0..6]
  {
    auto* rtv = get_rtv(tex0);
    render_context->SetRenderTargets(
        1, &rtv, nullptr,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    float clr[] = {0, 0, 0, 0};
    render_context->ClearRenderTarget(
        rtv, clr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    set_viewport(native);

    render_context->SetPipelineState(states.anime4k_restore_pass7);
    gpu_.mode_a_restore_merge_binding.u_texture->Set(get_srv(screen));
    gpu_.mode_a_restore_merge_binding.u_texture1->Set(get_srv(rt[0]));
    gpu_.mode_a_restore_merge_binding.u_texture2->Set(get_srv(rt[1]));
    gpu_.mode_a_restore_merge_binding.u_texture3->Set(get_srv(rt[2]));
    gpu_.mode_a_restore_merge_binding.u_texture4->Set(get_srv(rt[3]));
    gpu_.mode_a_restore_merge_binding.u_texture5->Set(get_srv(rt[4]));
    gpu_.mode_a_restore_merge_binding.u_texture6->Set(get_srv(rt[5]));
    gpu_.mode_a_restore_merge_binding.u_texture7->Set(get_srv(rt[6]));
    gpu_.mode_a_restore_merge_binding.u_params->Set(gpu_.upscale_params_buffer);
    render_context->CommitShaderResources(
        *gpu_.mode_a_restore_merge_binding,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    DrawScalingQuad(gpu_.scaling_quads, render_context, quad_index, index_type);
  }

  // ════════════════════════════════════════════
  // Phase 3: Upscale_CNN_x2 (9 passes → 1280×960)
  // ════════════════════════════════════════════
  // Pass 0-6: 7 conv passes at native res, each writes to a unique
  //   intermediate texture (reuse mode_a_restore_tex[0..6]).
  // Pass 7: merge – reads 7 intermediate textures (u_Texture1..u_Texture7),
  //   writes to tex1 at native res.
  // Pass 8: depth-to-space – reads merge output (tex1) + screen residual,
  //   writes 2× result to tex2.

  // Pass 0: read from tex0 (restored output), write to rt[0]
  write_params(native_params);
  run_pass(states.anime4k_upscale_pass0, rt[0], tex0, native);

  // Passes 1-6: read from previous intermediate, write to next
  Diligent::IPipelineState* upscale_conv_psos[] = {
      states.anime4k_upscale_pass1,
      states.anime4k_upscale_pass2,
      states.anime4k_upscale_pass3,
      states.anime4k_upscale_pass4,
      states.anime4k_upscale_pass5,
      states.anime4k_upscale_pass6,
  };
  for (int upi = 0; upi < 6; upi++) {
    run_pass(upscale_conv_psos[upi], rt[upi + 1], rt[upi], native);
  }

  // Pass 7: Merge – reads 7 intermediate layers → tex1 at native res
  // No u_Texture (screen residual) – upscale merge only uses intermediate layers
  {
    write_params(native_params);
    auto* rtv_m = get_rtv(tex1);
    render_context->SetRenderTargets(
        1, &rtv_m, nullptr,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    float clr[] = {0, 0, 0, 0};
    render_context->ClearRenderTarget(
        rtv_m, clr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    set_viewport(native);

    render_context->SetPipelineState(states.anime4k_upscale_pass7);
    gpu_.mode_a_upscale_merge_binding.u_texture1->Set(get_srv(rt[0]));
    gpu_.mode_a_upscale_merge_binding.u_texture2->Set(get_srv(rt[1]));
    gpu_.mode_a_upscale_merge_binding.u_texture3->Set(get_srv(rt[2]));
    gpu_.mode_a_upscale_merge_binding.u_texture4->Set(get_srv(rt[3]));
    gpu_.mode_a_upscale_merge_binding.u_texture5->Set(get_srv(rt[4]));
    gpu_.mode_a_upscale_merge_binding.u_texture6->Set(get_srv(rt[5]));
    gpu_.mode_a_upscale_merge_binding.u_texture7->Set(get_srv(rt[6]));
    gpu_.mode_a_upscale_merge_binding.u_params->Set(gpu_.upscale_params_buffer);
    render_context->CommitShaderResources(
        *gpu_.mode_a_upscale_merge_binding,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    DrawScalingQuad(gpu_.scaling_quads, render_context, quad_index, index_type);
  }

  // Pass 8: Depth-to-space — reads merge output (tex1, native res) + screen,
  //         writes 2× result to tex2.
  // u_Texture = merge output, u_Texture1 = screen residual
  {
    write_params(upscale_params);
    auto* rtv = get_rtv(tex2);
    render_context->SetRenderTargets(
        1, &rtv, nullptr,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    float clr[] = {0, 0, 0, 0};
    render_context->ClearRenderTarget(
        rtv, clr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    set_viewport(upscaled);
    render_context->SetPipelineState(states.anime4k_upscale_pass8);
    gpu_.mode_a_upscale_d2s_binding.u_texture->Set(get_srv(tex1));
    gpu_.mode_a_upscale_d2s_binding.u_params->Set(gpu_.upscale_params_buffer);
    auto* u_tex1_residual = (*gpu_.mode_a_upscale_d2s_binding)
        ->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "u_Texture1");
    if (u_tex1_residual)
      u_tex1_residual->Set(get_srv(screen));
    render_context->CommitShaderResources(
        *gpu_.mode_a_upscale_d2s_binding,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    DrawScalingQuad(gpu_.scaling_quads, render_context, quad_index, index_type);
  }

  // ════════════════════════════════════════════
  // Phase 4: Anime4K_Enhance(DoG) on upscaled result
  // ════════════════════════════════════════════
  // Read from tex2 (Pass4 d2s output), write to enhanced_tex at 2×.
  {
    GPURecreateAnime4KTargetsInternal(upscaled);
    if (gpu_.enhanced_tex) {
      auto* rtv_e = get_rtv(gpu_.enhanced_tex);
      render_context->SetRenderTargets(
          1, &rtv_e, nullptr,
          Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
      float clr[] = {0, 0, 0, 0};
      render_context->ClearRenderTarget(
          rtv_e, clr,
          Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
      set_viewport(upscaled);

      // Set up DoG params
      auto* profile_p = context()->engine_profile;
      renderer::Binding_Upscale::ScalingParams dog_params;
      dog_params.input_size = upscaled.Recast<float>();
      dog_params.output_size = upscaled.Recast<float>();
      dog_params.input_pt = base::Vec2(1.0f / upscaled.x, 1.0f / upscaled.y);
      dog_params.output_pt = base::Vec2(1.0f / upscaled.x, 1.0f / upscaled.y);
      dog_params.mode = 0;  // DoG mode (no Sobel)
      dog_params.bicubic_c = profile_p->scaling_darken_strength;
      dog_params.ar_strength = profile_p->scaling_sobel_strength;
      dog_params.bicubic_b = profile_p->scaling_warp_strength;
      write_params(dog_params);

      render_context->SetPipelineState(states.anime4k_enhance);
      gpu_.anime4k_enhance_binding.u_texture->Set(get_srv(tex2));
      gpu_.anime4k_enhance_binding.u_params->Set(gpu_.upscale_params_buffer);
      render_context->CommitShaderResources(
          *gpu_.anime4k_enhance_binding,
          Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
      DrawScalingQuad(gpu_.scaling_quads, render_context,
                      quad_index, index_type);
    }
  }
}

static void SetAnime4KScissorAndViewport(Diligent::IDeviceContext* ctx,
                                         const base::Vec2i& size) {
  Diligent::Viewport vp;
  vp.Width = static_cast<float>(size.x);
  vp.Height = static_cast<float>(size.y);
  vp.MinDepth = 0.0f;
  vp.MaxDepth = 1.0f;
  ctx->SetViewports(1, &vp, size.x, size.y);
  Diligent::Rect sc(0, 0, size.x, size.y);
  ctx->SetScissorRects(1, &sc, UINT32_MAX, UINT32_MAX);
}

static void DrawScalingQuad(renderer::QuadBatch& quads,
                            Diligent::IDeviceContext* ctx,
                            Diligent::IBuffer* quad_index,
                            Diligent::VALUE_TYPE index_type) {
  renderer::Quad quad;
  renderer::Quad::SetPositionRect(&quad,
      base::RectF(-1.0f, 1.0f, 2.0f, -2.0f));
  renderer::Quad::SetTexCoordRectNorm(&quad,
      base::RectF(base::Vec2(0), base::Vec2(1)));
  renderer::Quad::SetColor(&quad, base::Vec4(1, 1, 1, 1));
  quads.QueueWrite(ctx, &quad);

  Diligent::IBuffer* vb = *quads;
  ctx->SetVertexBuffers(0, 1, &vb, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  ctx->SetIndexBuffer(quad_index, 0,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  Diligent::DrawIndexedAttribs da;
  da.NumIndices = 6;
  da.IndexType = index_type;
  da.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
  ctx->DrawIndexed(da);
}

static void WriteScalingParams(
    Diligent::IDeviceContext* ctx,
    Diligent::IBuffer* buf,
    const renderer::Binding_Upscale::ScalingParams& params) {
  Diligent::MapHelper<renderer::Binding_Upscale::ScalingParams> map(
      ctx, buf, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
  if (map)
    *map = params;
}

bool RenderScreenImpl::GPUScalingPassInternal(
    Diligent::IDeviceContext* render_context) {
  if (!gpu_.upscale_buffer || !gpu_.upscale_params_buffer)
    return false;

  auto* swapchain = context()->render_device->GetSwapChain();
  if (!swapchain)
    return false;
  base::Vec2i output_size(swapchain->GetDesc().Width,
                          swapchain->GetDesc().Height);
  base::Vec2i input_size = context()->resolution;
  auto* profile = context()->engine_profile;
  int mode = profile->scaling_mode;
  bool anime4k_active = false;

  // --- Anime4K Mode A multi-pass path (Clamp + Restore + Upscale) ---
  if (mode == 6) {
    GPURunModeAPassesInternal(render_context);
    if (gpu_.enhanced_tex)
      anime4k_active = true;
    // Mode A produces 2× output in enhanced_tex; existing upscale path will read from it
    input_size = input_size * 2;
    mode = 2;  // Lanczos3 for final upscale to window
  } else if (mode == 4 || mode == 5) {
    // --- Anime4K single-pass path (inline 7x7 gauss + DoG clamp) ---
    // mode 5 = Anime4K + Sobel adaptive blend
    anime4k_active = true;
    GPURecreateAnime4KTargetsInternal();
    if (!gpu_.enhanced_tex)
      return false;

    base::Vec2i native_size = input_size;
    base::Vec2 input_pt = {1.0f / native_size.x, 1.0f / native_size.y};

    auto* rtv_e = gpu_.enhanced_tex->GetDefaultView(
        Diligent::TEXTURE_VIEW_RENDER_TARGET);
    render_context->SetRenderTargets(
        1, &rtv_e, nullptr,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    float clr[] = {0, 0, 0, 0};
    render_context->ClearRenderTarget(
        rtv_e, clr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    SetAnime4KScissorAndViewport(render_context, native_size);

    renderer::Binding_Upscale::ScalingParams params;
    params.input_size = native_size.Recast<float>();
    params.output_size = native_size.Recast<float>();
    params.input_pt = input_pt;
    params.output_pt = input_pt;
    params.mode = (mode == 5) ? 1 : 0;
    params.bicubic_c = profile->scaling_darken_strength;
    if (mode == 5) {
      params.ar_strength = profile->scaling_sobel_strength;
      params.bicubic_b = profile->scaling_warp_strength;
    }
    WriteScalingParams(render_context, gpu_.upscale_params_buffer.RawPtr(),
                       params);

    render_context->SetPipelineState(
        context()->render.pipeline_states->anime4k_enhance.RawPtr());
    gpu_.anime4k_enhance_binding.u_texture->Set(
        gpu_.screen_buffer->GetDefaultView(
            Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
    gpu_.anime4k_enhance_binding.u_params->Set(gpu_.upscale_params_buffer);

    render_context->CommitShaderResources(
        *gpu_.anime4k_enhance_binding,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    DrawScalingQuad(gpu_.scaling_quads, render_context,
                    **context()->render.quad_index,
                    context()->render.quad_index->GetIndexType());

    // Pass 5: Upscale
    mode = 2;
    input_size = native_size;
  }

  // --- Common upscale path (mode 0-3 or Anime4K pass 5) ---
  {
    auto* rtv = gpu_.upscale_buffer->GetDefaultView(
        Diligent::TEXTURE_VIEW_RENDER_TARGET);
    render_context->SetRenderTargets(
        1, &rtv, nullptr,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    float clear_color[] = {0, 0, 0, 0};
    render_context->ClearRenderTarget(
        rtv, clear_color,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    Diligent::Viewport viewport;
    viewport.Width = static_cast<float>(output_size.x);
    viewport.Height = static_cast<float>(output_size.y);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    render_context->SetViewports(1, &viewport, output_size.x, output_size.y);

    Diligent::Rect scissor(0, 0, output_size.x, output_size.y);
    render_context->SetScissorRects(1, &scissor, UINT32_MAX, UINT32_MAX);

    render_context->SetPipelineState(
        context()->render.pipeline_states->upscale.RawPtr());

    renderer::Binding_Upscale::ScalingParams params;
    params.input_size = input_size.Recast<float>();
    params.output_size = output_size.Recast<float>();
    params.input_pt =
        base::Vec2(1.0f / input_size.x, 1.0f / input_size.y);
    params.output_pt =
        base::Vec2(1.0f / output_size.x, 1.0f / output_size.y);
    params.mode = static_cast<uint32_t>(mode);
    params.ar_strength = profile->scaling_ar_strength;
    params.bicubic_b = profile->scaling_bicubic_b;
    params.bicubic_c = profile->scaling_bicubic_c;
    WriteScalingParams(render_context, gpu_.upscale_params_buffer.RawPtr(),
                       params);

    auto* source_tex_srv =
        (anime4k_active && gpu_.enhanced_tex ? gpu_.enhanced_tex
                                             : gpu_.screen_buffer)
            ->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
    gpu_.upscale_binding.u_texture->Set(source_tex_srv);
    gpu_.upscale_binding.u_params->Set(gpu_.upscale_params_buffer);

    render_context->CommitShaderResources(
        *gpu_.upscale_binding,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawScalingQuad(gpu_.scaling_quads, render_context,
                    **context()->render.quad_index,
                    context()->render.quad_index->GetIndexType());
  }

  // --- CAS post-process (optional, applies to any upscale mode) ---
  if (profile->cas_enabled &&
      context()->render.pipeline_states->cas.RawPtr()) {
    GPURecreateSharpenedBufferInternal();
    if (!gpu_.sharpened_buffer)
      return false;

    auto* rtv_cas = gpu_.sharpened_buffer->GetDefaultView(
        Diligent::TEXTURE_VIEW_RENDER_TARGET);
    render_context->SetRenderTargets(
        1, &rtv_cas, nullptr,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    Diligent::Viewport vp;
    vp.Width = static_cast<float>(output_size.x);
    vp.Height = static_cast<float>(output_size.y);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    render_context->SetViewports(1, &vp, output_size.x, output_size.y);

    Diligent::Rect sc(0, 0, output_size.x, output_size.y);
    render_context->SetScissorRects(1, &sc, UINT32_MAX, UINT32_MAX);

    render_context->SetPipelineState(
        context()->render.pipeline_states->cas.RawPtr());

    renderer::Binding_Upscale::ScalingParams params = {};
    params.output_pt =
        base::Vec2(1.0f / output_size.x, 1.0f / output_size.y);
    params.cas_sharpness = profile->cas_sharpness;
    WriteScalingParams(render_context, gpu_.upscale_params_buffer.RawPtr(),
                       params);

    gpu_.cas_binding.u_texture->Set(
        gpu_.upscale_buffer->GetDefaultView(
            Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
    gpu_.cas_binding.u_params->Set(gpu_.upscale_params_buffer);

    render_context->CommitShaderResources(
        *gpu_.cas_binding,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawScalingQuad(gpu_.scaling_quads, render_context,
                    **context()->render.quad_index,
                    context()->render.quad_index->GetIndexType());
  }
  return true;
}

void RenderScreenImpl::GPUPresentScreenBufferInternal(
    Diligent::IDeviceContext* render_context,
    Diligent::ImGuiDiligentRenderer* gui_renderer) {
  // Initial swapchain attribute
  Diligent::ISwapChain* swapchain = context()->render_device->GetSwapChain();
  if (!swapchain)
    return;
  auto* render_target_view = swapchain->GetCurrentBackBufferRTV();
  auto* depth_stencil_view = swapchain->GetDepthBufferDSV();

  // Prepare for rendering
  float clear_color[] = {0, 0, 0, 1};
  render_context->SetRenderTargets(
      1, &render_target_view, depth_stencil_view,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  render_context->ClearRenderTarget(
      render_target_view, clear_color,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Apply present scissor
  Diligent::Rect present_scissor(0, 0, swapchain->GetDesc().Width,
                                 swapchain->GetDesc().Height);
  render_context->SetScissorRects(1, &present_scissor, UINT32_MAX, UINT32_MAX);

  // Render GUI and present
  gui_renderer->RenderDrawData(render_context, ImGui::GetDrawData());

  // Flush command buffer and present GPU surface
  swapchain->Present(context()->engine_profile->vsync);
}

void RenderScreenImpl::GPUFrameBeginRenderPassInternal(
    Diligent::IDeviceContext* render_context,
    Diligent::ITexture* render_target,
    Diligent::ITexture* depth_stencil) {
  // Setup screen effect params
  if (brightness_ < 255) {
    renderer::Quad effect_quad;
    renderer::Quad::SetPositionRect(&effect_quad,
                                    base::Rect(context()->resolution));
    renderer::Quad::SetColor(&effect_quad,
                             base::Vec4(0, 0, 0, (255 - brightness_) / 255.0f));
    gpu_.effect_quads.QueueWrite(render_context, &effect_quad);
  }

  // Setup render pass
  auto* render_target_view =
      render_target->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
  auto* depth_stencil_view =
      depth_stencil->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
  render_context->SetRenderTargets(
      1, &render_target_view, depth_stencil_view,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  const float clear_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
  render_context->ClearRenderTarget(
      render_target_view, clear_color,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  render_context->ClearDepthStencil(
      depth_stencil_view, Diligent::CLEAR_DEPTH_FLAG, 1.0f, 0,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Push scissor
  Diligent::Rect render_scissor(0, 0, render_target->GetDesc().Width,
                                render_target->GetDesc().Height);
  render_context->SetScissorRects(1, &render_scissor, UINT32_MAX, UINT32_MAX);
}

void RenderScreenImpl::GPUFrameEndRenderPassInternal(
    Diligent::IDeviceContext* render_context) {
  // Render screen effect if need
  if (brightness_ < 255) {
    // Apply brightness effect
    auto* pipeline = context()->render.pipeline_states->brightness.RawPtr();

    // Set world transform
    gpu_.effect_binding.u_transform->Set(gpu_.world_transform);

    // Apply pipeline state
    render_context->SetPipelineState(pipeline);
    render_context->CommitShaderResources(
        *gpu_.effect_binding,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Apply vertex index
    Diligent::IBuffer* const vertex_buffer = *gpu_.effect_quads;
    render_context->SetVertexBuffers(
        0, 1, &vertex_buffer, nullptr,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    render_context->SetIndexBuffer(
        **context()->render.quad_index, 0,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Execute render command
    Diligent::DrawIndexedAttribs draw_indexed_attribs;
    draw_indexed_attribs.NumIndices = 6;
    draw_indexed_attribs.IndexType =
        context()->render.quad_index->GetIndexType();
    render_context->DrawIndexed(draw_indexed_attribs);
  }
}

void RenderScreenImpl::GPURenderAlphaTransitionFrameInternal(
    Diligent::IDeviceContext* render_context,
    float progress) {
  // Update transition uniform
  renderer::Quad transient_quad;
  renderer::Quad::SetPositionRect(&transient_quad,
                                  base::RectF(-1.0f, 1.0f, 2.0f, -2.0f));
  renderer::Quad::SetTexCoordRectNorm(
      &transient_quad, base::RectF(base::Vec2(0), base::Vec2(1)));
  renderer::Quad::SetColor(&transient_quad, base::Vec4(progress));
  gpu_.transition_quads.QueueWrite(render_context, &transient_quad);

  // Composite transition frame
  auto render_target_view =
      gpu_.screen_buffer->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
  render_context->SetRenderTargets(
      1, &render_target_view, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  const float clear_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
  render_context->ClearRenderTarget(
      render_target_view, clear_color,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Derive pipeline sets
  auto* pipeline = context()->render.pipeline_states->alpha_transition.RawPtr();

  // Set uniform texture
  gpu_.transition_binding_alpha.u_current_texture->Set(
      gpu_.transition_buffer->GetDefaultView(
          Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
  gpu_.transition_binding_alpha.u_frozen_texture->Set(
      gpu_.frozen_buffer->GetDefaultView(
          Diligent::TEXTURE_VIEW_SHADER_RESOURCE));

  // Apply pipeline state
  render_context->SetPipelineState(pipeline);
  render_context->CommitShaderResources(
      *gpu_.transition_binding_alpha,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Apply vertex index
  Diligent::IBuffer* const vertex_buffer = *gpu_.transition_quads;
  render_context->SetVertexBuffers(
      0, 1, &vertex_buffer, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  render_context->SetIndexBuffer(
      **context()->render.quad_index, 0,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Execute render command
  Diligent::DrawIndexedAttribs draw_indexed_attribs;
  draw_indexed_attribs.NumIndices = 6;
  draw_indexed_attribs.IndexType = context()->render.quad_index->GetIndexType();
  render_context->DrawIndexed(draw_indexed_attribs);
}

void RenderScreenImpl::GPURenderVagueTransitionFrameInternal(
    Diligent::IDeviceContext* render_context,
    Diligent::ITextureView* trans_mapping,
    float progress,
    float vague) {
  // Update transition uniform
  renderer::Quad transient_quad;
  renderer::Quad::SetPositionRect(&transient_quad,
                                  base::RectF(-1.0f, 1.0f, 2.0f, -2.0f));
  renderer::Quad::SetTexCoordRectNorm(
      &transient_quad, base::RectF(base::Vec2(0), base::Vec2(1)));
  renderer::Quad::SetColor(&transient_quad, base::Vec4(vague, 0, 0, progress));
  gpu_.transition_quads.QueueWrite(render_context, &transient_quad);

  // Composite transition frame
  auto render_target_view =
      gpu_.screen_buffer->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
  render_context->SetRenderTargets(
      1, &render_target_view, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  const float clear_color[] = {0, 0, 0, 1};
  render_context->ClearRenderTarget(
      render_target_view, clear_color,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Derive pipeline sets
  auto* pipeline = context()->render.pipeline_states->vague_transition.RawPtr();

  // Set uniform texture
  gpu_.transition_binding_vague.u_current_texture->Set(
      gpu_.transition_buffer->GetDefaultView(
          Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
  gpu_.transition_binding_vague.u_frozen_texture->Set(
      gpu_.frozen_buffer->GetDefaultView(
          Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
  gpu_.transition_binding_vague.u_trans_texture->Set(trans_mapping);

  // Apply pipeline state
  render_context->SetPipelineState(pipeline);
  render_context->CommitShaderResources(
      *gpu_.transition_binding_vague,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Apply vertex index
  Diligent::IBuffer* const vertex_buffer = *gpu_.transition_quads;
  render_context->SetVertexBuffers(
      0, 1, &vertex_buffer, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  render_context->SetIndexBuffer(
      **context()->render.quad_index, 0,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Execute render command
  Diligent::DrawIndexedAttribs draw_indexed_attribs;
  draw_indexed_attribs.NumIndices = 6;
  draw_indexed_attribs.IndexType = context()->render.quad_index->GetIndexType();
  render_context->DrawIndexed(draw_indexed_attribs);
}

}  // namespace content
