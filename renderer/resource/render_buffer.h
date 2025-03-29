// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_RESOURCE_RENDER_BUFFER_H_
#define RENDERER_RESOURCE_RENDER_BUFFER_H_

#include "renderer/renderer_config.h"
#include "renderer/vertex/vertex_layout.h"

namespace renderer {

class QuadIndexCache {
 public:
  ~QuadIndexCache() = default;

  QuadIndexCache(const QuadIndexCache&) = delete;
  QuadIndexCache& operator=(const QuadIndexCache&) = delete;

  // Make quad index(6) buffer cache
  static std::unique_ptr<QuadIndexCache> Make(const wgpu::Device& device);

  // Allocate a new capacity index buffer for drawcall using,
  // |quadrangle_size| is the count not the byte size.
  wgpu::Buffer* Allocate(uint32_t quadrangle_size);

  wgpu::Buffer& operator*() { return index_buffer_; }
  wgpu::IndexFormat format() const { return format_; }

 private:
  QuadIndexCache(const wgpu::Device& device);

  wgpu::Device device_;
  wgpu::IndexFormat format_;
  wgpu::Buffer index_buffer_;
  std::vector<uint16_t> cache_;
};

template <typename TargetType, wgpu::BufferUsage BatchUsage>
class BatchBuffer {
 public:
  ~BatchBuffer() = default;

  BatchBuffer(const BatchBuffer&) = delete;
  BatchBuffer& operator=(const BatchBuffer&) = delete;

  static std::unique_ptr<BatchBuffer> Make(const wgpu::Device& device,
                                           uint64_t initial_count = 0) {
    wgpu::Buffer result_buffer;
    if (initial_count) {
      wgpu::BufferDescriptor buffer_desc;
      buffer_desc.label = "generic.batch.buffer";
      buffer_desc.usage = BatchUsage | wgpu::BufferUsage::CopyDst;
      buffer_desc.size = initial_count * sizeof(TargetType);
      buffer_desc.mappedAtCreation = false;
      result_buffer = device.CreateBuffer(&buffer_desc);
    }

    return std::unique_ptr<BatchBuffer>(new BatchBuffer(device, result_buffer));
  }

  wgpu::Buffer& operator*() { return buffer_; }

  bool QueueWrite(const wgpu::CommandEncoder& encoder,
                  const TargetType* data,
                  uint32_t count = 1) {
    if (!buffer_ || buffer_.GetSize() < sizeof(TargetType) * count) {
      wgpu::BufferDescriptor buffer_desc;
      buffer_desc.label = "generic.batch.buffer";
      buffer_desc.usage = BatchUsage | wgpu::BufferUsage::CopyDst;
      buffer_desc.size = count * sizeof(TargetType);
      buffer_desc.mappedAtCreation = true;
      buffer_ = device_.CreateBuffer(&buffer_desc);
      size_t buffer_size = count * sizeof(TargetType);
      std::memcpy(buffer_.GetMappedRange(0, buffer_size), data, buffer_size);
      buffer_.Unmap();
      return true;
    }

    encoder.WriteBuffer(buffer_, 0, reinterpret_cast<const uint8_t*>(data),
                        count * sizeof(TargetType));
    return false;
  }

 private:
  BatchBuffer(const wgpu::Device& device, const wgpu::Buffer& buffer)
      : device_(device), buffer_(buffer) {}

  wgpu::Device device_;
  wgpu::Buffer buffer_;
};

using QuadBatch = BatchBuffer<Quad, wgpu::BufferUsage::Vertex>;

}  // namespace renderer

#endif  //! RENDERER_RESOURCE_RENDER_BUFFER_H_
