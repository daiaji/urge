// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMPONENTS_SPRITE_BATCH_H_
#define CONTENT_COMPONENTS_SPRITE_BATCH_H_

#include <vector>

#include "content/canvas/canvas_impl.h"
#include "renderer/device/render_device.h"

namespace content {

struct SpriteQuad {
  renderer::SpriteVertex vertices[4];

  static void SetPositionRect(SpriteQuad* data, const base::RectF& pos);
  static void SetTexCoordRect(SpriteQuad* data, const base::RectF& texcoord);
  static void SetColor(SpriteQuad* data, const base::Vec4& color);
};

using SpriteQuadBuffer =
    renderer::BatchBuffer<SpriteQuad, wgpu::BufferUsage::Storage>;

using SpriteBatchBuffer =
    renderer::BatchBuffer<renderer::SpriteUniform, wgpu::BufferUsage::Storage>;

class SpriteBatch {
 public:
  ~SpriteBatch();

  SpriteBatch(const SpriteBatch&) = delete;
  SpriteBatch& operator=(const SpriteBatch&) = delete;

  static std::unique_ptr<SpriteBatch> Make(renderer::RenderDevice* device);

  TextureAgent* GetCurrentTexture() const { return current_texture_; }
  wgpu::BindGroup* GetUniformBinding() { return &uniform_binding_; }
  wgpu::Buffer* GetBatchVertexBuffer() { return &**vertex_batch_; }

  void BeginBatch(TextureAgent* texture);

  void PushSprite(const SpriteQuad& quad,
                  const renderer::SpriteUniform& uniform);

  void EndBatch(uint32_t* instance_offset, uint32_t* instance_count);

  void SubmitBatchDataAndResetCache(wgpu::CommandEncoder* encoder);

 private:
  SpriteBatch(renderer::RenderDevice* device,
              std::unique_ptr<renderer::QuadBatch> vertex_batch,
              std::unique_ptr<SpriteQuadBuffer> quad_batch,
              std::unique_ptr<SpriteBatchBuffer> uniform_batch);
  renderer::RenderDevice* device_;
  TextureAgent* current_texture_;
  int32_t last_batch_index_;

  std::vector<SpriteQuad> quad_cache_;
  std::vector<renderer::SpriteUniform> uniform_cache_;

  std::unique_ptr<renderer::QuadBatch> vertex_batch_;
  std::unique_ptr<SpriteQuadBuffer> quad_batch_;
  std::unique_ptr<SpriteBatchBuffer> uniform_batch_;

  wgpu::BindGroup uniform_binding_;
};

}  // namespace content

#endif  //! CONTENT_COMPONENTS_SPRITE_BATCH_H_
