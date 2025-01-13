// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/pipeline/render_pipeline.h"

#include "renderer/pipeline/builtin_wgsl.h"

namespace renderer {

namespace {

wgpu::BlendState GetWGPUBlendState(BlendType type) {
  wgpu::BlendState state;
  switch (type) {
    default:
    case renderer::BlendType::kNormal:
      state.color.operation = wgpu::BlendOperation::Add;
      state.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
      state.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
      state.alpha.operation = wgpu::BlendOperation::Add;
      state.alpha.srcFactor = wgpu::BlendFactor::One;
      state.alpha.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
      break;
    case renderer::BlendType::kAddition:
      state.color.operation = wgpu::BlendOperation::Add;
      state.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
      state.color.dstFactor = wgpu::BlendFactor::One;
      state.alpha.operation = wgpu::BlendOperation::Add;
      state.alpha.srcFactor = wgpu::BlendFactor::One;
      state.alpha.dstFactor = wgpu::BlendFactor::One;
      break;
    case renderer::BlendType::kSubstraction:
      state.color.operation = wgpu::BlendOperation::ReverseSubtract;
      state.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
      state.color.dstFactor = wgpu::BlendFactor::One;
      state.alpha.operation = wgpu::BlendOperation::ReverseSubtract;
      state.alpha.srcFactor = wgpu::BlendFactor::Zero;
      state.alpha.dstFactor = wgpu::BlendFactor::One;
      break;
    case renderer::BlendType::kNoBlend:
      break;
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

  bindings_ = std::move(bind_layout);
  wgpu::PipelineLayoutDescriptor pipeline_layout_desc;
  pipeline_layout_desc.bindGroupLayoutCount = bindings_.size();
  pipeline_layout_desc.bindGroupLayouts = bindings_.data();
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

  for (int i = 0; i < BlendType::kBlendNums; ++i) {
    auto blend_state = GetWGPUBlendState(static_cast<BlendType>(i));
    color_target.blend = &blend_state;
    pipelines_.push_back(device_.CreateRenderPipeline(&descriptor));
  }
}

Pipeline_Base::Pipeline_Base(const wgpu::Device& device,
                             wgpu::TextureFormat target)
    : RenderPipelineBase(device) {
  wgpu::BindGroupLayout binding1, binding2;

  {
    wgpu::BindGroupLayoutEntry entries[1];
    entries[0].binding = 0;
    entries[0].visibility = wgpu::ShaderStage::Vertex;
    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

    wgpu::BindGroupLayoutDescriptor binding_desc;
    binding_desc.entryCount = _countof(entries);
    binding_desc.entries = entries;

    binding1 = device.CreateBindGroupLayout(&binding_desc);
  }

  {
    wgpu::BindGroupLayoutEntry entries[3];
    entries[0].binding = 0;
    entries[0].visibility = wgpu::ShaderStage::Fragment;
    entries[0].texture.sampleType = wgpu::TextureSampleType::Float;
    entries[0].texture.viewDimension = wgpu::TextureViewDimension::e2D;
    entries[1].binding = 1;
    entries[1].visibility = wgpu::ShaderStage::Fragment;
    entries[1].sampler.type = wgpu::SamplerBindingType::Filtering;
    entries[2].binding = 2;
    entries[2].visibility = wgpu::ShaderStage::Vertex;
    entries[2].buffer.type = wgpu::BufferBindingType::Uniform;

    wgpu::BindGroupLayoutDescriptor binding_desc;
    binding_desc.entryCount = _countof(entries);
    binding_desc.entries = entries;

    binding2 = device.CreateBindGroupLayout(&binding_desc);
  }

  BuildPipeline(kBaseRenderWGSL, "vertexMain", "fragmentMain",
                {
                    VertexType::GetLayout(),
                },
                {
                    binding1,
                },
                target);
}

Pipeline_Color::Pipeline_Color(const wgpu::Device& device,
                               wgpu::TextureFormat target)
    : RenderPipelineBase(device) {
  wgpu::BindGroupLayout binding;

  {
    wgpu::BindGroupLayoutEntry entries[1];
    entries[0].binding = 0;
    entries[0].visibility = wgpu::ShaderStage::Vertex;
    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

    wgpu::BindGroupLayoutDescriptor binding_desc;
    binding_desc.entryCount = _countof(entries);
    binding_desc.entries = entries;
    binding = device.CreateBindGroupLayout(&binding_desc);
  }

  BuildPipeline(kColorRenderWGSL, "vertexMain", "fragmentMain",
                {
                    VertexType::GetLayout(),
                },
                {
                    binding,
                },
                target);
}

}  // namespace renderer
