// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/pipeline/render_pipeline.h"

#include "base/debug/logging.h"
#include "renderer/pipeline/builtin_hlsl.h"

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

RenderPipelineBase::RenderPipelineBase(Diligent::IRenderDevice* device)
    : device_(device) {}

void RenderPipelineBase::BuildPipeline(
    const ShaderSource& vertex_shader,
    const ShaderSource& pixel_shader,
    const std::vector<Diligent::LayoutElement>& input_layout,
    const std::vector<Diligent::ShaderResourceVariableDesc>& variables,
    const std::vector<Diligent::ImmutableSamplerDesc>& samplers,
    Diligent::TEXTURE_FORMAT target_format) {
  std::stringstream pipeline_debug_name;
  pipeline_debug_name << "pipeline<" << vertex_shader.name << ", "
                      << pixel_shader.name << ">";
  pipeline_name_ = pipeline_debug_name.str();

  Diligent::GraphicsPipelineStateCreateInfo pipeline_state_desc;
  pipeline_state_desc.PSODesc.Name = pipeline_name_.c_str();
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
    shader_desc.SourceLength = vertex_shader.source.size();
    shader_desc.Macros.Count = vertex_shader.macros.size();
    shader_desc.Macros.Elements = vertex_shader.macros.data();
    device_->CreateShader(shader_desc, &vertex_shader_object);

    shader_desc.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
    shader_desc.EntryPoint = pixel_shader.entry.c_str();
    shader_desc.Desc.Name = pixel_shader.name.c_str();
    shader_desc.Source = pixel_shader.source.c_str();
    shader_desc.SourceLength = pixel_shader.source.size();
    shader_desc.Macros.Count = pixel_shader.macros.size();
    shader_desc.Macros.Elements = pixel_shader.macros.data();
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

  for (int32_t i = 0; i < BlendType::TYPE_NUMS; ++i) {
    pipeline_state_desc.GraphicsPipeline.BlendDesc.RenderTargets[0] =
        GetBlendState(static_cast<BlendType>(i));

    Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
    device_->CreateGraphicsPipelineState(pipeline_state_desc, &pipeline_state);

    if (i > 0 && !pipeline_state->IsCompatibleWith(pipelines_[0]))
      LOG(ERROR) << "[Renderer] Pipeline is not compatible with other state.";

    pipelines_.push_back(pipeline_state);
  }
}

Pipeline_Base::Pipeline_Base(Diligent::IRenderDevice* device,
                             Diligent::TEXTURE_FORMAT target_format)
    : RenderPipelineBase(device) {
  const ShaderSource vertex_shader{
      kHLSL_BaseRender_VertexShader, "main", "base.vertex", {}};
  const ShaderSource pixel_shader{
      kHLSL_BaseRender_PixelShader, "main", "base.pixel", {}};

  const std::vector<Diligent::ShaderResourceVariableDesc> variables = {
      {Diligent::SHADER_TYPE_VERTEX, "WorldMatrixBuffer",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
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

  BuildPipeline(vertex_shader, pixel_shader, Vertex::GetLayout(), variables,
                samplers, target_format);
}

Pipeline_Color::Pipeline_Color(Diligent::IRenderDevice* device,
                               Diligent::TEXTURE_FORMAT target_format)
    : RenderPipelineBase(device) {
  const ShaderSource vertex_shader{
      kHLSL_ColorRender_VertexShader, "main", "color.vertex", {}};
  const ShaderSource pixel_shader{
      kHLSL_ColorRender_PixelShader, "main", "color.pixel", {}};

  const std::vector<Diligent::ShaderResourceVariableDesc> variables = {
      {Diligent::SHADER_TYPE_VERTEX, "WorldMatrixBuffer",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  BuildPipeline(vertex_shader, pixel_shader, Vertex::GetLayout(), variables, {},
                target_format);
}

Pipeline_Flat::Pipeline_Flat(Diligent::IRenderDevice* device,
                             Diligent::TEXTURE_FORMAT target_format)
    : RenderPipelineBase(device) {
  const ShaderSource vertex_shader{
      kHLSL_FlatRender_VertexShader, "main", "flat.vertex", {}};
  const ShaderSource pixel_shader{
      kHLSL_FlatRender_PixelShader, "main", "flat.pixel", {}};

  const std::vector<Diligent::ShaderResourceVariableDesc> variables = {
      {Diligent::SHADER_TYPE_VERTEX, "WorldMatrixBuffer",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "FlatUniformConstants",
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

  BuildPipeline(vertex_shader, pixel_shader, Vertex::GetLayout(), variables,
                samplers, target_format);
}

Pipeline_Sprite::Pipeline_Sprite(Diligent::IRenderDevice* device,
                                 Diligent::TEXTURE_FORMAT target_format)
    : RenderPipelineBase(device) {
  storage_buffer_support = device->GetAdapterInfo().Features.ComputeShaders ==
                           Diligent::DEVICE_FEATURE_STATE_ENABLED;

  Diligent::ShaderMacro vertex_macro = {"STORAGE_BUFFER_SUPPORT",
                                        storage_buffer_support ? "1" : "0"};

  const ShaderSource vertex_shader{
      kHLSL_SpriteRender_VertexShader, "main", "sprite.vertex", {vertex_macro}};
  const ShaderSource pixel_shader{
      kHLSL_SpriteRender_PixelShader, "main", "sprite.pixel", {}};

  const std::vector<Diligent::ShaderResourceVariableDesc> variables = {
      {Diligent::SHADER_TYPE_VERTEX, "WorldMatrixBuffer",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_VERTEX, "u_Vertices",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_VERTEX,
       storage_buffer_support ? "u_Params" : "SpriteUniformParam",
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

  BuildPipeline(vertex_shader, pixel_shader, Vertex::GetLayout(), variables,
                samplers, target_format);
}

Pipeline_AlphaTransition::Pipeline_AlphaTransition(
    Diligent::IRenderDevice* device,
    Diligent::TEXTURE_FORMAT target_format)
    : RenderPipelineBase(device) {
  const ShaderSource vertex_shader{kHLSL_AlphaTransitionRender_VertexShader,
                                   "main",
                                   "alphatrans.vertex",
                                   {}};
  const ShaderSource pixel_shader{
      kHLSL_AlphaTransitionRender_PixelShader, "main", "alphatrans.pixel", {}};

  const std::vector<Diligent::ShaderResourceVariableDesc> variables = {
      {Diligent::SHADER_TYPE_PIXEL, "u_FrozenTexture",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_CurrentTexture",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_FrozenTexture",
          {Diligent::FILTER_TYPE_POINT, Diligent::FILTER_TYPE_POINT,
           Diligent::FILTER_TYPE_POINT, Diligent::TEXTURE_ADDRESS_CLAMP,
           Diligent::TEXTURE_ADDRESS_CLAMP, Diligent::TEXTURE_ADDRESS_CLAMP},
      },
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_CurrentTexture",
          {Diligent::FILTER_TYPE_POINT, Diligent::FILTER_TYPE_POINT,
           Diligent::FILTER_TYPE_POINT, Diligent::TEXTURE_ADDRESS_CLAMP,
           Diligent::TEXTURE_ADDRESS_CLAMP, Diligent::TEXTURE_ADDRESS_CLAMP},
      },
  };

  BuildPipeline(vertex_shader, pixel_shader, Vertex::GetLayout(), variables,
                samplers, target_format);
}

Pipeline_VagueTransition::Pipeline_VagueTransition(
    Diligent::IRenderDevice* device,
    Diligent::TEXTURE_FORMAT target_format)
    : RenderPipelineBase(device) {
  const ShaderSource vertex_shader{kHLSL_MappingTransitionRender_VertexShader,
                                   "main",
                                   "vaguetrans.vertex",
                                   {}};
  const ShaderSource pixel_shader{kHLSL_MappingTransitionRender_PixelShader,
                                  "main",
                                  "vaguetrans.pixel",
                                  {}};

  const std::vector<Diligent::ShaderResourceVariableDesc> variables = {
      {Diligent::SHADER_TYPE_PIXEL, "u_FrozenTexture",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_CurrentTexture",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_TransTexture",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_FrozenTexture",
          {Diligent::FILTER_TYPE_POINT, Diligent::FILTER_TYPE_POINT,
           Diligent::FILTER_TYPE_POINT, Diligent::TEXTURE_ADDRESS_CLAMP,
           Diligent::TEXTURE_ADDRESS_CLAMP, Diligent::TEXTURE_ADDRESS_CLAMP},
      },
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_CurrentTexture",
          {Diligent::FILTER_TYPE_POINT, Diligent::FILTER_TYPE_POINT,
           Diligent::FILTER_TYPE_POINT, Diligent::TEXTURE_ADDRESS_CLAMP,
           Diligent::TEXTURE_ADDRESS_CLAMP, Diligent::TEXTURE_ADDRESS_CLAMP},
      },
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_TransTexture",
          {Diligent::FILTER_TYPE_POINT, Diligent::FILTER_TYPE_POINT,
           Diligent::FILTER_TYPE_POINT, Diligent::TEXTURE_ADDRESS_CLAMP,
           Diligent::TEXTURE_ADDRESS_CLAMP, Diligent::TEXTURE_ADDRESS_CLAMP},
      },
  };

  BuildPipeline(vertex_shader, pixel_shader, Vertex::GetLayout(), variables,
                samplers, target_format);
}

Pipeline_Tilemap::Pipeline_Tilemap(Diligent::IRenderDevice* device,
                                   Diligent::TEXTURE_FORMAT target_format)
    : RenderPipelineBase(device) {
  const ShaderSource vertex_shader{
      kHLSL_TilemapRender_VertexShader, "main", "tilemap.vertex", {}};
  const ShaderSource pixel_shader{
      kHLSL_TilemapRender_PixelShader, "main", "tilemap.pixel", {}};

  const std::vector<Diligent::ShaderResourceVariableDesc> variables = {
      {Diligent::SHADER_TYPE_VERTEX, "WorldMatrixBuffer",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_VERTEX, "TilemapUniformBuffer",
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

  BuildPipeline(vertex_shader, pixel_shader, Vertex::GetLayout(), variables,
                samplers, target_format);
}

Pipeline_Tilemap2::Pipeline_Tilemap2(Diligent::IRenderDevice* device,
                                     Diligent::TEXTURE_FORMAT target_format)
    : RenderPipelineBase(device) {
  const ShaderSource vertex_shader{
      kHLSL_Tilemap2Render_VertexShader, "main", "tilemap2.vertex", {}};
  const ShaderSource pixel_shader{
      kHLSL_Tilemap2Render_PixelShader, "main", "tilemap2.pixel", {}};

  const std::vector<Diligent::ShaderResourceVariableDesc> variables = {
      {Diligent::SHADER_TYPE_VERTEX, "WorldMatrixBuffer",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_VERTEX, "Tilemap2UniformBuffer",
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

  BuildPipeline(vertex_shader, pixel_shader, Vertex::GetLayout(), variables,
                samplers, target_format);
}

Pipeline_BitmapHue::Pipeline_BitmapHue(Diligent::IRenderDevice* device,
                                       Diligent::TEXTURE_FORMAT target_format)
    : RenderPipelineBase(device) {
  const ShaderSource vertex_shader{
      kHLSL_BitmapHueRender_VertexShader, "main", "bitmap.hue.vertex", {}};
  const ShaderSource pixel_shader{
      kHLSL_BitmapHueRender_PixelShader, "main", "bitmap.hue.pixel", {}};

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

  BuildPipeline(vertex_shader, pixel_shader, Vertex::GetLayout(), variables,
                samplers, target_format);
}

Pipeline_Present::Pipeline_Present(Diligent::IRenderDevice* device,
                                   Diligent::TEXTURE_FORMAT target_format,
                                   bool setup_gamma_convert)
    : RenderPipelineBase(device) {
  Diligent::ShaderMacro pixel_macro = {"CONVERT_PS_GAMMA_TO_OUTPUT",
                                       setup_gamma_convert ? "1" : "0"};

  const ShaderSource vertex_shader{
      kHLSL_PresentRender_VertexShader, "main", "present.vertex", {}};
  const ShaderSource pixel_shader{
      kHLSL_PresentRender_PixelShader, "main", "present.pixel", {pixel_macro}};

  const std::vector<Diligent::ShaderResourceVariableDesc> variables = {
      {Diligent::SHADER_TYPE_VERTEX, "WorldMatrixBuffer",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture_sampler",
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  BuildPipeline(vertex_shader, pixel_shader, Vertex::GetLayout(), variables, {},
                target_format);
}

}  // namespace renderer
