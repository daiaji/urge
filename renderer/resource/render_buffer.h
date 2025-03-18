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
  std::vector<uint16_t> cached_indices_;
  wgpu::Buffer index_buffer_;
  uint32_t count_;
};

class QuadBatch {
 public:
  ~QuadBatch() = default;

  QuadBatch(const QuadBatch&) = delete;
  QuadBatch& operator=(const QuadBatch&) = delete;

  static std::unique_ptr<QuadBatch> Make(const wgpu::Device& device,
                                         uint64_t initial_count = 0);

  wgpu::Buffer& operator*() { return buffer_; }
  void QueueWrite(const wgpu::CommandEncoder& encoder,
                  const Quad* data,
                  uint32_t count = 1,
                  uint32_t offset = 0);

 private:
  QuadBatch(const wgpu::Device& device, const wgpu::Buffer& vertex_buffer);

  wgpu::Device device_;
  wgpu::Buffer buffer_;
};

}  // namespace renderer

#endif  //! RENDERER_RESOURCE_RENDER_BUFFER_H_
