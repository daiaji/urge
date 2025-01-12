#include "execution_context.h"
// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/context/execution_context.h"

namespace content {

std::unique_ptr<ExecutionContext> ExecutionContext::MakeContext() {
  return std::unique_ptr<ExecutionContext>(new ExecutionContext());
}

ScopedFontData* ExecutionContext::GetFontContext() {
  return nullptr;
}

CanvasScheduler* ExecutionContext::GetCanvasScheduler() {
  return nullptr;
}

}  // namespace content
