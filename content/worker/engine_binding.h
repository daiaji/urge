// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_WORKER_ENGINE_BINDING_H_
#define CONTENT_WORKER_ENGINE_BINDING_H_

#include "components/filesystem/io_service.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"
#include "content/profile/content_profile.h"
#include "content/public/engine_audio.h"
#include "content/public/engine_graphics.h"
#include "content/public/engine_input.h"
#include "content/public/engine_mouse.h"

namespace content {

class EngineBindingBase {
 public:
  virtual ~EngineBindingBase() = default;

  // Scoped script context modules info.
  // Usage: module-like class method calling.
  struct ScopedModuleContext {
    scoped_refptr<Graphics> graphics;
    scoped_refptr<Input> input;
    scoped_refptr<Audio> audio;
    scoped_refptr<Mouse> mouse;
  };

  // Raised before engine running, after engine other module initialized,
  // prepared for binding creation.
  virtual void PreEarlyInitialization(ContentProfile* profile,
                                      filesystem::IOService* io_service);

  // Raise for running main loop.
  virtual void OnMainMessageLoopRun(ExecutionContext* execution_context,
                                    ScopedModuleContext* module_context);

  // After running and release engine resource.
  virtual void PostMainLoopRunning();

  // Engine loop require an exit signal.
  virtual void ExitSignalRequired();

  // Engine loop require a reset signal.
  virtual void ResetSignalRequired();
};

}  // namespace content

#endif  //! CONTENT_WORKER_ENGINE_BINDING_H_
