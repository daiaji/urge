// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_GPUVIEWPORT_H_
#define CONTENT_PUBLIC_ENGINE_GPUVIEWPORT_H_

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
/*--urge(name:GPUViewport)--*/
class URGE_RUNTIME_API GPUViewport : public base::RefCounted<GPUViewport> {
 public:
  virtual ~GPUViewport() = default;

  /*--urge(name:initialize)--*/
  static scoped_refptr<GPUViewport> New(ExecutionContext* execution_context,
                                        float x,
                                        float y,
                                        float width,
                                        float height,
                                        float min_depth,
                                        float max_depth,
                                        ExceptionState& exception_state);

  /*--urge(name:x)--*/
  URGE_EXPORT_ATTRIBUTE(X, float);

  /*--urge(name:y)--*/
  URGE_EXPORT_ATTRIBUTE(Y, float);

  /*--urge(name:width)--*/
  URGE_EXPORT_ATTRIBUTE(Width, float);

  /*--urge(name:height)--*/
  URGE_EXPORT_ATTRIBUTE(Height, float);

  /*--urge(name:min_depth)--*/
  URGE_EXPORT_ATTRIBUTE(MinDepth, float);

  /*--urge(name:max_depth)--*/
  URGE_EXPORT_ATTRIBUTE(MaxDepth, float);
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_GPUVIEWPORT_H_
