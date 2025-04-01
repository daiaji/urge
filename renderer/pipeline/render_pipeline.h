// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_PIPELINE_RENDER_PIPELINE_H_
#define RENDERER_PIPELINE_RENDER_PIPELINE_H_

#include "renderer/pipeline/binding_layout.h"
#include "renderer/vertex/vertex_layout.h"

namespace renderer {

enum BlendType {
  NORMAL = 0,
  ADDITION,
  SUBSTRACTION,
  NO_BLEND,

  TYPE_NUMS,
};

class RenderPipelineBase {
 public:
  virtual ~RenderPipelineBase() = default;

  RenderPipelineBase(const RenderPipelineBase&) = delete;
  RenderPipelineBase& operator=(const RenderPipelineBase&) = delete;

  wgpu::RenderPipeline* GetPipeline(BlendType blend) {
    return &pipelines_[blend];
  }

  wgpu::BindGroupLayout* GetLayout(size_t n) { return &layouts_[n]; }

 protected:
  RenderPipelineBase(const wgpu::Device& device);

  void BuildPipeline(const std::string& shader_source,
                     const std::string& vs_entry,
                     const std::string& fs_entry,
                     std::vector<wgpu::VertexBufferLayout> vertex_layout,
                     std::vector<wgpu::BindGroupLayout> bind_layout,
                     wgpu::TextureFormat target_format);

 private:
  wgpu::Device device_;
  std::vector<wgpu::BindGroupLayout> layouts_;
  std::vector<wgpu::RenderPipeline> pipelines_;
};

class Pipeline_Base : public RenderPipelineBase {
 public:
  Pipeline_Base(const wgpu::Device& device, wgpu::TextureFormat target);
};

class Pipeline_Color : public RenderPipelineBase {
 public:
  Pipeline_Color(const wgpu::Device& device, wgpu::TextureFormat target);
};

class Pipeline_Viewport : public RenderPipelineBase {
 public:
  Pipeline_Viewport(const wgpu::Device& device, wgpu::TextureFormat target);
};

class Pipeline_Sprite : public RenderPipelineBase {
 public:
  Pipeline_Sprite(const wgpu::Device& device, wgpu::TextureFormat target);
};

class Pipeline_AlphaTransition : public RenderPipelineBase {
 public:
  Pipeline_AlphaTransition(const wgpu::Device& device,
                           wgpu::TextureFormat target);
};

class Pipeline_MappedTransition : public RenderPipelineBase {
 public:
  Pipeline_MappedTransition(const wgpu::Device& device,
                            wgpu::TextureFormat target);
};

class Pipeline_Tilemap : public RenderPipelineBase {
 public:
  Pipeline_Tilemap(const wgpu::Device& device, wgpu::TextureFormat target);
};

class Pipeline_Tilemap2 : public RenderPipelineBase {
 public:
  Pipeline_Tilemap2(const wgpu::Device& device, wgpu::TextureFormat target);
};

}  // namespace renderer

#endif  //! RENDERER_PIPELINE_RENDER_PIPELINE_H_
