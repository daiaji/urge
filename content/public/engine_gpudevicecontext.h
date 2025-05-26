// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_GPUDEVICECONTEXT_H_
#define CONTENT_PUBLIC_ENGINE_GPUDEVICECONTEXT_H_

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"
#include "content/public/engine_gpucommandlist.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
/*--urge(name:GPUDeviceContext)--*/
class URGE_RUNTIME_API GPUDeviceContext
    : public base::RefCounted<GPUDeviceContext> {
 public:
  virtual ~GPUDeviceContext() = default;

  /*--urge(name:execute_command_list)--*/
  virtual void ExecuteCommandList(scoped_refptr<GPUCommandList> commands,
                                  ExceptionState& exception_state) = 0;

  /*--urge(name:wait_for_idle)--*/
  virtual void WaitForIdle(ExceptionState& exception_state) = 0;
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_GPUDEVICECONTEXT_H_
