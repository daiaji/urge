// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_PIPELINE_RENDER_PIPELINE_H_
#define RENDERER_PIPELINE_RENDER_PIPELINE_H_

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/PipelineState.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsTools/interface/GraphicsUtilities.h"
#include "Graphics/GraphicsTools/interface/MapHelper.hpp"

#include "renderer/pipeline/binding_layout.h"
#include "renderer/vertex/vertex_layout.h"

namespace renderer {

/// Color Blend Type of Pipeline
///
enum BlendType {
  NORMAL = 0,
  ADDITION,
  SUBSTRACTION,
  NO_BLEND,

  TYPE_NUMS,
};

/// Pipeline base manager
///
class RenderPipelineBase {
 public:
  struct ShaderSource {
    std::string source;
    std::string entry = "main";
    std::string name = "shader";
  };

  virtual ~RenderPipelineBase() = default;

  RenderPipelineBase(const RenderPipelineBase&) = delete;
  RenderPipelineBase& operator=(const RenderPipelineBase&) = delete;

  Diligent::IPipelineState* GetPipeline(BlendType blend) {
    return pipelines_[blend];
  }

 protected:
  RenderPipelineBase(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device);

  void BuildPipeline(
      const ShaderSource& vertex_shader,
      const ShaderSource& pixel_shader,
      const std::vector<Diligent::LayoutElement>& input_layout,
      const std::vector<Diligent::ShaderResourceVariableDesc>& variables,
      const std::vector<Diligent::ImmutableSamplerDesc>& samplers,
      Diligent::TEXTURE_FORMAT target_format);

 private:
  Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device_;
  std::vector<Diligent::RefCntAutoPtr<Diligent::IPipelineState>> pipelines_;
};

class Pipeline_Base : public RenderPipelineBase {
 public:
  Pipeline_Base(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device,
                Diligent::TEXTURE_FORMAT target_format);
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
