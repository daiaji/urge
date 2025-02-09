// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/screen/renderscreen_impl.h"

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"
#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_sdl3.h"
#include "third_party/imgui/imgui_impl_wgpu.h"

#include "content/canvas/canvas_scheduler.h"

namespace content {

RenderScreenImpl::RenderScreenImpl(CoroutineContext* cc,
                                   const base::Vec2i& resolution,
                                   int frame_rate)
    : cc_(cc),
      render_worker_(nullptr),
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
      frame_skip_required_(false) {}

RenderScreenImpl::~RenderScreenImpl() {
  base::ThreadWorker::PostTask(
      render_worker_,
      base::BindOnce(&RenderScreenImpl::DestroyGraphicsDeviceInternal,
                     base::Unretained(this), agent_));
  base::ThreadWorker::WaitWorkerSynchronize(render_worker_);
}

void RenderScreenImpl::InitWithRenderWorker(base::ThreadWorker* render_worker,
                                            base::WeakPtr<ui::Widget> window) {
  // Setup render thread worker if needed
  render_worker_ = render_worker;

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup imgui platform backends
  ImGui_ImplSDL3_InitForOther(window->AsSDLWindow());

  // Setup render device
  agent_ = new RenderGraphicsAgent;
  base::ThreadWorker::PostTask(
      render_worker,
      base::BindOnce(&RenderScreenImpl::InitGraphicsDeviceInternal,
                     base::Unretained(this), window));
  base::ThreadWorker::WaitWorkerSynchronize(render_worker);
}

bool RenderScreenImpl::ExecuteEventMainLoop() {
  // Poll event queue
  SDL_Event queued_event;
  while (SDL_PollEvent(&queued_event)) {
    // Quit event
    if (queued_event.type == SDL_EVENT_QUIT)
      return false;

    // IMGUI Event
    ImGui_ImplSDL3_ProcessEvent(&queued_event);
  }

  // Start IMGUI frame
  ImGui_ImplSDL3_NewFrame();

  // Layout new frame
  ImGui::NewFrame();
  ImGui::ShowDemoWindow();
  ImGui::EndFrame();

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
                     base::Unretained(this), &agent_->screen_buffer));
  base::ThreadWorker::WaitWorkerSynchronize(render_worker_);

  return true;
}

renderer::RenderDevice* RenderScreenImpl::GetDevice() const {
  return agent_->device.get();
}

renderer::DeviceContext* RenderScreenImpl::GetContext() const {
  return agent_->context.get();
}

renderer::QuadrangleIndexCache* RenderScreenImpl::GetCommonIndexBuffer() const {
  return agent_->index_cache.get();
}

CanvasScheduler* RenderScreenImpl::GetCanvasScheduler() const {
  return agent_->canvas_scheduler.get();
}

void RenderScreenImpl::PostTask(base::OnceClosure task) {
  base::ThreadWorker::PostTask(render_worker_, std::move(task));
}

void RenderScreenImpl::WaitWorkerSynchronize() {
  base::ThreadWorker::WaitWorkerSynchronize(render_worker_);
}

void RenderScreenImpl::Update(ExceptionState& exception_state) {
  if (!frozen_ && !frame_skip_required_)
    RenderFrameInternal(&agent_->screen_buffer);

  // Process frame delay
  FrameProcessInternal();
}

void RenderScreenImpl::Wait(uint32_t duration,
                            ExceptionState& exception_state) {
  for (int32_t i = 0; i < duration; ++i)
    Update(exception_state);
}

void RenderScreenImpl::FadeOut(uint32_t duration,
                               ExceptionState& exception_state) {}

void RenderScreenImpl::FadeIn(uint32_t duration,
                              ExceptionState& exception_state) {}

void RenderScreenImpl::Freeze(ExceptionState& exception_state) {
  if (!frozen_) {
    // Get frozen scene snapshot for transition
    RenderFrameInternal(&agent_->frozen_buffer);

    // Set forzen flag for blocking frame update
    frozen_ = true;
  }
}

void RenderScreenImpl::Transition(uint32_t duration,
                                  const std::string& filename,
                                  uint32_t vague,
                                  ExceptionState& exception_state) {
  if (!frozen_)
    return;

  // Fix screen attribute
  Put_Brightness(255, exception_state);
  vague = std::clamp<int>(vague, 1, 256);

  // Get current scene snapshot for transition
  RenderFrameInternal(&agent_->transition_buffer);

  for (int i = 0; i < duration; ++i) {
    // Norm transition progress
    float progress = i * (1.0f / duration);

    // Render per transition frame

    // Present to screen
    FrameProcessInternal();
  }

  // Transition process complete
  frozen_ = false;
}

scoped_refptr<Bitmap> RenderScreenImpl::SnapToBitmap(
    ExceptionState& exception_state) {
  return nullptr;
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

void RenderScreenImpl::PlayMovie(const std::string& filename,
                                 ExceptionState& exception_state) {
  exception_state.ThrowContentError(ExceptionCode::kContentError,
                                    "unimplement Graphics.play_movie");
}

void RenderScreenImpl::InitGraphicsDeviceInternal(
    base::WeakPtr<ui::Widget> window) {
  // Create device on window
  agent_->device = renderer::RenderDevice::Create(window);

  // Create immediate command encoder
  agent_->context =
      renderer::DeviceContext::MakeContextFor(agent_->device.get());

  // Quad index generic buffer (reserved 1024)
  agent_->index_cache =
      renderer::QuadrangleIndexCache::Make(agent_->device.get());
  agent_->index_cache->Allocate(1 << 10);

  // Create global canvas scheduler
  agent_->canvas_scheduler = CanvasScheduler::MakeInstance(
      agent_->device.get(), agent_->context.get(), agent_->index_cache.get());
  agent_->canvas_scheduler->InitWithRenderWorker(render_worker_);

  // Setup imgui renderer backends
  ImGui_ImplWGPU_InitInfo init_info;
  init_info.Device = (*agent_->device)->Get();
  init_info.NumFramesInFlight = 3;
  init_info.RenderTargetFormat =
      static_cast<WGPUTextureFormat>(agent_->device->SurfaceFormat());
  init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;
  ImGui_ImplWGPU_Init(&init_info);

  ImGui_ImplWGPU_NewFrame();

  // Create screen buffer
  ResetScreenBufferInternal();
}

void RenderScreenImpl::DestroyGraphicsDeviceInternal(
    RenderGraphicsAgent* agent) {
  // Cleanup
  ImGui_ImplWGPU_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  // Release resource
  delete agent;
}

void RenderScreenImpl::RenderFrameInternal(wgpu::Texture* render_target) {
  // Screen texture size
  base::Vec2i screen_size(render_target->GetWidth(),
                          render_target->GetHeight());

  // Submit pending canvas commands
  agent_->canvas_scheduler->SubmitPendingPaintCommands();

  // Prepare for rendering context
  DrawableNode::RenderControllerParams controller_params;
  controller_params.device = agent_->device.get();
  controller_params.index_cache = agent_->index_cache.get();
  controller_params.command_encoder = agent_->context->GetImmediateEncoder();
  controller_params.screen_buffer = render_target;
  controller_params.screen_size = screen_size;
  controller_params.viewport = screen_size;

  // 1) Execute pre-composite handler
  controller_.BroadCastNotification(DrawableNode::kBeforeRender,
                                    &controller_params);

  // 2) Setup renderpass
  base::ThreadWorker::PostTask(
      render_worker_,
      base::BindOnce(&RenderScreenImpl::FrameBeginRenderPassInternal,
                     base::Unretained(this), render_target));

  // 3) Notify render a frame
  controller_params.world_binding = &agent_->world_binding;
  controller_params.renderpass_encoder = &agent_->renderpass;
  controller_.BroadCastNotification(DrawableNode::kOnRendering,
                                    &controller_params);

  // 4) End render pass and process after-render effect
  base::ThreadWorker::PostTask(
      render_worker_,
      base::BindOnce(&RenderScreenImpl::FrameEndRenderPassInternal,
                     base::Unretained(this)));
}

void RenderScreenImpl::PresentScreenBufferInternal(
    wgpu::Texture* render_target) {
  // Update drawing viewport
  UpdateWindowViewportInternal();

  // Flip screen for Y
  base::WeakPtr<ui::Widget> window = agent_->device->GetWindow();
  window->GetMouseState().resolution = resolution_;
  window->GetMouseState().screen_offset = display_viewport_.Position();
  window->GetMouseState().screen = display_viewport_.Size();

  // Clear WGPU surface
  auto* hardware_surface = agent_->device->GetSurface();

  wgpu::SurfaceTexture surface_texture;
  hardware_surface->GetCurrentTexture(&surface_texture);

  // Blit internal screen buffer to screen surface
  auto* commander = agent_->context->GetImmediateEncoder();

  wgpu::RenderPassColorAttachment attachment;
  attachment.view = surface_texture.texture.CreateView();
  attachment.loadOp = wgpu::LoadOp::Clear;
  attachment.storeOp = wgpu::StoreOp::Store;
  attachment.clearValue = {0, 0, 1, 1};

  wgpu::RenderPassDescriptor renderpass;
  renderpass.colorAttachmentCount = 1;
  renderpass.colorAttachments = &attachment;

  {
    // Prepare renderer parameters
    if (!agent_->screen_pipeline) {
      wgpu::SurfaceCapabilities caps;
      agent_->device->GetSurface()->GetCapabilities(
          *agent_->device->GetAdapter(), &caps);

      agent_->screen_pipeline.reset(
          new renderer::Pipeline_Base(**agent_->device, caps.formats[0]));
    }

    // Setup binding and vertex
    wgpu::RenderPipeline& pipeline =
        *agent_->screen_pipeline->GetPipeline(renderer::BlendType::kNoBlend);
    wgpu::BindGroup world_binding, texture_binding;
    auto vertex_controller =
        renderer::VertexBufferController<renderer::FullVertexLayout>::Make(
            agent_->device.get(), 4);

    {
      renderer::FullVertexLayout transient_vertices[4];
      renderer::FullVertexLayout::SetPositionRect(transient_vertices,
                                                  display_viewport_);
      renderer::FullVertexLayout::SetTexCoordRect(
          transient_vertices, base::Rect(window->GetSize()));
      renderer::FullVertexLayout::SetColor(transient_vertices, base::Vec4(1));
      vertex_controller->QueueWrite(*commander, transient_vertices,
                                    _countof(transient_vertices));
    }

    {
      float world_matrix[32];
      renderer::MakeProjectionMatrix(world_matrix, window->GetSize());
      renderer::MakeIdentityMatrix(world_matrix + 16);

      wgpu::BufferDescriptor uniform_desc;
      uniform_desc.label = "bitmap.world.uniform";
      uniform_desc.size = sizeof(world_matrix);
      uniform_desc.mappedAtCreation = true;
      uniform_desc.usage = wgpu::BufferUsage::Uniform;
      wgpu::Buffer world_matrix_uniform =
          (*agent_->device)->CreateBuffer(&uniform_desc);

      {
        memcpy(world_matrix_uniform.GetMappedRange(), world_matrix,
               sizeof(world_matrix));
        world_matrix_uniform.Unmap();
      }

      wgpu::BindGroupEntry entries;
      entries.binding = 0;
      entries.buffer = world_matrix_uniform;

      wgpu::BindGroupDescriptor binding_desc;
      binding_desc.entryCount = 1;
      binding_desc.entries = &entries;
      binding_desc.layout = pipeline.GetBindGroupLayout(0);

      world_binding = (*agent_->device)->CreateBindGroup(&binding_desc);
    }

    {
      wgpu::BufferDescriptor uniform_desc;
      uniform_desc.label = "screen.binding.uniform";
      uniform_desc.size = sizeof(base::Vec2);
      uniform_desc.mappedAtCreation = true;
      uniform_desc.usage = wgpu::BufferUsage::Uniform;
      wgpu::Buffer texture_size_uniform =
          (*agent_->device)->CreateBuffer(&uniform_desc);

      {
        auto texture_size = base::MakeInvert(
            base::Vec2i(render_target->GetWidth(), render_target->GetHeight()));
        memcpy(texture_size_uniform.GetMappedRange(), &texture_size,
               sizeof(texture_size));
        texture_size_uniform.Unmap();
      }

      wgpu::BindGroupEntry entries[3];
      entries[0].binding = 0;
      entries[0].textureView = render_target->CreateView();
      entries[1].binding = 1;
      entries[1].sampler = (*agent_->device)->CreateSampler();
      entries[2].binding = 2;
      entries[2].buffer = texture_size_uniform;

      wgpu::BindGroupDescriptor binding_desc;
      binding_desc.entryCount = _countof(entries);
      binding_desc.entries = entries;
      binding_desc.layout = pipeline.GetBindGroupLayout(1);

      texture_binding = (*agent_->device)->CreateBindGroup(&binding_desc);
    }

    // Start screen render
    {
      auto pass = commander->BeginRenderPass(&renderpass);
      pass.SetPipeline(
          *agent_->screen_pipeline->GetPipeline(renderer::BlendType::kNoBlend));
      pass.SetViewport(0, 0, window->GetSize().x, window->GetSize().y, 0, 0);
      pass.SetBindGroup(0, world_binding);
      pass.SetBindGroup(1, texture_binding);
      pass.SetVertexBuffer(0, **vertex_controller);
      pass.SetIndexBuffer(**agent_->index_cache, agent_->index_cache->format());
      pass.DrawIndexed(6);
      pass.End();
    }

    // Start imgui layer render
    {
      // New frame
      ImGui_ImplWGPU_NewFrame();

      // Rendering IMGUI
      ImGui::Render();

      attachment.loadOp = wgpu::LoadOp::Load;
      auto pass = commander->BeginRenderPass(&renderpass);
      ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());
      pass.End();
    }
  }

  // Flush command buffer and present WGPU surface
  agent_->context->Flush();
  hardware_surface->Present();
}

void RenderScreenImpl::FrameProcessInternal() {
  // Increase frame render count
  ++frame_count_;

  // Switch to primary fiber
  fiber_switch(cc_->primary_fiber);
}

void RenderScreenImpl::UpdateWindowViewportInternal() {
  auto window_size = agent_->device->GetWindow()->GetSize();

  if (!(window_size == window_size_)) {
    window_size_ = window_size;

    wgpu::SurfaceConfiguration config;
    config.device = **agent_->device;
    config.format = agent_->device->SurfaceFormat();
    config.width = window_size_.x;
    config.height = window_size_.y;

    ImGui_ImplWGPU_InvalidateDeviceObjects();

    // Resize screen surface
    auto* hardware_surface = agent_->device->GetSurface();
    hardware_surface->Configure(&config);

    ImGui_ImplWGPU_CreateDeviceObjects();
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
  // Setup target device
  renderer::RenderDevice& device = *agent_->device;

  // Reset screen buffers if needed
  wgpu::TextureDescriptor buffer_desc;
  buffer_desc.dimension = wgpu::TextureDimension::e2D;
  buffer_desc.usage =
      wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc |
      wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
  buffer_desc.size.width = resolution_.x;
  buffer_desc.size.height = resolution_.y;
  buffer_desc.format = wgpu::TextureFormat::RGBA8Unorm;

  agent_->screen_buffer = device->CreateTexture(&buffer_desc);
  agent_->frozen_buffer = device->CreateTexture(&buffer_desc);
  agent_->transition_buffer = device->CreateTexture(&buffer_desc);

  // Create screen world binding
  renderer::WorldMatrixUniform world_matrix;
  renderer::MakeProjectionMatrix(world_matrix.projection, resolution_);
  renderer::MakeIdentityMatrix(world_matrix.transform);

  agent_->world_buffer =
      renderer::CreateUniformBuffer<renderer::WorldMatrixUniform>(
          *device, "screen.world.matrix", wgpu::BufferUsage::None,
          &world_matrix);

  // Create uniform binding
  agent_->world_binding =
      renderer::WorldMatrixUniform::CreateGroup(*device, agent_->world_buffer);
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
    wgpu::Texture* render_target) {
  auto* encoder = agent_->context->GetImmediateEncoder();

  wgpu::RenderPassColorAttachment attachment;
  attachment.view = render_target->CreateView();
  attachment.loadOp = wgpu::LoadOp::Clear;
  attachment.storeOp = wgpu::StoreOp::Store;
  attachment.clearValue = {0, 0, 0, 1};

  wgpu::RenderPassDescriptor renderpass;
  renderpass.colorAttachmentCount = 1;
  renderpass.colorAttachments = &attachment;
  agent_->renderpass = encoder->BeginRenderPass(&renderpass);
  agent_->renderpass.SetViewport(0, 0, render_target->GetWidth(),
                                 render_target->GetHeight(), 0, 0);
  agent_->renderpass.SetScissorRect(0, 0, render_target->GetWidth(),
                                    render_target->GetHeight());
}

void RenderScreenImpl::FrameEndRenderPassInternal() {
  agent_->renderpass.End();

  // Apply brightness
}

void RenderScreenImpl::AddDisposable(Disposable* disp) {
  disposable_elements_.Append(disp);
}

void RenderScreenImpl::RemoveDisposable(Disposable* disp) {
  disp->RemoveFromList();
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
  brightness_ = value;
}

}  // namespace content
