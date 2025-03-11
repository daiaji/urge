// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/canvas/canvas_scheduler.h"

namespace content {

CanvasScheduler::~CanvasScheduler() = default;

std::unique_ptr<CanvasScheduler> CanvasScheduler::MakeInstance(
    renderer::RenderDevice* device,
    renderer::DeviceContext* context,
    renderer::QuadrangleIndexCache* index_cache,
    filesystem::IOService* io_service) {
  return std::unique_ptr<CanvasScheduler>(
      new CanvasScheduler(device, context, index_cache, io_service));
}

renderer::RenderDevice* CanvasScheduler::GetDevice() {
  return device_base_;
}

renderer::DeviceContext* CanvasScheduler::GetContext() {
  return immediate_context_;
}

filesystem::IOService* CanvasScheduler::GetIO() {
  return io_service_;
}

void CanvasScheduler::InitWithRenderWorker(base::ThreadWorker* worker) {
  render_worker_ = worker;

  // Make canvas drawing common transient vertex buffer
  common_vertex_buffer_controller_ =
      renderer::VertexBufferController<renderer::FullVertexLayout>::Make(
          device_base_, 4);
}

void CanvasScheduler::AttachChildCanvas(CanvasImpl* child) {
  children_.Append(child);
}

void CanvasScheduler::SubmitPendingPaintCommands() {
  for (auto it = children_.head(); it != children_.end(); it = it->next()) {
    // Submit all pending commands (except Blt, StretchBlt)
    it->value()->SubmitQueuedCommands();
  }
}

CanvasScheduler::CanvasScheduler(renderer::RenderDevice* device,
                                 renderer::DeviceContext* context,
                                 renderer::QuadrangleIndexCache* index_cache,
                                 filesystem::IOService* io_service)
    : device_base_(device),
      immediate_context_(context),
      render_worker_(nullptr),
      index_cache_(index_cache),
      io_service_(io_service) {}

}  // namespace content
