// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/screen/viewport_impl.h"

#include "content/common/rect_impl.h"

namespace content {

namespace {

void GPUCreateViewportAgent(renderer::RenderDevice* device,
                            ViewportAgent* agent) {
  agent->world_uniform =
      renderer::CreateUniformBuffer<renderer::WorldMatrixUniform>(
          **device, "viewport.world.uniform");
  agent->world_binding =
      renderer::WorldMatrixUniform::CreateGroup(**device, agent->world_uniform);
}

void GPUDestroyViewportAgent(ViewportAgent* agent) {
  agent->world_binding = nullptr;
  agent->world_uniform = nullptr;

  delete agent;
}

void GPUUpdateViewportWorldMatrix(wgpu::CommandEncoder* encoder,
                                  ViewportAgent* agent,
                                  const base::Vec2i& viewport_size) {
  renderer::WorldMatrixUniform world_matrix;
  renderer::MakeProjectionMatrix(world_matrix.projection, viewport_size);
  renderer::MakeIdentityMatrix(world_matrix.transform);

  encoder->WriteBuffer(agent->world_uniform, 0,
                       reinterpret_cast<uint8_t*>(&world_matrix),
                       sizeof(world_matrix));
}

void GPUResetViewportRegion(wgpu::RenderPassEncoder* agent,
                            const base::Rect& region) {
  agent->SetViewport(region.x, region.y, region.width, region.height, 0, 0);
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
    : GraphicsChild(screen), node_(SortKey()), region_(region) {
  node_.RegisterEventHandler(base::BindRepeating(
      &ViewportImpl::DrawableNodeHandlerInternal, base::Unretained(this)));
  node_.RebindController(screen->GetDrawableController());

  agent_ = new ViewportAgent;
  screen->PostTask(base::BindOnce(&GPUCreateViewportAgent, screen->GetDevice(),
                                  agent_, region));
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

    screen()->PostTask(base::BindOnce(&GPUDestroyViewportAgent, agent_));
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
  transient_params.viewport = params->viewport;

  // Calculate viewport region
  base::Vec2i viewport_position = region_.Position() - origin_;
  base::Rect viewport_region(viewport_position, region_.Size());
  base::Rect interact_viewport =
      base::MakeIntersect(params->viewport, viewport_region);

  if (stage == DrawableNode::kBeforeRender) {
    // Update uniform buffer if viewport region changed.
    if (!(region_ == agent_->region_cache)) {
      agent_->region_cache = region_;
      screen()->PostTask(base::BindOnce(&GPUUpdateViewportWorldMatrix,
                                        params->command_encoder, agent_,
                                        interact_viewport.Size()));
    }
  }

  if (transient_params.main_pass) {
    // Reset viewport's world settings
    transient_params.world_binding = &agent_->world_binding;
    transient_params.viewport = interact_viewport;

    // Set new viewport
    screen()->PostTask(base::BindOnce(&GPUResetViewportRegion,
                                      transient_params.main_pass,
                                      transient_params.viewport));
  }

  // Notify children drawable node
  controller_.BroadCastNotification(stage, &transient_params);

  // Restore last viewport settings
  if (params->main_pass)
    screen()->PostTask(base::BindOnce(&GPUResetViewportRegion,
                                      params->main_pass, params->viewport));
}

}  // namespace content
