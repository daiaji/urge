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

QuadIndexCache::QuadIndexCache(const wgpu::Device& device)
    : device_(device), format_(wgpu::IndexFormat::Uint16), count_(0) {}

std::unique_ptr<QuadIndexCache> renderer::QuadIndexCache::Make(
    const wgpu::Device& device) {
  return std::unique_ptr<QuadIndexCache>(new QuadIndexCache(device));
}

wgpu::Buffer* QuadIndexCache::Allocate(uint32_t quadrangle_size) {
  uint32_t required_indices_size =
      quadrangle_size * kQuadrangleDrawIndices.size();

  // Generate
  if (cached_indices_.size() < required_indices_size) {
    if (cached_indices_.capacity() <
        cached_indices_.size() + required_indices_size)
      cached_indices_.reserve(required_indices_size);
    for (uint32_t i = 0; i < quadrangle_size; ++i)
      for (const auto& it : kQuadrangleDrawIndices)
        cached_indices_.push_back((count_ + i) * 4 + it);

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
    std::memcpy(dest_memory, cached_indices_.data(), buffer_desc.size);
    index_buffer_.Unmap();
  }

  return &index_buffer_;
}

QuadBatch::QuadBatch(const wgpu::Device& device,
                     const wgpu::Buffer& vertex_buffer)
    : device_(device), buffer_(vertex_buffer) {}

std::unique_ptr<QuadBatch> QuadBatch::Make(const wgpu::Device& device,
                                           uint64_t initial_count) {
  wgpu::Buffer result_buffer;
  if (initial_count) {
    wgpu::BufferDescriptor buffer_desc;
    buffer_desc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    buffer_desc.size = initial_count * sizeof(Quad);
    result_buffer = device.CreateBuffer(&buffer_desc);
  }

  return std::unique_ptr<QuadBatch>(new QuadBatch(device, result_buffer));
}

void QuadBatch::QueueWrite(const wgpu::CommandEncoder& encoder,
                           const Quad* data,
                           uint32_t count,
                           uint32_t offset) {
  if (!buffer_ || buffer_.GetSize() < sizeof(Quad) * (offset + count)) {
    wgpu::BufferDescriptor buffer_desc;
    buffer_desc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    buffer_desc.size = (offset + count) * sizeof(Quad);
    buffer_ = device_.CreateBuffer(&buffer_desc);
  }

  encoder.WriteBuffer(buffer_, offset * sizeof(Quad),
                      reinterpret_cast<const uint8_t*>(data),
                      count * sizeof(Quad));
}

}  // namespace renderer
