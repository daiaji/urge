// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CONTEXT_EXECUTION_CONTEXT_H_
#define CONTENT_CONTEXT_EXECUTION_CONTEXT_H_

#include <memory>

namespace filesystem {
class IOService;
}

namespace content {

struct ScopedFontData;
class EventController;
class CanvasScheduler;
class RenderScreenImpl;

struct ExecutionContext {
  ScopedFontData* font_context = nullptr;
  EventController* event_controller = nullptr;
  CanvasScheduler* canvas_scheduler = nullptr;
  RenderScreenImpl* graphics = nullptr;
  filesystem::IOService* io_service = nullptr;

  ExecutionContext();
  ~ExecutionContext();

  ExecutionContext(const ExecutionContext&) = delete;
  ExecutionContext& operator=(const ExecutionContext&) = delete;
};

}  // namespace content

#endif  //! CONTENT_CONTEXT_EXECUTION_CONTEXT_H_
