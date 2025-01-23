// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/screen/viewport_impl.h"

#include "content/common/rect_impl.h"

namespace content {

namespace {

void GPUCreateViewportAgent(renderer::RenderDevice* device,
                            ViewportAgent* agent,
                            const base::Vec2i& viewport_size) {
  float world_matrix[32];
  renderer::MakeProjectionMatrix(world_matrix, viewport_size);
  renderer::MakeIdentityMatrix(world_matrix + 16);

  wgpu::BufferDescriptor uniform_desc;
  uniform_desc.label = "viewport.world.uniform";
  uniform_desc.size = sizeof(world_matrix);
  uniform_desc.mappedAtCreation = true;
  uniform_desc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
  agent->world_uniform = (*device)->CreateBuffer(&uniform_desc);

  {
    memcpy(agent->world_uniform.GetMappedRange(), world_matrix,
           sizeof(world_matrix));
    agent->world_uniform.Unmap();
  }

  wgpu::BindGroupEntry entries;
  entries.binding = 0;
  entries.buffer = agent->world_uniform;

  wgpu::BindGroupDescriptor binding_desc;
  binding_desc.entryCount = 1;
  binding_desc.entries = &entries;
  binding_desc.layout = *device->GetPipelines()->base.GetLayout(0);

  agent->world_binding = (*device)->CreateBindGroup(&binding_desc);
  agent->projection_size = viewport_size;
}

void GPUDestroyViewportAgent(ViewportAgent* agent) {
  agent->world_binding = nullptr;
  agent->world_uniform = nullptr;

  delete agent;
}

void GPUUpdateViewportWorldMatrix(wgpu::CommandEncoder* encoder,
                                  ViewportAgent* agent) {
  float world_matrix[32];
  renderer::MakeProjectionMatrix(world_matrix, agent->projection_size);
  renderer::MakeIdentityMatrix(world_matrix + 16);

  encoder->WriteBuffer(agent->world_uniform, 0,
                       reinterpret_cast<uint8_t*>(world_matrix),
                       sizeof(world_matrix));
}

}  // namespace

scoped_refptr<Viewport> Viewport::New(ExecutionContext* execution_context,
                                      ExceptionState& exception_state) {
  return new ViewportImpl(execution_context->graphics,
                          execution_context->graphics->Resolution());
}

scoped_refptr<Viewport> Viewport::New(ExecutionContext* execution_context,
                                      scoped_refptr<Rect> rect,
                                      ExceptionState& exception_state) {
  return new ViewportImpl(execution_context->graphics,
                          RectImpl::From(rect)->AsBaseRect());
}

scoped_refptr<Viewport> Viewport::New(ExecutionContext* execution_context,
                                      int32_t x,
                                      int32_t y,
                                      int32_t width,
                                      int32_t height,
                                      ExceptionState& exception_state) {
  return new ViewportImpl(execution_context->graphics,
                          base::Rect(x, y, width, height));
}

ViewportImpl::ViewportImpl(RenderScreenImpl* screen, const base::Rect& region)
    : screen_(screen), node_(SortKey()), region_(region) {
  node_.RegisterEventHandler(base::BindRepeating(
      &ViewportImpl::DrawableNodeHandlerInternal, base::Unretained(this)));
  node_.RebindController(screen_->GetDrawableController());

  agent_ = new ViewportAgent;
  screen_->PostTask(base::BindOnce(
      &GPUCreateViewportAgent, screen_->GetDevice(), agent_, region.Size()));
}

ViewportImpl::~ViewportImpl() {
  ExceptionState exception_state;
  Dispose(exception_state);
}

scoped_refptr<ViewportImpl> ViewportImpl::From(scoped_refptr<Viewport> host) {
  return static_cast<ViewportImpl*>(host.get());
}

void ViewportImpl::Dispose(ExceptionState& exception_state) {
  if (!IsDisposed(exception_state)) {
    node_.DisposeNode();

    screen_->PostTask(base::BindOnce(&GPUDestroyViewportAgent, agent_));
    agent_ = nullptr;
  }
}

bool ViewportImpl::IsDisposed(ExceptionState& exception_state) {
  return !agent_;
}

void ViewportImpl::Flash(scoped_refptr<Color> color,
                         uint32_t duration,
                         ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;
}

void ViewportImpl::Update(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;
}

scoped_refptr<Rect> ViewportImpl::Get_Rect(ExceptionState& exception_state) {
  return new RectImpl(region_);
}

void ViewportImpl::Put_Rect(const scoped_refptr<Rect>& value,
                            ExceptionState& exception_state) {
  region_ = RectImpl::From(value)->AsBaseRect();
}

bool ViewportImpl::Get_Visible(ExceptionState& exception_state) {
  return node_.GetVisibility() == DrawableNode::VisibilityState::kVisible;
}

void ViewportImpl::Put_Visible(const bool& value,
                               ExceptionState& exception_state) {
  node_.SetNodeVisibility(value ? DrawableNode::VisibilityState::kVisible
                                : DrawableNode::VisibilityState::kInVisible);
}

int32_t ViewportImpl::Get_Z(ExceptionState& exception_state) {
  return static_cast<int32_t>(node_.GetSortKeys()->weight[0]);
}

void ViewportImpl::Put_Z(const int32_t& value,
                         ExceptionState& exception_state) {
  node_.SetNodeSortWeight(value);
}

int32_t ViewportImpl::Get_Ox(ExceptionState& exception_state) {
  return origin_.x;
}

void ViewportImpl::Put_Ox(const int32_t& value,
                          ExceptionState& exception_state) {
  origin_.x = value;
}

int32_t ViewportImpl::Get_Oy(ExceptionState& exception_state) {
  return origin_.y;
}

void ViewportImpl::Put_Oy(const int32_t& value,
                          ExceptionState& exception_state) {
  origin_.y = value;
}

scoped_refptr<Color> ViewportImpl::Get_Color(ExceptionState& exception_state) {
  return color_;
}

void ViewportImpl::Put_Color(const scoped_refptr<Color>& value,
                             ExceptionState& exception_state) {
  color_ = ColorImpl::From(value);
}

scoped_refptr<Tone> ViewportImpl::Get_Tone(ExceptionState& exception_state) {
  return tone_;
}

void ViewportImpl::Put_Tone(const scoped_refptr<Tone>& value,
                            ExceptionState& exception_state) {
  tone_ = ToneImpl::From(value);
}

bool ViewportImpl::CheckDisposed(ExceptionState& exception_state) {
  if (IsDisposed(exception_state)) {
    exception_state.ThrowContentError(ExceptionCode::kDisposedObject,
                                      "disposed object: viewport");
    return true;
  }

  return false;
}

void ViewportImpl::DrawableNodeHandlerInternal(
    DrawableNode::RenderStage stage,
    DrawableNode::RenderControllerParams* params) {
  // Stack storage last render state
  DrawableNode::RenderControllerParams transient_params;
  transient_params.device = params->device;
  transient_params.command_encoder = params->command_encoder;
  transient_params.main_pass = params->main_pass;
  transient_params.screen_buffer = params->screen_buffer;

  // Calculate viewport region
  base::Vec2i viewport_position = region_.Position() - origin_;
  base::Rect viewport_region(viewport_position, region_.Size());

  if (stage == DrawableNode::kBeforeRender) {
    // Update uniform buffer if viewport region changed.
    if (!(region_.Size() == agent_->projection_size)) {
      agent_->projection_size = region_.Size();
      screen_->PostTask(base::BindOnce(&GPUUpdateViewportWorldMatrix,
                                       params->command_encoder, agent_));
    }
  }

  if (stage == DrawableNode::kOnRendering ||
      stage == DrawableNode::kAfterRender) {
    // Reset viewport's world settings
    transient_params.world_binding = &agent_->world_binding;
    transient_params.viewport_region =
        base::MakeIntersect(params->viewport_region, viewport_region);
  }

  // Skip if invalid
  if (transient_params.viewport_region.width <= 0 ||
      transient_params.viewport_region.height <= 0)
    return;

  // Set new viewport
  if (transient_params.main_pass)
    transient_params.main_pass->SetViewport(
        transient_params.viewport_region.x, transient_params.viewport_region.y,
        transient_params.viewport_region.width,
        transient_params.viewport_region.height, 0, 0);

  // Notify children drawable node
  controller_.BroadCastNotification(stage, &transient_params);
}

}  // namespace content
