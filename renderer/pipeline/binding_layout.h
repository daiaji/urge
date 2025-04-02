// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_PIPELINE_BINDING_LAYOUT_H_
#define RENDERER_PIPELINE_BINDING_LAYOUT_H_

#include "base/math/rectangle.h"
#include "renderer/vertex/vertex_layout.h"

namespace renderer {

#define WGPU_ALIGN_TYPE(ty) alignas(sizeof(ty)) ty

struct WorldMatrixUniform {
  WGPU_ALIGN_TYPE(float) projection[16];
  WGPU_ALIGN_TYPE(float) transform[16];

  static wgpu::BindGroupLayout GetLayout(const wgpu::Device& device);
  static wgpu::BindGroup CreateGroup(const wgpu::Device& device,
                                     const wgpu::Buffer& buffer);
};

struct TextureBindingUniform {
  WGPU_ALIGN_TYPE(base::Vec2) texture_size;

  static wgpu::BindGroupLayout GetLayout(const wgpu::Device& device);
  static wgpu::BindGroup CreateGroup(const wgpu::Device& device,
                                     const wgpu::TextureView& view,
                                     const wgpu::Sampler& sampler,
                                     const wgpu::Buffer& buffer);
};

struct ViewportFragmentUniform {
  WGPU_ALIGN_TYPE(base::Vec4) color;
  WGPU_ALIGN_TYPE(base::Vec4) tone;

  static wgpu::BindGroupLayout GetLayout(const wgpu::Device& device);
  static wgpu::BindGroup CreateGroup(const wgpu::Device& device,
                                     const wgpu::Buffer& buffer);
};

struct SpriteVertex {
  WGPU_ALIGN_TYPE(base::Vec4) position;
  WGPU_ALIGN_TYPE(base::Vec2) texcoord;
};

struct SpriteUniform {
  WGPU_ALIGN_TYPE(base::Vec2) position;
  WGPU_ALIGN_TYPE(base::Vec2) origin;
  WGPU_ALIGN_TYPE(base::Vec2) scale;
  WGPU_ALIGN_TYPE(float) rotation;

  WGPU_ALIGN_TYPE(base::Vec4) color;
  WGPU_ALIGN_TYPE(base::Vec4) tone;
  WGPU_ALIGN_TYPE(float) opacity;
  WGPU_ALIGN_TYPE(float) bush_depth;
  WGPU_ALIGN_TYPE(float) bush_opacity;

  static wgpu::BindGroupLayout GetLayout(const wgpu::Device& device);
  static wgpu::BindGroup CreateGroup(const wgpu::Device& device,
                                     const wgpu::Buffer& vertex,
                                     const wgpu::Buffer& buffer);
};

struct AlphaTransitionUniform {
  static wgpu::BindGroupLayout GetLayout(const wgpu::Device& device);
  static wgpu::BindGroup CreateGroup(const wgpu::Device& device,
                                     const wgpu::TextureView& frozen,
                                     const wgpu::TextureView& current,
                                     const wgpu::Sampler& sampler);
};

struct VagueTransitionUniform {
  static wgpu::BindGroupLayout GetLayout(const wgpu::Device& device);
  static wgpu::BindGroup CreateGroup(const wgpu::Device& device,
                                     const wgpu::TextureView& frozen,
                                     const wgpu::TextureView& current,
                                     const wgpu::TextureView& trans,
                                     const wgpu::Sampler& sampler);
};

struct TilemapUniform {
  WGPU_ALIGN_TYPE(base::Vec2) offset;
  WGPU_ALIGN_TYPE(float) tile_size;
  WGPU_ALIGN_TYPE(float) anim_index;

  static wgpu::BindGroupLayout GetLayout(const wgpu::Device& device);
  static wgpu::BindGroup CreateGroup(const wgpu::Device& device,
                                     const wgpu::Buffer& buffer);
};

struct Tilemap2Uniform {
  WGPU_ALIGN_TYPE(base::Vec2) offset;
  WGPU_ALIGN_TYPE(base::Vec2) animation_offset;
  WGPU_ALIGN_TYPE(float) tile_size;

  static wgpu::BindGroupLayout GetLayout(const wgpu::Device& device);
  static wgpu::BindGroup CreateGroup(const wgpu::Device& device,
                                     const wgpu::Buffer& buffer);
};

}  // namespace renderer

#endif  //! RENDERER_PIPELINE_BINDING_LAYOUT_H_
