// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/worker/single_worker.h"

#include "fiber/fiber.h"

#include "base/worker/isolate_scheduler.h"

namespace base {

SingleWorker::SingleWorker(WorkerScheduleMode mode)
    : scheduler_(nullptr), mode_(mode), task_queue_() {}

SingleWorker::~SingleWorker() {}

std::unique_ptr<SingleWorker> SingleWorker::CreateWorker(
    WorkerScheduleMode mode) {
  return std::unique_ptr<SingleWorker>(new SingleWorker(mode));
}

bool SingleWorker::PostTask(OnceClosure task) {
  return task_queue_.enqueue(std::move(task));
}

bool SingleWorker::WaitWorkerSynchronize() {
  moodycamel::LightweightSemaphore semaphore;
  OnceClosure required_task = base::BindOnce(
      [](moodycamel::LightweightSemaphore* semaphore) { semaphore->signal(); },
      &semaphore);
  task_queue_.enqueue(std::move(required_task));
  return semaphore.tryWait();
}

void SingleWorker::FlushInternal() {
  OnceClosure queued_task;
  if (!task_queue_.try_dequeue(queued_task) &&
      mode_ == WorkerScheduleMode::kAsync)
    std::this_thread::yield();

  if (!queued_task.is_null())
    std::move(queued_task).Run();
}

bool SingleWorker::DeleteOrReleaseSoonInternal(void (*deleter)(const void*),
                                               const void* object) {
  PostTask(base::BindOnce(deleter, object));
  return !!object;
}

void SingleWorker::YieldFiber() {
  if (scheduler_ && mode_ == WorkerScheduleMode::kCoroutine)
    fiber_switch(scheduler_->primary_coroutine_);
}

}  // namespace base
