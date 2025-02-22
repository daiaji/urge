// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/components/tilequad.h"

namespace content {

int CalculateQuadTileCount(int tile, int dest) {
  if (tile && dest)
    return (dest + tile - 1) / tile;
  return 0;
}

}  // namespace content
