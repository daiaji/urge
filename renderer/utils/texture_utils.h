// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_UTILS_TEXTURE_UTILS_H_
#define RENDERER_UTILS_TEXTURE_UTILS_H_

#include "base/math/rectangle.h"
#include "webgpu/webgpu_cpp.h"

namespace renderer {

wgpu::Texture CreateTexture2D(const wgpu::Device& device,
                              const std::string_view& label,
                              wgpu::TextureUsage usage,
                              const base::Vec2i& size);

}  // namespace renderer

#endif  //! RENDERER_UTILS_TEXTURE_UTILS_H_
