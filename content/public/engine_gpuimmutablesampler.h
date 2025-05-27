// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_GPUIMMUTABLESAMPLER_H_
#define CONTENT_PUBLIC_ENGINE_GPUIMMUTABLESAMPLER_H_

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
/*--urge(name:GPUImmutableSampler)--*/
class URGE_RUNTIME_API GPUImmutableSampler
    : public base::RefCounted<GPUImmutableSampler> {
 public:
  virtual ~GPUImmutableSampler() = default;
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_GPUIMMUTABLESAMPLER_H_
