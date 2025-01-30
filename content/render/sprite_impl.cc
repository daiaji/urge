// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/render/sprite_impl.h"

#include "content/screen/renderscreen_impl.h"
#include "content/screen/viewport_impl.h"

namespace content {

namespace {

void GPUCreateSpriteInternal(renderer::RenderDevice* device,
                             SpriteAgent* agent) {
  wgpu::BufferDescriptor buffer_desc;
  buffer_desc.label = "sprite.vertex.buffer";
  buffer_desc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
  buffer_desc.size = sizeof(renderer::FullVertexLayout) * 4;
  buffer_desc.mappedAtCreation = false;
  agent->vertex_buffer = (*device)->CreateBuffer(&buffer_desc);
}

void GPUDestroySpriteInternal(SpriteAgent* agent) {
  agent->vertex_buffer = nullptr;
  delete agent;
}

void GPUUpdateSpriteVerticesInternal(
    wgpu::CommandEncoder* encoder,
    SpriteAgent* agent,
    const SpriteImpl::SpriteTransform& transform) {
  renderer::FullVertexLayout vertices[4] = {
      renderer::FullVertexLayout{
          base::Vec4i{transform.position.x, transform.position.y, 0, 1},
          base::Vec2i{transform.texcoord.x, transform.texcoord.y},
          base::Vec4{0, 0, 0, 1},
      },
      renderer::FullVertexLayout{
          base::Vec4i{transform.position.x + transform.position.width,
                      transform.position.y, 0, 1},
          base::Vec2i{transform.texcoord.x + transform.texcoord.width,
                      transform.texcoord.y},
          base::Vec4{0, 0, 0, 1},
      },
      renderer::FullVertexLayout{
          base::Vec4i{transform.position.x + transform.position.width,
                      transform.position.y + transform.position.height, 0, 1},
          base::Vec2i{transform.texcoord.x + transform.texcoord.width,
                      transform.texcoord.y + transform.texcoord.height},
          base::Vec4{0, 0, 0, 1},
      },
      renderer::FullVertexLayout{
          base::Vec4i{transform.position.x,
                      transform.position.y + transform.position.height, 0, 1},
          base::Vec2i{transform.texcoord.x,
                      transform.texcoord.y + transform.texcoord.height},
          base::Vec4{0, 0, 0, 1},
      },
  };

  float angle = transform.rotation;
  if (angle >= 360 || angle < -360)
    angle = std::fmod(angle, 360.f);
  if (angle < 0)
    angle += 360;

  angle = angle * 3.141592654f / 180.0f;
  float cosine = std::cos(angle);
  float sine = std::sin(angle);
  float sxc = transform.scale.x * cosine;
  float syc = transform.scale.y * cosine;
  float sxs = transform.scale.x * sine;
  float sys = transform.scale.y * sine;
  float tx =
      -transform.origin.x * sxc - transform.origin.y * sys + transform.offset.x;
  float ty =
      transform.origin.x * sxs - transform.origin.y * syc + transform.offset.y;

  for (int i = 0; i < _countof(vertices); ++i) {
    float tsx =
        sxc * vertices[i].position.x + sys * vertices[i].position.y + tx;
    float tsy =
        -sxs * vertices[i].position.x + syc * vertices[i].position.y + ty;

    vertices[i].position.x = tsx;
    vertices[i].position.y = tsy;
  }

  encoder->WriteBuffer(agent->vertex_buffer, 0,
                       reinterpret_cast<uint8_t*>(vertices), sizeof(vertices));
}

void GPUOnSpriteRenderingInternal(renderer::RenderDevice* device,
                                  renderer::QuadrangleIndexCache* index_cache,
                                  wgpu::RenderPassEncoder* renderpass_encoder,
                                  wgpu::BindGroup* world_binding,
                                  SpriteAgent* agent,
                                  TextureAgent* texture,
                                  int32_t blend_type) {
  auto& pipeline_set = device->GetPipelines()->base;
  auto* pipeline =
      pipeline_set.GetPipeline(static_cast<renderer::BlendType>(blend_type));

  renderpass_encoder->SetPipeline(*pipeline);
  renderpass_encoder->SetVertexBuffer(0, agent->vertex_buffer);
  renderpass_encoder->SetIndexBuffer(**index_cache, index_cache->format());
  renderpass_encoder->SetBindGroup(0, *world_binding);
  renderpass_encoder->SetBindGroup(1, texture->binding);
  renderpass_encoder->DrawIndexed(6);
}

}  // namespace

scoped_refptr<Sprite> Sprite::New(ExecutionContext* execution_context,
                                  scoped_refptr<Viewport> viewport,
                                  ExceptionState& exception_state) {
  DrawNodeController* parent =
      execution_context->graphics->GetDrawableController();

  if (viewport)
    parent = ViewportImpl::From(viewport)->GetDrawableController();

  return new SpriteImpl(execution_context->graphics, parent);
}

SpriteImpl::SpriteImpl(RenderScreenImpl* screen, DrawNodeController* parent)
    : GraphicsChild(screen), Disposable(screen), node_(SortKey()) {
  node_.RebindController(parent);
  node_.RegisterEventHandler(base::BindRepeating(
      &SpriteImpl::DrawableNodeHandlerInternal, base::Unretained(this)));

  agent_ = new SpriteAgent;
  screen->PostTask(
      base::BindOnce(&GPUCreateSpriteInternal, screen->GetDevice(), agent_));
}

SpriteImpl::~SpriteImpl() {
  ExceptionState exception_state;
  Dispose(exception_state);
}

void SpriteImpl::Dispose(ExceptionState& exception_state) {
  Disposable::Dispose(exception_state);
}

bool SpriteImpl::IsDisposed(ExceptionState& exception_state) {
  return Disposable::IsDisposed(exception_state);
}

void SpriteImpl::Flash(scoped_refptr<Color> color,
                       uint32_t duration,
                       ExceptionState& exception_state) {}

void SpriteImpl::Update(ExceptionState& exception_state) {}

uint32_t SpriteImpl::Width(ExceptionState& exception_state) {
  return src_rect_.width;
}

uint32_t SpriteImpl::Height(ExceptionState& exception_state) {
  return src_rect_.height;
}

scoped_refptr<Bitmap> SpriteImpl::Get_Bitmap(ExceptionState& exception_state) {
  return bitmap_;
}

void SpriteImpl::Put_Bitmap(const scoped_refptr<Bitmap>& value,
                            ExceptionState& exception_state) {
  bitmap_ = CanvasImpl::FromBitmap(value);
  src_rect_ = bitmap_->AsBaseSize();
  transform_.position = src_rect_;
  transform_.texcoord = src_rect_;
  transform_.dirty = true;
}

scoped_refptr<Rect> SpriteImpl::Get_SrcRect(ExceptionState& exception_state) {
  return new RectImpl(src_rect_);
}

void SpriteImpl::Put_SrcRect(const scoped_refptr<Rect>& value,
                             ExceptionState& exception_state) {
  src_rect_ = RectImpl::From(value)->AsBaseRect();
  transform_.position = src_rect_.Size();
  transform_.texcoord = src_rect_;
  transform_.dirty = true;
}

scoped_refptr<Viewport> SpriteImpl::Get_Viewport(
    ExceptionState& exception_state) {
  return viewport_;
}

void SpriteImpl::Put_Viewport(const scoped_refptr<Viewport>& value,
                              ExceptionState& exception_state) {
  viewport_ = ViewportImpl::From(value);
  node_.RebindController(viewport_->GetDrawableController());
}

bool SpriteImpl::Get_Visible(ExceptionState& exception_state) {
  return node_.GetVisibility();
}

void SpriteImpl::Put_Visible(const bool& value,
                             ExceptionState& exception_state) {
  node_.SetNodeVisibility(value);
}

int32_t SpriteImpl::Get_X(ExceptionState& exception_state) {
  return transform_.offset.x;
}

void SpriteImpl::Put_X(const int32_t& value, ExceptionState& exception_state) {
  transform_.offset.x = value;
  transform_.dirty = true;
}

int32_t SpriteImpl::Get_Y(ExceptionState& exception_state) {
  return transform_.offset.y;
}

void SpriteImpl::Put_Y(const int32_t& value, ExceptionState& exception_state) {
  transform_.offset.y = value;
  transform_.dirty = true;
}

int32_t SpriteImpl::Get_Z(ExceptionState& exception_state) {
  return node_.GetSortKeys()->weight[0];
}

void SpriteImpl::Put_Z(const int32_t& value, ExceptionState& exception_state) {
  node_.SetNodeSortWeight(value);
}

int32_t SpriteImpl::Get_Ox(ExceptionState& exception_state) {
  return transform_.origin.x;
}

void SpriteImpl::Put_Ox(const int32_t& value, ExceptionState& exception_state) {
  transform_.origin.x = value;
  transform_.dirty = true;
}

int32_t SpriteImpl::Get_Oy(ExceptionState& exception_state) {
  return transform_.origin.y;
}

void SpriteImpl::Put_Oy(const int32_t& value, ExceptionState& exception_state) {
  transform_.origin.y = value;
  transform_.dirty = true;
}

float SpriteImpl::Get_ZoomX(ExceptionState& exception_state) {
  return transform_.scale.x;
}

void SpriteImpl::Put_ZoomX(const float& value,
                           ExceptionState& exception_state) {
  transform_.scale.x = value;
  transform_.dirty = true;
}

float SpriteImpl::Get_ZoomY(ExceptionState& exception_state) {
  return transform_.scale.y;
}

void SpriteImpl::Put_ZoomY(const float& value,
                           ExceptionState& exception_state) {
  transform_.scale.y = value;
  transform_.dirty = true;
}

float SpriteImpl::Get_Angle(ExceptionState& exception_state) {
  return transform_.rotation;
}

void SpriteImpl::Put_Angle(const float& value,
                           ExceptionState& exception_state) {
  transform_.rotation = value;
  transform_.dirty = true;
}

int32_t SpriteImpl::Get_WaveAmp(ExceptionState& exception_state) {
  return wave_.amp;
}

void SpriteImpl::Put_WaveAmp(const int32_t& value,
                             ExceptionState& exception_state) {
  wave_.amp = value;
}

int32_t SpriteImpl::Get_WaveLength(ExceptionState& exception_state) {
  return wave_.length;
}

void SpriteImpl::Put_WaveLength(const int32_t& value,
                                ExceptionState& exception_state) {
  wave_.length = value;
}

int32_t SpriteImpl::Get_WaveSpeed(ExceptionState& exception_state) {
  return wave_.speed;
}

void SpriteImpl::Put_WaveSpeed(const int32_t& value,
                               ExceptionState& exception_state) {
  wave_.speed = value;
}

int32_t SpriteImpl::Get_WavePhase(ExceptionState& exception_state) {
  return wave_.phase;
}

void SpriteImpl::Put_WavePhase(const int32_t& value,
                               ExceptionState& exception_state) {
  wave_.phase = value;
}

bool SpriteImpl::Get_Mirror(ExceptionState& exception_state) {
  return transform_.mirror;
}

void SpriteImpl::Put_Mirror(const bool& value,
                            ExceptionState& exception_state) {
  transform_.mirror = value;
  transform_.dirty = true;
}

int32_t SpriteImpl::Get_BushDepth(ExceptionState& exception_state) {
  return bush_.depth;
}

void SpriteImpl::Put_BushDepth(const int32_t& value,
                               ExceptionState& exception_state) {
  bush_.depth = value;
}

int32_t SpriteImpl::Get_BushOpacity(ExceptionState& exception_state) {
  return bush_.opacity;
}

void SpriteImpl::Put_BushOpacity(const int32_t& value,
                                 ExceptionState& exception_state) {
  bush_.opacity = value;
}

int32_t SpriteImpl::Get_Opacity(ExceptionState& exception_state) {
  return opacity_;
}

void SpriteImpl::Put_Opacity(const int32_t& value,
                             ExceptionState& exception_state) {
  opacity_ = value;
}

int32_t SpriteImpl::Get_BlendType(ExceptionState& exception_state) {
  return blend_type_;
}

void SpriteImpl::Put_BlendType(const int32_t& value,
                               ExceptionState& exception_state) {
  blend_type_ = value;
}

scoped_refptr<Color> SpriteImpl::Get_Color(ExceptionState& exception_state) {
  return color_;
}

void SpriteImpl::Put_Color(const scoped_refptr<Color>& value,
                           ExceptionState& exception_state) {
  *color_ = *ColorImpl::From(value);
}

scoped_refptr<Tone> SpriteImpl::Get_Tone(ExceptionState& exception_state) {
  return tone_;
}

void SpriteImpl::Put_Tone(const scoped_refptr<Tone>& value,
                          ExceptionState& exception_state) {
  *tone_ = *ToneImpl::From(value);
}

void SpriteImpl::OnObjectDisposed() {
  screen()->PostTask(base::BindOnce(&GPUDestroySpriteInternal, agent_));
  agent_ = nullptr;
}

void SpriteImpl::DrawableNodeHandlerInternal(
    DrawableNode::RenderStage stage,
    DrawableNode::RenderControllerParams* params) {
  if (stage == DrawableNode::RenderStage::kBeforeRender) {
    if (transform_.dirty) {
      transform_.dirty = false;
      screen()->PostTask(base::BindOnce(&GPUUpdateSpriteVerticesInternal,
                                        params->command_encoder, agent_,
                                        transform_));
    }
  } else if (stage == DrawableNode::RenderStage::kOnRendering) {
    screen()->PostTask(base::BindOnce(
        &GPUOnSpriteRenderingInternal, params->device,
        screen()->GetCommonIndexBuffer(), params->renderpass_encoder,
        params->world_binding, agent_, bitmap_->GetAgent(), blend_type_));
  }
}

}  // namespace content
