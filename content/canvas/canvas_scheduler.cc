// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/canvas/canvas_scheduler.h"

namespace content {

namespace {

void ReleaseGPUObjectInternal(
    std::unique_ptr<
        renderer::VertexBufferController<renderer::FullVertexLayout>>,
    base::SingleWorker* worker) {}

}  // namespace

CanvasScheduler::~CanvasScheduler() {
  if (render_worker_)
    render_worker_->SendTask(
        base::BindOnce(&ReleaseGPUObjectInternal,
                       std::move(common_vertex_buffer_controller_)));
}

std::unique_ptr<CanvasScheduler> CanvasScheduler::MakeInstance(
    renderer::RenderDevice* device,
    renderer::DeviceContext* context,
    renderer::QuadrangleIndexCache* index_cache) {
  return std::unique_ptr<CanvasScheduler>(
      new CanvasScheduler(device, context, index_cache));
}

renderer::RenderDevice* CanvasScheduler::GetDevice() {
  return device_base_;
}

renderer::DeviceContext* CanvasScheduler::GetContext() {
  return immediate_context_;
}

void CanvasScheduler::BindRenderWorker(base::SingleWorker* worker) {
  render_worker_ = worker;

  // Init common vertex buffer
  render_worker_->SendTask(
      base::BindOnce(&InitSchedulerInternal, base::Unretained(this)));
  render_worker_->WaitWorkerSynchronize();
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
                                 renderer::QuadrangleIndexCache* index_cache)
    : device_base_(device),
      immediate_context_(context),
      render_worker_(nullptr),
      index_cache_(index_cache) {}

void CanvasScheduler::InitSchedulerInternal(base::SingleWorker* worker) {
  common_vertex_buffer_controller_ =
      renderer::VertexBufferController<renderer::FullVertexLayout>::Make(
          device_base_, sizeof(renderer::FullVertexLayout) * 4);
}

}  // namespace content
