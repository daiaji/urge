// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_DEVICE_RENDER_DEVICE_H_
#define RENDERER_DEVICE_RENDER_DEVICE_H_

#include "renderer/pipeline/render_pipeline.h"
#include "ui/widget/widget.h"
#include "webgpu/webgpu_cpp.h"

#include <memory>

namespace renderer {

inline void MakeProjectionMatrix(float* out, const base::Vec2& size) {
  const float aa = 2.0f / size.x;
  const float bb = -2.0f / size.y;
  const float cc = 1.0f;

  memset(out, 0, sizeof(float) * 16);
  out[0] = aa;
  out[5] = bb;
  out[10] = cc;

  out[12] = -1.0f;
  out[13] = 1.0f;
  out[15] = 1.0f;
}

inline void MakeIdentityMatrix(float* out) {
  memset(out, 0, sizeof(float) * 16);
  out[0] = 1.0f;
  out[5] = 1.0f;
  out[10] = 1.0f;
  out[15] = 1.0f;
}

class RenderDevice {
 public:
  struct PipelineSet {
    Pipeline_Base base;
    Pipeline_Color color;

    PipelineSet(const wgpu::Device& device, wgpu::TextureFormat target)
        : base(device, target), color(device, target) {}
  };

  static std::unique_ptr<RenderDevice> Create(
      base::WeakPtr<ui::Widget> window_target,
      wgpu::BackendType required_backend);

  static wgpu::Instance* GetGPUInstance();

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

 private:
  RenderDevice(base::WeakPtr<ui::Widget> window,
               const wgpu::Adapter& adapter,
               const wgpu::Device& device,
               const wgpu::Queue& queue,
               const wgpu::Surface& surface);

  base::WeakPtr<ui::Widget> window_;

  wgpu::Adapter adapter_;
  wgpu::Device device_;
  wgpu::Queue queue_;
  wgpu::Surface surface_;

  std::unique_ptr<PipelineSet> pipelines_;
};

}  // namespace renderer

#endif  //! RENDERER_DEVICE_RENDER_DEVICE_H_
