// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_GPURENDERDEVICE_H_
#define CONTENT_PUBLIC_ENGINE_GPURENDERDEVICE_H_

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"
#include "content/public/engine_gpubuffer.h"
#include "content/public/engine_gpufence.h"
#include "content/public/engine_gpupipelinesignature.h"
#include "content/public/engine_gpupipelinestate.h"
#include "content/public/engine_gpuquery.h"
#include "content/public/engine_gpusampler.h"
#include "content/public/engine_gpushader.h"
#include "content/public/engine_gputexture.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
/*--urge(name:GPURenderDevice)--*/
class URGE_RUNTIME_API GPURenderDevice
    : public base::RefCounted<GPURenderDevice> {
 public:
  virtual ~GPURenderDevice() = default;

  /*--urge(name:create_buffer)--*/
  virtual scoped_refptr<GPUBuffer> CreateBuffer(
      ExceptionState& exception_state) = 0;

  /*--urge(name:create_shader)--*/
  virtual scoped_refptr<GPUShader> CreateShader(
      ExceptionState& exception_state) = 0;

  /*--urge(name:create_texture)--*/
  virtual scoped_refptr<GPUTexture> CreateTexture(
      ExceptionState& exception_state) = 0;

  /*--urge(name:create_sampler)--*/
  virtual scoped_refptr<GPUSampler> CreateSampler(
      ExceptionState& exception_state) = 0;

  /*--urge(name:create_graphics_pipeline_state)--*/
  virtual scoped_refptr<GPUPipelineState> CreateGraphicsPipelineState(
      ExceptionState& exception_state) = 0;

  /*--urge(name:create_compute_pipeline_state)--*/
  virtual scoped_refptr<GPUPipelineState> CreateComputePipelineState(
      ExceptionState& exception_state) = 0;

  /*--urge(name:create_fence)--*/
  virtual scoped_refptr<GPUFence> CreateFence(
      ExceptionState& exception_state) = 0;

  /*--urge(name:create_query)--*/
  virtual scoped_refptr<GPUQuery> CreateQuery(
      ExceptionState& exception_state) = 0;

  /*--urge(name:create_pipeline_signature)--*/
  virtual scoped_refptr<GPUPipelineSignature> CreatePipelineSignature(
      ExceptionState& exception_state) = 0;
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_GPURENDERDEVICE_H_
