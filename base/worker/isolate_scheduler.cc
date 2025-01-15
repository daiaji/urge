// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/worker/isolate_scheduler.h"

namespace base {

WorkerScheduler::WorkerScheduler(fiber_t* primary)
    : primary_coroutine_(primary), quit_flag_(false) {}

WorkerScheduler::~WorkerScheduler() {}

std::unique_ptr<WorkerScheduler> WorkerScheduler::Create() {
  return std::unique_ptr<WorkerScheduler>(
      new WorkerScheduler(fiber_create(nullptr, 0, nullptr, nullptr)));
}

void WorkerScheduler::AddChildWorker(std::unique_ptr<SingleWorker> child) {
  // Set owned
  child->scheduler_ = this;

  // Insert to managed list
  if (child->GetSchedulerMode() == WorkerScheduleMode::kCoroutine) {
    child->sequence_thread_ = std::this_thread::get_id();
    coroutine_workers_.emplace(
        std::move(child),
        fiber_create(primary_coroutine_, 0, FiberRunnerInternal, child.get()));
  } else if (child->GetSchedulerMode() == WorkerScheduleMode::kAsync) {
    std::thread runner(&WorkerScheduler::ThreadRunnerInternal, this,
                       child.get());
    child->sequence_thread_ = runner.get_id();
    async_workers_.emplace(std::move(child), std::move(runner));
  }
}

void WorkerScheduler::Flush() {
  for (auto& it : coroutine_workers_)
    fiber_switch(it.second);
}

void WorkerScheduler::FiberRunnerInternal(fiber_t* coroutine) {
  auto* worker = static_cast<SingleWorker*>(coroutine->userdata);
  while (!worker->scheduler_->quit_flag_) {
    worker->FlushInternal();

    // Return to main fiber
    fiber_switch(worker->scheduler_->primary_coroutine_);
  }
}

void WorkerScheduler::ThreadRunnerInternal(SingleWorker* worker) {
  while (!worker->scheduler_->quit_flag_)
    worker->FlushInternal();
}

}  // namespace base
