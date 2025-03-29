// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_RENDER_RENDER_PASS_H_
#define RENDERER_RENDER_RENDER_PASS_H_

#include "renderer/device/render_device.h"

namespace renderer {

class RenderPass {
 public:
  RenderPass();
  RenderPass(const wgpu::RenderPassEncoder& encoder);

  RenderPass(const RenderPass& other);
  RenderPass& operator=(const wgpu::RenderPassEncoder& encoder);

  inline void BeginOcclusionQuery(uint32_t queryIndex) const {
    encoder_.BeginOcclusionQuery(queryIndex);
  }

  inline void Draw(uint32_t vertexCount,
                   uint32_t instanceCount = 1,
                   uint32_t firstVertex = 0,
                   uint32_t firstInstance = 0) const {
    encoder_.Draw(vertexCount, instanceCount, firstVertex, firstInstance);
  }

  inline void DrawIndexed(uint32_t indexCount,
                          uint32_t instanceCount = 1,
                          uint32_t firstIndex = 0,
                          int32_t baseVertex = 0,
                          uint32_t firstInstance = 0) const {
    encoder_.DrawIndexed(indexCount, instanceCount, firstIndex, baseVertex,
                         firstInstance);
  }

  inline void DrawIndexedIndirect(wgpu::Buffer const& indirectBuffer,
                                  uint64_t indirectOffset) const {
    encoder_.DrawIndexedIndirect(indirectBuffer, indirectOffset);
  }

  inline void DrawIndirect(wgpu::Buffer const& indirectBuffer,
                           uint64_t indirectOffset) const {
    encoder_.DrawIndirect(indirectBuffer, indirectOffset);
  }

  inline void End() const { encoder_.End(); }

  inline void EndOcclusionQuery() const { encoder_.EndOcclusionQuery(); }

  inline void ExecuteBundles(size_t bundleCount,
                             wgpu::RenderBundle const* bundles) const {
    encoder_.ExecuteBundles(bundleCount, bundles);
  }

  inline void InsertDebugMarker(wgpu::StringView markerLabel) const {
    encoder_.InsertDebugMarker(markerLabel);
  }

  inline void MultiDrawIndexedIndirect(
      wgpu::Buffer const& indirectBuffer,
      uint64_t indirectOffset,
      uint32_t maxDrawCount,
      wgpu::Buffer const& drawCountBuffer = nullptr,
      uint64_t drawCountBufferOffset = 0) const {
    encoder_.MultiDrawIndexedIndirect(indirectBuffer, indirectOffset,
                                      maxDrawCount, drawCountBuffer,
                                      drawCountBufferOffset);
  }

  inline void MultiDrawIndirect(wgpu::Buffer const& indirectBuffer,
                                uint64_t indirectOffset,
                                uint32_t maxDrawCount,
                                wgpu::Buffer const& drawCountBuffer = nullptr,
                                uint64_t drawCountBufferOffset = 0) const {
    encoder_.MultiDrawIndirect(indirectBuffer, indirectOffset, maxDrawCount,
                               drawCountBuffer, drawCountBufferOffset);
  }

  inline void PixelLocalStorageBarrier() const {
    encoder_.PixelLocalStorageBarrier();
  }

  inline void PopDebugGroup() const { encoder_.PopDebugGroup(); }

  inline void PushDebugGroup(wgpu::StringView groupLabel) const {
    encoder_.PushDebugGroup(groupLabel);
  }

  inline void SetBindGroup(uint32_t groupIndex,
                           wgpu::BindGroup const& group = nullptr,
                           size_t dynamicOffsetCount = 0,
                           uint32_t const* dynamicOffsets = nullptr) {
    if (bindgroup_cache_[groupIndex].Get() != group.Get()) {
      bindgroup_cache_[groupIndex] = group;
      encoder_.SetBindGroup(groupIndex, group, dynamicOffsetCount,
                            dynamicOffsets);
    }
  }

  inline void SetBlendConstant(wgpu::Color const* color) const {
    encoder_.SetBlendConstant(color);
  }

  inline void SetImmediateData(uint32_t offset,
                               void const* data,
                               size_t size) const {
    encoder_.SetImmediateData(offset, data, size);
  }

  inline void SetIndexBuffer(wgpu::Buffer const& buffer,
                             wgpu::IndexFormat format,
                             uint64_t offset = 0,
                             uint64_t size = wgpu::kWholeSize) const {
    encoder_.SetIndexBuffer(buffer, format, offset, size);
  }

  inline void SetLabel(wgpu::StringView label) const {
    encoder_.SetLabel(label);
  }

  inline void SetPipeline(wgpu::RenderPipeline const& pipeline) const {
    encoder_.SetPipeline(pipeline);
  }

  inline void SetScissorRect(uint32_t x,
                             uint32_t y,
                             uint32_t width,
                             uint32_t height) const {
    encoder_.SetScissorRect(x, y, width, height);
  }

  inline void SetStencilReference(uint32_t reference) const {
    encoder_.SetStencilReference(reference);
  }

  inline void SetVertexBuffer(uint32_t slot,
                              wgpu::Buffer const& buffer = nullptr,
                              uint64_t offset = 0,
                              uint64_t size = wgpu::kWholeSize) const {
    encoder_.SetVertexBuffer(slot, buffer, offset, size);
  }

  inline void SetViewport(float x,
                          float y,
                          float width,
                          float height,
                          float minDepth,
                          float maxDepth) const {
    encoder_.SetViewport(x, y, width, height, minDepth, maxDepth);
  }

  inline void WriteTimestamp(wgpu::QuerySet const& querySet,
                             uint32_t queryIndex) const {
    encoder_.WriteTimestamp(querySet, queryIndex);
  }

 private:
  wgpu::RenderPassEncoder encoder_;

  std::array<wgpu::BindGroup, 4> bindgroup_cache_;
};

}  // namespace renderer

#endif  //! RENDERER_RENDER_RENDER_PASS_H_
