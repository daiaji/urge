// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_VERTEX_VERTEX_LAYOUT_H_
#define RENDERER_VERTEX_VERTEX_LAYOUT_H_

#include "webgpu/webgpu_cpp.h"

#include "base/math/rectangle.h"
#include "base/math/vector.h"

namespace renderer {

inline void MakeIdentityMatrix(float* out) {
  memset(out, 0, sizeof(float) * 16);
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

  memset(out, 0, sizeof(float) * 16);
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

  memset(out, 0, sizeof(float) * 16);
  out[0] = aa;
  out[5] = bb;
  out[10] = cc;
  out[15] = 1.0f;

  out[12] = aa * offset.x - 1.0f;
  out[13] = bb * offset.y + 1.0f;
}

struct Vertex {
  base::Vec4 position;
  base::Vec2 texcoord;
  base::Vec4 color;

  Vertex() : position(0), texcoord(0), color(1) {}
  Vertex(const base::Vec4& pos, const base::Vec2& tex, const base::Vec4& cr)
      : position(pos), texcoord(tex), color(cr) {}

  Vertex(const Vertex&) = default;
  Vertex& operator=(const Vertex&) = default;

  static wgpu::VertexBufferLayout GetLayout();
};

struct Quad {
  Vertex vertices[4];

  Quad() = default;

  static void SetPositionRect(Quad* data, const base::RectF& pos);
  static void SetTexCoordRectNorm(Quad* data, const base::RectF& texcoord);
  static void SetTexCoordRect(Quad* data,
                              const base::RectF& texcoord,
                              const base::Vec2& size);
  static void SetColor(Quad* data, const base::Vec4& color);

  Vertex* data() { return vertices; }

  friend std::ostream& operator<<(std::ostream& os, const Quad& value);
};

}  // namespace renderer

#endif  //! RENDERER_VERTEX_VERTEX_LAYOUT_H_
