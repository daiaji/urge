// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SCREEN_RENDERSCREEN_IMPL_H_
#define CONTENT_SCREEN_RENDERSCREEN_IMPL_H_

#include "base/worker/thread_worker.h"
#include "content/components/disposable.h"
#include "content/components/font_context.h"
#include "content/public/engine_graphics.h"
#include "content/render/drawable_controller.h"
#include "content/worker/coroutine_context.h"
#include "renderer/context/device_context.h"
#include "renderer/device/render_device.h"
#include "renderer/resource/render_buffer.h"

namespace content {

class CanvasScheduler;

struct RenderGraphicsAgent {
  std::unique_ptr<renderer::RenderDevice> device;
  std::unique_ptr<renderer::DeviceContext> context;

  std::unique_ptr<renderer::QuadrangleIndexCache> index_cache;
  std::unique_ptr<CanvasScheduler> canvas_scheduler;

  wgpu::Texture* present_target;
  wgpu::Texture screen_buffer;
  wgpu::Texture frozen_buffer;
  wgpu::Texture transition_buffer;

  wgpu::RenderPassEncoder render_pass;
  wgpu::BindGroup world_binding;
  wgpu::Buffer world_buffer;
  wgpu::Buffer effect_vertex;
  wgpu::BindGroup transition_binding;

  std::unique_ptr<renderer::Pipeline_Base> present_pipeline;
  wgpu::Buffer present_vertex;
};

class GraphicsChild {
 public:
  GraphicsChild(RenderScreenImpl* screen) : screen_(screen) {}
  virtual ~GraphicsChild() = default;

  GraphicsChild(const GraphicsChild&) = delete;
  GraphicsChild& operator=(const GraphicsChild&) = delete;

  RenderScreenImpl* screen() const { return screen_; }

 private:
  RenderScreenImpl* screen_;
};

class RenderScreenImpl : public Graphics, public DisposableCollection {
 public:
  RenderScreenImpl(CoroutineContext* cc,
                   ScopedFontData* scoped_font,
                   const base::Vec2i& resolution,
                   int frame_rate);
  ~RenderScreenImpl() override;

  RenderScreenImpl(const RenderScreenImpl&) = delete;
  RenderScreenImpl& operator=(const RenderScreenImpl&) = delete;

  void InitWithRenderWorker(base::ThreadWorker* render_worker,
                            base::WeakPtr<ui::Widget> window);
  bool ExecuteEventMainLoop();

  renderer::RenderDevice* GetDevice() const;
  renderer::DeviceContext* GetContext() const;

  renderer::QuadrangleIndexCache* GetQuadIndexCache() const;
  CanvasScheduler* GetCanvasScheduler() const;

  DrawNodeController* GetDrawableController() { return &controller_; }
  base::ThreadWorker* GetRenderRunner() const { return render_worker_; }
  base::Vec2i GetResolution() const { return resolution_; }

  void PostTask(base::OnceClosure task);
  void WaitWorkerSynchronize();

  base::CallbackListSubscription AddTickObserver(
      const base::RepeatingClosure& handler);

 public:
  void Update(ExceptionState& exception_state) override;
  void Wait(uint32_t duration, ExceptionState& exception_state) override;
  void FadeOut(uint32_t duration, ExceptionState& exception_state) override;
  void FadeIn(uint32_t duration, ExceptionState& exception_state) override;
  void Freeze(ExceptionState& exception_state) override;
  void Transition(ExceptionState& exception_state) override;
  void Transition(uint32_t duration, ExceptionState& exception_state) override;
  void Transition(uint32_t duration,
                  const std::string& filename,
                  ExceptionState& exception_state) override;
  void Transition(uint32_t duration,
                  const std::string& filename,
                  uint32_t vague,
                  ExceptionState& exception_state) override;
  void TransitionWithBitmap(uint32_t duration,
                            scoped_refptr<Bitmap> bitmap,
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
  void InitGraphicsDeviceInternal(base::WeakPtr<ui::Widget> window);
  void DestroyGraphicsDeviceInternal();

  void PresentScreenBufferInternal(wgpu::Texture* render_target);
  void FrameProcessInternal(wgpu::Texture* present_target);
  void UpdateWindowViewportInternal();
  void ResetScreenBufferInternal();
  int DetermineRepeatNumberInternal(double delta_rate);

  void RenderFrameInternal(DrawNodeController* controller,
                           wgpu::Texture* render_target,
                           const base::Vec2i& target_size);

  void FrameBeginRenderPassInternal(wgpu::Texture* render_target);
  void FrameEndRenderPassInternal();

  void CreateTransitionUniformInternal(wgpu::Texture* transition_mapping);
  void RenderAlphaTransitionFrameInternal(float progress);
  void RenderVagueTransitionFrameInternal(float progress, float vague);

  void AddDisposable(Disposable* disp) override;
  void RemoveDisposable(Disposable* disp) override;

  DrawNodeController controller_;
  base::RepeatingClosureList tick_observers_;

  CoroutineContext* cc_;
  base::ThreadWorker* render_worker_;
  ScopedFontData* scoped_font_;

  RenderGraphicsAgent* agent_;
  base::LinkedList<Disposable> disposable_elements_;

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
