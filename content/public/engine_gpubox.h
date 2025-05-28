// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_GPUBOX_H_
#define CONTENT_PUBLIC_ENGINE_GPUBOX_H_

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
/*--urge(name:GPUBox)--*/
class URGE_RUNTIME_API GPUBox : public base::RefCounted<GPUBox> {
 public:
  virtual ~GPUBox() = default;

  /*--urge(name:initialize)--*/
  static scoped_refptr<GPUBox> New(ExecutionContext* execution_context,
                                   uint32_t min_x,
                                   uint32_t max_x,
                                   uint32_t min_y,
                                   uint32_t max_y,
                                   uint32_t min_z,
                                   uint32_t max_z,
                                   ExceptionState& exception_state);

  /*--urge(name:min_x)--*/
  URGE_EXPORT_ATTRIBUTE(MinX, uint32_t);

  /*--urge(name:max_x)--*/
  URGE_EXPORT_ATTRIBUTE(MaxX, uint32_t);

  /*--urge(name:min_y)--*/
  URGE_EXPORT_ATTRIBUTE(MinY, uint32_t);

  /*--urge(name:max_y)--*/
  URGE_EXPORT_ATTRIBUTE(MaxY, uint32_t);

  /*--urge(name:min_z)--*/
  URGE_EXPORT_ATTRIBUTE(MinZ, uint32_t);

  /*--urge(name:max_z)--*/
  URGE_EXPORT_ATTRIBUTE(MaxZ, uint32_t);
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_GPUBOX_H_
