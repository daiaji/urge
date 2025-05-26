// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_GPUCOMMANDLIST_H_
#define CONTENT_PUBLIC_ENGINE_GPUCOMMANDLIST_H_

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"
#include "content/public/engine_color.h"
#include "content/public/engine_gpubuffer.h"
#include "content/public/engine_gpufence.h"
#include "content/public/engine_gpupipelinesignature.h"
#include "content/public/engine_gpupipelinestate.h"
#include "content/public/engine_gpuquery.h"
#include "content/public/engine_gpuresourcebinding.h"
#include "content/public/engine_gpusampler.h"
#include "content/public/engine_gpushader.h"
#include "content/public/engine_gputexture.h"
#include "content/public/engine_gputextureview.h"
#include "content/public/engine_gpuviewport.h"
#include "content/public/engine_rect.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
/*--urge(name:GPUCommandList)--*/
class URGE_RUNTIME_API GPUCommandList
    : public base::RefCounted<GPUCommandList> {
 public:
  virtual ~GPUCommandList() = default;

  /*--urge(name:ResourceStateTransitionMode)--*/
  enum ResourceStateTransitionMode {
    RESOURCE_STATE_TRANSITION_MODE_NONE = 0,
    RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
    RESOURCE_STATE_TRANSITION_MODE_VERIFY,
  };

  /*--urge(name:ValueType)--*/
  enum ValueType {
    VT_UNDEFINED = 0,
    VT_INT8,
    VT_INT16,
    VT_INT32,
    VT_UINT8,
    VT_UINT16,
    VT_UINT32,
    VT_FLOAT16,
    VT_FLOAT32,
    VT_FLOAT64,
  };

  /*--urge(name:ClearDepthStencilFlags)--*/
  enum ClearDepthStencilFlags {
    CLEAR_DEPTH_FLAG_NONE = 0x00,
    CLEAR_DEPTH_FLAG = 0x01,
    CLEAR_STENCIL_FLAG = 0x02,
  };

  /*--urge(name:initialize)--*/
  static scoped_refptr<GPUCommandList> New(ExecutionContext* execution_context,
                                           ExceptionState& exception_state);

  /*--urge(name:set_pipeline_state)--*/
  virtual void SetPipelineState(scoped_refptr<GPUPipelineState> pipeline,
                                ExceptionState& exception_state) = 0;

  /*--urge(name:transition_shader_resources)--*/
  virtual void TransitionShaderResources(
      scoped_refptr<GPUResourceBinding> binding,
      ExceptionState& exception_state) = 0;

  /*--urge(name:commit_shader_resources)--*/
  virtual void CommitShaderResources(scoped_refptr<GPUResourceBinding> binding,
                                     ResourceStateTransitionMode mode,
                                     ExceptionState& exception_state) = 0;

  /*--urge(name:set_stencil_ref)--*/
  virtual void SetStencilRef(uint32_t ref, ExceptionState& exception_state) = 0;

  /*--urge(name:set_blend_factors)--*/
  virtual void SetBlendFactors(const std::vector<float>& factors,
                               ExceptionState& exception_state) = 0;

  /*--urge(name:set_vertex_buffers)--*/
  virtual void SetVertexBuffers(
      uint32_t start_slot,
      const std::vector<scoped_refptr<GPUBuffer>>& buffers,
      const std::vector<uint64_t>& offsets,
      ResourceStateTransitionMode mode,
      ExceptionState& exception_state) = 0;

  /*--urge(name:invalidate_state)--*/
  virtual void InvalidateState(ExceptionState& exception_state) = 0;

  /*--urge(name:set_index_buffer)--*/
  virtual void SetIndexBuffer(scoped_refptr<GPUBuffer> buffer,
                              uint64_t byte_offset,
                              ResourceStateTransitionMode mode,
                              ExceptionState& exception_state) = 0;

  /*--urge(name:set_viewports)--*/
  virtual void SetViewports(
      const std::vector<scoped_refptr<GPUViewport>>& viewports,
      ExceptionState& exception_state) = 0;

  /*--urge(name:set_scissor_rects)--*/
  virtual void SetScissorRects(const std::vector<scoped_refptr<Rect>>& rects,
                               ExceptionState& exception_state) = 0;

  /*--urge(name:set_render_targets)--*/
  virtual void SetRenderTargets(
      const std::vector<scoped_refptr<GPUTextureView>>& render_targets,
      scoped_refptr<GPUTextureView> depth_stencil,
      ExceptionState& exception_state) = 0;

  /*--urge(name:draw)--*/
  virtual void Draw(uint32_t num_vertices,
                    uint32_t num_instances,
                    uint32_t first_vertex,
                    uint32_t first_instance,
                    ExceptionState& exception_state) = 0;

  /*--urge(name:draw_indexed)--*/
  virtual void DrawIndexed(uint32_t num_indices,
                           uint32_t num_instances,
                           uint32_t first_index,
                           uint32_t base_vertex,
                           uint32_t first_instance,
                           ValueType index_type,
                           ExceptionState& exception_state) = 0;

  /*--urge(name:draw_indirect)--*/
  virtual void DrawIndirect(scoped_refptr<GPUBuffer> attribs_buffer,
                            uint64_t draw_args_offset,
                            uint32_t draw_count,
                            uint32_t draw_args_stride,
                            ResourceStateTransitionMode attribs_buffer_mode,
                            scoped_refptr<GPUBuffer> counter_buffer,
                            uint64_t counter_offset,
                            ResourceStateTransitionMode counter_buffer_mode,
                            ExceptionState& exception_state) = 0;

  /*--urge(name:draw_indexed_indirect)--*/
  virtual void DrawIndexedIndirect(
      scoped_refptr<GPUBuffer> attribs_buffer,
      uint64_t draw_args_offset,
      uint32_t draw_count,
      uint32_t draw_args_stride,
      ResourceStateTransitionMode attribs_buffer_mode,
      scoped_refptr<GPUBuffer> counter_buffer,
      uint64_t counter_offset,
      ResourceStateTransitionMode counter_buffer_mode,
      ValueType index_type,
      ExceptionState& exception_state) = 0;

  /*--urge(name:dispatch_compute)--*/
  virtual void DispatchCompute(uint32_t thread_group_count_x,
                               uint32_t thread_group_count_y,
                               uint32_t thread_group_count_z,
                               ExceptionState& exception_state) = 0;

  /*--urge(name:dispatch_compute_indirect)--*/
  virtual void DispatchComputeIndirect(
      scoped_refptr<GPUBuffer> attribs_buffer,
      ResourceStateTransitionMode attribs_buffer_mode,
      uint64_t dispatch_args_byte_offset,
      ExceptionState& exception_state) = 0;

  /*--urge(name:clear_depth_stencil)--*/
  virtual void ClearDepthStencil(scoped_refptr<GPUTextureView> view,
                                 ClearDepthStencilFlags clear_flags,
                                 float depth,
                                 uint32_t stencil,
                                 ExceptionState& exception_state) = 0;

  /*--urge(name:clear_render_target)--*/
  virtual void ClearRenderTarget(scoped_refptr<GPUTextureView> view,
                                 scoped_refptr<Color> color,
                                 ResourceStateTransitionMode mode,
                                 ExceptionState& exception_state) = 0;

  /*--urge(name:enqueue_signal)--*/
  virtual void EnqueueSignal(scoped_refptr<GPUFence> fence,
                             uint64_t value,
                             ExceptionState& exception_state) = 0;

  /*--urge(name:device_wait_for_fence)--*/
  virtual void DeviceWaitForFence(scoped_refptr<GPUFence> fence,
                                  uint64_t value,
                                  ExceptionState& exception_state) = 0;

  /*--urge(name:wait_for_idle)--*/
  virtual void WaitForIdle(ExceptionState& exception_state) = 0;

  /*--urge(name:begin_query)--*/
  virtual void BeginQuery(scoped_refptr<GPUQuery> query,
                          ExceptionState& exception_state) = 0;

  /*--urge(name:end_query)--*/
  virtual void EndQuery(scoped_refptr<GPUQuery> query,
                        ExceptionState& exception_state) = 0;

  /*--urge(name:flush)--*/
  virtual void Flush(ExceptionState& exception_state) = 0;

  /*--urge(name:update_buffer)--*/
  virtual void UpdateBuffer(scoped_refptr<GPUBuffer> buffer,
                            uint64_t offset,
                            const std::string& data,
                            ExceptionState& exception_state) = 0;

  /*--urge(name:copy_buffer)--*/
  virtual void CopyBuffer(scoped_refptr<GPUBuffer> src_buffer,
                          uint64_t src_offset,
                          ResourceStateTransitionMode src_mode,
                          scoped_refptr<GPUBuffer> dst_buffer,
                          uint64_t dst_offset,
                          ResourceStateTransitionMode dst_mode,
                          uint64_t copy_size,
                          ExceptionState& exception_state) = 0;

  /*--urge(name:update_texture)--*/
  virtual void UpdateTexture(scoped_refptr<GPUTexture> texture,
                             uint32_t mip_level,
                             uint32_t slice,
                             scoped_refptr<GPUViewport> box,
                             const std::string& data,
                             uint64_t stride,
                             uint64_t depth_stride,
                             ResourceStateTransitionMode texture_mode,
                             ExceptionState& exception_state) = 0;

  /*--urge(name:update_texture)--*/
  virtual void UpdateTexture(scoped_refptr<GPUTexture> texture,
                             uint32_t mip_level,
                             uint32_t slice,
                             scoped_refptr<GPUViewport> box,
                             scoped_refptr<GPUBuffer> data,
                             uint64_t src_offset,
                             uint64_t stride,
                             uint64_t depth_stride,
                             ResourceStateTransitionMode src_buffer_mode,
                             ResourceStateTransitionMode texture_mode,
                             ExceptionState& exception_state) = 0;

  /*--urge(name:copy_texture)--*/
  virtual void CopyTexture(scoped_refptr<GPUTexture> src_texture,
                           uint32_t src_mip_level,
                           uint32_t src_slice,
                           scoped_refptr<GPUViewport> src_box,
                           ResourceStateTransitionMode src_mode,
                           scoped_refptr<GPUTexture> dst_texture,
                           uint32_t dst_mip_level,
                           uint32_t dst_slice,
                           uint32_t dst_x,
                           uint32_t dst_y,
                           uint32_t dst_z,
                           ResourceStateTransitionMode dst_mode,
                           ExceptionState& exception_state) = 0;

  /*--urge(name:generate_mips)--*/
  virtual void GenerateMips(scoped_refptr<GPUTextureView> view,
                            ExceptionState& exception_state) = 0;

  /*--urge(name:begin_debug_group)--*/
  virtual void BeginDebugGroup(const std::string& name,
                               scoped_refptr<Color> color,
                               ExceptionState& exception_state) = 0;

  /*--urge(name:end_debug_group)--*/
  virtual void EndDebugGroup(ExceptionState& exception_state) = 0;

  /*--urge(name:insert_debug_group)--*/
  virtual void InsertDebugGroup(const std::string& name,
                                scoped_refptr<Color> color,
                                ExceptionState& exception_state) = 0;
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_GPUCOMMANDLIST_H_
