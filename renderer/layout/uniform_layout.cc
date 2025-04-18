// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/layout/uniform_layout.h"

namespace renderer {

void MakeIdentityMatrix(float* out) {
  std::memset(out, 0, sizeof(float) * 16);
  out[0] = 1.0f;
  out[5] = 1.0f;
  out[10] = 1.0f;
  out[15] = 1.0f;
}

void MakeProjectionMatrix(float* out, const base::Vec2& size, bool flip) {
  const float y_scale = flip ? -1.0f : 1.0f;
  const float aa = 2.0f / size.x;
  const float bb = y_scale * (-2.0f / size.y);
  const float cc = 1.0f;

  std::memset(out, 0, sizeof(float) * 16);
  out[0] = aa;
  out[5] = bb;
  out[10] = cc;
  out[15] = 1.0f;

  out[12] = -1.0f;
  out[13] = y_scale;
}

void MakeProjectionMatrix(float* out,
                          const base::Vec2& size,
                          const base::Vec2& offset,
                          bool flip) {
  const float y_scale = flip ? -1.0f : 1.0f;
  const float aa = 2.0f / size.x;
  const float bb = y_scale * (-2.0f / size.y);
  const float cc = 1.0f;

  std::memset(out, 0, sizeof(float) * 16);
  out[0] = aa;
  out[5] = bb;
  out[10] = cc;
  out[15] = 1.0f;

  out[12] = aa * offset.x - 1.0f;
  out[13] = bb * offset.y + y_scale;
}

}  // namespace renderer
