// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMPONENTS_SPRITE_BATCH_H_
#define CONTENT_COMPONENTS_SPRITE_BATCH_H_

#include <vector>

#include "content/canvas/canvas_impl.h"
#include "renderer/device/render_device.h"
#include "renderer/pipeline/render_binding.h"

namespace content {

struct SpriteQuad {
  renderer::Binding_Sprite::Vertex vertices[4];

  static void SetPositionRect(SpriteQuad* data, const base::RectF& pos);
  static void SetTexCoordRect(SpriteQuad* data,
                              const base::RectF& texcoord,
                              const base::Vec2i& size);
};

using SpriteQuadBuffer =
    renderer::BatchBuffer<SpriteQuad,
                          Diligent::BIND_FLAGS::BIND_UNORDERED_ACCESS,
                          Diligent::BUFFER_MODE::BUFFER_MODE_STRUCTURED>;

using SpriteBatchBuffer =
    renderer::BatchBuffer<renderer::Binding_Sprite::Params,
                          Diligent::BIND_FLAGS::BIND_UNORDERED_ACCESS,
                          Diligent::BUFFER_MODE::BUFFER_MODE_STRUCTURED>;

class SpriteBatch {
 public:
  ~SpriteBatch();

  SpriteBatch(const SpriteBatch&) = delete;
  SpriteBatch& operator=(const SpriteBatch&) = delete;

  static std::unique_ptr<SpriteBatch> Make(renderer::RenderDevice* device);

  TextureAgent* GetCurrentTexture() const { return current_texture_; }

  Diligent::IBuffer* GetVertexBuffer() { return **vertex_batch_; }
  Diligent::IBufferView* GetQuadBinding() { return quad_binding_; }
  Diligent::IBufferView* GetUniformBinding() { return uniform_binding_; }

  void BeginBatch(TextureAgent* texture);

  void PushSprite(const SpriteQuad& quad,
                  const renderer::Binding_Sprite::Params& uniform);

  void EndBatch(uint32_t* instance_offset, uint32_t* instance_count);

  void SubmitBatchDataAndResetCache(Diligent::IDeviceContext* context);

 private:
  SpriteBatch(renderer::RenderDevice* device,
              std::unique_ptr<renderer::QuadBatch> vertex_batch,
              std::unique_ptr<SpriteQuadBuffer> quad_batch,
              std::unique_ptr<SpriteBatchBuffer> uniform_batch);
  renderer::RenderDevice* device_;
  TextureAgent* current_texture_;
  int32_t last_batch_index_;

  std::vector<SpriteQuad> quad_cache_;
  std::vector<renderer::Binding_Sprite::Params> uniform_cache_;

  std::unique_ptr<renderer::QuadBatch> vertex_batch_;
  std::unique_ptr<SpriteQuadBuffer> quad_batch_;
  RRefPtr<Diligent::IBufferView> quad_binding_;
  std::unique_ptr<SpriteBatchBuffer> uniform_batch_;
  RRefPtr<Diligent::IBufferView> uniform_binding_;
};

}  // namespace content

#endif  //! CONTENT_COMPONENTS_SPRITE_BATCH_H_
