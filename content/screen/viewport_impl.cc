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

  agent->effect.uniform_buffer =
      renderer::CreateUniformBuffer<renderer::ViewportFragmentUniform>(
          **device, "viewport.uniform", wgpu::BufferUsage::CopyDst);
  agent->effect.vertex_buffer =
      renderer::CreateVertexBuffer<renderer::FullVertexLayout>(
          **device, "viewport.effect", wgpu::BufferUsage::CopyDst, 4);
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
                                const base::Vec2i& viewport_size,
                                const base::Vec2i& offset,
                                const base::Vec2i& effect_size) {
  renderer::WorldMatrixUniform world_matrix;
  renderer::MakeProjectionMatrix(world_matrix.projection, viewport_size,
                                 offset);
  renderer::MakeIdentityMatrix(world_matrix.transform);

  encoder->WriteBuffer(agent->world_uniform, 0,
                       reinterpret_cast<uint8_t*>(&world_matrix),
                       sizeof(world_matrix));

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
  bool is_effect_valid =
      color.w != 0 || tone.x != 0 || tone.y != 0 || tone.z != 0 || tone.w != 0;

  if (is_effect_valid) {
    last_renderpass->End();

    wgpu::ImageCopyTexture src_texture, dst_texture;
    wgpu::Extent3D extent;
    src_texture.texture = *screen_buffer;
    src_texture.origin.x = effect_region.x;
    src_texture.origin.y = effect_region.y;
    dst_texture.texture = agent->effect.intermediate_layer;
    extent.width = effect_region.width;
    extent.height = effect_region.height;
    command_encoder->CopyTextureToTexture(&src_texture, &dst_texture, &extent);

    renderer::FullVertexLayout transient_vertices[4];
    renderer::FullVertexLayout::SetPositionRect(transient_vertices,
                                                effect_region);
    renderer::FullVertexLayout::SetTexCoordRect(
        transient_vertices, base::Rect(effect_region.Size()));
    command_encoder->WriteBuffer(agent->effect.vertex_buffer, 0,
                                 reinterpret_cast<uint8_t*>(transient_vertices),
                                 sizeof(transient_vertices));

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
    auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::kNoBlend);

    wgpu::RenderPassEncoder pass =
        command_encoder->BeginRenderPass(&renderpass);
    pass.SetPipeline(*pipeline);
    pass.SetVertexBuffer(0, agent->effect.vertex_buffer);
    pass.SetIndexBuffer(**index_cache, index_cache->format());
    pass.SetBindGroup(0, agent->world_binding);
    pass.SetBindGroup(1, agent->effect.layer_binding);
    pass.SetBindGroup(2, agent->effect.uniform_binding);
    pass.DrawIndexed(6);
    pass.End();

    *last_renderpass = command_encoder->BeginRenderPass(&renderpass);
  }

  // Restore viewport region
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
          agent_, params->viewport_size, viewport_position, region_.Size()));
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
  if (params->renderpass_encoder) {
    base::Vec4 composite_color = color_->AsNormColor();
    base::Vec4 flash_color = flash_emitter_.GetColor();
    base::Vec4 target_color = composite_color;
    if (flash_emitter_.IsFlashing())
      target_color =
          (flash_color.w > composite_color.w ? flash_color : composite_color);

    screen()->PostTask(base::BindOnce(
        &GPUApplyViewportEffectAndRestore, params->device,
        params->command_encoder, params->index_cache, agent_,
        params->screen_buffer, region_, target_color, tone_->AsNormColor(),
        params->renderpass_encoder, params->clip_region));
  }
}

}  // namespace content
