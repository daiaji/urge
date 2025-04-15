// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SCREEN_RENDERSCREEN_IMPL_H_
#define CONTENT_SCREEN_RENDERSCREEN_IMPL_H_

#include "base/worker/thread_worker.h"
#include "content/canvas/canvas_scheduler.h"
#include "content/components/disposable.h"
#include "content/components/font_context.h"
#include "content/components/sprite_batch.h"
#include "content/profile/content_profile.h"
#include "content/profile/i18n_profile.h"
#include "content/public/engine_graphics.h"
#include "content/render/drawable_controller.h"
#include "content/worker/coroutine_context.h"
#include "renderer/device/render_device.h"
#include "renderer/layout/uniform_layout.h"

namespace content {

struct RenderGraphicsAgent {
  std::unique_ptr<renderer::RenderDevice> device;
  std::unique_ptr<CanvasScheduler> canvas_scheduler;
  std::unique_ptr<SpriteBatch> sprite_batch;

  RRefPtr<Diligent::ITexture> screen_buffer;
  RRefPtr<Diligent::ITexture> frozen_buffer;
  RRefPtr<Diligent::ITexture> transition_buffer;
  RRefPtr<Diligent::IBuffer> world_transform;
  RRefPtr<Diligent::IBufferView> world_binding;

  std::unique_ptr<renderer::QuadBatch> effect_quads;
  std::unique_ptr<renderer::Binding_Color> effect_binding;

  std::unique_ptr<renderer::QuadBatch> transition_quads;
  std::unique_ptr<renderer::RenderBindingBase> transition_binding;

  Diligent::ITexture* present_target = nullptr;
  std::unique_ptr<renderer::Pipeline_Base> present_pipeline;
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
                   ContentProfile* profile,
                   I18NProfile* i18n_profile,
                   filesystem::IOService* io_service,
                   ScopedFontData* scoped_font,
                   const base::Vec2i& resolution,
                   int frame_rate);
  ~RenderScreenImpl() override;

  RenderScreenImpl(const RenderScreenImpl&) = delete;
  RenderScreenImpl& operator=(const RenderScreenImpl&) = delete;

  void InitWithRenderWorker(base::ThreadWorker* render_worker,
                            base::WeakPtr<ui::Widget> window,
                            const std::string& wgpu_backend);

  void PresentScreen();

  renderer::RenderDevice* GetDevice() const;
  CanvasScheduler* GetCanvasScheduler() const;
  SpriteBatch* GetSpriteBatch() const;
  ScopedFontData* GetScopedFontContext() const;

  DrawNodeController* GetDrawableController() { return &controller_; }
  base::ThreadWorker* GetRenderRunner() const { return render_worker_; }
  base::Vec2i GetResolution() const { return resolution_; }

  ContentProfile::APIVersion GetAPIVersion() const {
    return profile_->api_version;
  }

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
  void Reset(ExceptionState& exception_state) override;
  void PlayMovie(const std::string& filename,
                 ExceptionState& exception_state) override;
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(FrameRate, uint32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(FrameCount, uint32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Brightness, uint32_t);

 private:
  void InitGraphicsDeviceInternal(base::WeakPtr<ui::Widget> window,
                                  const std::string& wgpu_backend);
  void DestroyGraphicsDeviceInternal();

  void PresentScreenBufferInternal(Diligent::ITexture* render_target);
  void FrameProcessInternal(Diligent::ITexture* present_target);
  void UpdateWindowViewportInternal();
  void ResetScreenBufferInternal();
  int DetermineRepeatNumberInternal(double delta_rate);

  void RenderFrameInternal(DrawNodeController* controller,
                           Diligent::ITexture* render_target,
                           const base::Vec2i& target_size);

  void FrameBeginRenderPassInternal(Diligent::ITexture* render_target);
  void FrameEndRenderPassInternal();

  void RenderAlphaTransitionFrameInternal(float progress);
  void RenderVagueTransitionFrameInternal(float progress,
                                          float vague,
                                          Diligent::ITexture* trans_mapping);

  // DisposableCollection methods:
  void AddDisposable(Disposable* disp) override;

  DrawNodeController controller_;
  base::RepeatingClosureList tick_observers_;

  CoroutineContext* cc_;
  ContentProfile* profile_;
  I18NProfile* i18n_profile_;
  filesystem::IOService* io_service_;
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

  bool keep_ratio_;
  bool smooth_scale_;
  bool allow_skip_frame_;
  bool allow_background_running_;

  struct {
    std::string device;
    std::string vendor;
    std::string description;
  } renderer_info_;
};

}  // namespace content

#endif  //! CONTENT_SCREEN_RENDERSCREEN_IMPL_H_
