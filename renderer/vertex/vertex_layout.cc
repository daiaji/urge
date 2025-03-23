// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/vertex/vertex_layout.h"

namespace renderer {

wgpu::VertexBufferLayout Vertex::GetLayout() {
  static wgpu::VertexAttribute attrs[3];

  // Position: vec4<f32>
  attrs[0].format = wgpu::VertexFormat::Float32x4;
  attrs[0].offset = 0 * sizeof(float);
  attrs[0].shaderLocation = 0;

  // TexCoord: vec2<f32>
  attrs[1].format = wgpu::VertexFormat::Float32x2;
  attrs[1].offset = 4 * sizeof(float);
  attrs[1].shaderLocation = 1;

  // Color: vec4<f32>
  attrs[2].format = wgpu::VertexFormat::Float32x4;
  attrs[2].offset = 6 * sizeof(float);
  attrs[2].shaderLocation = 2;

  wgpu::VertexBufferLayout layout;
  layout.arrayStride = sizeof(Vertex);
  layout.attributeCount = _countof(attrs);
  layout.attributes = attrs;
  layout.stepMode = wgpu::VertexStepMode::Vertex;

  return layout;
}

void Quad::SetPositionRect(Quad* data, const base::RectF& pos) {
  int i = 0;
  data->vertices[i++].position = base::Vec4(pos.x, pos.y, 0, 1);
  data->vertices[i++].position = base::Vec4(pos.x + pos.width, pos.y, 0, 1);
  data->vertices[i++].position =
      base::Vec4(pos.x + pos.width, pos.y + pos.height, 0, 1);
  data->vertices[i++].position = base::Vec4(pos.x, pos.y + pos.height, 0, 1);
}

void Quad::SetTexCoordRect(Quad* data, const base::RectF& texcoord) {
  int i = 0;
  data->vertices[i++].texcoord = base::Vec2(texcoord.x, texcoord.y);
  data->vertices[i++].texcoord =
      base::Vec2(texcoord.x + texcoord.width, texcoord.y);
  data->vertices[i++].texcoord =
      base::Vec2(texcoord.x + texcoord.width, texcoord.y + texcoord.height);
  data->vertices[i++].texcoord =
      base::Vec2(texcoord.x, texcoord.y + texcoord.height);
}

void Quad::SetColor(Quad* data, const base::Vec4& color) {
  int i = 0;
  data->vertices[i++].color = color;
  data->vertices[i++].color = color;
  data->vertices[i++].color = color;
  data->vertices[i++].color = color;
}

std::ostream& operator<<(std::ostream& os, const Quad& value) {
  os << "Quad<";
  for (int32_t i = 0; i < _countof(value.vertices); ++i)
    os << "{ " << value.vertices[i].position << ", "
       << value.vertices[i].texcoord << " }, ";
  os << ">";
  return os;
}

}  // namespace renderer
