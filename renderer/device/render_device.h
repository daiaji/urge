// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_DEVICE_RENDER_DEVICE_H_
#define RENDERER_DEVICE_RENDER_DEVICE_H_

#include "renderer/context/scissor_controller.h"
#include "renderer/pipeline/render_pipeline.h"
#include "renderer/resource/render_buffer.h"
#include "ui/widget/widget.h"

#include <memory>

template <typename Ty>
using RRefPtr = Diligent::RefCntAutoPtr<Ty>;

namespace renderer {

enum class DriverType {
  UNDEFINED = 0,
  OPENGL,
  VULKAN,
  D3D11,
  D3D12,
};

class RenderDevice {
 public:
  struct PipelineSet {
    Pipeline_Base base;
    Pipeline_Color color;
    Pipeline_Flat viewport;
    Pipeline_Sprite sprite;
    Pipeline_AlphaTransition alphatrans;
    Pipeline_VagueTransition mappedtrans;
    Pipeline_Tilemap tilemap;
    Pipeline_Tilemap2 tilemap2;
    Pipeline_BitmapHue bitmaphue;

    PipelineSet(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device,
                Diligent::TEXTURE_FORMAT target_format)
        : base(device, target_format),
          color(device, target_format),
          viewport(device, target_format),
          sprite(device, target_format),
          alphatrans(device, target_format),
          mappedtrans(device, target_format),
          tilemap(device, target_format),
          tilemap2(device, target_format),
          bitmaphue(device, target_format) {}
  };

  static std::unique_ptr<RenderDevice> Create(
      base::WeakPtr<ui::Widget> window_target,
      DriverType driver_type);

  ~RenderDevice();

  RenderDevice(const RenderDevice&) = delete;
  RenderDevice& operator=(const RenderDevice&) = delete;

  // Device access
  inline Diligent::IRenderDevice* operator->() { return device_; }
  inline Diligent::IRenderDevice* operator*() { return device_; }

  // Device Attribute interface
  Diligent::IDeviceContext* GetContext() const { return context_; }
  Diligent::ISwapChain* GetSwapchain() const { return swapchain_; }

  // Render window device
  base::WeakPtr<ui::Widget> GetWindow() { return window_; }

  // Pre-compile shaders set storage
  PipelineSet* GetPipelines() const { return pipelines_.get(); }
  QuadIndexCache* GetQuadIndex() const { return quad_index_.get(); }
  ScissorController* Scissor() const { return scissor_.get(); }

  // Platform specific
  inline bool IsUVFlip() const { return device_->GetDeviceInfo().IsGLDevice(); }

 private:
  RenderDevice(base::WeakPtr<ui::Widget> window,
               Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device,
               Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context,
               Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapchain,
               std::unique_ptr<PipelineSet> pipelines,
               std::unique_ptr<QuadIndexCache> quad_index,
               std::unique_ptr<ScissorController> scissor,
               SDL_GLContext gl_context);

  base::WeakPtr<ui::Widget> window_;

  Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device_;
  Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context_;
  Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapchain_;

  std::unique_ptr<PipelineSet> pipelines_;
  std::unique_ptr<QuadIndexCache> quad_index_;
  std::unique_ptr<ScissorController> scissor_;

  SDL_GLContext gl_context_;
};

}  // namespace renderer

#endif  //! RENDERER_DEVICE_RENDER_DEVICE_H_
