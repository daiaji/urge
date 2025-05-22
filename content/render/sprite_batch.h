// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDER_SPRITE_BATCH_H_
#define CONTENT_RENDER_SPRITE_BATCH_H_

#include <vector>

#include "content/canvas/canvas_impl.h"
#include "renderer/device/render_device.h"
#include "renderer/pipeline/render_binding.h"

namespace content {

using SpriteBatchBuffer =
    renderer::BatchBuffer<renderer::Binding_Sprite::Params,
                          Diligent::BIND_FLAGS::BIND_SHADER_RESOURCE,
                          Diligent::BUFFER_MODE::BUFFER_MODE_STRUCTURED,
                          Diligent::CPU_ACCESS_WRITE,
                          Diligent::USAGE_DYNAMIC>;

class SpriteBatch {
 public:
  ~SpriteBatch();

  SpriteBatch(const SpriteBatch&) = delete;
  SpriteBatch& operator=(const SpriteBatch&) = delete;

  static std::unique_ptr<SpriteBatch> Make(renderer::RenderDevice* device);

  TextureAgent* GetCurrentTexture() const { return current_texture_; }

  renderer::Binding_Sprite* GetShaderBinding() { return binding_.get(); }
  Diligent::IBuffer* GetVertexBuffer() { return **vertex_batch_; }
  Diligent::IBufferView* GetUniformBinding() { return uniform_binding_; }

  // Setup a sprite batch
  void BeginBatch(TextureAgent* texture);

  void PushSprite(const renderer::Quad& quad,
                  const renderer::Binding_Sprite::Params& uniform);

  void EndBatch(uint32_t* instance_offset, uint32_t* instance_count);

  // Summit pending batch data to rendering queue
  void SubmitBatchDataAndResetCache(renderer::RenderDevice* device,
                                    renderer::RenderContext* context);

 private:
  SpriteBatch(renderer::RenderDevice* device,
              std::unique_ptr<renderer::Binding_Sprite> binding,
              std::unique_ptr<renderer::QuadBatch> vertex_batch,
              std::unique_ptr<SpriteBatchBuffer> uniform_batch);
  renderer::RenderDevice* device_;
  TextureAgent* current_texture_;
  int32_t last_batch_index_;

  std::vector<renderer::Quad> quad_cache_;
  std::vector<renderer::Binding_Sprite::Params> uniform_cache_;

  std::unique_ptr<renderer::Binding_Sprite> binding_;
  std::unique_ptr<renderer::QuadBatch> vertex_batch_;
  std::unique_ptr<SpriteBatchBuffer> uniform_batch_;
  RRefPtr<Diligent::IBufferView> uniform_binding_;
};

}  // namespace content

#endif  //! CONTENT_RENDER_SPRITE_BATCH_H_
