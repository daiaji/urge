// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/render/plane_impl.h"

#include <algorithm>

namespace content {

namespace {

float fwrap(float value, float range) {
  float res = std::fmod(value, range);
  return res < 0 ? res + range : res;
}

void GPUCreatePlaneInternal(renderer::RenderDevice* device, PlaneAgent* agent) {
  agent->quad_controller = renderer::FullQuadController::Make(device, 0);
  agent->uniform_buffer =
      renderer::CreateUniformBuffer<renderer::ViewportFragmentUniform>(
          **device, "plane.uniform", wgpu::BufferUsage::CopyDst);
  agent->uniform_binding = renderer::ViewportFragmentUniform::CreateGroup(
      **device, agent->uniform_buffer);
}

void GPUDestroyPlaneInternal(PlaneAgent* agent) {
  agent->quad_controller.reset();
  delete agent;
}

void GPUUpdatePlaneQuadArrayInternal(
    wgpu::CommandEncoder* command_encoder,
    renderer::QuadrangleIndexCache* index_cache,
    PlaneAgent* agent,
    TextureAgent* texture,
    const base::Vec2i& viewport_size,
    const base::Vec2& scale,
    const base::Vec2i& origin,
    const base::Vec4& color,
    const base::Vec4& tone,
    int32_t opacity) {
  const float item_x =
      std::max(1.0f, static_cast<float>(texture->size.x * scale.x));
  const float item_y =
      std::max(1.0f, static_cast<float>(texture->size.y * scale.y));

  const float wrap_ox = fwrap(origin.x, item_x);
  const float wrap_oy = fwrap(origin.y, item_y);

  const int tile_x =
      std::ceil((viewport_size.x + wrap_ox - item_x) / item_x) + 1;
  const int tile_y =
      std::ceil((viewport_size.y + wrap_oy - item_y) / item_y) + 1;

  const int quad_size = tile_x * tile_y;
  agent->vertices.resize(quad_size * 4);

  base::Vec4 opacity_norm(static_cast<float>(opacity) / 255.0f);

  for (int y = 0; y < tile_y; ++y) {
    for (int x = 0; x < tile_x; ++x) {
      size_t index = (y * tile_x + x) * 4;
      renderer::FullVertexLayout* vert = &agent->vertices[index];
      base::RectF pos(x * item_x - wrap_ox, y * item_y - wrap_oy, item_x,
                      item_y);

      renderer::FullVertexLayout::SetPositionRect(vert, pos);
      renderer::FullVertexLayout::SetTexCoordRect(vert,
                                                  base::Vec2(item_x, item_y));
      renderer::FullVertexLayout::SetColor(vert, opacity_norm);
    }
  }

  index_cache->Allocate(quad_size);
  agent->quad_size = quad_size;
  agent->quad_controller->QueueWrite(*command_encoder, agent->vertices.data(),
                                     agent->vertices.size());

  renderer::ViewportFragmentUniform transient_uniform;
  transient_uniform.color = color;
  transient_uniform.tone = tone;
  command_encoder->WriteBuffer(agent->uniform_buffer, 0,
                               reinterpret_cast<uint8_t*>(&transient_uniform),
                               sizeof(transient_uniform));
}

void GPUOnViewportRenderingInternal(renderer::RenderDevice* device,
                                    wgpu::RenderPassEncoder* encoder,
                                    renderer::QuadrangleIndexCache* index_cache,
                                    wgpu::BindGroup* world_binding,
                                    PlaneAgent* agent,
                                    TextureAgent* texture,
                                    int32_t blend_type) {
  auto& pipeline_set = device->GetPipelines()->viewport;
  auto* pipeline =
      pipeline_set.GetPipeline(static_cast<renderer::BlendType>(blend_type));

  encoder->SetPipeline(*pipeline);
  encoder->SetVertexBuffer(0, **agent->quad_controller);
  encoder->SetIndexBuffer(**index_cache, index_cache->format());
  encoder->SetBindGroup(0, *world_binding);
  encoder->SetBindGroup(1, texture->binding);
  encoder->SetBindGroup(2, agent->uniform_binding);
  encoder->DrawIndexed(agent->quad_size * 6);
}

}  // namespace

scoped_refptr<Plane> Plane::New(ExecutionContext* execution_context,
                                scoped_refptr<Viewport> viewport,
                                ExceptionState& exception_state) {
  return new PlaneImpl(execution_context->graphics,
                       ViewportImpl::From(viewport));
}

PlaneImpl::PlaneImpl(RenderScreenImpl* screen,
                     scoped_refptr<ViewportImpl> parent)
    : GraphicsChild(screen),
      Disposable(screen),
      node_(SortKey()),
      viewport_(parent),
      color_(new ColorImpl(base::Vec4())),
      tone_(new ToneImpl(base::Vec4())) {
  node_.RebindController(parent ? parent->GetDrawableController()
                                : screen->GetDrawableController());
  node_.RegisterEventHandler(base::BindRepeating(
      &PlaneImpl::DrawableNodeHandlerInternal, base::Unretained(this)));

  agent_ = new PlaneAgent;
  screen->PostTask(
      base::BindOnce(&GPUCreatePlaneInternal, screen->GetDevice(), agent_));
}

PlaneImpl::~PlaneImpl() {
  ExceptionState exception_state;
  Dispose(exception_state);
}

void PlaneImpl::Dispose(ExceptionState& exception_state) {
  Disposable::Dispose(exception_state);
}

bool PlaneImpl::IsDisposed(ExceptionState& exception_state) {
  return Disposable::IsDisposed(exception_state);
}

scoped_refptr<Bitmap> PlaneImpl::Get_Bitmap(ExceptionState& exception_state) {
  return bitmap_;
}

void PlaneImpl::Put_Bitmap(const scoped_refptr<Bitmap>& value,
                           ExceptionState& exception_state) {
  bitmap_ = CanvasImpl::FromBitmap(value);
  quad_array_dirty_ = true;
}

scoped_refptr<Viewport> PlaneImpl::Get_Viewport(
    ExceptionState& exception_state) {
  return viewport_;
}

void PlaneImpl::Put_Viewport(const scoped_refptr<Viewport>& value,
                             ExceptionState& exception_state) {
  viewport_ = ViewportImpl::From(value);
  node_.RebindController(viewport_->GetDrawableController());
  quad_array_dirty_ = true;
}

bool PlaneImpl::Get_Visible(ExceptionState& exception_state) {
  return node_.GetVisibility();
}

void PlaneImpl::Put_Visible(const bool& value,
                            ExceptionState& exception_state) {
  node_.SetNodeVisibility(value);
}

int32_t PlaneImpl::Get_Z(ExceptionState& exception_state) {
  return node_.GetSortKeys()->weight[0];
}

void PlaneImpl::Put_Z(const int32_t& value, ExceptionState& exception_state) {
  node_.SetNodeSortWeight(value);
}

int32_t PlaneImpl::Get_Ox(ExceptionState& exception_state) {
  return origin_.x;
}

void PlaneImpl::Put_Ox(const int32_t& value, ExceptionState& exception_state) {
  origin_.x = value;
  quad_array_dirty_ = true;
}

int32_t PlaneImpl::Get_Oy(ExceptionState& exception_state) {
  return origin_.y;
}

void PlaneImpl::Put_Oy(const int32_t& value, ExceptionState& exception_state) {
  origin_.y = value;
  quad_array_dirty_ = true;
}

float PlaneImpl::Get_ZoomX(ExceptionState& exception_state) {
  return scale_.x;
}

void PlaneImpl::Put_ZoomX(const float& value, ExceptionState& exception_state) {
  scale_.x = value;
  quad_array_dirty_ = true;
}

float PlaneImpl::Get_ZoomY(ExceptionState& exception_state) {
  return scale_.y;
}

void PlaneImpl::Put_ZoomY(const float& value, ExceptionState& exception_state) {
  scale_.y = value;
  quad_array_dirty_ = true;
}

uint32_t PlaneImpl::Get_Opacity(ExceptionState& exception_state) {
  return opacity_;
}

void PlaneImpl::Put_Opacity(const uint32_t& value,
                            ExceptionState& exception_state) {
  opacity_ = value;
}

uint32_t PlaneImpl::Get_BlendType(ExceptionState& exception_state) {
  return blend_type_;
}

void PlaneImpl::Put_BlendType(const uint32_t& value,
                              ExceptionState& exception_state) {
  blend_type_ = value;
}

scoped_refptr<Color> PlaneImpl::Get_Color(ExceptionState& exception_state) {
  return color_;
}

void PlaneImpl::Put_Color(const scoped_refptr<Color>& value,
                          ExceptionState& exception_state) {
  color_ = ColorImpl::From(value);
}

scoped_refptr<Tone> PlaneImpl::Get_Tone(ExceptionState& exception_state) {
  return tone_;
}

void PlaneImpl::Put_Tone(const scoped_refptr<Tone>& value,
                         ExceptionState& exception_state) {
  tone_ = ToneImpl::From(value);
}

void PlaneImpl::OnObjectDisposed() {
  screen()->PostTask(base::BindOnce(&GPUDestroyPlaneInternal, agent_));
  agent_ = nullptr;
}

void PlaneImpl::DrawableNodeHandlerInternal(
    DrawableNode::RenderStage stage,
    DrawableNode::RenderControllerParams* params) {
  if (!bitmap_)
    return;

  if (stage == DrawableNode::RenderStage::kBeforeRender) {
    if (quad_array_dirty_) {
      quad_array_dirty_ = false;
      screen()->PostTask(base::BindOnce(
          &GPUUpdatePlaneQuadArrayInternal, params->command_encoder,
          params->index_cache, agent_, bitmap_->GetAgent(),
          params->viewport.Size(), scale_, origin_, color_->AsNormColor(),
          tone_->AsNormColor(), opacity_));
    }
  } else if (stage == DrawableNode::RenderStage::kOnRendering) {
    screen()->PostTask(base::BindOnce(
        &GPUOnViewportRenderingInternal, params->device,
        params->renderpass_encoder, screen()->GetCommonIndexBuffer(),
        params->world_binding, agent_, bitmap_->GetAgent(), blend_type_));
  }
}

}  // namespace content
