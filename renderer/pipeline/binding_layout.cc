// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/pipeline/binding_layout.h"
#include "binding_layout.h"

namespace renderer {

/// <summary>
/// World Binding
/// </summary>

wgpu::BindGroupLayout WorldMatrixUniform::GetLayout(
    const wgpu::Device& device) {
  wgpu::BindGroupLayoutEntry entries[1];
  entries[0].binding = 0;
  entries[0].visibility = wgpu::ShaderStage::Vertex;
  entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

  wgpu::BindGroupLayoutDescriptor binding_desc;
  binding_desc.label = "world.matrix.binding";
  binding_desc.entryCount = _countof(entries);
  binding_desc.entries = entries;

  return device.CreateBindGroupLayout(&binding_desc);
}

wgpu::BindGroup WorldMatrixUniform::CreateGroup(const wgpu::Device& device,
                                                const wgpu::Buffer& buffer) {
  wgpu::BindGroupEntry entries;
  entries.binding = 0;
  entries.buffer = buffer;

  wgpu::BindGroupDescriptor binding_desc;
  binding_desc.entryCount = 1;
  binding_desc.entries = &entries;
  binding_desc.layout = GetLayout(device);

  return device.CreateBindGroup(&binding_desc);
}

/// <summary>
/// Texture Binding
/// </summary>

wgpu::BindGroupLayout TextureBindingUniform::GetLayout(
    const wgpu::Device& device) {
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
  binding_desc.label = "texture.binding";
  binding_desc.entryCount = _countof(entries);
  binding_desc.entries = entries;

  return device.CreateBindGroupLayout(&binding_desc);
}

wgpu::BindGroup TextureBindingUniform::CreateGroup(
    const wgpu::Device& device,
    const wgpu::TextureView& view,
    const wgpu::Sampler& sampler,
    const wgpu::Buffer& buffer) {
  wgpu::BindGroupEntry entries[3];
  entries[0].binding = 0;
  entries[0].textureView = view;
  entries[1].binding = 1;
  entries[1].sampler = sampler;
  entries[2].binding = 2;
  entries[2].buffer = buffer;

  wgpu::BindGroupDescriptor binding_desc;
  binding_desc.entryCount = _countof(entries);
  binding_desc.entries = entries;
  binding_desc.layout = GetLayout(device);

  return device.CreateBindGroup(&binding_desc);
}

/// <summary>
/// Viewport Binding
/// </summary>

wgpu::BindGroupLayout ViewportFragmentUniform::GetLayout(
    const wgpu::Device& device) {
  wgpu::BindGroupLayoutEntry entries[1];
  entries[0].binding = 0;
  entries[0].visibility = wgpu::ShaderStage::Fragment;
  entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

  wgpu::BindGroupLayoutDescriptor binding_desc;
  binding_desc.label = "viewport.fragment.binding";
  binding_desc.entryCount = _countof(entries);
  binding_desc.entries = entries;

  return device.CreateBindGroupLayout(&binding_desc);
}

wgpu::BindGroup ViewportFragmentUniform::CreateGroup(
    const wgpu::Device& device,
    const wgpu::Buffer& buffer) {
  wgpu::BindGroupEntry entries;
  entries.binding = 0;
  entries.buffer = buffer;

  wgpu::BindGroupDescriptor binding_desc;
  binding_desc.entryCount = 1;
  binding_desc.entries = &entries;
  binding_desc.layout = GetLayout(device);

  return device.CreateBindGroup(&binding_desc);
}

}  // namespace renderer
