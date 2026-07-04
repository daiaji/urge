// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SCREEN_RENDERSCREEN_IMPL_H_
#define CONTENT_SCREEN_RENDERSCREEN_IMPL_H_

#include <array>

#include "imgui/backends/imgui_impl_diligent.h"

#include "base/worker/thread_worker.h"
#include "components/fpslimiter/fpslimiter.h"
#include "content/canvas/canvas_scheduler.h"
#include "content/canvas/font_context.h"
#include "content/context/disposable.h"
#include "content/profile/content_profile.h"
#include "content/profile/i18n_profile.h"
#include "content/public/engine_graphics.h"
#include "content/render/drawable_controller.h"
#include "content/render/sprite_batch.h"
#include "renderer/device/render_device.h"
#include "renderer/layout/uniform_layout.h"

namespace content {

class RenderScreenImpl : public Graphics, public EngineObject {
 public:
  struct GPUData {
    RRefPtr<Diligent::ITexture> screen_buffer;
    RRefPtr<Diligent::ITexture> frozen_buffer;
    RRefPtr<Diligent::ITexture> transition_buffer;

    RRefPtr<Diligent::ITexture> screen_depth_stencil;
    RRefPtr<Diligent::ITexture> frozen_depth_stencil;
    RRefPtr<Diligent::ITexture> transition_depth_stencil;

    RRefPtr<Diligent::IBuffer> root_transform;
    RRefPtr<Diligent::IBuffer> world_transform;

    renderer::QuadBatch effect_quads;
    renderer::Binding_Color effect_binding;

    renderer::QuadBatch transition_quads;
    renderer::Binding_AlphaTrans transition_binding_alpha;
    renderer::Binding_VagueTrans transition_binding_vague;

    RRefPtr<Diligent::ITexture> upscale_buffer;
    RRefPtr<Diligent::IBuffer> upscale_params_buffer;
    renderer::Binding_Upscale upscale_binding;

    renderer::QuadBatch scaling_quads;
    RRefPtr<Diligent::ITexture> enhanced_tex;
    renderer::Binding_Upscale anime4k_enhance_binding;

    RRefPtr<Diligent::ITexture> sharpened_buffer;
    renderer::Binding_Upscale cas_binding;

    // Anime4K Upscale_Denoise_L intermediate targets
    RRefPtr<Diligent::ITexture> udl_tex1;  // 640×480 RGBA16F
    RRefPtr<Diligent::ITexture> udl_tex2;  // 640×480 RGBA16F
    RRefPtr<Diligent::ITexture> udl_tex3;  // 640×480 RGBA16F
    RRefPtr<Diligent::ITexture> udl_tex4;  // 640×480 RGBA16F
    renderer::Binding_CuNNy_Compute udl_pass0_binding;
    renderer::Binding_CuNNy_Compute udl_pass1_binding;
    renderer::Binding_CuNNy_Compute udl_pass2_binding;
    renderer::Binding_CuNNy_Compute udl_pass3_binding;
    // enhanced_tex reused as 1280×960 output target

    // CuNNy intermediate targets
    std::vector<RRefPtr<Diligent::ITexture>> cunny_tex;
    std::array<renderer::Binding_CuNNy_Compute, 6> cunny_4x16_bindings;
    std::array<renderer::Binding_CuNNy_Compute, 6> cunny_4x24_bindings;
  };

  RenderScreenImpl(ExecutionContext* execution_context, uint32_t frame_rate);
  ~RenderScreenImpl() override;

  RenderScreenImpl(const RenderScreenImpl&) = delete;
  RenderScreenImpl& operator=(const RenderScreenImpl&) = delete;

  // Present current screen buffer to window.
  // This function will wait for delta time to clamp fps.
  void PresentScreenBuffer(Diligent::ImGuiDiligentRenderer* gui_renderer);

  // Reset frame counter
  void ResetFPSCounter();

  // Create graphics parition gui
  void CreateButtonGUISettings();

  // Add tick monitor handler
  using FrameTickHandler = base::RepeatingCallback<void(Diligent::ITexture*)>;
  void SetupTicker(const FrameTickHandler& handler) {
    frame_tick_handler_ = handler;
  }

  // Global drawable parent (default)
  DrawNodeController* GetDrawableController() { return &controller_; }

  // Current frame rate
  int32_t FrameRate() const { return frame_rate_; }

  // Sync to display refresh rate (disable limiter, set vsync=1)
  void SyncToRefreshRate(int32_t refresh_rate);

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
  void MoveWindow(int32_t x,
                  int32_t y,
                  int32_t width,
                  int32_t height,
                  ExceptionState& exception_state) override;
  scoped_refptr<Rect> GetWindowRect(ExceptionState& exception_state) override;
  void Center(ExceptionState& exception_state) override;
  uint32_t GetDisplayID(ExceptionState& exception_state) override;
  void SetWindowIcon(scoped_refptr<Bitmap> icon,
                     ExceptionState& exception_state) override;
  int32_t GetMaxTextureSize(ExceptionState& exception_state) override;
  scoped_refptr<GPURenderDevice> GetRenderDevice(
      ExceptionState& exception_state) override;
  scoped_refptr<GPUDeviceContext> GetImmediateContext(
      ExceptionState& exception_state) override;
  scoped_refptr<GPUBuffer> GetGenericQuadIndexBuffer(
      uint32_t draw_quad_count,
      ExceptionState& exception_state) override;
  GPU::ValueType GetInternalIndexType(ExceptionState& exception_state) override;

  URGE_DECLARE_OVERRIDE_ATTRIBUTE(FrameRate, uint32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(FrameCount, uint32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Brightness, uint32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(VSync, uint32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Fullscreen, bool);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Skipframe, bool);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(KeepRatio, bool);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(IntegerScaling, bool);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(SmoothScaling, int32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(SmoothScalingDown, int32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(ScalingMode, int32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(BackgroundRunning, bool);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Ox, int32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(Oy, int32_t);
  URGE_DECLARE_OVERRIDE_ATTRIBUTE(WindowTitle, std::string);

 private:
  enum class LazyPipelineState { kUninitialized, kReady, kFailed };

  void FrameProcessInternal(Diligent::ITexture* present_target);
  void RenderFrameInternal(Diligent::ITexture* render_target,
                           Diligent::ITexture* depth_stencil);

  void GPUCreateGraphicsHostInternal();
  void GPUUpdateScreenWorldInternal();
  void GPUResetScreenBufferInternal();
  void GPUPresentScreenBufferInternal(
      Diligent::IDeviceContext* render_context,
      Diligent::ImGuiDiligentRenderer* gui_renderer);
  void GPUFrameBeginRenderPassInternal(Diligent::IDeviceContext* render_context,
                                       Diligent::ITexture* render_target,
                                       Diligent::ITexture* depth_stencil);
  void GPUFrameEndRenderPassInternal(Diligent::IDeviceContext* render_context);
  void GPURenderAlphaTransitionFrameInternal(
      Diligent::IDeviceContext* render_context,
      float progress);
  void GPURenderVagueTransitionFrameInternal(
      Diligent::IDeviceContext* render_context,
      Diligent::ITextureView* trans_mapping,
      float progress,
      float vague);

  Diligent::ITexture* GPUScalingPassInternal(
      Diligent::IDeviceContext* render_context);
  void GPURecreateUpscaleBufferInternal();
  void GPURecreateAnime4KTargetsInternal();
  void GPURecreateAnime4KTargetsInternal(const base::Vec2i& size);
  void GPURunUDLPassesInternal(Diligent::IDeviceContext* render_context);
  void GPURunCuNNyPassesInternal(Diligent::IDeviceContext* render_context,
                                   int variant);
  bool EnsureAnime4KUDLReadyInternal();
  bool EnsureCuNNyReadyInternal(int variant);
  bool EnsureAnime4KUDLBindingsInternal();
  bool EnsureCuNNyBindingsInternal(int variant);
  void GPURecreateCuNNyTargetsInternal(int tex_count);
  void GPURecreateSharpenedBufferInternal();

  GPUData gpu_;
  DrawNodeController controller_;

  fpslimiter::FPSLimiter limiter_;
  FrameTickHandler frame_tick_handler_;

  bool frozen_;
  bool rebuild_buffers_pending_ = false;
  bool show_auto_fit_window_on_present_ = false;
  LazyPipelineState anime4k_udl_state_ = LazyPipelineState::kUninitialized;
  LazyPipelineState cunny_4x16_state_ = LazyPipelineState::kUninitialized;
  LazyPipelineState cunny_4x24_state_ = LazyPipelineState::kUninitialized;
  int32_t brightness_;
  uint64_t frame_count_;
  uint32_t frame_rate_;
  bool unlimited_fps_;
  base::Vec2i origin_;
};

}  // namespace content

#endif  //! CONTENT_SCREEN_RENDERSCREEN_IMPL_H_
