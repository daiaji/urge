// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_GPULAYOUTELEMENT_H_
#define CONTENT_PUBLIC_ENGINE_GPULAYOUTELEMENT_H_

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
/*--urge(name:GPULayoutElement)--*/
class URGE_RUNTIME_API GPULayoutElement
    : public base::RefCounted<GPULayoutElement> {
 public:
  virtual ~GPULayoutElement() = default;
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_GPULAYOUTELEMENT_H_
