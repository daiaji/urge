// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/worker/engine_binding.h"

namespace content {

void EngineBindingBase::PreEarlyInitialization(ContentProfile* profile) {}

void EngineBindingBase::OnMainMessageLoopRun(ExecutionContext* execution) {}

void EngineBindingBase::PostMainLoopRunning() {}

void EngineBindingBase::ExitSignalRequired() {}

void EngineBindingBase::ResetSignalRequired() {}

}  // namespace content
