// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/screen/renderscreen_impl.h"

#include "SDL3/SDL_timer.h"

namespace content {

RenderScreenImpl::RenderScreenImpl(
    std::unique_ptr<renderer::RenderDevice> device,
    int frame_rate)
    : device_(std::move(device)),
      context_(renderer::DeviceContext::MakeContextFor(device_.get())),
      brightness_(255),
      frame_count_(0),
      frame_rate_(frame_rate),
      elapsed_time_(0),
      smooth_delta_time_(1),
      last_count_time_(SDL_GetPerformanceCounter()),
      desired_delta_time_(SDL_GetPerformanceFrequency() / frame_rate_) {}

RenderScreenImpl::~RenderScreenImpl() {}

renderer::RenderDevice* RenderScreenImpl::GetDevice() const {
  return device_.get();
}

void RenderScreenImpl::Update(ExceptionState& exception_state) {}

void RenderScreenImpl::Wait(uint32_t duration,
                            ExceptionState& exception_state) {}

void RenderScreenImpl::FadeOut(uint32_t duration,
                               ExceptionState& exception_state) {}

void RenderScreenImpl::FadeIn(uint32_t duration,
                              ExceptionState& exception_state) {}

void RenderScreenImpl::Freeze(ExceptionState& exception_state) {}

void RenderScreenImpl::Transition(uint32_t duration,
                                  const std::string& filename,
                                  uint32_t vague,
                                  ExceptionState& exception_state) {}

scoped_refptr<Bitmap> RenderScreenImpl::SnapToBitmap(
    ExceptionState& exception_state) {
  return scoped_refptr<Bitmap>();
}

void RenderScreenImpl::FrameReset(ExceptionState& exception_state) {}

uint32_t RenderScreenImpl::Width(ExceptionState& exception_state) {
  return resolution_.x;
}

uint32_t RenderScreenImpl::Height(ExceptionState& exception_state) {
  return resolution_.y;
}

void RenderScreenImpl::ResizeScreen(uint32_t width,
                                    uint32_t height,
                                    ExceptionState& exception_state) {}

void RenderScreenImpl::PlayMovie(const std::string& filename,
                                 ExceptionState& exception_state) {
  exception_state.ThrowContentError(ExceptionCode::kContentError,
                                    "unimplement Graphics.play_movie");
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
