// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BINDING_UNITTESTS_ENGINE_BINDING_UNITTESTS_H_
#define BINDING_UNITTESTS_ENGINE_BINDING_UNITTESTS_H_

#include <atomic>

#include "content/worker/engine_binding.h"

class EngineBindingUnittests : public content::EngineBindingBase {
 public:
  EngineBindingUnittests();
  ~EngineBindingUnittests() override;

  void PreEarlyInitialization(content::ContentProfile* profile) override;
  void OnMainMessageLoopRun(content::ExecutionContext* execution) override;
  void PostMainLoopRunning() override;
  void ExitSignalRequired() override;
  void ResetSignalRequired() override;

 private:
  std::atomic<int32_t> exit_flag_;
};

#endif  //! BINDING_UNITTESTS_ENGINE_BINDING_UNITTESTS_H_
