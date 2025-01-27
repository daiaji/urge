// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/screen/viewport_impl.h"

#include "content/common/rect_impl.h"
#include "renderer/utils/buffer_utils.h"
#include "renderer/utils/texture_utils.h"

namespace content {

namespace {

void GPUCreateViewportAgent(renderer::RenderDevice* device,
                            ViewportAgent* agent) {
  agent->world_uniform =
      renderer::CreateUniformBuffer<renderer::WorldMatrixUniform>(
          **device, "viewport.world.uniform", wgpu::BufferUsage::CopyDst);
  agent->world_binding =
      renderer::WorldMatrixUniform::CreateGroup(**device, agent->world_uniform);
}

void GPUDestroyViewportAgent(ViewportAgent* agent) {
  agent->world_binding = nullptr;
  agent->world_uniform = nullptr;

  delete agent;
}

void GPUUpdateViewportAgentData(renderer::RenderDevice* device,
                                wgpu::CommandEncoder* encoder,
                                ViewportAgent* agent,
                                const base::Vec2i& viewport_size,
                                const base::Vec2i& offset) {
  renderer::WorldMatrixUniform world_matrix;
  renderer::MakeProjectionMatrix(world_matrix.projection, viewport_size,
                                 offset);
  renderer::MakeIdentityMatrix(world_matrix.transform);

  encoder->WriteBuffer(agent->world_uniform, 0,
                       reinterpret_cast<uint8_t*>(&world_matrix),
                       sizeof(world_matrix));
}

void GPUResetViewportRegion(wgpu::RenderPassEncoder* agent,
                            const base::Rect& region) {
  agent->SetScissorRect(region.x, region.y, region.width, region.height);
}

void GPUApplyViewportEffectAndRestore(
    renderer::RenderDevice* device,
    wgpu::CommandEncoder* command_encoder,
    renderer::QuadrangleIndexCache* index_cache,
    ViewportAgent* agent,
    wgpu::Texture* screen_buffer,
    const base::Rect& effect_region,
    const base::Vec4& color,
    const base::Vec4& tone,
    wgpu::RenderPassEncoder* last_renderpass,
    const base::Rect& last_viewport) {
  last_renderpass->End();

  wgpu::RenderPassColorAttachment attachment;
  attachment.view = screen_buffer->CreateView();
  attachment.loadOp = wgpu::LoadOp::Load;
  attachment.storeOp = wgpu::StoreOp::Store;

  wgpu::RenderPassDescriptor renderpass;
  renderpass.colorAttachmentCount = 1;
  renderpass.colorAttachments = &attachment;

  *last_renderpass = command_encoder->BeginRenderPass(&renderpass);
  last_renderpass->SetScissorRect(last_viewport.x, last_viewport.y,
                                  last_viewport.width, last_viewport.height);
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
    : GraphicsChild(screen),
      Disposable(screen),
      node_(SortKey()),
      region_(region),
      color_(new ColorImpl(base::Vec4())),
      tone_(new ToneImpl(base::Vec4())) {
  node_.RegisterEventHandler(base::BindRepeating(
      &ViewportImpl::DrawableNodeHandlerInternal, base::Unretained(this)));
  node_.RebindController(screen->GetDrawableController());

  agent_ = new ViewportAgent;
  screen->PostTask(
      base::BindOnce(&GPUCreateViewportAgent, screen->GetDevice(), agent_));
}

ViewportImpl::~ViewportImpl() {
  ExceptionState exception_state;
  Dispose(exception_state);
}

scoped_refptr<ViewportImpl> ViewportImpl::From(scoped_refptr<Viewport> host) {
  return static_cast<ViewportImpl*>(host.get());
}

void ViewportImpl::Dispose(ExceptionState& exception_state) {
  Disposable::Dispose(exception_state);
}

bool ViewportImpl::IsDisposed(ExceptionState& exception_state) {
  return Disposable::IsDisposed(exception_state);
}

void ViewportImpl::Flash(scoped_refptr<Color> color,
                         uint32_t duration,
                         ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  std::optional<base::Vec4> flash_color = std::nullopt;
  if (color)
    flash_color = ColorImpl::From(color)->AsNormColor();
  flash_emitter_.Setup(flash_color, duration);
}

void ViewportImpl::Update(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  flash_emitter_.Update();
}

scoped_refptr<Rect> ViewportImpl::Get_Rect(ExceptionState& exception_state) {
  return new RectImpl(region_);
}

void ViewportImpl::Put_Rect(const scoped_refptr<Rect>& value,
                            ExceptionState& exception_state) {
  region_ = RectImpl::From(value)->AsBaseRect();
}

bool ViewportImpl::Get_Visible(ExceptionState& exception_state) {
  return node_.GetVisibility();
}

void ViewportImpl::Put_Visible(const bool& value,
                               ExceptionState& exception_state) {
  node_.SetNodeVisibility(value);
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
  *color_ = *ColorImpl::From(value);
}

scoped_refptr<Tone> ViewportImpl::Get_Tone(ExceptionState& exception_state) {
  return tone_;
}

void ViewportImpl::Put_Tone(const scoped_refptr<Tone>& value,
                            ExceptionState& exception_state) {
  *tone_ = *ToneImpl::From(value);
}

void ViewportImpl::OnObjectDisposed() {
  node_.DisposeNode();

  screen()->PostTask(base::BindOnce(&GPUDestroyViewportAgent, agent_));
  agent_ = nullptr;
}

void ViewportImpl::DrawableNodeHandlerInternal(
    DrawableNode::RenderStage stage,
    DrawableNode::RenderControllerParams* params) {
  // Stack storage last render state
  DrawableNode::RenderControllerParams transient_params = *params;

  if (stage == DrawableNode::kBeforeRender) {
    // Calculate viewport offset
    base::Vec2i viewport_position = region_.Position() - origin_;

    // Update uniform buffer if viewport region changed.
    if (!(region_ == agent_->region_cache)) {
      agent_->region_cache = region_;
      screen()->PostTask(base::BindOnce(
          &GPUUpdateViewportAgentData, params->device, params->command_encoder,
          agent_, params->viewport_size, viewport_position));
    }
  }

  if (transient_params.renderpass_encoder) {
    // Scissor region
    base::Rect scissor_region =
        base::MakeIntersect(params->clip_region, region_);

    // Reset viewport's world settings
    transient_params.world_binding = &agent_->world_binding;
    transient_params.clip_region = scissor_region;

    // Adjust visibility
    if (transient_params.clip_region.width <= 0 ||
        transient_params.clip_region.height <= 0)
      return;

    // Set new viewport
    screen()->PostTask(base::BindOnce(
        &GPUResetViewportRegion, params->renderpass_encoder, scissor_region));
  }

  // Notify children drawable node
  controller_.BroadCastNotification(stage, &transient_params);

  // Restore last viewport settings and apply effect
  if (params->renderpass_encoder)
    screen()->PostTask(base::BindOnce(
        &GPUApplyViewportEffectAndRestore, params->device,
        params->command_encoder, params->index_cache, agent_,
        params->screen_buffer, region_, color_->AsNormColor(),
        tone_->AsNormColor(), params->renderpass_encoder, params->clip_region));
}

}  // namespace content
