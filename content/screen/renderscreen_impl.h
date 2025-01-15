// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SCREEN_RENDERSCREEN_IMPL_H_
#define CONTENT_SCREEN_RENDERSCREEN_IMPL_H_

#include "base/worker/single_worker.h"
#include "content/public/engine_graphics.h"
#include "content/render/drawable_controller.h"
#include "renderer/context/device_context.h"
#include "renderer/device/render_device.h"
#include "renderer/resource/render_buffer.h"

namespace content {

class CanvasScheduler;

struct FrameBufferAgent {
  wgpu::Texture color_buffer;
  wgpu::TextureView color_view;
};

class RenderScreenImpl : public Graphics {
 public:
  RenderScreenImpl(base::SingleWorker* current_worker,
                   const base::Vec2i& resolution,
                   int frame_rate);
  ~RenderScreenImpl() override;

  RenderScreenImpl(const RenderScreenImpl&) = delete;
  RenderScreenImpl& operator=(const RenderScreenImpl&) = delete;

  void InitWithRenderWorker(base::SingleWorker* render_worker,
                            std::unique_ptr<renderer::RenderDevice> device);
  bool ExecuteEventMainLoop();

  renderer::RenderDevice* GetDevice() const;
  renderer::DeviceContext* GetContext() const;

  renderer::QuadrangleIndexCache* GetCommonIndexBuffer() const;
  CanvasScheduler* GetCanvasScheduler() const;

 protected:
  void Update(ExceptionState& exception_state) override;
  void Wait(uint32_t duration, ExceptionState& exception_state) override;
  void FadeOut(uint32_t duration, ExceptionState& exception_state) override;
  void FadeIn(uint32_t duration, ExceptionState& exception_state) override;
  void Freeze(ExceptionState& exception_state) override;
  void Transition(uint32_t duration,
                  const std::string& filename,
                  uint32_t vague,
                  ExceptionState& exception_state) override;
  scoped_refptr<Bitmap> SnapToBitmap(ExceptionState& exception_state) override;
  void FrameReset(ExceptionState& exception_state) override;
  uint32_t Width(ExceptionState& exception_state) override;
  uint32_t Height(ExceptionState& exception_state) override;
  void ResizeScreen(uint32_t width,
                    uint32_t height,
                    ExceptionState& exception_state) override;
  void PlayMovie(const std::string& filename,
                 ExceptionState& exception_state) override;
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(FrameRate, uint32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(FrameCount, uint32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Brightness, uint32_t);

 private:
  void InitGraphicsDeviceInternal(
      std::unique_ptr<renderer::RenderDevice> device);
  void RenderSingleFrameInternal(FrameBufferAgent* agent);
  void PresentScreenBufferInternal(FrameBufferAgent* agent);
  void FrameProcessInternal();
  void UpdateWindowViewportInternal();
  void ResetScreenBufferInternal();
  int DetermineRepeatNumberInternal(double delta_rate);

  // All visible elements drawing controller
  DrawNodeController controller_;

  base::SingleWorker* current_worker_;
  base::SingleWorker* render_worker_;

  std::unique_ptr<renderer::RenderDevice> device_;
  std::unique_ptr<renderer::DeviceContext> context_;

  std::unique_ptr<renderer::QuadrangleIndexCache> index_buffer_cache_;
  std::unique_ptr<CanvasScheduler> canvas_scheduler_;

  std::unique_ptr<FrameBufferAgent> screen_buffer_;
  std::unique_ptr<FrameBufferAgent> frozen_buffer_;
  std::unique_ptr<FrameBufferAgent> transition_buffer_;

  bool frozen_;
  base::Vec2i resolution_;
  base::Rect display_viewport_;
  base::Vec2i window_size_;
  int32_t brightness_;
  uint64_t frame_count_;
  uint32_t frame_rate_;

  double elapsed_time_;
  double smooth_delta_time_;
  uint64_t last_count_time_;
  uint64_t desired_delta_time_;
  bool frame_skip_required_;
};

}  // namespace content

#endif  //! CONTENT_SCREEN_RENDERSCREEN_IMPL_H_
