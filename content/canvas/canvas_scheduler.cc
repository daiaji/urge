// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/canvas/canvas_scheduler.h"

namespace content {

CanvasScheduler::~CanvasScheduler() = default;

std::unique_ptr<CanvasScheduler> CanvasScheduler::MakeInstance(
    renderer::RenderDevice* device,
    filesystem::IOService* io_service) {
  return std::unique_ptr<CanvasScheduler>(
      new CanvasScheduler(device, io_service));
}

renderer::RenderDevice* CanvasScheduler::GetDevice() {
  return device_;
}

filesystem::IOService* CanvasScheduler::GetIO() {
  return io_service_;
}

void CanvasScheduler::InitWithRenderWorker(base::ThreadWorker* worker) {
  render_worker_ = worker;

  // Make canvas drawing common transient vertex buffer
  common_quad_batch_ = renderer::QuadBatch::Make(**device_);
}

void CanvasScheduler::SubmitPendingPaintCommands() {
  for (auto it = children_.head(); it != children_.end(); it = it->next()) {
    // Submit all pending commands (except Blt, StretchBlt)
    it->value()->SubmitQueuedCommands();
  }
}

CanvasScheduler::CanvasScheduler(renderer::RenderDevice* device,
                                 filesystem::IOService* io_service)
    : device_(device), render_worker_(nullptr), io_service_(io_service) {}

}  // namespace content
