// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_DEVICE_RENDER_DEVICE_H_
#define RENDERER_DEVICE_RENDER_DEVICE_H_

#include "renderer/pipeline/render_pipeline.h"
#include "renderer/utils/buffer_utils.h"
#include "ui/widget/widget.h"

#include <memory>

namespace renderer {

class RenderDevice {
 public:
  struct PipelineSet {
    Pipeline_Base base;
    Pipeline_Color color;
    Pipeline_Viewport viewport;
    Pipeline_Sprite sprite;
    Pipeline_SpriteInstance spriteinstance;
    Pipeline_AlphaTransition alphatrans;
    Pipeline_MappedTransition mappedtrans;

    PipelineSet(const wgpu::Device& device, wgpu::TextureFormat target)
        : base(device, target),
          color(device, target),
          viewport(device, target),
          sprite(device, target),
          spriteinstance(device, target),
          alphatrans(device, target),
          mappedtrans(device, target) {}
  };

  static std::unique_ptr<RenderDevice> Create(
      base::WeakPtr<ui::Widget> window_target,
      wgpu::BackendType required_backend = wgpu::BackendType::Undefined);

  static wgpu::Instance* GetGPUInstance();

  ~RenderDevice();

  RenderDevice(const RenderDevice&) = delete;
  RenderDevice& operator=(const RenderDevice&) = delete;

  // WGPU Device access
  wgpu::Device* operator->() { return &device_; }
  wgpu::Device& operator*() { return device_; }

  // Device Attribute interface
  wgpu::Adapter* GetAdapter() { return &adapter_; }
  wgpu::Queue* GetQueue() { return &queue_; }
  wgpu::Surface* GetSurface() { return &surface_; }

  // Render window device
  base::WeakPtr<ui::Widget> GetWindow() { return window_; }

  // Pre-compile shaders set storage
  PipelineSet* GetPipelines() const { return pipelines_.get(); }
  wgpu::TextureFormat SurfaceFormat() const { return surface_format_; }

 private:
  RenderDevice(base::WeakPtr<ui::Widget> window,
               const wgpu::Adapter& adapter,
               const wgpu::Device& device,
               const wgpu::Queue& queue,
               const wgpu::Surface& surface,
               wgpu::TextureFormat surface_format,
               std::unique_ptr<PipelineSet> pipelines);

  base::WeakPtr<ui::Widget> window_;

  wgpu::Adapter adapter_;
  wgpu::Device device_;
  wgpu::Queue queue_;
  wgpu::Surface surface_;

  wgpu::TextureFormat surface_format_;
  std::unique_ptr<PipelineSet> pipelines_;
};

}  // namespace renderer

#endif  //! RENDERER_DEVICE_RENDER_DEVICE_H_
