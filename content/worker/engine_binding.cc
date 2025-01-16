// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/worker/engine_binding.h"

namespace content {

void EngineBindingBase::PreEarlyInitialization(ContentProfile* profile) {}

int EngineBindingBase::OnMainMessageLoopRun(ExecutionContext* execution) {
  return 0;
}

void EngineBindingBase::PostMainLoopRunning() {}

}  // namespace content
