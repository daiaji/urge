// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/worker/content_runner.h"

#include "content/canvas/font_impl.h"
#include "content/context/execution_context.h"

namespace content {

ContentRunner::ContentRunner(std::unique_ptr<ContentProfile> profile,
                             std::unique_ptr<base::ThreadWorker> render_worker,
                             std::unique_ptr<EngineBindingBase> binding,
                             base::WeakPtr<ui::Widget> window)
    : profile_(std::move(profile)),
      render_worker_(std::move(render_worker)),
      window_(window),
      exit_code_(0),
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

  io_service_ = filesystem::IO::Create();
  scoped_font_.reset(
      new ScopedFontData(io_service_.get(), profile_->default_font_path));
  graphics_impl_.reset(new RenderScreenImpl(cc_.get(), scoped_font_.get(),
                                            resolution, frame_rate));

  // Init all module workers
  graphics_impl_->InitWithRenderWorker(render_worker_.get(), window_);

  // Reset exit code
  exit_code_.store(1);
}

ContentRunner::~ContentRunner() = default;

bool ContentRunner::RunMainLoop() {
  if (!graphics_impl_->ExecuteEventMainLoop())
    binding_->ExitSignalRequired();

  return exit_code_.load();
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
  ExecutionContext ec;
  ec.font_context = self->scoped_font_.get();
  ec.canvas_scheduler = self->graphics_impl_->GetCanvasScheduler();
  ec.graphics = self->graphics_impl_.get();

  // Execute main loop
  self->binding_->OnMainMessageLoopRun(&ec);

  // End of running
  self->binding_->PostMainLoopRunning();
  self->exit_code_.store(0);

  // To primary fiber
  fiber_switch(self->cc_->primary_fiber);
}

}  // namespace content
