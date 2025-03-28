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
  SpriteBatch();
  ~SpriteBatch();

  SpriteBatch(const SpriteBatch&) = delete;
  SpriteBatch& operator=(const SpriteBatch&) = delete;

 private:
  TextureAgent* current_batch_texture_;


  std::vector<renderer::Quad> quads_cache_;
  std::vector<renderer::SpriteUniform> uniform_cache_;

  std::unique_ptr<renderer::QuadBatch> quads_batch_;
  std::unique_ptr<SpriteBatchBuffer> uniform_batch_;
};

}  // namespace content

#endif  //! CONTENT_COMPONENTS_SPRITE_BATCH_H_
