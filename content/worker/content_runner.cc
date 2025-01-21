// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/worker/content_runner.h"

#include "content/context/execution_context.h"

namespace content {

ContentRunner::ContentRunner(std::unique_ptr<ContentProfile> profile,
                             std::unique_ptr<base::ThreadWorker> render_worker,
                             std::unique_ptr<EngineBindingBase> binding,
                             base::WeakPtr<ui::Widget> window)
    : profile_(std::move(profile)),
      render_worker_(std::move(render_worker)),
      window_(window),
      binding_(std::move(binding)) {}

void ContentRunner::InitializeContentInternal() {
  // Initialize CC
  cc_.reset(new CoroutineContext);
  cc_->primary_fiber = fiber_create(nullptr, 0, nullptr, nullptr);
  cc_->main_loop_fiber =
      fiber_create(cc_->primary_fiber, 0, EngineEntryFunctionInternal, this);

  // Graphics initialize settings
  int frame_rate = 40;
  if (profile_->api_version >= ContentProfile::APIVersion::kRGSS2)
    frame_rate = 60;

  base::Vec2i resolution(640, 480);
  if (profile_->api_version >= ContentProfile::APIVersion::kRGSS2)
    resolution = base::Vec2i(544, 416);

  graphics_impl_.reset(new RenderScreenImpl(cc_.get(), resolution, frame_rate));

  // Init all module workers
  graphics_impl_->InitWithRenderWorker(render_worker_.get(), window_);
}

ContentRunner::~ContentRunner() {}

bool ContentRunner::RunMainLoop() {
  return graphics_impl_->ExecuteEventMainLoop();
}

std::unique_ptr<ContentRunner> ContentRunner::Create(InitParams params) {
  std::unique_ptr<base::ThreadWorker> render_worker =
      base::ThreadWorker::Create();

  auto* runner =
      new ContentRunner(std::move(params.profile), std::move(render_worker),
                        std::move(params.entry), params.window);
  runner->InitializeContentInternal();

  return std::unique_ptr<ContentRunner>(runner);
}

void ContentRunner::EngineEntryFunctionInternal(fiber_t* fiber) {
  auto* self = static_cast<ContentRunner*>(fiber->userdata);

  // Before running loop handler
  self->binding_->PreEarlyInitialization(self->profile_.get());

  // Make script binding execution context
  // Call binding boot handler before running loop handler
  auto ec = ExecutionContext::MakeContext();
  ec->font_context = nullptr;
  ec->canvas_scheduler = self->graphics_impl_->GetCanvasScheduler();
  ec->graphics = self->graphics_impl_.get();

  self->binding_->OnMainMessageLoopRun(ec.get());

  // End of running
  self->binding_->PostMainLoopRunning();
}

}  // namespace content
