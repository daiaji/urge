// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_WORKER_SINGLE_WORKER_H_
#define BASE_WORKER_SINGLE_WORKER_H_

#include <memory>

#include "concurrentqueue/blockingconcurrentqueue.h"

#include "base/bind/callback.h"

namespace base {

class SingleWorker;

// Template helpers which use function indirection to erase T from the
// function signature while still remembering it so we can call the
// correct destructor/release function.
//
// We use this trick so we don't need to include bind.h in a header
// file like sequenced_task_runner.h. We also wrap the helpers in a
// templated class to make it easier for users of DeleteSoon to
// declare the helper as a friend.
template <class T>
class DeleteHelper {
 private:
  static void DoDelete(const void* object) {
    delete static_cast<const T*>(object);
  }

  friend class SingleWorker;
};

template <class T>
class DeleteUniquePtrHelper {
 private:
  static void DoDelete(const void* object) {
    // Carefully unwrap `object`. T could have originally been const-qualified
    // or not, and it is important to ensure that the constness matches in order
    // to use the right specialization of std::default_delete<T>...
    std::unique_ptr<T> destroyer(const_cast<T*>(static_cast<const T*>(object)));
  }

  friend class SingleWorker;
};

template <class T>
class ReleaseHelper {
 private:
  static void DoRelease(const void* object) {
    static_cast<const T*>(object)->Release();
  }

  friend class SingleWorker;
};

enum class WorkerScheduleMode {
  // Running on the thread with the main scheduler created.
  kCoroutine = 0,

  // Launch a new thread for current worker running.
  kAsync,
};

class SingleWorker {
 public:
  ~SingleWorker();

  SingleWorker(const SingleWorker&) = delete;
  SingleWorker& operator=(const SingleWorker&) = delete;

  static std::unique_ptr<SingleWorker> CreateWorker(WorkerScheduleMode mode);

  // Post a task closure for current worker.
  bool PostTask(OnceClosure task);
  static bool PostTask(SingleWorker* worker, OnceClosure task);

  // Post a semaphore flag in queue, blocking caller thread for synchronization.
  bool WaitWorkerSynchronize();
  static bool WaitWorkerSynchronize(SingleWorker* worker);

  // Is current context running on current single worker?
  bool RunsTasksInCurrentSequence();

  // Return worker scheduler mode.
  WorkerScheduleMode GetSchedulerMode() { return mode_; }

  // Used in coroutine schedule mode worker,
  // yield the context to next coroutine worker.
  void YieldFiber();

  template <class T>
  bool DeleteSoon(const T* object) {
    return DeleteOrReleaseSoonInternal(&DeleteHelper<T>::DoDelete, object);
  }

  template <class T>
  static bool DeleteSoon(SingleWorker* worker, const T* object) {
    if (worker)
      return worker->DeleteOrReleaseSoonInternal(&DeleteHelper<T>::DoDelete,
                                                 object);
    delete object;
    return true;
  }

  template <class T>
  bool DeleteSoon(std::unique_ptr<T> object) {
    return DeleteOrReleaseSoonInternal(&DeleteUniquePtrHelper<T>::DoDelete,
                                       object.release());
  }

  template <class T>
  static bool DeleteSoon(SingleWorker* worker, std::unique_ptr<T> object) {
    if (worker)
      return worker->DeleteOrReleaseSoonInternal(
          &DeleteUniquePtrHelper<T>::DoDelete, object.release());
    object.reset();
    return true;
  }

  template <class T>
  void ReleaseSoon(scoped_refptr<T>&& object) {
    if (!object)
      return;
    DeleteOrReleaseSoonInternal(&ReleaseHelper<T>::DoRelease, object.release());
  }

  template <class T>
  static void ReleaseSoon(SingleWorker* worker, scoped_refptr<T>&& object) {
    if (!object)
      return;
    if (worker)
      return worker->DeleteOrReleaseSoonInternal(&ReleaseHelper<T>::DoRelease,
                                                 object.release());
    object.reset();
  }

 private:
  friend class WorkerScheduler;
  SingleWorker(WorkerScheduleMode mode);
  void FlushInternal();
  bool DeleteOrReleaseSoonInternal(void (*deleter)(const void*),
                                   const void* object);

  WorkerScheduler* scheduler_;
  WorkerScheduleMode mode_;
  moodycamel::ConcurrentQueue<OnceClosure> task_queue_;
  std::thread::id sequence_thread_;
};

}  // namespace base

#endif  // ! BASE_WORKER_SINGLE_WORKER_H_
