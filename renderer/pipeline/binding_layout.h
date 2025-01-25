// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_PIPELINE_BINDING_LAYOUT_H_
#define RENDERER_PIPELINE_BINDING_LAYOUT_H_

#include "base/math/rectangle.h"
#include "webgpu/webgpu_cpp.h"

namespace renderer {

template <typename BufferType>
inline wgpu::Buffer CreateUniformBuffer(const wgpu::Device& device,
                                        const std::string_view& label,
                                        BufferType* data = nullptr) {
  wgpu::BufferDescriptor buffer_desc;
  buffer_desc.label = label;
  buffer_desc.size = sizeof(BufferType);
  buffer_desc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
  buffer_desc.mappedAtCreation = !!data;
  auto result = device.CreateBuffer(&buffer_desc);
  if (data) {
    std::memcpy(result.GetMappedRange(), data, sizeof(BufferType));
    result.Unmap();
  }

  return result;
}

struct WorldMatrixUniform {
  float projection[16];
  float transform[16];

  static wgpu::BindGroupLayout GetLayout(const wgpu::Device& device);
  static wgpu::BindGroup CreateGroup(const wgpu::Device& device,
                                     const wgpu::Buffer& buffer);
};

struct TextureBindingUniform {
  base::Vec2 texture_size;

  static wgpu::BindGroupLayout GetLayout(const wgpu::Device& device);
  static wgpu::BindGroup CreateGroup(const wgpu::Device& device,
                                     const wgpu::TextureView& view,
                                     const wgpu::Sampler& sampler,
                                     const wgpu::Buffer& buffer);
};

struct ViewportFragmentUniform {
  base::Vec4 color;
  base::Vec4 tone;

  static wgpu::BindGroupLayout GetLayout(const wgpu::Device& device);
  static wgpu::BindGroup CreateGroup(const wgpu::Device& device,
                                     const wgpu::Buffer& buffer);
};

}  // namespace renderer

#endif  //! RENDERER_PIPELINE_BINDING_LAYOUT_H_
