// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/render/render_pass.h"

namespace renderer {

RenderPass::RenderPass() {}

RenderPass::RenderPass(const wgpu::RenderPassEncoder& encoder)
    : encoder_(encoder) {}

RenderPass::RenderPass(const RenderPass& other) : encoder_(other.encoder_) {}

RenderPass& RenderPass::operator=(const wgpu::RenderPassEncoder& encoder) {
  encoder_ = encoder;

  std::array<wgpu::BindGroup, 4> empty_cache;
  bindgroup_cache_.swap(empty_cache);

  return *this;
}

}  // namespace renderer
