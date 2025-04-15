// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_LAYOUT_UNIFORM_LAYOUT_H_
#define RENDERER_LAYOUT_UNIFORM_LAYOUT_H_

#include "base/math/vector.h"

namespace renderer {

struct WorldTransform {
  float projection[16];
  float transform[16];
};

inline void MakeIdentityMatrix(float* out) {
  std::memset(out, 0, sizeof(float) * 16);
  out[0] = 1.0f;
  out[5] = 1.0f;
  out[10] = 1.0f;
  out[15] = 1.0f;
}

inline void MakeProjectionMatrix(float* out, const base::Vec2& size) {
  const float aa = 2.0f / size.x;
  const float bb = -2.0f / size.y;
  const float cc = 1.0f;

  // Note: wgpu required column major data layout.
  //
  // Projection matrix data view: (transpose)
  // | 2/w                  |
  // |      -2/h            |
  // |              1       |
  // | -1     1     0     1 |
  //
  // wgpu coordinate system:
  // pos:              tex:
  // |      1      |      |(0,0) -+   |
  // |      |      |      | |         |
  // | -1 <-+->  1 |      | +         |
  // |      |      |      |           |
  // |     -1      |      |      (1,1)|
  //
  // Result calculate:
  // ndc_x = (in_x * 2) / w - 1
  // in_x = 100, w = 500, ndc_x = -0.6

  std::memset(out, 0, sizeof(float) * 16);
  out[0] = aa;
  out[5] = bb;
  out[10] = cc;
  out[15] = 1.0f;

  out[12] = -1.0f;
  out[13] = 1.0f;
}

inline void MakeProjectionMatrix(float* out,
                                 const base::Vec2& size,
                                 const base::Vec2& offset) {
  const float aa = 2.0f / size.x;
  const float bb = -2.0f / size.y;
  const float cc = 1.0f;

  std::memset(out, 0, sizeof(float) * 16);
  out[0] = aa;
  out[5] = bb;
  out[10] = cc;
  out[15] = 1.0f;

  out[12] = aa * offset.x - 1.0f;
  out[13] = bb * offset.y + 1.0f;
}

}  // namespace renderer

#endif  //! RENDERER_LAYOUT_UNIFORM_LAYOUT_H_
