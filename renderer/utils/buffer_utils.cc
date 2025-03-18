// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/utils/buffer_utils.h"

namespace renderer {

wgpu::Buffer CreateQuadBuffer(const wgpu::Device& device,
                              const std::string_view& label,
                              wgpu::BufferUsage usage,
                              size_t count,
                              Quad* data) {
  wgpu::BufferDescriptor buffer_desc;
  buffer_desc.label = label;
  buffer_desc.size = sizeof(Quad) * count;
  buffer_desc.usage = wgpu::BufferUsage::Vertex | usage;
  buffer_desc.mappedAtCreation = !!data;
  auto result = device.CreateBuffer(&buffer_desc);
  if (data) {
    std::memcpy(result.GetMappedRange(), data, buffer_desc.size);
    result.Unmap();
  }

  return result;
}

}  // namespace renderer
