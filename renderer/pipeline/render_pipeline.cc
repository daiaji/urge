// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/pipeline/render_pipeline.h"

#include <optional>

#include "renderer/pipeline/builtin_wgsl.h"

namespace renderer {

namespace {

std::optional<wgpu::BlendState> GetWGPUBlendState(BlendType type) {
  wgpu::BlendState state;
  switch (type) {
    default:
    case renderer::BlendType::NORMAL:
      state.color.operation = wgpu::BlendOperation::Add;
      state.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
      state.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
      state.alpha.operation = wgpu::BlendOperation::Add;
      state.alpha.srcFactor = wgpu::BlendFactor::One;
      state.alpha.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
      break;
    case renderer::BlendType::ADDITION:
      state.color.operation = wgpu::BlendOperation::Add;
      state.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
      state.color.dstFactor = wgpu::BlendFactor::One;
      state.alpha.operation = wgpu::BlendOperation::Add;
      state.alpha.srcFactor = wgpu::BlendFactor::One;
      state.alpha.dstFactor = wgpu::BlendFactor::One;
      break;
    case renderer::BlendType::SUBSTRACTION:
      state.color.operation = wgpu::BlendOperation::ReverseSubtract;
      state.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
      state.color.dstFactor = wgpu::BlendFactor::One;
      state.alpha.operation = wgpu::BlendOperation::ReverseSubtract;
      state.alpha.srcFactor = wgpu::BlendFactor::Zero;
      state.alpha.dstFactor = wgpu::BlendFactor::One;
      break;
    case renderer::BlendType::NO_BLEND:
      return std::nullopt;
  }

  return state;
}

}  // namespace

RenderPipelineBase::RenderPipelineBase(const wgpu::Device& device)
    : device_(device) {}

void RenderPipelineBase::BuildPipeline(
    const std::string& shader_source,
    const std::string& vs_entry,
    const std::string& fs_entry,
    std::vector<wgpu::VertexBufferLayout> vertex_layout,
    std::vector<wgpu::BindGroupLayout> bind_layout,
    wgpu::TextureFormat target_format) {
  wgpu::ShaderModuleWGSLDescriptor wgsl_desc;
  wgsl_desc.code = std::string_view(shader_source);

  wgpu::ShaderModuleDescriptor shader_module_desc;
  shader_module_desc.nextInChain = &wgsl_desc;
  wgpu::ShaderModule shader_module =
      device_.CreateShaderModule(&shader_module_desc);

  layouts_ = std::move(bind_layout);
  wgpu::PipelineLayoutDescriptor pipeline_layout_desc;
  pipeline_layout_desc.bindGroupLayoutCount = layouts_.size();
  pipeline_layout_desc.bindGroupLayouts = layouts_.data();
  wgpu::PipelineLayout pipeline_layout =
      device_.CreatePipelineLayout(&pipeline_layout_desc);

  wgpu::VertexState vertex_state;
  vertex_state.module = shader_module;
  vertex_state.entryPoint = std::string_view(vs_entry);
  vertex_state.bufferCount = vertex_layout.size();
  vertex_state.buffers = vertex_layout.data();

  wgpu::ColorTargetState color_target;
  color_target.format = target_format;

  wgpu::FragmentState fragment_state;
  fragment_state.module = shader_module;
  fragment_state.entryPoint = std::string_view(fs_entry);
  fragment_state.targetCount = 1;
  fragment_state.targets = &color_target;

  wgpu::RenderPipelineDescriptor descriptor;
  descriptor.layout = pipeline_layout;
  descriptor.vertex = vertex_state;
  descriptor.fragment = &fragment_state;

  for (int i = 0; i < BlendType::TYPE_NUMS; ++i) {
    // Generate color blend structure.
    auto blend_state = GetWGPUBlendState(static_cast<BlendType>(i));
    color_target.blend = nullptr;
    if (blend_state.has_value())
      color_target.blend = &blend_state.value();

    // Create graphics pipeline
    pipelines_.push_back(device_.CreateRenderPipeline(&descriptor));
  }
}

Pipeline_Base::Pipeline_Base(const wgpu::Device& device,
                             wgpu::TextureFormat target)
    : RenderPipelineBase(device) {
  BuildPipeline(kBaseRenderWGSL, "vertexMain", "fragmentMain",
                {
                    Vertex::GetLayout(),
                },
                {
                    WorldMatrixUniform::GetLayout(device),
                    TextureBindingUniform::GetLayout(device),
                },
                target);
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
