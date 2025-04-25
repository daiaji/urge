// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/components/sprite_batch.h"

namespace content {

void SpriteQuad::SetPositionRect(SpriteQuad* data, const base::RectF& pos) {
  int32_t i = 0;
  data->vertices[i++].position = base::Vec4(pos.x, pos.y, 0, 1);
  data->vertices[i++].position = base::Vec4(pos.x + pos.width, pos.y, 0, 1);
  data->vertices[i++].position =
      base::Vec4(pos.x + pos.width, pos.y + pos.height, 0, 1);
  data->vertices[i++].position = base::Vec4(pos.x, pos.y + pos.height, 0, 1);
}

void SpriteQuad::SetTexCoordRect(SpriteQuad* data,
                                 const base::RectF& texcoord,
                                 const base::Vec2i& size) {
  const base::Vec2 tex_pos = texcoord.Position() / size;
  const base::Vec2 tex_size = (texcoord.Position() + texcoord.Size()) / size;

  int32_t i = 0;
  data->vertices[i++].texcoord = base::MakeVec4(tex_pos, 0);
  data->vertices[i++].texcoord = base::Vec4(tex_size.x, tex_pos.y, 0, 0);
  data->vertices[i++].texcoord = base::MakeVec4(tex_size, 0);
  data->vertices[i++].texcoord = base::Vec4(tex_pos.x, tex_size.y, 0, 0);
}

SpriteBatch::SpriteBatch(renderer::RenderDevice* device,
                         std::unique_ptr<renderer::Binding_Sprite> binding,
                         std::unique_ptr<renderer::QuadBatch> vertex_batch,
                         std::unique_ptr<SpriteQuadBuffer> quad_batch,
                         std::unique_ptr<SpriteBatchBuffer> uniform_batch)
    : device_(device),
      current_texture_(nullptr),
      last_batch_index_(-1),
      binding_(std::move(binding)),
      vertex_batch_(std::move(vertex_batch)),
      quad_batch_(std::move(quad_batch)),
      uniform_batch_(std::move(uniform_batch)) {}

SpriteBatch::~SpriteBatch() {}

std::unique_ptr<SpriteBatch> SpriteBatch::Make(renderer::RenderDevice* device) {
  auto binding =
      device->GetPipelines()->sprite.CreateBinding<renderer::Binding_Sprite>();
  auto vertex_batch = renderer::QuadBatch::Make(**device, 1);
  auto quad_batch = SpriteQuadBuffer::Make(**device);
  auto uniform_batch = SpriteBatchBuffer::Make(**device);

  return std::unique_ptr<SpriteBatch>(
      new SpriteBatch(device, std::move(binding), std::move(vertex_batch),
                      std::move(quad_batch), std::move(uniform_batch)));
}

void SpriteBatch::BeginBatch(TextureAgent* texture) {
  current_texture_ = texture;
  last_batch_index_ = uniform_cache_.size();
}

void SpriteBatch::PushSprite(const SpriteQuad& quad,
                             const renderer::Binding_Sprite::Params& uniform) {
  quad_cache_.push_back(quad);
  uniform_cache_.push_back(uniform);
}

void SpriteBatch::EndBatch(uint32_t* instance_offset,
                           uint32_t* instance_count) {
  const int32_t draw_count = uniform_cache_.size() - last_batch_index_;

  *instance_offset = last_batch_index_;
  *instance_count = draw_count;

  current_texture_ = nullptr;
  last_batch_index_ = -1;
}

void SpriteBatch::SubmitBatchDataAndResetCache(renderer::RenderDevice* device) {
  // Setup index buffer
  device_->GetQuadIndex()->Allocate(uniform_cache_.size());

  // Upload data and rebuild binding
  if (quad_cache_.size()) {
    vertex_batch_->QueueWrite(device->GetContext(), nullptr,
                              quad_cache_.size());
    quad_batch_->QueueWrite(device->GetContext(), quad_cache_.data(),
                            quad_cache_.size());
    quad_binding_ =
        (**quad_batch_)->GetDefaultView(Diligent::BUFFER_VIEW_SHADER_RESOURCE);
  }

  if (uniform_cache_.size()) {
    uniform_batch_->QueueWrite(device->GetContext(), uniform_cache_.data(),
                               uniform_cache_.size());
    uniform_binding_ =
        (**uniform_batch_)
            ->GetDefaultView(Diligent::BUFFER_VIEW_SHADER_RESOURCE);
  }

  // Reset cache
  quad_cache_.clear();
  uniform_cache_.clear();
}

}  // namespace content
