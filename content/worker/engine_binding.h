// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_WORKER_ENGINE_BINDING_H_
#define CONTENT_WORKER_ENGINE_BINDING_H_

#include "content/context/execution_context.h"
#include "content/profile/content_profile.h"

namespace content {

class EngineBindingBase {
 public:
  virtual ~EngineBindingBase() = default;

  // Raised before engine running, after engine other module initialized,
  // prepared for binding creation.
  virtual void PreEarlyInitialization(ContentProfile* profile);

  // Raise for running main loop.
  virtual void OnMainMessageLoopRun(ExecutionContext* execution);

  // After running and release engine resource.
  virtual void PostMainLoopRunning();

  // Engine loop require an exit signal.
  virtual void ExitSignalRequired();

  // Engine loop require a reset signal.
  virtual void ResetSignalRequired();
};

}  // namespace content

#endif  //! CONTENT_WORKER_ENGINE_BINDING_H_
