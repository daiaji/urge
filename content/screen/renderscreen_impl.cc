// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/screen/renderscreen_impl.h"

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"

#include "content/canvas/canvas_scheduler.h"

namespace content {

RenderScreenImpl::RenderScreenImpl(base::SingleWorker* current_worker,
                                   const base::Vec2i& resolution,
                                   int frame_rate)
    : current_worker_(current_worker),
      render_worker_(nullptr),
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
  canvas_scheduler_.reset();
  base::SingleWorker::DeleteSoon(render_worker_,
                                 std::move(index_buffer_cache_));
  base::SingleWorker::DeleteSoon(render_worker_, std::move(context_));
  base::SingleWorker::DeleteSoon(render_worker_, std::move(device_));
  base::SingleWorker::WaitWorkerSynchronize(render_worker_);
}

void RenderScreenImpl::InitWithRenderWorker(
    base::SingleWorker* render_worker,
    std::unique_ptr<renderer::RenderDevice> device) {
  render_worker_ = render_worker;

  base::SingleWorker::PostTask(
      render_worker,
      base::BindOnce(&RenderScreenImpl::InitGraphicsDeviceInternal,
                     base::Unretained(this), std::move(device)));
  base::SingleWorker::WaitWorkerSynchronize(render_worker);
}

bool RenderScreenImpl::ExecuteEventMainLoop() {
  // Poll event queue
  SDL_Event queued_event;
  while (SDL_PollEvent(&queued_event)) {
    // Quit event
    if (queued_event.type == SDL_EVENT_QUIT)
      return false;
  }

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
    current_worker_->YieldFiber();
  }

  // Present to screen surface
  base::SingleWorker::PostTask(
      render_worker_,
      base::BindOnce(&RenderScreenImpl::PresentScreenBufferInternal,
                     base::Unretained(this), screen_buffer_.get()));
  base::SingleWorker::WaitWorkerSynchronize(render_worker_);

  return true;
}

renderer::RenderDevice* RenderScreenImpl::GetDevice() const {
  return device_.get();
}

renderer::DeviceContext* RenderScreenImpl::GetContext() const {
  return context_.get();
}

renderer::QuadrangleIndexCache* RenderScreenImpl::GetCommonIndexBuffer() const {
  return index_buffer_cache_.get();
}

CanvasScheduler* RenderScreenImpl::GetCanvasScheduler() const {
  return canvas_scheduler_.get();
}

void RenderScreenImpl::Update(ExceptionState& exception_state) {
  if (!frozen_ && !frame_skip_required_)
    RenderSingleFrameInternal(screen_buffer_.get());

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
    RenderSingleFrameInternal(frozen_buffer_.get());

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
  RenderSingleFrameInternal(transition_buffer_.get());

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
  return scoped_refptr<Bitmap>();
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
  base::SingleWorker::PostTask(
      render_worker_,
      base::BindOnce(&RenderScreenImpl::ResetScreenBufferInternal,
                     base::Unretained(this)));
  base::SingleWorker::WaitWorkerSynchronize(render_worker_);
}

void RenderScreenImpl::PlayMovie(const std::string& filename,
                                 ExceptionState& exception_state) {
  exception_state.ThrowContentError(ExceptionCode::kContentError,
                                    "unimplement Graphics.play_movie");
}

void RenderScreenImpl::InitGraphicsDeviceInternal(
    std::unique_ptr<renderer::RenderDevice> device) {
  device_ = std::move(device);
  context_ = renderer::DeviceContext::MakeContextFor(device_.get());
  index_buffer_cache_ = renderer::QuadrangleIndexCache::Make(device_.get());
  canvas_scheduler_ = CanvasScheduler::MakeInstance(
      device_.get(), context_.get(), index_buffer_cache_.get());

  ResetScreenBufferInternal();
}

void RenderScreenImpl::RenderSingleFrameInternal(FrameBufferAgent* agent) {}

void RenderScreenImpl::PresentScreenBufferInternal(FrameBufferAgent* agent) {
  // Update drawing viewport
  UpdateWindowViewportInternal();

  // Flip screen for Y
  base::WeakPtr<ui::Widget> window = device_->GetWindow();
  window->GetMouseState().resolution = resolution_;
  window->GetMouseState().screen_offset = display_viewport_.Position();
  window->GetMouseState().screen = display_viewport_.Size();

  // Blit internal screen buffer to screen surface

  // Present WGPU surface
  auto* hardware_surface = device_->GetSurface();
  hardware_surface->Present();
}

void RenderScreenImpl::FrameProcessInternal() {
  // Increase frame render count
  ++frame_count_;

  // Switch to primary fiber
  current_worker_->YieldFiber();
}

void RenderScreenImpl::UpdateWindowViewportInternal() {
  auto window_size = device_->GetWindow()->GetSize();

  if (!(window_size == window_size_)) {
    window_size_ = window_size;

    auto* hardware_surface = device_->GetSurface();
    wgpu::SurfaceCapabilities capabilities;
    hardware_surface->GetCapabilities(*device_->GetAdapter(), &capabilities);

    wgpu::SurfaceConfiguration config;
    config.device = **device_;
    config.format = capabilities.formats[0];
    config.width = window_size_.x;
    config.height = window_size_.y;

    // Resize screen surface
    hardware_surface->Configure(&config);
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
  wgpu::TextureDescriptor buffer_desc;
  buffer_desc.dimension = wgpu::TextureDimension::e2D;
  buffer_desc.usage =
      wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc |
      wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
  buffer_desc.size.width = resolution_.x;
  buffer_desc.size.height = resolution_.y;
  buffer_desc.format = wgpu::TextureFormat::RGBA8Unorm;

  screen_buffer_.reset(new FrameBufferAgent);
  screen_buffer_->color_buffer = (*device_)->CreateTexture(&buffer_desc);
  screen_buffer_->color_view = screen_buffer_->color_buffer.CreateView();

  frozen_buffer_.reset(new FrameBufferAgent);
  frozen_buffer_->color_buffer = (*device_)->CreateTexture(&buffer_desc);
  frozen_buffer_->color_view = screen_buffer_->color_buffer.CreateView();

  transition_buffer_.reset(new FrameBufferAgent);
  transition_buffer_->color_buffer = (*device_)->CreateTexture(&buffer_desc);
  transition_buffer_->color_view = screen_buffer_->color_buffer.CreateView();
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
