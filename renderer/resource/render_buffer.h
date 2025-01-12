// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_RESOURCE_RENDER_BUFFER_H_
#define RENDERER_RESOURCE_RENDER_BUFFER_H_

#include "renderer/device/render_device.h"
#include "renderer/renderer_config.h"
#include "renderer/vertex/vertex_layout.h"

namespace renderer {

class QuadrangleIndexCache {
 public:
  ~QuadrangleIndexCache() = default;

  QuadrangleIndexCache(const QuadrangleIndexCache&) = delete;
  QuadrangleIndexCache& operator=(const QuadrangleIndexCache&) = delete;

  // Make quad index(6) buffer cache
  static std::unique_ptr<QuadrangleIndexCache> Make(RenderDevice* device);

  // Allocate a new capacity index buffer for drawcall using,
  // |quadrangle_size| is the count not the byte size.
  wgpu::Buffer* Allocate(uint32_t quadrangle_size);

  wgpu::Buffer& operator*() { return index_buffer_; }

 private:
  QuadrangleIndexCache(const wgpu::Device& device);

  wgpu::Device device_;
  std::vector<uint16_t> cached_indices_;
  wgpu::Buffer index_buffer_;
  uint32_t count_;
};

template <typename VertexType>
class VertexBufferController {
 public:
  ~VertexBufferController() = default;

  VertexBufferController(const VertexBufferController&) = delete;
  VertexBufferController& operator=(const VertexBufferController&) = delete;

  // Make new buffer instance
  static std::unique_ptr<VertexBufferController> Make(
      RenderDevice* device,
      uint64_t initial_size = 0);

  // Manage current vertex buffer.
  void QueueWrite(const wgpu::CommandEncoder& encoder,
                  const VertexType* data,
                  uint32_t size,
                  uint32_t offset = 0);

  wgpu::Buffer& operator*() { return vertex_buffer_; }

 private:
  VertexBufferController(const wgpu::Device& device,
                         const wgpu::Buffer& vertex_buffer);

  wgpu::Device device_;
  wgpu::Buffer vertex_buffer_;
};

template <typename VertexType>
inline VertexBufferController<VertexType>::VertexBufferController(
    const wgpu::Device& device,
    const wgpu::Buffer& vertex_buffer)
    : device_(device), vertex_buffer_(vertex_buffer) {}

template <typename VertexType>
inline std::unique_ptr<VertexBufferController<VertexType>>
VertexBufferController<VertexType>::Make(RenderDevice* device,
                                         uint64_t initial_size) {
  wgpu::Buffer result_buffer;
  if (initial_size) {
    wgpu::BufferDescriptor buffer_desc;
    buffer_desc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    buffer_desc.size = initial_size * sizeof(VertexType);
    result_buffer = device->GetDevice()->CreateBuffer(&buffer_desc);
  }

  return std::unique_ptr<VertexBufferController<VertexType>>(
      new VertexBufferController<VertexType>(*device->GetDevice(),
                                             result_buffer));
}

template <typename VertexType>
inline void VertexBufferController<VertexType>::QueueWrite(
    const wgpu::CommandEncoder& encoder,
    const VertexType* data,
    uint32_t size,
    uint32_t offset) {
  if (!vertex_buffer_ ||
      vertex_buffer_.GetSize() < sizeof(VertexType) * (offset + size)) {
    wgpu::BufferDescriptor buffer_desc;
    buffer_desc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    buffer_desc.size = size * sizeof(VertexType);
    vertex_buffer_ = device_.CreateBuffer(&buffer_desc);
  }

  encoder.WriteBuffer(vertex_buffer_, offset * sizeof(VertexType),
                      reinterpret_cast<const uint8_t*>(data),
                      size * sizeof(VertexType));
}

}  // namespace renderer

#endif  //! RENDERER_RESOURCE_RENDER_BUFFER_H_
