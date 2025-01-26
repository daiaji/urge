// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/resource/render_buffer.h"

#include <array>

namespace renderer {

// Primitive draw order:
//   Data: 6 index to 4 vertex
//   View:
//  0 +----+ 1
//    | \  |
//    |  \ |
//    |   \|
//  3 +----+ 2
// order: lt -> rt -> rb -> lb
static std::array<uint16_t, 6> kQuadrangleDrawIndices = {
    0, 1, 2, 2, 3, 0,
};

QuadrangleIndexCache::QuadrangleIndexCache(const wgpu::Device& device)
    : device_(device), format_(wgpu::IndexFormat::Uint16), count_(0) {}

std::unique_ptr<QuadrangleIndexCache> renderer::QuadrangleIndexCache::Make(
    RenderDevice* device) {
  return std::unique_ptr<QuadrangleIndexCache>(
      new QuadrangleIndexCache(**device));
}

wgpu::Buffer* QuadrangleIndexCache::Allocate(uint32_t quadrangle_size) {
  uint32_t required_indices_size =
      quadrangle_size * kQuadrangleDrawIndices.size();

  // Generate
  if (cached_indices_.size() < required_indices_size) {
    if (cached_indices_.capacity() <
        cached_indices_.size() + required_indices_size)
      cached_indices_.reserve(required_indices_size);
    for (uint32_t i = 0; i < quadrangle_size; ++i)
      for (const auto& it : kQuadrangleDrawIndices)
        cached_indices_.push_back((count_ + i) * kQuadrangleDrawIndices.size() +
                                  it);

    // Reset current count and buffer
    count_ = quadrangle_size;
    index_buffer_ = nullptr;
  }

  // Upload to GPU
  if (!index_buffer_) {
    // Allocate bigger buffer
    wgpu::BufferDescriptor buffer_desc;
    buffer_desc.label = "IndexBuffer.Immutable.Quadrangle";
    buffer_desc.usage = wgpu::BufferUsage::Index;
    buffer_desc.mappedAtCreation = true;
    buffer_desc.size = required_indices_size *
                       sizeof(decltype(kQuadrangleDrawIndices)::value_type);
    index_buffer_ = device_.CreateBuffer(&buffer_desc);

    // Re-Write indices data to buffer
    void* dest_memory = index_buffer_.GetMappedRange();
    DCHECK(dest_memory);
    std::memcpy(dest_memory, cached_indices_.data(), buffer_desc.size);
    index_buffer_.Unmap();
  }

  return &index_buffer_;
}

}  // namespace renderer
