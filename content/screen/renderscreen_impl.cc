// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/screen/renderscreen_impl.h"

#include <unordered_map>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"
#include "magic_enum/magic_enum.hpp"

#include "content/canvas/canvas_scheduler.h"
#include "content/profile/command_ids.h"
#include "renderer/utils/texture_utils.h"

namespace content {

RenderScreenImpl::RenderScreenImpl(CoroutineContext* cc,
                                   ContentProfile* profile,
                                   I18NProfile* i18n_profile,
                                   filesystem::IOService* io_service,
                                   ScopedFontData* scoped_font,
                                   const base::Vec2i& resolution,
                                   int frame_rate)
    : cc_(cc),
      profile_(profile),
      i18n_profile_(i18n_profile),
      io_service_(io_service),
      render_worker_(nullptr),
      scoped_font_(scoped_font),
      agent_(nullptr),
      frozen_(false),
      resolution_(resolution),
      brightness_(255),
      frame_count_(0),
      frame_rate_(frame_rate),
      elapsed_time_(0.0),
      smooth_delta_time_(1.0),
      last_count_time_(SDL_GetPerformanceCounter()),
      desired_delta_time_(SDL_GetPerformanceFrequency() / frame_rate_),
      frame_skip_required_(false),
      keep_ratio_(true),
      smooth_scale_(false),
      allow_skip_frame_(true),
      allow_background_running_(true) {}

RenderScreenImpl::~RenderScreenImpl() {
  base::ThreadWorker::PostTask(
      render_worker_,
      base::BindOnce(&RenderScreenImpl::DestroyGraphicsDeviceInternal,
                     base::Unretained(this)));
  base::ThreadWorker::WaitWorkerSynchronize(render_worker_);
}

void RenderScreenImpl::InitWithRenderWorker(base::ThreadWorker* render_worker,
                                            base::WeakPtr<ui::Widget> window,
                                            const std::string& wgpu_backend) {
  // Setup render thread worker (maybe null)
  render_worker_ = render_worker;

  // Setup render device on render thread if possible
  agent_ = new RenderGraphicsAgent;
  base::ThreadWorker::PostTask(
      render_worker,
      base::BindOnce(&RenderScreenImpl::InitGraphicsDeviceInternal,
                     base::Unretained(this), window, wgpu_backend));
  base::ThreadWorker::WaitWorkerSynchronize(render_worker);
}

void RenderScreenImpl::PresentScreen() {
  // Determine update repeat time
  const uint64_t now_time = SDL_GetPerformanceCounter();
  const uint64_t delta_time = now_time - last_count_time_;
  last_count_time_ = now_time;

  // Calculate smooth frame rate
  const double delta_rate =
      delta_time / static_cast<double>(desired_delta_time_);
  const int repeat_time = DetermineRepeatNumberInternal(delta_rate);

  for (int i = 0; i < repeat_time; ++i) {
    frame_skip_required_ = (i != 0);
    fiber_switch(cc_->main_loop_fiber);
  }

  // Present to screen surface
  base::ThreadWorker::PostTask(
      render_worker_,
      base::BindOnce(&RenderScreenImpl::PresentScreenBufferInternal,
                     base::Unretained(this),
                     base::Unretained(agent_->present_target)));
  base::ThreadWorker::WaitWorkerSynchronize(render_worker_);
}

renderer::RenderDevice* RenderScreenImpl::GetDevice() const {
  return agent_->device.get();
}

CanvasScheduler* RenderScreenImpl::GetCanvasScheduler() const {
  return agent_->canvas_scheduler.get();
}

SpriteBatch* RenderScreenImpl::GetSpriteBatch() const {
  return agent_->sprite_batch.get();
}

ScopedFontData* RenderScreenImpl::GetScopedFontContext() const {
  return scoped_font_;
}

void RenderScreenImpl::PostTask(base::OnceClosure task) {
  base::ThreadWorker::PostTask(render_worker_, std::move(task));
}

void RenderScreenImpl::WaitWorkerSynchronize() {
  base::ThreadWorker::WaitWorkerSynchronize(render_worker_);
}

base::CallbackListSubscription RenderScreenImpl::AddTickObserver(
    const base::RepeatingClosure& handler) {
  return tick_observers_.Add(handler);
}

void RenderScreenImpl::RenderFrameInternal(DrawNodeController* controller,
                                           Diligent::ITexture* render_target,
                                           const base::Vec2i& target_size) {
  // Submit pending canvas commands
  GetCanvasScheduler()->SubmitPendingPaintCommands();

  // Prepare for rendering context
  DrawableNode::RenderControllerParams controller_params;
  controller_params.device = GetDevice();
  controller_params.screen_buffer = render_target;
  controller_params.screen_size = target_size;
  controller_params.viewport = target_size;
  controller_params.origin = base::Vec2i();

  // 1) Execute pre-composite handler
  controller->BroadCastNotification(DrawableNode::BEFORE_RENDER,
                                    &controller_params);

  // 1.5) Update sprite batch
  base::ThreadWorker::PostTask(
      render_worker_, base::BindOnce(&SpriteBatch::SubmitBatchDataAndResetCache,
                                     base::Unretained(GetSpriteBatch()),
                                     controller_params.device));

  // 2) Setup renderpass
  base::ThreadWorker::PostTask(
      render_worker_,
      base::BindOnce(&RenderScreenImpl::FrameBeginRenderPassInternal,
                     base::Unretained(this), base::Unretained(render_target)));

  // 3) Notify render a frame
  controller_params.root_world = agent_->world_transform;
  controller_params.world_binding = agent_->world_transform;
  controller->BroadCastNotification(DrawableNode::ON_RENDERING,
                                    &controller_params);

  // 4) End render pass and process after-render effect
  base::ThreadWorker::PostTask(
      render_worker_,
      base::BindOnce(&RenderScreenImpl::FrameEndRenderPassInternal,
                     base::Unretained(this)));
}

void RenderScreenImpl::Update(ExceptionState& exception_state) {
  if (!frozen_) {
    if (!(allow_skip_frame_ && frame_skip_required_)) {
      // Render a frame and push into render queue
      RenderFrameInternal(&controller_, agent_->screen_buffer, resolution_);
    }
  }

  // Process frame delay
  FrameProcessInternal(agent_->screen_buffer);
}

void RenderScreenImpl::Wait(uint32_t duration,
                            ExceptionState& exception_state) {
  for (int32_t i = 0; i < duration; ++i)
    Update(exception_state);
}

void RenderScreenImpl::FadeOut(uint32_t duration,
                               ExceptionState& exception_state) {
  duration = std::max(duration, 1u);

  float current_brightness = static_cast<float>(brightness_);
  for (int i = 0; i < duration; ++i) {
    brightness_ = current_brightness -
                  current_brightness * (i / static_cast<float>(duration));
    if (frozen_) {
      FrameProcessInternal(agent_->frozen_buffer);
    } else {
      Update(exception_state);
    }
  }

  // Set final brightness
  brightness_ = 0;
}

void RenderScreenImpl::FadeIn(uint32_t duration,
                              ExceptionState& exception_state) {
  duration = std::max(duration, 1u);

  float current_brightness = static_cast<float>(brightness_);
  float diff = 255.0f - current_brightness;
  for (int i = 0; i < duration; ++i) {
    brightness_ =
        current_brightness + diff * (i / static_cast<float>(duration));

    if (frozen_) {
      FrameProcessInternal(agent_->frozen_buffer);
    } else {
      Update(exception_state);
    }
  }

  // Set final brightness
  brightness_ = 255;
}

void RenderScreenImpl::Freeze(ExceptionState& exception_state) {
  if (!frozen_) {
    // Get frozen scene snapshot for transition
    RenderFrameInternal(&controller_, agent_->frozen_buffer, resolution_);

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
    transition_mapping = CanvasImpl::Create(
        GetCanvasScheduler(), this, scoped_font_, filename, exception_state);
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

  // Fetch screen attribute
  Put_Brightness(255, exception_state);
  vague = std::clamp<int>(vague, 1, 256);
  float vague_norm = vague / 255.0f;

  // Fetch transmapping if available
  Diligent::ITexture* transition_mapping = nullptr;
  if (bitmap)
    transition_mapping = CanvasImpl::FromBitmap(bitmap)->GetAgent()->data;

  // Get current scene snapshot for transition
  RenderFrameInternal(&controller_, agent_->transition_buffer, resolution_);

  for (int i = 0; i < duration; ++i) {
    // Norm transition progress
    float progress = i * (1.0f / duration);

    // Render per transition frame
    if (transition_mapping)
      base::ThreadWorker::PostTask(
          render_worker_,
          base::BindOnce(&RenderScreenImpl::RenderVagueTransitionFrameInternal,
                         base::Unretained(this), progress, vague_norm,
                         base::Unretained(transition_mapping)));
    else
      base::ThreadWorker::PostTask(
          render_worker_,
          base::BindOnce(&RenderScreenImpl::RenderAlphaTransitionFrameInternal,
                         base::Unretained(this), progress));

    // Present to screen
    FrameProcessInternal(agent_->screen_buffer);
  }

  // Transition process complete
  frozen_ = false;
}

scoped_refptr<Bitmap> RenderScreenImpl::SnapToBitmap(
    ExceptionState& exception_state) {
  scoped_refptr<CanvasImpl> target = CanvasImpl::Create(
      GetCanvasScheduler(), this, scoped_font_, resolution_, exception_state);

  if (target) {
    RenderFrameInternal(&controller_, target->GetAgent()->data, resolution_);
    base::ThreadWorker::WaitWorkerSynchronize(render_worker_);
  }

  return target;
}

void RenderScreenImpl::FrameReset(ExceptionState& exception_state) {
  elapsed_time_ = 0.0;
  smooth_delta_time_ = 1.0;
  last_count_time_ = SDL_GetPerformanceCounter();
  desired_delta_time_ = SDL_GetPerformanceFrequency() / frame_rate_;
}

uint32_t RenderScreenImpl::Width(ExceptionState& exception_state) {
  return resolution_.x;
}

uint32_t RenderScreenImpl::Height(ExceptionState& exception_state) {
  return resolution_.y;
}

void RenderScreenImpl::ResizeScreen(uint32_t width,
                                    uint32_t height,
                                    ExceptionState& exception_state) {
  resolution_ = base::Vec2i(width, height);
  base::ThreadWorker::PostTask(
      render_worker_,
      base::BindOnce(&RenderScreenImpl::ResetScreenBufferInternal,
                     base::Unretained(this)));
  base::ThreadWorker::WaitWorkerSynchronize(render_worker_);
}

void RenderScreenImpl::Reset(ExceptionState& exception_state) {
  /* Reset freeze */
  frozen_ = false;

  /* Disposed all elements */
  for (auto it = disposable_elements_.tail(); it != disposable_elements_.end();
       it = it->previous()) {
    it->value()->Dispose(exception_state);
  }

  /* Reset attribute */
  frame_rate_ =
      profile_->api_version == ContentProfile::APIVersion::RGSS1 ? 40 : 60;
  brightness_ = 255;
  FrameReset(exception_state);
}

void RenderScreenImpl::PlayMovie(const std::string& filename,
                                 ExceptionState& exception_state) {
  exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                    "unimplement Graphics.play_movie");
}

void RenderScreenImpl::InitGraphicsDeviceInternal(
    base::WeakPtr<ui::Widget> window,
    const std::string& wgpu_backend) {
  // Create device on window
  agent_->device = renderer::RenderDevice::Create(window);

  // Create global canvas scheduler
  agent_->canvas_scheduler =
      CanvasScheduler::MakeInstance(agent_->device.get(), io_service_);
  agent_->canvas_scheduler->InitWithRenderWorker(render_worker_);

  // Create global sprite batch scheduler
  agent_->sprite_batch = SpriteBatch::Make(agent_->device.get());

  // Create screen present pipeline and buffer
  agent_->present_pipeline.reset(new renderer::Pipeline_Base(
      **agent_->device,
      agent_->device->GetSwapchain()->GetDesc().ColorBufferFormat));

  // Create generic quads batch
  agent_->effect_quads = renderer::QuadBatch::Make(**agent_->device);
  agent_->effect_binding = agent_->device->GetPipelines()
                               ->color.CreateBinding<renderer::Binding_Color>();

  // Create transition generic quads
  agent_->transition_quads = renderer::QuadBatch::Make(**agent_->device);

  // Setup renderer info
  Diligent::GraphicsAdapterInfo adapter_info =
      (**GetDevice())->GetAdapterInfo();
  renderer_info_.device = adapter_info.DeviceId;
  renderer_info_.vendor = adapter_info.VendorId;
  renderer_info_.description = adapter_info.Description;

  // Create screen buffer
  ResetScreenBufferInternal();
}

void RenderScreenImpl::DestroyGraphicsDeviceInternal() {
  // Release agent resource
  delete agent_;
}

void RenderScreenImpl::PresentScreenBufferInternal(
    Diligent::ITexture* render_target) {
  // Initial device attribute
  Diligent::IDeviceContext* context = GetDevice()->GetContext();
  Diligent::ISwapChain* swapchain = GetDevice()->GetSwapchain();

  // Update drawing viewport
  UpdateWindowViewportInternal();

  // Process mouse coordinate and viewport rect
  base::Rect target_rect;
  base::WeakPtr<ui::Widget> window = GetDevice()->GetWindow();
  window->GetMouseState().resolution = resolution_;
  if (keep_ratio_) {
    target_rect = display_viewport_;
    window->GetMouseState().screen_offset = display_viewport_.Position();
    window->GetMouseState().screen = display_viewport_.Size();
  } else {
    target_rect = window_size_;
    window->GetMouseState().screen_offset = base::Vec2i();
    window->GetMouseState().screen = window_size_;
  }

  // Setup render params
  auto* render_target_view = swapchain->GetCurrentBackBufferRTV();
  auto& pipeline_set = *agent_->present_pipeline;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NO_BLEND);
  std::unique_ptr<renderer::QuadBatch> present_quads =
      renderer::QuadBatch::Make(**GetDevice());
  std::unique_ptr<renderer::Binding_Base> present_binding =
      pipeline_set.CreateBinding<renderer::Binding_Base>();
  RRefPtr<Diligent::IBuffer> present_uniform;

  if (render_target) {
    // Update vertex
    renderer::Quad transient_quad;
    renderer::Quad::SetPositionRect(&transient_quad, target_rect);
    renderer::Quad::SetTexCoordRectNorm(&transient_quad,
                                        base::Rect(0, 0, 1, 1));
    present_quads->QueueWrite(context, &transient_quad);

    // Update window screen transform
    renderer::WorldTransform world_matrix;
    renderer::MakeProjectionMatrix(world_matrix.projection, window->GetSize());
    renderer::MakeIdentityMatrix(world_matrix.transform);

    Diligent::CreateUniformBuffer(**GetDevice(), sizeof(world_matrix),
                                  "present.world.uniform", &present_uniform,
                                  Diligent::USAGE_IMMUTABLE,
                                  Diligent::BIND_UNIFORM_BUFFER,
                                  Diligent::CPU_ACCESS_NONE, &world_matrix);
  }

  // Prepare for rendering
  float clear_color[] = {1, 1, 0, 1};
  context->SetRenderTargets(
      1, &render_target_view, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->ClearRenderTarget(
      render_target_view, clear_color,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  Diligent::Rect scissor;
  scissor.right = window->GetSize().x;
  scissor.bottom = window->GetSize().y;
  context->SetScissorRects(1, &scissor, 1, scissor.bottom + scissor.top);

  // Start screen render
  if (render_target) {
    // Set world transform
    present_binding->u_transform->Set(present_uniform);
    present_binding->u_texture->Set(
        render_target->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));

    // Apply pipeline state
    context->SetPipelineState(pipeline);
    context->CommitShaderResources(
        **present_binding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Apply vertex index
    Diligent::IBuffer* const vertex_buffer = **present_quads;
    context->SetVertexBuffers(
        0, 1, &vertex_buffer, nullptr,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    context->SetIndexBuffer(
        **GetDevice()->GetQuadIndex(), 0,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Execute render command
    Diligent::DrawIndexedAttribs draw_indexed_attribs;
    draw_indexed_attribs.NumIndices = 6;
    draw_indexed_attribs.IndexType = GetDevice()->GetQuadIndex()->format();
    context->DrawIndexed(draw_indexed_attribs);
  }

  // Flush command buffer and present GPU surface
  swapchain->Present();
}

void RenderScreenImpl::FrameProcessInternal(
    Diligent::ITexture* present_target) {
  // Setup target
  agent_->present_target = present_target;

  // Increase frame render count
  ++frame_count_;

  // Tick callback
  tick_observers_.Notify();

  // Switch to primary fiber
  fiber_switch(cc_->primary_fiber);
}

void RenderScreenImpl::UpdateWindowViewportInternal() {
  auto window_size = GetDevice()->GetWindow()->GetSize();

  if (!(window_size == window_size_)) {
    window_size_ = window_size;

    // Resize screen surface
    GetDevice()->GetSwapchain()->Resize(window_size_.x, window_size_.y);
  }

  float window_ratio = static_cast<float>(window_size.x) / window_size.y;
  float screen_ratio = static_cast<float>(resolution_.x) / resolution_.y;

  display_viewport_.width = window_size.x;
  display_viewport_.height = window_size.y;

  if (screen_ratio > window_ratio)
    display_viewport_.height = display_viewport_.width / screen_ratio;
  else if (screen_ratio < window_ratio)
    display_viewport_.width = display_viewport_.height * screen_ratio;

  display_viewport_.x = (window_size.x - display_viewport_.width) / 2.0f;
  display_viewport_.y = (window_size.y - display_viewport_.height) / 2.0f;
}

void RenderScreenImpl::ResetScreenBufferInternal() {
  constexpr Diligent::BIND_FLAGS bind_flags =
      Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;

  renderer::CreateTexture2D(**GetDevice(), &agent_->screen_buffer,
                            "screen.main.buffer", resolution_,
                            Diligent::USAGE_DEFAULT, bind_flags);
  renderer::CreateTexture2D(**GetDevice(), &agent_->frozen_buffer,
                            "screen.frozen.buffer", resolution_,
                            Diligent::USAGE_DEFAULT, bind_flags);
  renderer::CreateTexture2D(**GetDevice(), &agent_->transition_buffer,
                            "screen.transition.buffer", resolution_,
                            Diligent::USAGE_DEFAULT, bind_flags);

  renderer::WorldTransform world_transform;
  renderer::MakeIdentityMatrix(world_transform.transform);
  renderer::MakeProjectionMatrix(world_transform.projection, resolution_);

  agent_->world_transform.Release();
  Diligent::CreateUniformBuffer(
      **GetDevice(), sizeof(renderer::WorldTransform),
      "graphics.world.transform", &agent_->world_transform,
      Diligent::USAGE_IMMUTABLE, Diligent::BIND_UNIFORM_BUFFER,
      Diligent::CPU_ACCESS_WRITE, &world_transform);
}

int RenderScreenImpl::DetermineRepeatNumberInternal(double delta_rate) {
  smooth_delta_time_ *= 0.8;
  smooth_delta_time_ += std::fmin(delta_rate, 2) * 0.2;

  if (smooth_delta_time_ >= 0.9) {
    elapsed_time_ = 0;
    return std::round(smooth_delta_time_);
  } else {
    elapsed_time_ += delta_rate;
    if (elapsed_time_ >= 1) {
      elapsed_time_ -= 1;
      return 1;
    }
  }

  return 0;
}

void RenderScreenImpl::FrameBeginRenderPassInternal(
    Diligent::ITexture* render_target) {
  // Initial context
  Diligent::IDeviceContext* context = GetDevice()->GetContext();

  // Initial render target size
  const base::Vec2i target_size(render_target->GetDesc().Width,
                                render_target->GetDesc().Height);

  // Update screen effect params
  if (brightness_ < 255) {
    renderer::Quad effect_quad;
    renderer::Quad::SetPositionRect(&effect_quad, base::Rect(target_size));
    renderer::Quad::SetColor(&effect_quad,
                             base::Vec4(0, 0, 0, (255 - brightness_) / 255.0f));
    agent_->effect_quads->QueueWrite(context, &effect_quad);
  }

  // Set render target in context and clear previous buffer
  auto render_target_view =
      render_target->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
  const float clear_color[] = {0, 0, 0, 1};
  context->SetRenderTargets(
      1, &render_target_view, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->ClearRenderTarget(
      render_target_view, clear_color,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  Diligent::Rect render_scissor;
  render_scissor.right = target_size.x;
  render_scissor.bottom = target_size.y;
  context->SetScissorRects(1, &render_scissor, 1,
                           render_scissor.bottom + render_scissor.top);
}

void RenderScreenImpl::FrameEndRenderPassInternal() {
  // Initial context
  Diligent::IDeviceContext* context = GetDevice()->GetContext();

  // Render screen effect if need
  if (brightness_ < 255) {
    // Apply brightness effect
    auto& pipeline_set = GetDevice()->GetPipelines()->color;
    auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NORMAL);

    // Set world transform
    agent_->effect_binding->u_transform->Set(agent_->world_transform);

    // Apply pipeline state
    context->SetPipelineState(pipeline);
    context->CommitShaderResources(
        **agent_->effect_binding,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Apply vertex index
    Diligent::IBuffer* const vertex_buffer = **agent_->effect_quads;
    context->SetVertexBuffers(
        0, 1, &vertex_buffer, nullptr,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    context->SetIndexBuffer(
        **GetDevice()->GetQuadIndex(), 0,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Execute render command
    Diligent::DrawIndexedAttribs draw_indexed_attribs;
    draw_indexed_attribs.NumIndices = 6;
    draw_indexed_attribs.IndexType = GetDevice()->GetQuadIndex()->format();
    context->DrawIndexed(draw_indexed_attribs);
  }
}

void RenderScreenImpl::RenderAlphaTransitionFrameInternal(float progress) {
  // Initial context
  Diligent::IDeviceContext* context = GetDevice()->GetContext();

  // Create binding if need
  if (!agent_->transition_binding)
    agent_->transition_binding =
        GetDevice()
            ->GetPipelines()
            ->alphatrans.CreateBinding<renderer::Binding_AlphaTrans>();

  // Update transition uniform
  renderer::Quad transient_quad;
  renderer::Quad::SetPositionRect(&transient_quad,
                                  base::RectF(-1.0f, 1.0f, 2.0f, -2.0f));
  renderer::Quad::SetTexCoordRectNorm(&transient_quad,
                                      base::RectF(0.0f, 0.0f, 1.0f, 1.0f));
  renderer::Quad::SetColor(&transient_quad, base::Vec4(progress));
  agent_->transition_quads->QueueWrite(context, &transient_quad);

  // Composite transition frame
  auto render_target_view = agent_->screen_buffer->GetDefaultView(
      Diligent::TEXTURE_VIEW_RENDER_TARGET);
  const float clear_color[] = {0, 0, 0, 1};
  context->SetRenderTargets(
      1, &render_target_view, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->ClearRenderTarget(
      render_target_view, clear_color,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  Diligent::Rect render_scissor;
  render_scissor.right = resolution_.x;
  render_scissor.bottom = resolution_.y;
  context->SetScissorRects(1, &render_scissor, 1,
                           render_scissor.bottom + render_scissor.top);

  // Apply brightness effect
  auto& pipeline_set = GetDevice()->GetPipelines()->alphatrans;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NO_BLEND);

  // Set uniform texture
  static_cast<renderer::Binding_AlphaTrans*>(agent_->transition_binding.get())
      ->u_current_texture->Set(agent_->transition_buffer->GetDefaultView(
          Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
  static_cast<renderer::Binding_AlphaTrans*>(agent_->transition_binding.get())
      ->u_frozen_texture->Set(agent_->frozen_buffer->GetDefaultView(
          Diligent::TEXTURE_VIEW_SHADER_RESOURCE));

  // Apply pipeline state
  context->SetPipelineState(pipeline);
  context->CommitShaderResources(
      **agent_->transition_binding,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Apply vertex index
  Diligent::IBuffer* const vertex_buffer = **agent_->transition_quads;
  context->SetVertexBuffers(
      0, 1, &vertex_buffer, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->SetIndexBuffer(**GetDevice()->GetQuadIndex(), 0,
                          Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Execute render command
  Diligent::DrawIndexedAttribs draw_indexed_attribs;
  draw_indexed_attribs.NumIndices = 6;
  draw_indexed_attribs.IndexType = GetDevice()->GetQuadIndex()->format();
  context->DrawIndexed(draw_indexed_attribs);
}

void RenderScreenImpl::RenderVagueTransitionFrameInternal(
    float progress,
    float vague,
    Diligent::ITexture* trans_mapping) {
  // Initial context
  Diligent::IDeviceContext* context = GetDevice()->GetContext();

  // Create binding if need
  if (!agent_->transition_binding)
    agent_->transition_binding =
        GetDevice()
            ->GetPipelines()
            ->mappedtrans.CreateBinding<renderer::Binding_VagueTrans>();

  // Update transition uniform
  renderer::Quad transient_quad;
  renderer::Quad::SetPositionRect(&transient_quad,
                                  base::RectF(-1.0f, 1.0f, 2.0f, -2.0f));
  renderer::Quad::SetTexCoordRectNorm(&transient_quad,
                                      base::RectF(0.0f, 0.0f, 1.0f, 1.0f));
  renderer::Quad::SetColor(&transient_quad, base::Vec4(vague, 0, 0, progress));
  agent_->transition_quads->QueueWrite(context, &transient_quad);

  // Composite transition frame
  auto render_target_view = agent_->screen_buffer->GetDefaultView(
      Diligent::TEXTURE_VIEW_RENDER_TARGET);
  const float clear_color[] = {0, 0, 0, 1};
  context->SetRenderTargets(
      1, &render_target_view, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->ClearRenderTarget(
      render_target_view, clear_color,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  Diligent::Rect render_scissor;
  render_scissor.right = resolution_.x;
  render_scissor.bottom = resolution_.y;
  context->SetScissorRects(1, &render_scissor, 1,
                           render_scissor.bottom + render_scissor.top);

  // Apply brightness effect
  auto& pipeline_set = GetDevice()->GetPipelines()->mappedtrans;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NO_BLEND);

  // Set uniform texture
  static_cast<renderer::Binding_VagueTrans*>(agent_->transition_binding.get())
      ->u_current_texture->Set(agent_->transition_buffer->GetDefaultView(
          Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
  static_cast<renderer::Binding_VagueTrans*>(agent_->transition_binding.get())
      ->u_frozen_texture->Set(agent_->frozen_buffer->GetDefaultView(
          Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
  static_cast<renderer::Binding_VagueTrans*>(agent_->transition_binding.get())
      ->u_trans_texture->Set(trans_mapping->GetDefaultView(
          Diligent::TEXTURE_VIEW_SHADER_RESOURCE));

  // Apply pipeline state
  context->SetPipelineState(pipeline);
  context->CommitShaderResources(
      **agent_->transition_binding,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Apply vertex index
  Diligent::IBuffer* const vertex_buffer = **agent_->transition_quads;
  context->SetVertexBuffers(
      0, 1, &vertex_buffer, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->SetIndexBuffer(**GetDevice()->GetQuadIndex(), 0,
                          Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Execute render command
  Diligent::DrawIndexedAttribs draw_indexed_attribs;
  draw_indexed_attribs.NumIndices = 6;
  draw_indexed_attribs.IndexType = GetDevice()->GetQuadIndex()->format();
  context->DrawIndexed(draw_indexed_attribs);
}

void RenderScreenImpl::AddDisposable(Disposable* disp) {
  disposable_elements_.Append(disp);
}

uint32_t RenderScreenImpl::Get_FrameRate(ExceptionState&) {
  return frame_rate_;
}

void RenderScreenImpl::Put_FrameRate(const uint32_t& rate, ExceptionState&) {
  frame_rate_ = rate;
}

uint32_t RenderScreenImpl::Get_FrameCount(ExceptionState&) {
  return frame_count_;
}

void RenderScreenImpl::Put_FrameCount(const uint32_t& count, ExceptionState&) {
  frame_count_ = count;
}

uint32_t RenderScreenImpl::Get_Brightness(ExceptionState&) {
  return brightness_;
}

void RenderScreenImpl::Put_Brightness(const uint32_t& value, ExceptionState&) {
  brightness_ = std::clamp<uint32_t>(value, 0, 255);
}

}  // namespace content
