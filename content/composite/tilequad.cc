// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/composite/tilequad.h"

namespace content {

int CalculateQuadTileCount(int tile, int dest) {
  if (tile && dest)
    return (dest + tile - 1) / tile;
  return 0;
}

int BuildTiles(const base::Rect& src_rect,
               const base::Rect& dest_rect,
               const base::Vec4& color,
               renderer::Quad* quads) {
  if (src_rect.IsInvalid() || dest_rect.IsInvalid())
    return 0;

  const int tiles_per_row =
      CalculateQuadTileCount(src_rect.width, dest_rect.width);
  const int full_rows = dest_rect.height / src_rect.height;
  const int remaining_height = dest_rect.height % src_rect.height;

  int quad_count = 0;
  int offset = 0;
  int current_y = dest_rect.y;

  // Process full-height rows
  for (int row = 0; row < full_rows; ++row) {
    const int row_quads = BuildTilesAlongAxis<TileAxis::HORIZONTAL>(
        src_rect, color, dest_rect.width, current_y, dest_rect.x,
        quads + offset);

    quad_count += row_quads;
    offset += row_quads;
    current_y += src_rect.height;
  }

  // Process partial-height row
  if (remaining_height > 0) {
    base::Rect partial_src = src_rect;
    partial_src.height = remaining_height;

    const int row_quads = BuildTilesAlongAxis<TileAxis::HORIZONTAL>(
        partial_src, color, dest_rect.width, current_y, dest_rect.x,
        quads + offset);

    quad_count += row_quads;
  }

  return quad_count;
}

}  // namespace content
