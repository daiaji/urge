// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_WORKER_THREAD_WORKER_H_
#define BASE_WORKER_THREAD_WORKER_H_

#include <condition_variable>
#include <memory>
#include <mutex>

#include "base/bind/callback.h"

namespace base {

class Semaphore {
 public:
  explicit Semaphore() : mutex_(), cv_(), count_(0) {}

  inline void Acquire() {
    std::unique_lock lock(mutex_);
    ++count_;
    cv_.wait(lock, [this] { return count_ == 0; });
  }

  inline void Release() {
    std::scoped_lock lock(mutex_);
    --count_;
    cv_.notify_all();
  }

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  int32_t count_;
};

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

  friend class ThreadWorker;
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

  friend class ThreadWorker;
};

template <class T>
class ReleaseHelper {
 private:
  static void DoRelease(const void* object) {
    static_cast<const T*>(object)->Release();
  }

  friend class ThreadWorker;
};

class ThreadWorker {
 public:
  struct QueueInternal;

  ~ThreadWorker();

  ThreadWorker(const ThreadWorker&) = delete;
  ThreadWorker& operator=(const ThreadWorker&) = delete;

  static std::unique_ptr<ThreadWorker> Create();

  // Post a task closure for current worker.
  bool PostTask(OnceClosure task);
  static bool PostTask(ThreadWorker* worker, OnceClosure task);

  // Post a semaphore flag in queue, blocking caller thread for synchronization.
  void WaitWorkerSynchronize();
  static void WaitWorkerSynchronize(ThreadWorker* worker);

  // Is current context running on current single worker?
  bool RunsTasksInCurrentSequence();
  static bool RunsTasksInCurrentSequence(ThreadWorker* worker);

  template <class T>
  bool DeleteSoon(const T* object) {
    return DeleteOrReleaseSoonInternal(&DeleteHelper<T>::DoDelete, object);
  }

  template <class T>
  static bool DeleteSoon(ThreadWorker* worker, const T* object) {
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
  static bool DeleteSoon(ThreadWorker* worker, std::unique_ptr<T> object) {
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
  static void ReleaseSoon(ThreadWorker* worker, scoped_refptr<T>&& object) {
    if (!object)
      return;
    if (worker)
      return worker->DeleteOrReleaseSoonInternal(&ReleaseHelper<T>::DoRelease,
                                                 object.release());
    object.reset();
  }

 private:
  ThreadWorker();
  void ThreadMainFunctionInternal();
  bool DeleteOrReleaseSoonInternal(void (*deleter)(const void*),
                                   const void* object);

  std::thread thread_;
  std::atomic<int32_t> quit_flag_;
  std::unique_ptr<QueueInternal> task_queue_;
};

}  // namespace base

#endif  //! BASE_WORKER_THREAD_WORKER_H_
