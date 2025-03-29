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
  entries[2].visibility =
      wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
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

/// <summary>
/// Sprite Binding
/// </summary>

wgpu::BindGroupLayout SpriteUniform::GetLayout(const wgpu::Device& device) {
  wgpu::BindGroupLayoutEntry entries[1];
  entries[0].binding = 0;
  entries[0].visibility =
      wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
  entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

  wgpu::BindGroupLayoutDescriptor binding_desc;
  binding_desc.label = "sprite.fragment.binding";
  binding_desc.entryCount = _countof(entries);
  binding_desc.entries = entries;

  return device.CreateBindGroupLayout(&binding_desc);
}

wgpu::BindGroup SpriteUniform::CreateGroup(const wgpu::Device& device,
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

wgpu::BindGroupLayout SpriteUniform::GetInstanceLayout(
    const wgpu::Device& device) {
  wgpu::BindGroupLayoutEntry entries[1];
  entries[0].binding = 0;
  entries[0].visibility =
      wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
  entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
  entries[0].buffer.minBindingSize = sizeof(SpriteUniform);

  wgpu::BindGroupLayoutDescriptor binding_desc;
  binding_desc.label = "sprite.batch.fragment.binding";
  binding_desc.entryCount = _countof(entries);
  binding_desc.entries = entries;

  return device.CreateBindGroupLayout(&binding_desc);
}

wgpu::BindGroup SpriteUniform::CreateInstanceGroup(const wgpu::Device& device,
                                                   const wgpu::Buffer& buffer) {
  wgpu::BindGroupEntry entries;
  entries.binding = 0;
  entries.buffer = buffer;

  wgpu::BindGroupDescriptor binding_desc;
  binding_desc.entryCount = 1;
  binding_desc.entries = &entries;
  binding_desc.layout = GetInstanceLayout(device);

  return device.CreateBindGroup(&binding_desc);
}

/// <summary>
/// Alpha Transition Uniform
/// </summary>

wgpu::BindGroupLayout AlphaTransitionUniform::GetLayout(
    const wgpu::Device& device) {
  wgpu::BindGroupLayoutEntry entries[3];
  entries[0].binding = 0;
  entries[0].visibility = wgpu::ShaderStage::Fragment;
  entries[0].texture.sampleType = wgpu::TextureSampleType::Float;
  entries[0].texture.viewDimension = wgpu::TextureViewDimension::e2D;
  entries[1].binding = 1;
  entries[1].visibility = wgpu::ShaderStage::Fragment;
  entries[1].texture.sampleType = wgpu::TextureSampleType::Float;
  entries[1].texture.viewDimension = wgpu::TextureViewDimension::e2D;
  entries[2].binding = 2;
  entries[2].visibility = wgpu::ShaderStage::Fragment;
  entries[2].sampler.type = wgpu::SamplerBindingType::Filtering;

  wgpu::BindGroupLayoutDescriptor binding_desc;
  binding_desc.label = "alpha.transition.binding";
  binding_desc.entryCount = _countof(entries);
  binding_desc.entries = entries;

  return device.CreateBindGroupLayout(&binding_desc);
}

wgpu::BindGroup AlphaTransitionUniform::CreateGroup(
    const wgpu::Device& device,
    const wgpu::TextureView& frozen,
    const wgpu::TextureView& current,
    const wgpu::Sampler& sampler) {
  wgpu::BindGroupEntry entries[3];
  entries[0].binding = 0;
  entries[0].textureView = frozen;
  entries[1].binding = 1;
  entries[1].textureView = current;
  entries[2].binding = 2;
  entries[2].sampler = sampler;

  wgpu::BindGroupDescriptor binding_desc;
  binding_desc.entryCount = _countof(entries);
  binding_desc.entries = entries;
  binding_desc.layout = GetLayout(device);

  return device.CreateBindGroup(&binding_desc);
}

/// <summary>
/// Vague Transition Uniform
/// </summary>

wgpu::BindGroupLayout VagueTransitionUniform::GetLayout(
    const wgpu::Device& device) {
  wgpu::BindGroupLayoutEntry entries[4];
  entries[0].binding = 0;
  entries[0].visibility = wgpu::ShaderStage::Fragment;
  entries[0].texture.sampleType = wgpu::TextureSampleType::Float;
  entries[0].texture.viewDimension = wgpu::TextureViewDimension::e2D;
  entries[1].binding = 1;
  entries[1].visibility = wgpu::ShaderStage::Fragment;
  entries[1].texture.sampleType = wgpu::TextureSampleType::Float;
  entries[1].texture.viewDimension = wgpu::TextureViewDimension::e2D;
  entries[2].binding = 2;
  entries[2].visibility = wgpu::ShaderStage::Fragment;
  entries[2].texture.sampleType = wgpu::TextureSampleType::Float;
  entries[2].texture.viewDimension = wgpu::TextureViewDimension::e2D;
  entries[3].binding = 3;
  entries[3].visibility = wgpu::ShaderStage::Fragment;
  entries[3].sampler.type = wgpu::SamplerBindingType::Filtering;

  wgpu::BindGroupLayoutDescriptor binding_desc;
  binding_desc.label = "vague.transition.binding";
  binding_desc.entryCount = _countof(entries);
  binding_desc.entries = entries;

  return device.CreateBindGroupLayout(&binding_desc);
}

wgpu::BindGroup VagueTransitionUniform::CreateGroup(
    const wgpu::Device& device,
    const wgpu::TextureView& frozen,
    const wgpu::TextureView& current,
    const wgpu::TextureView& trans,
    const wgpu::Sampler& sampler) {
  wgpu::BindGroupEntry entries[4];
  entries[0].binding = 0;
  entries[0].textureView = frozen;
  entries[1].binding = 1;
  entries[1].textureView = current;
  entries[2].binding = 2;
  entries[2].textureView = trans;
  entries[3].binding = 3;
  entries[3].sampler = sampler;

  wgpu::BindGroupDescriptor binding_desc;
  binding_desc.entryCount = _countof(entries);
  binding_desc.entries = entries;
  binding_desc.layout = GetLayout(device);

  return device.CreateBindGroup(&binding_desc);
}

/// <summary>
/// Tilemap Uniform
/// </summary>

wgpu::BindGroupLayout TilemapUniform::GetLayout(const wgpu::Device& device) {
  wgpu::BindGroupLayoutEntry entries[1];
  entries[0].binding = 0;
  entries[0].visibility =
      wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
  entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

  wgpu::BindGroupLayoutDescriptor binding_desc;
  binding_desc.label = "tilemap.fragment.binding";
  binding_desc.entryCount = _countof(entries);
  binding_desc.entries = entries;

  return device.CreateBindGroupLayout(&binding_desc);
}

wgpu::BindGroup TilemapUniform::CreateGroup(const wgpu::Device& device,
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
/// Tilemap2 Uniform
/// </summary>

wgpu::BindGroupLayout Tilemap2Uniform::GetLayout(const wgpu::Device& device) {
  wgpu::BindGroupLayoutEntry entries[1];
  entries[0].binding = 0;
  entries[0].visibility =
      wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
  entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

  wgpu::BindGroupLayoutDescriptor binding_desc;
  binding_desc.label = "tilemap.fragment.binding";
  binding_desc.entryCount = _countof(entries);
  binding_desc.entries = entries;

  return device.CreateBindGroupLayout(&binding_desc);
}

wgpu::BindGroup Tilemap2Uniform::CreateGroup(const wgpu::Device& device,
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
