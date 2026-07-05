// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDER_PIPELINE_COLLECTION_H_
#define CONTENT_RENDER_PIPELINE_COLLECTION_H_

#include <array>

#include "renderer/pipeline/render_pipeline.h"

namespace content {

/// Color Blend Type of Pipeline
///
enum BlendType {
  BLEND_TYPE_NORMAL = 0,
  BLEND_TYPE_ADDITION,
  BLEND_TYPE_SUBSTRACTION,
  BLEND_TYPE_MULTIPLY,
  BLEND_TYPE_KEEP_ALPHA,
  BLEND_TYPE_NO_BLEND,
  BLEND_TYPE_NUMS,
};

struct PipelineCollection {
  using PipelineObject = RRefPtr<Diligent::IPipelineState>;

  struct {
    std::array<PipelineObject, BLEND_TYPE_NUMS> base;
    PipelineObject color;
    PipelineObject bitmapblt;
    PipelineObject bitmapclip;
    PipelineObject bitmaphue;
  } bitmap;

  std::array<PipelineObject, BLEND_TYPE_NUMS> plane;
  std::array<PipelineObject, BLEND_TYPE_NUMS> sprite;

  PipelineObject viewport_flat;
  PipelineObject brightness;

  PipelineObject tilemap;
  PipelineObject tilemap2;

  PipelineObject alpha_transition;
  PipelineObject vague_transition;

  PipelineObject yuv;
  PipelineObject upscale;
  PipelineObject anime4k_enhance;
  PipelineObject cas;

  // Anime4K Mode A pipeline states

  // Anime4K Upscale_Denoise_L pipeline states (MRT for Pass0-2, single for Pass3)
  PipelineObject anime4k_udl_pass0;
  PipelineObject anime4k_udl_pass1;
  PipelineObject anime4k_udl_pass2;
  PipelineObject anime4k_udl_pass3;
  PipelineObject cunny_4x16_p1;
  PipelineObject cunny_4x16_p2;
  PipelineObject cunny_4x16_p3;
  PipelineObject cunny_4x16_p4;
  PipelineObject cunny_4x16_p5;
  PipelineObject cunny_4x16_p6;
  PipelineObject cunny_4x24_p1;
  PipelineObject cunny_4x24_p2;
  PipelineObject cunny_4x24_p3;
  PipelineObject cunny_4x24_p4;
  PipelineObject cunny_4x24_p5;
  PipelineObject cunny_4x24_p6;

  PipelineObject window;
  PipelineObject window_with_stencil;
  PipelineObject color_write_stencil;
  struct {
    PipelineObject viewport;
    PipelineObject viewport_alpha;
    PipelineObject base;
  } window2;

  PipelineCollection(renderer::PipelineSet* loader,
                     Diligent::IRenderDevice* device);

  bool EnsureAnime4KUDLPipelines(renderer::PipelineSet* loader);
  bool EnsureCuNNy4x16Pipelines(renderer::PipelineSet* loader);
  bool EnsureCuNNy4x24Pipelines(renderer::PipelineSet* loader);
};

}  // namespace content

#endif  //! CONTENT_RENDER_PIPELINE_COLLECTION_H_
