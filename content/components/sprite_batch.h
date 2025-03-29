// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMPONENTS_SPRITE_BATCH_H_
#define CONTENT_COMPONENTS_SPRITE_BATCH_H_

#include <vector>

#include "content/canvas/canvas_impl.h"
#include "renderer/device/render_device.h"

namespace content {

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
  wgpu::Buffer* GetBatchVertexBuffer() { return &**quads_batch_; }

  void BeginBatch(TextureAgent* texture);

  void PushSprite(const renderer::Quad& quad,
                  const renderer::SpriteUniform& uniform);

  void EndBatch(uint32_t* index_count,
                uint32_t* instance_count,
                uint32_t* first_index,
                uint32_t* first_instance);

  void SubmitBatchDataAndResetCache(wgpu::CommandEncoder* encoder);

 private:
  SpriteBatch(renderer::RenderDevice* device,
              std::unique_ptr<renderer::QuadBatch> quads_batch,
              std::unique_ptr<SpriteBatchBuffer> uniform_batch);
  renderer::RenderDevice* device_;
  TextureAgent* current_texture_;
  int32_t last_batch_index_;

  std::vector<renderer::Quad> quads_cache_;
  std::vector<renderer::SpriteUniform> uniform_cache_;

  std::unique_ptr<renderer::QuadBatch> quads_batch_;
  std::unique_ptr<SpriteBatchBuffer> uniform_batch_;

  wgpu::BindGroup uniform_binding_;
};

}  // namespace content

#endif  //! CONTENT_COMPONENTS_SPRITE_BATCH_H_
