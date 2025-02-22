// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMPONENTS_TILEQUAD_H_
#define CONTENT_COMPONENTS_TILEQUAD_H_

#include "base/math/rectangle.h"
#include "renderer/vertex/vertex_layout.h"

namespace content {

using TileAxis = enum { HORIZONTAL, VERTICAL };

// Get required quadangle's tile count.
int CalculateQuadTileCount(int tile, int dest);

// Axis direction repeat tile generator:
// (main_pos, cross_pos) -> o|---- main_size ----|
//   o <- (cross_pos, main_pos)
//  ---
//   |
//   | main_size
//   |
//  ---
template <TileAxis Axis, typename VertexType>
int BuildTilesAlongAxis(const base::Rect& src_rect,
                        int main_axis_size,
                        int cross_axis_pos,
                        int main_axis_pos,
                        VertexType* vertices) {
  if (main_axis_size <= 0)
    return 0;

  const int src_main =
      (Axis == TileAxis::HORIZONTAL) ? src_rect.width : src_rect.height;
  const int full_blocks = main_axis_size / src_main;
  const int partial_size = main_axis_size % src_main;

  base::Rect dest_rect;
  if (Axis == TileAxis::HORIZONTAL) {
    dest_rect = base::Rect(main_axis_pos, cross_axis_pos, src_rect.width,
                           src_rect.height);
  } else {
    dest_rect = base::Rect(cross_axis_pos, main_axis_pos, src_rect.width,
                           src_rect.height);
  }

  for (int i = 0; i < full_blocks; ++i) {
    VertexType* vert = vertices + i * 4;
    VertexType::SetTexCoordRect(vert, src_rect);
    VertexType::SetPositionRect(vert, dest_rect);

    if (Axis == TileAxis::HORIZONTAL) {
      dest_rect.x += src_main;
    } else {
      dest_rect.y += src_main;
    }
  }

  if (partial_size > 0) {
    base::Rect partial_src = src_rect;
    VertexType* vert = vertices + full_blocks * 4;

    if (Axis == TileAxis::HORIZONTAL) {
      partial_src.width = partial_size;
      dest_rect.width = partial_size;
    } else {
      partial_src.height = partial_size;
      dest_rect.height = partial_size;
    }

    VertexType::SetTexCoordRect(vert, partial_src);
    VertexType::SetPositionRect(vert, dest_rect);
  }

  return full_blocks + (partial_size > 0 ? 1 : 0);
}

// Region tile generator:
//  Fill size|src_rect| tiles in target|dest_rect|.
template <typename VertexType>
int BuildTiles(const base::Rect& src_rect,
               const base::Rect& dest_rect,
               VertexType* vertices) {
  if (src_rect.IsInvalid() || dest_rect.IsInvalid())
    return 0;

  const int tiles_per_row =
      CalculateQuadTileCount(src_rect.width, dest_rect.width);
  const int full_rows = dest_rect.height / src_rect.height;
  const int remaining_height = dest_rect.height % src_rect.height;

  int quad_count = 0;
  int vertex_offset = 0;
  int current_y = dest_rect.y;

  // Process full-height rows
  for (int row = 0; row < full_rows; ++row) {
    const int row_quads = BuildTilesAlongAxis<TileAxis::HORIZONTAL>(
        src_rect, dest_rect.width, current_y, dest_rect.x,
        vertices + vertex_offset);

    quad_count += row_quads;
    vertex_offset += row_quads * 4;
    current_y += src_rect.height;
  }

  // Process partial-height row
  if (remaining_height > 0) {
    base::Rect partial_src = src_rect;
    partial_src.height = remaining_height;

    const int row_quads = BuildTilesAlongAxis<TileAxis::HORIZONTAL>(
        partial_src, dest_rect.width, current_y, dest_rect.x,
        vertices + vertex_offset);

    quad_count += row_quads;
  }

  return quad_count;
}

}  // namespace content

#endif  //! CONTENT_COMPONENTS_TILEQUAD_H_
