// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_VERTEX_VERTEX_LAYOUT_H_
#define RENDERER_VERTEX_VERTEX_LAYOUT_H_

#include "base/math/rectangle.h"
#include "base/math/vector.h"

#include "webgpu/webgpu_cpp.h"

namespace renderer {

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

  memset(out, 0, sizeof(float) * 16);
  out[0] = aa;
  out[5] = bb;
  out[10] = cc;

  out[12] = -1.0f;
  out[13] = 1.0f;
  out[15] = 1.0f;
}

struct FullVertexLayout {
  base::Vec4 position;
  base::Vec2 texcoord;
  base::Vec4 color;

  FullVertexLayout() : position(0), texcoord(0), color(0, 0, 0, 1) {}
  FullVertexLayout(const FullVertexLayout&) = default;
  FullVertexLayout& operator=(const FullVertexLayout&) = default;

  static void SetPositionRect(FullVertexLayout* data, const base::RectF& pos);
  static void SetTexCoordRect(FullVertexLayout* data,
                              const base::RectF& texcoord);
  static void SetColor(FullVertexLayout* data,
                       const base::Vec4& color,
                       int index = -1);

  static wgpu::VertexBufferLayout GetLayout();
};

}  // namespace renderer

#endif  //! RENDERER_VERTEX_VERTEX_LAYOUT_H_
