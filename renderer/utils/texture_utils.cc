// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/utils/texture_utils.h"

namespace renderer {

wgpu::Texture CreateTexture2D(const wgpu::Device& device,
                              const std::string_view& label,
                              wgpu::TextureUsage usage,
                              const base::Vec2i& size) {
  wgpu::TextureDescriptor texture_desc;
  texture_desc.label = label;
  texture_desc.dimension = wgpu::TextureDimension::e2D;
  texture_desc.format = wgpu::TextureFormat::RGBA8Unorm;
  texture_desc.usage = usage;
  texture_desc.size.width = size.x;
  texture_desc.size.height = size.y;
  return device.CreateTexture(&texture_desc);
}

}  // namespace renderer
