// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CANVAS_CANVAS_SCHEDULER_H_
#define CONTENT_CANVAS_CANVAS_SCHEDULER_H_

#include "base/worker/thread_worker.h"
#include "content/canvas/canvas_impl.h"
#include "renderer/context/device_context.h"
#include "renderer/device/render_device.h"
#include "renderer/resource/render_buffer.h"

namespace content {

class CanvasScheduler {
 public:
  ~CanvasScheduler();

  CanvasScheduler(const CanvasScheduler&) = delete;
  CanvasScheduler& operator=(const CanvasScheduler&) = delete;

  static std::unique_ptr<CanvasScheduler> MakeInstance(
      renderer::RenderDevice* device,
      renderer::DeviceContext* context,
      renderer::QuadrangleIndexCache* index_cache);

  renderer::RenderDevice* GetDevice();
  renderer::DeviceContext* GetContext();

  // Bind a worker for current scheduler,
  // all bitmap/canvas draw command will be encoded on this worker.
  // If worker set to null, it will be executed immediately on caller thread.
  void BindRenderWorker(base::ThreadWorker* worker);

  // Bind child canvas in linked node,
  // scheduler will auto clear pending commands in canvas queue.
  void AttachChildCanvas(CanvasImpl* child);

  // Sync all pending command to device queue,
  // clear children canvas command queue.
  void SubmitPendingPaintCommands();

  base::ThreadWorker* render_worker() { return render_worker_; }
  renderer::QuadrangleIndexCache* index_cache() { return index_cache_; }
  renderer::VertexBufferController<renderer::FullVertexLayout>*
  vertex_buffer() {
    return common_vertex_buffer_controller_.get();
  }

 private:
  friend class CanvasImpl;
  CanvasScheduler(renderer::RenderDevice* device,
                  renderer::DeviceContext* context,
                  renderer::QuadrangleIndexCache* index_cache);
  void InitSchedulerInternal();

  base::LinkedList<CanvasImpl> children_;

  renderer::RenderDevice* device_base_;
  renderer::DeviceContext* immediate_context_;
  base::ThreadWorker* render_worker_;

  renderer::QuadrangleIndexCache* index_cache_;
  std::unique_ptr<renderer::VertexBufferController<renderer::FullVertexLayout>>
      common_vertex_buffer_controller_;
};

}  // namespace content

#endif  //! CONTENT_CANVAS_CANVAS_SCHEDULER_H_
