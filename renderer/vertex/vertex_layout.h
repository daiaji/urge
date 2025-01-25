// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_VERTEX_VERTEX_LAYOUT_H_
#define RENDERER_VERTEX_VERTEX_LAYOUT_H_

#include "base/math/rectangle.h"
#include "base/math/vector.h"

#include "webgpu/webgpu_cpp.h"

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

class TransformQuadVertices {
 public:
  TransformQuadVertices()
      : scale_(1.f, 1.f), rotation_(0), need_update_(true) {}

  TransformQuadVertices(const TransformQuadVertices&) = default;
  TransformQuadVertices& operator=(const TransformQuadVertices&) = default;

 public:
  const base::Vec2& GetPosition() const { return position_; }
  const base::Vec2& GetOrigin() const { return origin_; }
  const base::Vec2& GetScale() const { return scale_; }
  float GetRotation() const { return rotation_; }

  void SetPosition(const base::Vec2& value) {
    position_ = value;
    need_update_ = true;
  }

  void SetOrigin(const base::Vec2& value) {
    origin_ = value;
    need_update_ = true;
  }

  void SetScale(const base::Vec2& value) {
    scale_ = value;
    need_update_ = true;
  }

  void SetRotation(float value) {
    rotation_ = value;
    need_update_ = true;
  }

  void SetGlobalOffset(const base::Vec2i& value) {
    viewport_offset_ = value;
    need_update_ = true;
  }

  bool DataUpdateRequired() { return need_update_; }

  void TransformVertexUnsafe(base::Vec4* vertices) {
    if (need_update_) {
      UpdateMatrix();
      need_update_ = false;
    }

    vertices->x = transform_matrix_[0] * vertices->x +
                  transform_matrix_[1] * vertices->y +
                  transform_matrix_[2] * vertices->z +
                  transform_matrix_[3] * vertices->w;
    vertices->y = transform_matrix_[4] * vertices->x +
                  transform_matrix_[5] * vertices->y +
                  transform_matrix_[6] * vertices->z +
                  transform_matrix_[7] * vertices->w;
    vertices->z = transform_matrix_[8] * vertices->x +
                  transform_matrix_[9] * vertices->y +
                  transform_matrix_[10] * vertices->z +
                  transform_matrix_[11] * vertices->w;
    vertices->w = transform_matrix_[12] * vertices->x +
                  transform_matrix_[13] * vertices->y +
                  transform_matrix_[14] * vertices->z +
                  transform_matrix_[15] * vertices->w;
  }

 private:
  void UpdateMatrix() {
    float angle = rotation_;
    if (angle >= 360 || angle < -360)
      angle = std::fmod(angle, 360.f);
    if (angle < 0)
      angle += 360;

    angle = angle * 3.141592654f / 180.0f;
    float cosine = std::cos(angle);
    float sine = std::sin(angle);
    float sxc = scale_.x * cosine;
    float syc = scale_.y * cosine;
    float sxs = scale_.x * sine;
    float sys = scale_.y * sine;
    float tx =
        -origin_.x * sxc - origin_.y * sys + position_.x + viewport_offset_.x;
    float ty =
        origin_.x * sxs - origin_.y * syc + position_.y + viewport_offset_.y;

    transform_matrix_[0] = sxc;
    transform_matrix_[1] = sys;
    transform_matrix_[3] = tx;
    transform_matrix_[4] = -sxs;
    transform_matrix_[5] = syc;
    transform_matrix_[7] = ty;
  }

  base::Vec2 position_;
  base::Vec2 origin_;
  base::Vec2 scale_;
  float rotation_;
  base::Vec2i viewport_offset_;

  float transform_matrix_[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 1, 0, 0, 0, 0, 1};

  bool need_update_ = false;
};

}  // namespace renderer

#endif  //! RENDERER_VERTEX_VERTEX_LAYOUT_H_
