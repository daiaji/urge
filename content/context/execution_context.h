// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CONTEXT_EXECUTION_CONTEXT_H_
#define CONTENT_CONTEXT_EXECUTION_CONTEXT_H_

#include <memory>

namespace content {

struct ScopedFontData;
class CanvasScheduler;
class RenderScreenImpl;
class KeyboardControllerImpl;

struct ExecutionContext {
  ScopedFontData* font_context = nullptr;
  CanvasScheduler* canvas_scheduler = nullptr;

  RenderScreenImpl* graphics = nullptr;
  KeyboardControllerImpl* input = nullptr;

  ExecutionContext();
  ~ExecutionContext();

  ExecutionContext(const ExecutionContext&) = delete;
  ExecutionContext& operator=(const ExecutionContext&) = delete;
};

}  // namespace content

#endif  //! CONTENT_CONTEXT_EXECUTION_CONTEXT_H_
