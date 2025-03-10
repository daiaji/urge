// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_UTILS_BUFFER_UTILS_H_
#define RENDERER_UTILS_BUFFER_UTILS_H_

#include "base/math/rectangle.h"
#include "renderer/vertex/vertex_layout.h"

namespace renderer {

template <typename VertexType>
wgpu::Buffer CreateVertexBuffer(const wgpu::Device& device,
                                const std::string_view& label,
                                wgpu::BufferUsage usage,
                                size_t vertex_count,
                                VertexType* data = nullptr) {
  wgpu::BufferDescriptor buffer_desc;
  buffer_desc.label = label;
  buffer_desc.size = sizeof(VertexType) * vertex_count;
  buffer_desc.usage = wgpu::BufferUsage::Vertex | usage;
  buffer_desc.mappedAtCreation = !!data;
  auto result = device.CreateBuffer(&buffer_desc);
  if (data) {
    std::memcpy(result.GetMappedRange(), data, buffer_desc.size);
    result.Unmap();
  }

  return result;
}

template <typename BufferType>
inline wgpu::Buffer CreateUniformBuffer(const wgpu::Device& device,
                                        const std::string_view& label,
                                        wgpu::BufferUsage usage,
                                        BufferType* data = nullptr) {
  wgpu::BufferDescriptor buffer_desc;
  buffer_desc.label = label;
  buffer_desc.size = sizeof(BufferType);
  buffer_desc.usage = wgpu::BufferUsage::Uniform | usage;
  buffer_desc.mappedAtCreation = !!data;
  auto result = device.CreateBuffer(&buffer_desc);
  if (data) {
    std::memcpy(result.GetMappedRange(), data, sizeof(BufferType));
    result.Unmap();
  }

  return result;
}

}  // namespace renderer

#endif  //! RENDERER_UTILS_BUFFER_UTILS_H_
