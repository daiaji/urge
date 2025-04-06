// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/pipeline/render_pipeline.h"

#include <optional>

#include "base/debug/logging.h"
#include "renderer/pipeline/builtin_wgsl.h"

namespace renderer {

namespace {

Diligent::RenderTargetBlendDesc GetBlendState(BlendType type) {
  Diligent::RenderTargetBlendDesc state;
  switch (type) {
    default:
    case renderer::BlendType::NORMAL:
      state.BlendEnable = Diligent::True;
      state.BlendOp = Diligent::BLEND_OPERATION_ADD;
      state.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
      state.DestBlend = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
      state.SrcBlendAlpha = Diligent::BLEND_FACTOR_ONE;
      state.DestBlendAlpha = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
      break;
    case renderer::BlendType::ADDITION:
      state.BlendEnable = Diligent::True;
      state.BlendOp = Diligent::BLEND_OPERATION_ADD;
      state.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
      state.DestBlend = Diligent::BLEND_FACTOR_ONE;
      state.SrcBlendAlpha = Diligent::BLEND_FACTOR_ONE;
      state.DestBlendAlpha = Diligent::BLEND_FACTOR_ONE;
      break;
    case renderer::BlendType::SUBSTRACTION:
      state.BlendEnable = Diligent::True;
      state.BlendOp = Diligent::BLEND_OPERATION_REV_SUBTRACT;
      state.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
      state.DestBlend = Diligent::BLEND_FACTOR_ONE;
      state.SrcBlendAlpha = Diligent::BLEND_FACTOR_ZERO;
      state.DestBlendAlpha = Diligent::BLEND_FACTOR_ONE;
      break;
    case renderer::BlendType::NO_BLEND:
      state.BlendEnable = Diligent::False;
      break;
  }

  return state;
}

}  // namespace

RenderPipelineBase::RenderPipelineBase(
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device)
    : device_(device) {}

void RenderPipelineBase::BuildPipeline(
    const ShaderSource& vertex_shader,
    const ShaderSource& pixel_shader,
    const std::vector<Diligent::LayoutElement>& input_layout,
    const std::vector<Diligent::ShaderResourceVariableDesc>& variables,
    const std::vector<Diligent::ImmutableSamplerDesc>& samplers,
    Diligent::TEXTURE_FORMAT target_format) {
  Diligent::GraphicsPipelineStateCreateInfo pipeline_state_desc;
  pipeline_state_desc.PSODesc.Name = "";
  pipeline_state_desc.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;

  pipeline_state_desc.GraphicsPipeline.NumRenderTargets = 1;
  pipeline_state_desc.GraphicsPipeline.RTVFormats[0] = target_format;
  pipeline_state_desc.GraphicsPipeline.PrimitiveTopology =
      Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  pipeline_state_desc.GraphicsPipeline.RasterizerDesc.CullMode =
      Diligent::CULL_MODE_NONE;
  pipeline_state_desc.GraphicsPipeline.RasterizerDesc.ScissorEnable =
      Diligent::True;
  pipeline_state_desc.GraphicsPipeline.DepthStencilDesc.DepthEnable =
      Diligent::False;

  Diligent::ShaderCreateInfo shader_desc;
  Diligent::RefCntAutoPtr<Diligent::IShader> vertex_shader_object,
      pixel_shader_object;
  shader_desc.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
  shader_desc.Desc.UseCombinedTextureSamplers = Diligent::True;
  shader_desc.CompileFlags =
      Diligent::SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;

  {
    shader_desc.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
    shader_desc.EntryPoint = vertex_shader.entry.c_str();
    shader_desc.Desc.Name = vertex_shader.name.c_str();
    shader_desc.Source = vertex_shader.source.c_str();
    device_->CreateShader(shader_desc, &vertex_shader_object);

    shader_desc.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
    shader_desc.EntryPoint = pixel_shader.entry.c_str();
    shader_desc.Desc.Name = pixel_shader.name.c_str();
    shader_desc.Source = pixel_shader.source.c_str();
    device_->CreateShader(shader_desc, &pixel_shader_object);
  }

  pipeline_state_desc.pVS = vertex_shader_object;
  pipeline_state_desc.pPS = pixel_shader_object;

  pipeline_state_desc.GraphicsPipeline.InputLayout.LayoutElements =
      input_layout.data();
  pipeline_state_desc.GraphicsPipeline.InputLayout.NumElements =
      input_layout.size();

  pipeline_state_desc.PSODesc.ResourceLayout.Variables = variables.data();
  pipeline_state_desc.PSODesc.ResourceLayout.NumVariables = variables.size();

  pipeline_state_desc.PSODesc.ResourceLayout.ImmutableSamplers =
      samplers.data();
  pipeline_state_desc.PSODesc.ResourceLayout.NumImmutableSamplers =
      samplers.size();

  for (int i = 0; i < BlendType::TYPE_NUMS; ++i) {
    pipeline_state_desc.GraphicsPipeline.BlendDesc.RenderTargets[0] =
        GetBlendState(static_cast<BlendType>(i));

    Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
    device_->CreateGraphicsPipelineState(pipeline_state_desc, &pipeline_state);

    pipelines_.push_back(pipeline_state);

    if (i > 0 && !pipeline_state->IsCompatibleWith(pipelines_[0]))
      LOG(FATAL) << "[Renderer] Pipeline is not compatible with other state.";
  }
}

Pipeline_Base::Pipeline_Base(
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device,
    Diligent::TEXTURE_FORMAT target_format)
    : RenderPipelineBase(device) {
  const std::vector<Diligent::ShaderResourceVariableDesc> variables = {
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_Texture",
          {Diligent::FILTER_TYPE_POINT, Diligent::FILTER_TYPE_POINT,
           Diligent::FILTER_TYPE_POINT, Diligent::TEXTURE_ADDRESS_CLAMP,
           Diligent::TEXTURE_ADDRESS_CLAMP, Diligent::TEXTURE_ADDRESS_CLAMP},
      },
  };

  BuildPipeline(kBaseRenderWGSL, "vertexMain", "fragmentMain",
                {
                    Vertex::GetLayout(),
                },
                {
                    WorldMatrixUniform::GetLayout(device),
                    TextureBindingUniform::GetLayout(device),
                },
                target_format);
}

Pipeline_Color::Pipeline_Color(const wgpu::Device& device,
                               wgpu::TextureFormat target)
    : RenderPipelineBase(device) {
  BuildPipeline(kColorRenderWGSL, "vertexMain", "fragmentMain",
                {
                    Vertex::GetLayout(),
                },
                {
                    WorldMatrixUniform::GetLayout(device),
                },
                target);
}

Pipeline_Viewport::Pipeline_Viewport(const wgpu::Device& device,
                                     wgpu::TextureFormat target)
    : RenderPipelineBase(device) {
  BuildPipeline(kViewportBaseRenderWGSL, "vertexMain", "fragmentMain",
                {
                    Vertex::GetLayout(),
                },
                {
                    WorldMatrixUniform::GetLayout(device),
                    TextureBindingUniform::GetLayout(device),
                    ViewportFragmentUniform::GetLayout(device),
                },
                target);
}

Pipeline_Sprite::Pipeline_Sprite(const wgpu::Device& device,
                                 wgpu::TextureFormat target)
    : RenderPipelineBase(device) {
  BuildPipeline(kSpriteRenderWGSL, "vertexMain", "fragmentMain",
                {
                    Vertex::GetLayout(),
                },
                {
                    WorldMatrixUniform::GetLayout(device),
                    TextureBindingUniform::GetLayout(device),
                    SpriteUniform::GetLayout(device),
                },
                target);
}

Pipeline_AlphaTransition::Pipeline_AlphaTransition(const wgpu::Device& device,
                                                   wgpu::TextureFormat target)
    : RenderPipelineBase(device) {
  BuildPipeline(kAlphaTransitionRenderWGSL, "vertexMain", "fragmentMain",
                {
                    Vertex::GetLayout(),
                },
                {
                    AlphaTransitionUniform::GetLayout(device),
                },
                target);
}

Pipeline_MappedTransition::Pipeline_MappedTransition(const wgpu::Device& device,
                                                     wgpu::TextureFormat target)
    : RenderPipelineBase(device) {
  BuildPipeline(kMappedTransitionRenderWGSL, "vertexMain", "fragmentMain",
                {
                    Vertex::GetLayout(),
                },
                {
                    VagueTransitionUniform::GetLayout(device),
                },
                target);
}

Pipeline_Tilemap::Pipeline_Tilemap(const wgpu::Device& device,
                                   wgpu::TextureFormat target)
    : RenderPipelineBase(device) {
  BuildPipeline(kTilemapRenderWGSL, "vertexMain", "fragmentMain",
                {
                    Vertex::GetLayout(),
                },
                {
                    WorldMatrixUniform::GetLayout(device),
                    TextureBindingUniform::GetLayout(device),
                    TilemapUniform::GetLayout(device),
                },
                target);
}

Pipeline_Tilemap2::Pipeline_Tilemap2(const wgpu::Device& device,
                                     wgpu::TextureFormat target)
    : RenderPipelineBase(device) {
  BuildPipeline(kTilemap2RenderWGSL, "vertexMain", "fragmentMain",
                {
                    Vertex::GetLayout(),
                },
                {
                    WorldMatrixUniform::GetLayout(device),
                    TextureBindingUniform::GetLayout(device),
                    Tilemap2Uniform::GetLayout(device),
                },
                target);
}

}  // namespace renderer
