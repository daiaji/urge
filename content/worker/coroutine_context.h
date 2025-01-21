// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_WORKER_COROUTINE_CONTEXT_H_
#define CONTENT_WORKER_COROUTINE_CONTEXT_H_

#include "fiber/fiber.h"

namespace content {

// Content task scheduler data structure.
// Switch fibers for compating with Emscripten platform's event loop.
struct CoroutineContext {
  // Main fiber controller
  fiber_t* primary_fiber;

  // Binding fiber controller
  fiber_t* main_loop_fiber;

  // Frame render skip count
  bool frame_skip_require;

  CoroutineContext()
      : primary_fiber(nullptr),
        main_loop_fiber(nullptr),
        frame_skip_require(false) {}

  ~CoroutineContext() {
    if (main_loop_fiber)
      fiber_delete(main_loop_fiber);
    if (primary_fiber)
      fiber_delete(primary_fiber);
  }
};

}  // namespace content

#endif  //! CONTENT_WORKER_COROUTINE_CONTEXT_H_
