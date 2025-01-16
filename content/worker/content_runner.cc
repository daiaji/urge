// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/worker/content_runner.h"

#include "content/context/execution_context.h"

namespace content {

ContentRunner::ContentRunner(std::unique_ptr<ContentProfile> profile,
                             std::unique_ptr<base::WorkerScheduler> scheduler,
                             base::SingleWorker* engine_worker,
                             base::SingleWorker* render_worker,
                             std::unique_ptr<EngineBindingBase> binding,
                             base::WeakPtr<ui::Widget> window)
    : profile_(std::move(profile)),
      scheduler_(std::move(scheduler)),
      engine_worker_(engine_worker),
      render_worker_(render_worker),
      exit_code_(1),
      window_(window),
      binding_(std::move(binding)) {}

void ContentRunner::RunEngineRenderLoopInternal() {
  // Graphics initialize settings
  int frame_rate = 40;
  if (profile_->api_version >= ContentProfile::APIVersion::kRGSS2)
    frame_rate = 60;

  base::Vec2i resolution(640, 480);
  if (profile_->api_version >= ContentProfile::APIVersion::kRGSS2)
    resolution = base::Vec2i(544, 416);

  graphics_impl_.reset(
      new RenderScreenImpl(engine_worker_, resolution, frame_rate));

  // Init all module workers
  graphics_impl_->InitWithRenderWorker(render_worker_, window_);

  // Before running loop handler
  binding_->PreEarlyInitialization(profile_.get());

  // Make script binding execution context
  // Call binding boot handler before running loop handler
  execution_context_ = ExecutionContext::MakeContext();
  binding_->OnMainMessageLoopRun(execution_context_.get());
}

void ContentRunner::RunEventLoopInternal() {
  // Call event process
  if (graphics_impl_->ExecuteEventMainLoop()) {
    // Execute event loop
    engine_worker_->PostTask(base::BindOnce(
        &ContentRunner::RunEventLoopInternal, base::Unretained(this)));
  } else {
    // Set exit request
    exit_code_.store(0);

    // Call binding release handler
    binding_->PostMainLoopRunning();
  }
}

ContentRunner::~ContentRunner() {}

bool ContentRunner::RunMainLoop() {
  scheduler_->Flush();
  return exit_code_.load();
}

std::unique_ptr<ContentRunner> ContentRunner::Create(InitParams params) {
  std::unique_ptr<base::SingleWorker> engine_worker =
      base::SingleWorker::CreateWorker(base::WorkerScheduleMode::kCoroutine);
  std::unique_ptr<base::SingleWorker> render_worker =
      base::SingleWorker::CreateWorker(base::WorkerScheduleMode::kAsync);

  std::unique_ptr<base::WorkerScheduler> scheduler =
      base::WorkerScheduler::Create();

  auto* engine_worker_raw = engine_worker.get();
  auto* render_worker_raw = render_worker.get();

  scheduler->AddChildWorker(std::move(engine_worker));
  scheduler->AddChildWorker(std::move(render_worker));

  auto* runner = new ContentRunner(
      std::move(params.profile), std::move(scheduler), engine_worker_raw,
      render_worker_raw, std::move(params.entry), params.window);

  engine_worker_raw->PostTask(base::BindOnce(
      &ContentRunner::RunEngineRenderLoopInternal, base::Unretained(runner)));

  return std::unique_ptr<ContentRunner>(runner);
}

}  // namespace content
