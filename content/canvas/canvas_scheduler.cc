// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/canvas/canvas_scheduler.h"

namespace content {

CanvasScheduler::~CanvasScheduler() = default;

std::unique_ptr<CanvasScheduler> CanvasScheduler::MakeInstance(
    base::ThreadWorker* worker,
    renderer::RenderDevice* device,
    filesystem::IOService* io_service) {
  return std::unique_ptr<CanvasScheduler>(
      new CanvasScheduler(worker, device, io_service));
}

renderer::RenderDevice* CanvasScheduler::GetDevice() const {
  return device_;
}

filesystem::IOService* CanvasScheduler::GetIOService() const {
  return io_service_;
}

void CanvasScheduler::SubmitPendingPaintCommands() {
  for (auto it = children_.head(); it != children_.end(); it = it->next()) {
    // Submit all pending commands (except Blt, StretchBlt)
    it->value()->SubmitQueuedCommands();
  }
}

CanvasScheduler::CanvasScheduler(base::ThreadWorker* worker,
                                 renderer::RenderDevice* device,
                                 filesystem::IOService* io_service)
    : device_(device),
      render_worker_(worker),
      io_service_(io_service),
      generic_base_binding_(
          device->GetPipelines()->base.CreateBinding<renderer::Binding_Base>()),
      generic_color_binding_(
          device->GetPipelines()
              ->color.CreateBinding<renderer::Binding_Color>()),
      common_quad_batch_(renderer::QuadBatch::Make(**device)) {}

}  // namespace content
