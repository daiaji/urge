// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/screen/viewport_impl.h"

#include "content/canvas/canvas_impl.h"
#include "content/canvas/canvas_scheduler.h"
#include "content/common/rect_impl.h"
#include "content/context/execution_context.h"
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

  agent->effect.uniform_buffer =
      renderer::CreateUniformBuffer<renderer::ViewportFragmentUniform>(
          **device, "viewport.uniform", wgpu::BufferUsage::CopyDst);
  agent->effect.vertex_buffer = renderer::CreateQuadBuffer(
      **device, "viewport.effect", wgpu::BufferUsage::CopyDst);
  agent->effect.uniform_binding =
      renderer::ViewportFragmentUniform::CreateGroup(
          **device, agent->effect.uniform_buffer);
}

void GPUDestroyViewportAgent(ViewportAgent* agent) {
  agent->world_binding = nullptr;
  agent->world_uniform = nullptr;

  delete agent;
}

void GPUUpdateViewportAgentData(renderer::RenderDevice* device,
                                wgpu::CommandEncoder* encoder,
                                ViewportAgent* agent,
                                const base::Vec2i& screen_size,
                                const base::Vec2i& viewport_offset,
                                const base::Vec2i& effect_size) {
  // Reload viewport offset based transform matrix
  renderer::WorldMatrixUniform world_matrix;
  renderer::MakeProjectionMatrix(world_matrix.projection, screen_size,
                                 viewport_offset);
  renderer::MakeIdentityMatrix(world_matrix.transform);

  encoder->WriteBuffer(agent->world_uniform, 0,
                       reinterpret_cast<uint8_t*>(&world_matrix),
                       sizeof(world_matrix));

  // Reset viewport effect intermediate texture layer if needed
  auto effect_layer = agent->effect.intermediate_layer;
  if (!effect_layer || effect_layer.GetWidth() < effect_size.x ||
      effect_layer.GetHeight() < effect_size.y) {
    base::Vec2i new_size;
    new_size.x = std::max<int32_t>(effect_size.x,
                                   effect_layer ? effect_layer.GetWidth() : 0);
    new_size.y = std::max<int32_t>(effect_size.y,
                                   effect_layer ? effect_layer.GetHeight() : 0);
    agent->effect.intermediate_layer = renderer::CreateTexture2D(
        **device, "viewport.intermediate",
        wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding,
        new_size);

    renderer::TextureBindingUniform binding_uniform;
    binding_uniform.texture_size = base::MakeInvert(new_size);
    auto texture_uniform_buffer =
        renderer::CreateUniformBuffer<renderer::TextureBindingUniform>(
            **device, "viewport.layer.size", wgpu::BufferUsage::None,
            &binding_uniform);

    agent->effect.layer_binding = renderer::TextureBindingUniform::CreateGroup(
        **device, agent->effect.intermediate_layer.CreateView(),
        (*device)->CreateSampler(), texture_uniform_buffer);
  }
}

void GPUResetViewportRegion(wgpu::RenderPassEncoder* agent,
                            const base::Rect& region) {
  agent->SetScissorRect(region.x, region.y, region.width, region.height);
}

void GPUApplyViewportEffect(renderer::RenderDevice* device,
                            wgpu::CommandEncoder* command_encoder,
                            wgpu::Texture* screen_buffer,
                            wgpu::BindGroup* root_world,
                            ViewportAgent* agent,
                            const base::Rect& effect_region,
                            const base::Vec4& color,
                            const base::Vec4& tone) {
  if (effect_region.x > screen_buffer->GetWidth() ||
      effect_region.y > screen_buffer->GetHeight())
    return;

  wgpu::TexelCopyTextureInfo src_texture, dst_texture;
  wgpu::Extent3D extent;
  src_texture.texture = *screen_buffer;
  src_texture.origin.x = effect_region.x;
  src_texture.origin.y = effect_region.y;
  dst_texture.texture = agent->effect.intermediate_layer;
  extent.width = std::min<uint32_t>(
      effect_region.width, screen_buffer->GetWidth() - effect_region.x);
  extent.height = std::min<uint32_t>(
      effect_region.height, screen_buffer->GetHeight() - effect_region.y);
  command_encoder->CopyTextureToTexture(&src_texture, &dst_texture, &extent);

  renderer::Quad transient_quad;
  renderer::Quad::SetPositionRect(&transient_quad, effect_region);
  renderer::Quad::SetTexCoordRect(&transient_quad,
                                  base::Rect(effect_region.Size()));
  command_encoder->WriteBuffer(agent->effect.vertex_buffer, 0,
                               reinterpret_cast<uint8_t*>(&transient_quad),
                               sizeof(transient_quad));

  renderer::ViewportFragmentUniform transient_uniform;
  transient_uniform.color = color;
  transient_uniform.tone = tone;
  command_encoder->WriteBuffer(agent->effect.uniform_buffer, 0,
                               reinterpret_cast<uint8_t*>(&transient_uniform),
                               sizeof(transient_uniform));

  wgpu::RenderPassColorAttachment attachment;
  attachment.view = screen_buffer->CreateView();
  attachment.loadOp = wgpu::LoadOp::Load;
  attachment.storeOp = wgpu::StoreOp::Store;

  wgpu::RenderPassDescriptor renderpass;
  renderpass.colorAttachmentCount = 1;
  renderpass.colorAttachments = &attachment;

  auto& pipeline_set = device->GetPipelines()->viewport;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NO_BLEND);

  wgpu::RenderPassEncoder pass = command_encoder->BeginRenderPass(&renderpass);
  pass.SetPipeline(*pipeline);
  pass.SetVertexBuffer(0, agent->effect.vertex_buffer);
  pass.SetIndexBuffer(**device->GetQuadIndex(),
                      device->GetQuadIndex()->format());
  pass.SetBindGroup(0, *root_world);
  pass.SetBindGroup(1, agent->effect.layer_binding);
  pass.SetBindGroup(2, agent->effect.uniform_binding);
  pass.DrawIndexed(6);
  pass.End();
}

void GPUViewportProcessAfterRender(renderer::RenderDevice* device,
                                   wgpu::CommandEncoder* command_encoder,
                                   wgpu::RenderPassEncoder* last_renderpass,
                                   wgpu::BindGroup* root_world,
                                   const base::Rect& last_viewport,
                                   wgpu::Texture* screen_buffer,
                                   ViewportAgent* agent,
                                   const base::Rect& effect_region,
                                   const base::Vec4& color,
                                   const base::Vec4& tone) {
  bool is_effect_valid =
      color.w != 0 || tone.x != 0 || tone.y != 0 || tone.z != 0 || tone.w != 0;

  if (is_effect_valid) {
    last_renderpass->End();

    GPUApplyViewportEffect(device, command_encoder, screen_buffer, root_world,
                           agent, effect_region, color, tone);

    wgpu::RenderPassColorAttachment attachment;
    attachment.view = screen_buffer->CreateView();
    attachment.loadOp = wgpu::LoadOp::Load;
    attachment.storeOp = wgpu::StoreOp::Store;

    wgpu::RenderPassDescriptor renderpass;
    renderpass.colorAttachmentCount = 1;
    renderpass.colorAttachments = &attachment;

    *last_renderpass = command_encoder->BeginRenderPass(&renderpass);
  }

  // Restore viewport region
  last_renderpass->SetScissorRect(last_viewport.x, last_viewport.y,
                                  last_viewport.width, last_viewport.height);
}

void GPUFrameBeginRenderPassInternal(renderer::RenderDevice* device,
                                     renderer::DeviceContext* context,
                                     ViewportAgent* agent,
                                     wgpu::Texture* render_target,
                                     const base::Vec2i& offset) {
  auto* encoder = context->GetImmediateEncoder();
  base::Vec2i target_size(render_target->GetWidth(),
                          render_target->GetHeight());

  // Reload viewport offset based transform matrix
  renderer::WorldMatrixUniform world_matrix;
  renderer::MakeProjectionMatrix(world_matrix.projection, target_size, offset);
  renderer::MakeIdentityMatrix(world_matrix.transform);

  encoder->WriteBuffer(agent->world_uniform, 0,
                       reinterpret_cast<uint8_t*>(&world_matrix),
                       sizeof(world_matrix));

  wgpu::RenderPassColorAttachment attachment;
  attachment.view = render_target->CreateView();
  attachment.loadOp = wgpu::LoadOp::Clear;
  attachment.storeOp = wgpu::StoreOp::Store;
  attachment.clearValue = {0, 0, 0, 1};

  wgpu::RenderPassDescriptor renderpass;
  renderpass.colorAttachmentCount = 1;
  renderpass.colorAttachments = &attachment;

  // Begin render pass
  agent->render_pass = encoder->BeginRenderPass(&renderpass);
  agent->render_pass.SetViewport(0, 0, target_size.x, target_size.y, 0, 0);
  agent->render_pass.SetScissorRect(0, 0, target_size.x, target_size.y);
}

void GPUFrameEndRenderPassInternal(renderer::RenderDevice* device,
                                   renderer::DeviceContext* context,
                                   ViewportAgent* agent,
                                   wgpu::Texture* render_target,
                                   wgpu::BindGroup* root_world,
                                   const base::Rect& effect_region,
                                   const base::Vec4& color,
                                   const base::Vec4& tone) {
  // End render pass
  agent->render_pass.End();

  // Apply viewport effect
  GPUApplyViewportEffect(device, context->GetImmediateEncoder(), render_target,
                         root_world, agent, effect_region, color, tone);
}

}  // namespace

scoped_refptr<Viewport> Viewport::New(ExecutionContext* execution_context,
                                      ExceptionState& exception_state) {
  return new ViewportImpl(execution_context->graphics, nullptr,
                          execution_context->graphics->GetResolution());
}

scoped_refptr<Viewport> Viewport::New(ExecutionContext* execution_context,
                                      scoped_refptr<Rect> rect,
                                      ExceptionState& exception_state) {
  return new ViewportImpl(execution_context->graphics, nullptr,
                          RectImpl::From(rect)->AsBaseRect());
}

scoped_refptr<Viewport> Viewport::New(ExecutionContext* execution_context,
                                      int32_t x,
                                      int32_t y,
                                      int32_t width,
                                      int32_t height,
                                      ExceptionState& exception_state) {
  return new ViewportImpl(execution_context->graphics, nullptr,
                          base::Rect(x, y, width, height));
}

scoped_refptr<Viewport> Viewport::New(ExecutionContext* execution_context,
                                      scoped_refptr<Viewport> parent,
                                      scoped_refptr<Rect> rect,
                                      ExceptionState& exception_state) {
  scoped_refptr<RectImpl> region = RectImpl::From(rect);
  return new ViewportImpl(execution_context->graphics,
                          ViewportImpl::From(parent), region->AsBaseRect());
}

ViewportImpl::ViewportImpl(RenderScreenImpl* screen,
                           scoped_refptr<ViewportImpl> parent,
                           const base::Rect& region)
    : GraphicsChild(screen),
      Disposable(screen),
      node_(parent ? parent->GetDrawableController()
                   : screen->GetDrawableController(),
            SortKey()),
      rect_(new RectImpl(region)),
      color_(new ColorImpl(base::Vec4())),
      tone_(new ToneImpl(base::Vec4())) {
  node_.RegisterEventHandler(base::BindRepeating(
      &ViewportImpl::DrawableNodeHandlerInternal, base::Unretained(this)));

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

void ViewportImpl::Render(scoped_refptr<Bitmap> target,
                          ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  scoped_refptr<CanvasImpl> render_target = CanvasImpl::FromBitmap(target);
  TextureAgent* bitmap_agent =
      render_target ? render_target->GetAgent() : nullptr;
  if (!bitmap_agent) {
    exception_state.ThrowContentError(
        ExceptionCode::CONTENT_ERROR,
        "Viewport.Render: Invalid bitmap as render target.");
    return;
  }

  // Submit pending canvas commands
  screen()->GetCanvasScheduler()->SubmitPendingPaintCommands();

  // Prepare for rendering context
  DrawableNode::RenderControllerParams controller_params;
  controller_params.device = screen()->GetDevice();
  controller_params.command_encoder =
      screen()->GetContext()->GetImmediateEncoder();
  controller_params.screen_buffer = &bitmap_agent->data;
  controller_params.screen_size = bitmap_agent->size;
  controller_params.viewport = bitmap_agent->size;

  // 1) Execute pre-composite handler
  controller_.BroadCastNotification(DrawableNode::BEFORE_RENDER,
                                    &controller_params);

  // 2) Setup renderpass
  base::Rect viewport_rect = rect_->AsBaseRect();
  base::Vec2i offset = viewport_rect.Position() - origin_;
  agent_->region_cache = base::Rect(offset, bitmap_agent->size);
  screen()->PostTask(base::BindOnce(
      &GPUFrameBeginRenderPassInternal, screen()->GetDevice(),
      screen()->GetContext(), agent_, &bitmap_agent->data, offset));

  // 3) Notify render a frame
  controller_params.root_world = &bitmap_agent->world;
  controller_params.world_binding = &agent_->world_binding;
  controller_params.renderpass_encoder = &agent_->render_pass;
  controller_.BroadCastNotification(DrawableNode::ON_RENDERING,
                                    &controller_params);

  // 4) End render pass and process after-render effect
  base::Vec4 composite_color = color_->AsNormColor();
  base::Vec4 flash_color = flash_emitter_.GetColor();
  base::Vec4 target_color = composite_color;
  if (flash_emitter_.IsFlashing())
    target_color =
        (flash_color.w > composite_color.w ? flash_color : composite_color);

  screen()->PostTask(base::BindOnce(
      &GPUFrameEndRenderPassInternal, screen()->GetDevice(),
      screen()->GetContext(), agent_, &bitmap_agent->data, &bitmap_agent->world,
      viewport_rect, target_color, tone_->AsNormColor()));

  screen()->WaitWorkerSynchronize();
}

scoped_refptr<Viewport> ViewportImpl::Get_Viewport(
    ExceptionState& exception_state) {
  return viewport_;
}

void ViewportImpl::Put_Viewport(const scoped_refptr<Viewport>& value,
                                ExceptionState& exception_state) {
  viewport_ = ViewportImpl::From(value);
  node_.RebindController(viewport_->GetDrawableController());
}

scoped_refptr<Rect> ViewportImpl::Get_Rect(ExceptionState& exception_state) {
  return rect_;
}

void ViewportImpl::Put_Rect(const scoped_refptr<Rect>& value,
                            ExceptionState& exception_state) {
  *rect_ = *RectImpl::From(value);
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

  // Stack of viewport region rect
  base::Rect viewport_rect = rect_->AsBaseRect();
  viewport_rect.x += params->viewport.x;
  viewport_rect.y += params->viewport.y;
  transient_params.viewport = viewport_rect;
  transient_params.origin = origin_;

  if (stage == DrawableNode::BEFORE_RENDER) {
    // Calculate viewport offset
    base::Vec2i offset = viewport_rect.Position() - origin_;

    // Update uniform buffer if viewport region changed.
    base::Rect cache_rect(offset, viewport_rect.Size());
    if (!(agent_->region_cache == cache_rect)) {
      agent_->region_cache = cache_rect;
      screen()->PostTask(base::BindOnce(
          &GPUUpdateViewportAgentData, params->device, params->command_encoder,
          agent_, params->screen_size, offset, viewport_rect.Size()));
    }
  }

  if (transient_params.renderpass_encoder) {
    // Interact with last scissor region
    base::Rect scissor_region =
        base::MakeIntersect(params->viewport, viewport_rect);

    // Reset viewport's world settings
    transient_params.world_binding = &agent_->world_binding;

    // Adjust visibility
    if (scissor_region.width <= 0 || scissor_region.height <= 0)
      return;

    // Set new viewport
    screen()->PostTask(base::BindOnce(
        &GPUResetViewportRegion, params->renderpass_encoder, scissor_region));
  }

  // Notify children drawable node
  controller_.BroadCastNotification(stage, &transient_params);

  // Restore last viewport settings and apply effect
  if (params->renderpass_encoder) {
    base::Vec4 composite_color = color_->AsNormColor();
    base::Vec4 flash_color = flash_emitter_.GetColor();
    base::Vec4 target_color = composite_color;
    if (flash_emitter_.IsFlashing())
      target_color =
          (flash_color.w > composite_color.w ? flash_color : composite_color);

    screen()->PostTask(base::BindOnce(
        &GPUViewportProcessAfterRender, params->device, params->command_encoder,
        params->renderpass_encoder, params->root_world, params->viewport,
        params->screen_buffer, agent_, viewport_rect, target_color,
        tone_->AsNormColor()));
  }
}

}  // namespace content
