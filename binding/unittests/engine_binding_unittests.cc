// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "binding/unittests/engine_binding_unittests.h"

#include "base/debug/logging.h"
#include "content/screen/renderscreen_impl.h"

EngineBindingUnittests::EngineBindingUnittests() {}

EngineBindingUnittests::~EngineBindingUnittests() {}

void EngineBindingUnittests::PreEarlyInitialization(
    content::ContentProfile* profile) {
  LOG(INFO) << "preload engine";
}

void EngineBindingUnittests::OnMainMessageLoopRun(
    content::ExecutionContext* execution) {
  for (;;) {
    content::ExceptionState exception_state;
    execution->graphics->Update(exception_state);
  }
}

void EngineBindingUnittests::PostMainLoopRunning() {
  LOG(INFO) << "will close engine";
}
