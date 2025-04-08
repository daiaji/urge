// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_RESOURCE_RENDER_BUFFER_H_
#define RENDERER_RESOURCE_RENDER_BUFFER_H_

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/Buffer.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"

#include "renderer/renderer_config.h"
#include "renderer/vertex/vertex_layout.h"

namespace renderer {

class QuadIndexCache {
 public:
  ~QuadIndexCache() = default;

  QuadIndexCache(const QuadIndexCache&) = delete;
  QuadIndexCache& operator=(const QuadIndexCache&) = delete;

  // Make quad index(6) buffer cache
  static std::unique_ptr<QuadIndexCache> Make(
      Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device);

  // Allocate a new capacity index buffer for drawcall using,
  // |quadrangle_size| is the count not the byte size.
  void Allocate(uint32_t quadrangle_size);

  Diligent::RefCntAutoPtr<Diligent::IBuffer>& operator*() { return buffer_; }
  Diligent::VALUE_TYPE format() const { return format_; }

 private:
  QuadIndexCache(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device);

  Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device_;
  Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer_;
  Diligent::VALUE_TYPE format_;

  std::vector<uint16_t> cache_;
};

template <typename TargetType, Diligent::BIND_FLAGS BatchBind>
class BatchBuffer {
 public:
  ~BatchBuffer() = default;

  BatchBuffer(const BatchBuffer&) = delete;
  BatchBuffer& operator=(const BatchBuffer&) = delete;

  static std::unique_ptr<BatchBuffer> Make(
      Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device) {
    return std::unique_ptr<BatchBuffer>(new BatchBuffer(device));
  }

  Diligent::RefCntAutoPtr<Diligent::IBuffer>& operator*() { return buffer_; }

  bool QueueWrite(
      Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context,
      const TargetType* data,
      uint32_t count,
      Diligent::BUFFER_MODE buffer_mode = Diligent::BUFFER_MODE_STRUCTURED,
      Diligent::CPU_ACCESS_FLAGS cpu_access = Diligent::CPU_ACCESS_WRITE) {
    if (!buffer_ || buffer_->GetDesc().Size < sizeof(TargetType) * count) {
      size_t buffer_size = count * sizeof(TargetType);

      Diligent::BufferDesc buffer_desc;
      buffer_desc.Name = "generic.batch.buffer";
      buffer_desc.Usage = Diligent::USAGE_DYNAMIC;
      buffer_desc.BindFlags = BatchBind;
      buffer_desc.CPUAccessFlags = cpu_access;
      buffer_desc.Mode = buffer_mode;
      buffer_desc.ElementByteStride = sizeof(TargetType);

      Diligent::BufferData buffer_data;
      buffer_data.pData = data;
      buffer_data.DataSize = buffer_size;
      buffer_data.pContext = context;

      device_->CreateBuffer(buffer_desc, &buffer_data, &buffer_);

      return true;
    }

    if (data)
      context->UpdateBuffer(buffer_, 0, count * sizeof(TargetType),
                            reinterpret_cast<const uint8_t*>(data),
                            Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE);
    return false;
  }

 private:
  BatchBuffer(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device)
      : device_(device) {}

  Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device_;
  Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer_;
};

using QuadBatch = BatchBuffer<Quad, Diligent::BIND_FLAGS::BIND_VERTEX_BUFFER>;

}  // namespace renderer

#endif  //! RENDERER_RESOURCE_RENDER_BUFFER_H_
