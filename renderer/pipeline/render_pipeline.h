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

#include "renderer/layout/vertex_layout.h"
#include "renderer/pipeline/render_binding.h"

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
    std::vector<Diligent::ShaderMacro> macros;
  };

  virtual ~RenderPipelineBase() = default;

  RenderPipelineBase(const RenderPipelineBase&) = delete;
  RenderPipelineBase& operator=(const RenderPipelineBase&) = delete;

  inline Diligent::IPipelineState* GetPipeline(BlendType blend) const {
    return pipelines_[blend];
  }

  template <typename Ty>
  std::unique_ptr<Ty> CreateBinding() {
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> binding;
    pipelines_[0]->CreateShaderResourceBinding(&binding);
    return RenderBindingBase::Create<Ty>(binding);
  }

 protected:
  RenderPipelineBase(Diligent::IRenderDevice* device);

  void BuildPipeline(
      const ShaderSource& vertex_shader,
      const ShaderSource& pixel_shader,
      const std::vector<Diligent::LayoutElement>& input_layout,
      const std::vector<Diligent::ShaderResourceVariableDesc>& variables,
      const std::vector<Diligent::ImmutableSamplerDesc>& samplers,
      Diligent::TEXTURE_FORMAT target_format);

 private:
  std::string pipeline_name_;
  Diligent::IRenderDevice* device_;
  std::vector<Diligent::RefCntAutoPtr<Diligent::IPipelineState>> pipelines_;
};

class Pipeline_Base : public RenderPipelineBase {
 public:
  Pipeline_Base(Diligent::IRenderDevice* device,
                Diligent::TEXTURE_FORMAT target_format);
};

class Pipeline_Color : public RenderPipelineBase {
 public:
  Pipeline_Color(Diligent::IRenderDevice* device,
                 Diligent::TEXTURE_FORMAT target_format);
};

class Pipeline_Flat : public RenderPipelineBase {
 public:
  Pipeline_Flat(Diligent::IRenderDevice* device,
                Diligent::TEXTURE_FORMAT target_format);
};

class Pipeline_Sprite : public RenderPipelineBase {
 public:
  Pipeline_Sprite(Diligent::IRenderDevice* device,
                  Diligent::TEXTURE_FORMAT target_format);

  Diligent::Bool storage_buffer_support = Diligent::False;
};

class Pipeline_AlphaTransition : public RenderPipelineBase {
 public:
  Pipeline_AlphaTransition(Diligent::IRenderDevice* device,
                           Diligent::TEXTURE_FORMAT target_format);
};

class Pipeline_VagueTransition : public RenderPipelineBase {
 public:
  Pipeline_VagueTransition(Diligent::IRenderDevice* device,
                           Diligent::TEXTURE_FORMAT target_format);
};

class Pipeline_Tilemap : public RenderPipelineBase {
 public:
  Pipeline_Tilemap(Diligent::IRenderDevice* device,
                   Diligent::TEXTURE_FORMAT target_format);
};

class Pipeline_Tilemap2 : public RenderPipelineBase {
 public:
  Pipeline_Tilemap2(Diligent::IRenderDevice* device,
                    Diligent::TEXTURE_FORMAT target_format);
};

class Pipeline_BitmapHue : public RenderPipelineBase {
 public:
  Pipeline_BitmapHue(Diligent::IRenderDevice* device,
                     Diligent::TEXTURE_FORMAT target_format);
};

class Pipeline_Present : public RenderPipelineBase {
 public:
  Pipeline_Present(Diligent::IRenderDevice* device,
                   Diligent::TEXTURE_FORMAT target_format,
                   bool manual_srgb);
};

}  // namespace renderer

#endif  //! RENDERER_PIPELINE_RENDER_PIPELINE_H_
