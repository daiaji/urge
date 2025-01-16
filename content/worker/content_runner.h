// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_WORKER_CONTENT_RUNNER_H_
#define CONTENT_WORKER_CONTENT_RUNNER_H_

#include "base/worker/isolate_scheduler.h"
#include "base/worker/single_worker.h"
#include "content/profile/content_profile.h"
#include "content/screen/renderscreen_impl.h"
#include "content/worker/engine_binding.h"
#include "ui/widget/widget.h"

namespace content {

class ContentRunner {
 public:
  struct InitParams {
    std::unique_ptr<ContentProfile> profile;

    std::unique_ptr<EngineBindingBase> entry;

    base::WeakPtr<ui::Widget> window;
  };

  ~ContentRunner();

  ContentRunner(const ContentRunner&) = delete;
  ContentRunner& operator=(const ContentRunner&) = delete;

  static std::unique_ptr<ContentRunner> Create(InitParams params);

  bool RunMainLoop();

 private:
  ContentRunner(std::unique_ptr<ContentProfile> profile,
                std::unique_ptr<base::WorkerScheduler> scheduler,
                base::SingleWorker* engine_worker,
                base::SingleWorker* render_worker,
                std::unique_ptr<EngineBindingBase> binding,
                base::WeakPtr<ui::Widget> window);
  void RunEngineRenderLoopInternal();
  void RunEventLoopInternal();

  std::unique_ptr<ContentProfile> profile_;
  std::unique_ptr<base::WorkerScheduler> scheduler_;
  base::SingleWorker* engine_worker_;
  base::SingleWorker* render_worker_;

  std::atomic<int32_t> exit_code_;
  base::WeakPtr<ui::Widget> window_;

  std::unique_ptr<EngineBindingBase> binding_;
  std::unique_ptr<ExecutionContext> execution_context_;
  std::unique_ptr<RenderScreenImpl> graphics_impl_;
};

}  // namespace content

#endif  //! CONTENT_WORKER_CONTENT_RUNNER_H_
