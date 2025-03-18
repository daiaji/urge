// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMPOSITE_TILEQUAD_H_
#define CONTENT_COMPOSITE_TILEQUAD_H_

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
template <TileAxis Axis>
int BuildTilesAlongAxis(const base::Rect& src_rect,
                        const base::Vec4& color,
                        int main_axis_size,
                        int cross_axis_pos,
                        int main_axis_pos,
                        renderer::Quad* quads) {
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
    renderer::Quad* quad = quads + i;
    renderer::Quad::SetTexCoordRect(quad, src_rect);
    renderer::Quad::SetPositionRect(quad, dest_rect);
    renderer::Quad::SetColor(quad, color);

    if (Axis == TileAxis::HORIZONTAL) {
      dest_rect.x += src_main;
    } else {
      dest_rect.y += src_main;
    }
  }

  if (partial_size > 0) {
    base::Rect partial_src = src_rect;
    renderer::Quad* quad = quads + full_blocks;

    if (Axis == TileAxis::HORIZONTAL) {
      partial_src.width = partial_size;
      dest_rect.width = partial_size;
    } else {
      partial_src.height = partial_size;
      dest_rect.height = partial_size;
    }

    renderer::Quad::SetTexCoordRect(quad, partial_src);
    renderer::Quad::SetPositionRect(quad, dest_rect);
    renderer::Quad::SetColor(quad, color);
  }

  return full_blocks + (partial_size > 0 ? 1 : 0);
}

// Region tile generator:
//  Fill size|src_rect| tiles in target|dest_rect|.
int BuildTiles(const base::Rect& src_rect,
               const base::Rect& dest_rect,
               const base::Vec4& color,
               renderer::Quad* quads);

}  // namespace content

#endif  //! CONTENT_COMPOSITE_TILEQUAD_H_
