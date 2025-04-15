// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_RESOURCE_RENDER_BUFFER_H_
#define RENDERER_RESOURCE_RENDER_BUFFER_H_

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/Buffer.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"

#include "renderer/layout/vertex_layout.h"
#include "renderer/renderer_config.h"

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

template <typename TargetType,
          Diligent::BIND_FLAGS BatchBind,
          Diligent::BUFFER_MODE BufferMode = Diligent::BUFFER_MODE_UNDEFINED,
          Diligent::CPU_ACCESS_FLAGS CPUAccess = Diligent::CPU_ACCESS_WRITE>
class BatchBuffer {
 public:
  ~BatchBuffer() = default;

  BatchBuffer(const BatchBuffer&) = delete;
  BatchBuffer& operator=(const BatchBuffer&) = delete;

  static std::unique_ptr<BatchBuffer> Make(Diligent::IRenderDevice* device,
                                           uint32_t initial_count = 0) {
    Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer;

    if (initial_count > 0) {
      Diligent::BufferDesc buffer_desc;
      buffer_desc.Name = "generic.batch.buffer";
      buffer_desc.Size = initial_count * sizeof(TargetType);
      buffer_desc.BindFlags = BatchBind;
      buffer_desc.Usage = Diligent::USAGE_DYNAMIC;
      buffer_desc.CPUAccessFlags = CPUAccess;
      buffer_desc.Mode = BufferMode;
      buffer_desc.ElementByteStride = sizeof(TargetType);

      device->CreateBuffer(buffer_desc, nullptr, &buffer);
    }

    return std::unique_ptr<BatchBuffer>(new BatchBuffer(device, buffer));
  }

  Diligent::IBuffer* operator*() { return buffer_; }

  bool QueueWrite(Diligent::IDeviceContext* context,
                  const TargetType* data,
                  uint32_t count = 1) {
    if (!buffer_ || buffer_->GetDesc().Size < sizeof(TargetType) * count) {
      size_t buffer_size = count * sizeof(TargetType);

      Diligent::BufferDesc buffer_desc;
      buffer_desc.Name = "generic.batch.buffer";
      buffer_desc.Size = buffer_size;
      buffer_desc.BindFlags = BatchBind;
      buffer_desc.Usage = Diligent::USAGE_DYNAMIC;
      buffer_desc.CPUAccessFlags = CPUAccess;
      buffer_desc.Mode = BufferMode;
      buffer_desc.ElementByteStride = sizeof(TargetType);

      device_->CreateBuffer(buffer_desc, nullptr, &buffer_);
    }

    if (data)
      context->UpdateBuffer(buffer_, 0, count * sizeof(TargetType),
                            reinterpret_cast<const uint8_t*>(data),
                            Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE);

    return false;
  }

 private:
  BatchBuffer(Diligent::IRenderDevice* device,
              Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer)
      : device_(device), buffer_(buffer) {}

  Diligent::IRenderDevice* device_;
  Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer_;
};

using QuadBatch = BatchBuffer<Quad,
                              Diligent::BIND_VERTEX_BUFFER,
                              Diligent::BUFFER_MODE_UNDEFINED,
                              Diligent::CPU_ACCESS_WRITE>;

}  // namespace renderer

#endif  //! RENDERER_RESOURCE_RENDER_BUFFER_H_
