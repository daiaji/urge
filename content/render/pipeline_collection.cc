// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/render/pipeline_collection.h"

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"

namespace content {

namespace {

Diligent::RenderTargetBlendDesc GetBlendState(BlendType type) {
  Diligent::RenderTargetBlendDesc state;
  switch (type) {
    default:
    case BLEND_TYPE_NORMAL:
      state.BlendEnable = Diligent::True;
      state.BlendOp = Diligent::BLEND_OPERATION_ADD;
      state.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
      state.DestBlend = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
      state.SrcBlendAlpha = Diligent::BLEND_FACTOR_ONE;
      state.DestBlendAlpha = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
      break;
    case BLEND_TYPE_ADDITION:
      state.BlendEnable = Diligent::True;
      state.BlendOp = Diligent::BLEND_OPERATION_ADD;
      state.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
      state.DestBlend = Diligent::BLEND_FACTOR_ONE;
      state.SrcBlendAlpha = Diligent::BLEND_FACTOR_ONE;
      state.DestBlendAlpha = Diligent::BLEND_FACTOR_ONE;
      break;
    case BLEND_TYPE_SUBSTRACTION:
      state.BlendEnable = Diligent::True;
      state.BlendOp = Diligent::BLEND_OPERATION_REV_SUBTRACT;
      state.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
      state.DestBlend = Diligent::BLEND_FACTOR_ONE;
      state.SrcBlendAlpha = Diligent::BLEND_FACTOR_ZERO;
      state.DestBlendAlpha = Diligent::BLEND_FACTOR_ONE;
      break;
    case BLEND_TYPE_MULTIPLY:
      state.BlendEnable = Diligent::True;
      state.BlendOp = Diligent::BLEND_OPERATION_ADD;
      state.SrcBlend = Diligent::BLEND_FACTOR_DEST_COLOR;
      state.DestBlend = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
      state.SrcBlendAlpha = Diligent::BLEND_FACTOR_ZERO;
      state.DestBlendAlpha = Diligent::BLEND_FACTOR_ONE;
      break;
    case BLEND_TYPE_KEEP_ALPHA:
      state.BlendEnable = Diligent::True;
      state.BlendOp = Diligent::BLEND_OPERATION_ADD;
      state.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
      state.DestBlend = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
      state.SrcBlendAlpha = Diligent::BLEND_FACTOR_ZERO;
      state.DestBlendAlpha = Diligent::BLEND_FACTOR_ONE;
      break;
    case BLEND_TYPE_NO_BLEND:
      state.BlendEnable = Diligent::False;
      break;
  }

  return state;
}

Diligent::DepthStencilStateDesc GetDefaultDepthStencilState(bool enable_depth) {
  Diligent::DepthStencilStateDesc depth_stencil_state;
  if (enable_depth) {
    depth_stencil_state.DepthFunc = Diligent::COMPARISON_FUNC_LESS_EQUAL;
    depth_stencil_state.DepthEnable = Diligent::True;
  } else {
    depth_stencil_state.DepthEnable = Diligent::False;
  }

  return depth_stencil_state;
}

Diligent::RasterizerStateDesc Get2DRasterizerState() {
  Diligent::RasterizerStateDesc rasterizer_state;
  rasterizer_state.CullMode = Diligent::CULL_MODE_NONE;
  rasterizer_state.FrontCounterClockwise = Diligent::True;
  return rasterizer_state;
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// PipelineCollection Implement

PipelineCollection::PipelineCollection(renderer::PipelineSet* loader,
                                       Diligent::IRenderDevice* device) {
  constexpr auto target_format = Diligent::TEX_FORMAT_RGBA8_UNORM;
  constexpr auto depth_stencil_format = Diligent::TEX_FORMAT_D24_UNORM_S8_UINT;
  constexpr auto primitive_topology =
      Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  constexpr Diligent::SampleDesc default_sample;

  // Bitmap (canvas) - no scissor - no depth
  {
    // No depth test
    Diligent::DepthStencilStateDesc depth_stencil_state =
        GetDefaultDepthStencilState(false);

    Diligent::RasterizerStateDesc rasterizer_state = Get2DRasterizerState();
    rasterizer_state.ScissorEnable = Diligent::False;  // No scissor test

    // Base (blend + no depth)
    for (size_t i = 0; i < BLEND_TYPE_NUMS; ++i) {
      Diligent::BlendStateDesc blend_state;
      blend_state.RenderTargets[0] = GetBlendState(static_cast<BlendType>(i));

      loader->base.BuildPipeline(&bitmap.base[i], blend_state, rasterizer_state,
                                 depth_stencil_state, primitive_topology,
                                 {target_format}, Diligent::TEX_FORMAT_UNKNOWN,
                                 default_sample);
    }

    // Color no blend
    loader->color.BuildPipeline(&bitmap.color, Diligent::BlendStateDesc(),
                                rasterizer_state, depth_stencil_state,
                                primitive_topology, {target_format},
                                Diligent::TEX_FORMAT_UNKNOWN, default_sample);

    // Blt no blend
    loader->bitmapblt.BuildPipeline(
        &bitmap.bitmapblt, Diligent::BlendStateDesc(), rasterizer_state,
        depth_stencil_state, primitive_topology, {target_format},
        Diligent::TEX_FORMAT_UNKNOWN, default_sample);

    // Clip no blend
    loader->bitmapclipblt.BuildPipeline(
        &bitmap.bitmapclip, Diligent::BlendStateDesc(), rasterizer_state,
        depth_stencil_state, primitive_topology, {target_format},
        Diligent::TEX_FORMAT_UNKNOWN, default_sample);

    // Hue no blend
    loader->bitmaphue.BuildPipeline(
        &bitmap.bitmaphue, Diligent::BlendStateDesc(), rasterizer_state,
        depth_stencil_state, primitive_topology, {target_format},
        Diligent::TEX_FORMAT_UNKNOWN, default_sample);
  }

  // Plane (flat) - with scissor - with depth
  for (size_t i = 0; i < BLEND_TYPE_NUMS; ++i) {
    // With blend
    Diligent::BlendStateDesc blend_state;
    blend_state.RenderTargets[0] = GetBlendState(static_cast<BlendType>(i));

    // With depth test
    Diligent::DepthStencilStateDesc depth_stencil_state =
        GetDefaultDepthStencilState(true);

    Diligent::RasterizerStateDesc rasterizer_state = Get2DRasterizerState();
    rasterizer_state.ScissorEnable = Diligent::True;  // With scissor test

    loader->viewport.BuildPipeline(&plane[i], blend_state, rasterizer_state,
                                   depth_stencil_state, primitive_topology,
                                   {target_format}, depth_stencil_format,
                                   default_sample);
  }

  // Sprite - with scissor - with depth
  for (size_t i = 0; i < BLEND_TYPE_NUMS; ++i) {
    // With blend
    Diligent::BlendStateDesc blend_state;
    blend_state.RenderTargets[0] = GetBlendState(static_cast<BlendType>(i));

    // Enable depth test
    Diligent::DepthStencilStateDesc depth_stencil_state =
        GetDefaultDepthStencilState(true);

    Diligent::RasterizerStateDesc rasterizer_state = Get2DRasterizerState();
    rasterizer_state.ScissorEnable = Diligent::True;  // With scissor test

    loader->sprite.BuildPipeline(&sprite[i], blend_state, rasterizer_state,
                                 depth_stencil_state, primitive_topology,
                                 {target_format}, depth_stencil_format,
                                 default_sample);
  }

  // Viewport Effect (flat) - no scissor - no depth
  {
    // No blend
    Diligent::BlendStateDesc blend_state;

    // Disable depth test
    Diligent::DepthStencilStateDesc depth_stencil_state =
        GetDefaultDepthStencilState(false);

    Diligent::RasterizerStateDesc rasterizer_state = Get2DRasterizerState();
    rasterizer_state.ScissorEnable = Diligent::False;  // No scissor test

    // Base with blend
    loader->viewport.BuildPipeline(
        &viewport_flat, blend_state, rasterizer_state, depth_stencil_state,
        primitive_topology, {target_format}, Diligent::TEX_FORMAT_UNKNOWN,
        default_sample);
  }

  {  // Brightness (effect) - no scissor - with depth
    // With blend
    Diligent::BlendStateDesc blend_state;
    blend_state.RenderTargets[0] = GetBlendState(BLEND_TYPE_NORMAL);

    // With depth test
    Diligent::DepthStencilStateDesc depth_stencil_state =
        GetDefaultDepthStencilState(true);

    Diligent::RasterizerStateDesc rasterizer_state = Get2DRasterizerState();
    rasterizer_state.ScissorEnable = Diligent::False;  // No scissor test

    loader->color.BuildPipeline(&brightness, blend_state, rasterizer_state,
                                depth_stencil_state, primitive_topology,
                                {target_format}, depth_stencil_format,
                                default_sample);
  }

  {  // Tilemap - with scissor - with depth
    // With blend
    Diligent::BlendStateDesc blend_state;
    blend_state.RenderTargets[0] = GetBlendState(BLEND_TYPE_NORMAL);

    // Enable depth test
    Diligent::DepthStencilStateDesc depth_stencil_state =
        GetDefaultDepthStencilState(true);

    Diligent::RasterizerStateDesc rasterizer_state = Get2DRasterizerState();
    rasterizer_state.ScissorEnable = Diligent::True;  // With scissor test

    loader->tilemap.BuildPipeline(&tilemap, blend_state, rasterizer_state,
                                  depth_stencil_state, primitive_topology,
                                  {target_format}, depth_stencil_format,
                                  default_sample);
    loader->tilemap2.BuildPipeline(&tilemap2, blend_state, rasterizer_state,
                                   depth_stencil_state, primitive_topology,
                                   {target_format}, depth_stencil_format,
                                   default_sample);
  }

  {  // Transition - no scissor - no depth
    // No blend
    Diligent::BlendStateDesc blend_state;

    // Disable depth test
    Diligent::DepthStencilStateDesc depth_stencil_state =
        GetDefaultDepthStencilState(false);

    Diligent::RasterizerStateDesc rasterizer_state = Get2DRasterizerState();
    rasterizer_state.ScissorEnable = Diligent::False;  // With scissor test

    loader->alphatrans.BuildPipeline(
        &alpha_transition, blend_state, rasterizer_state, depth_stencil_state,
        primitive_topology, {target_format}, Diligent::TEX_FORMAT_UNKNOWN,
        default_sample);
    loader->mappedtrans.BuildPipeline(
        &vague_transition, blend_state, rasterizer_state, depth_stencil_state,
        primitive_topology, {target_format}, Diligent::TEX_FORMAT_UNKNOWN,
        default_sample);
  }

  {  // YUV - no scissor - no depth
    // No blend
    Diligent::BlendStateDesc blend_state;

    // Disable depth test
    Diligent::DepthStencilStateDesc depth_stencil_state =
        GetDefaultDepthStencilState(false);

    Diligent::RasterizerStateDesc rasterizer_state = Get2DRasterizerState();
    rasterizer_state.ScissorEnable = Diligent::False;  // With scissor test

    loader->yuv.BuildPipeline(&yuv, blend_state, rasterizer_state,
                              depth_stencil_state, primitive_topology,
                              {target_format}, Diligent::TEX_FORMAT_UNKNOWN,
                              default_sample);
  }

  {  // Upscale - no scissor - no depth
      Diligent::BlendStateDesc blend_state;

      Diligent::DepthStencilStateDesc depth_stencil_state =
          GetDefaultDepthStencilState(false);

      Diligent::RasterizerStateDesc rasterizer_state = Get2DRasterizerState();
      rasterizer_state.ScissorEnable = Diligent::False;

      loader->upscale.BuildPipeline(
          &upscale, blend_state, rasterizer_state, depth_stencil_state,
          Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, {target_format},
          Diligent::TEX_FORMAT_UNKNOWN, default_sample);
    }

    {  // Anime4K passes - no scissor - no depth
      Diligent::BlendStateDesc blend_state;

      Diligent::DepthStencilStateDesc depth_stencil_state =
          GetDefaultDepthStencilState(false);

      Diligent::RasterizerStateDesc rasterizer_state = Get2DRasterizerState();
      rasterizer_state.ScissorEnable = Diligent::False;

      auto make_anime4k_pso = [&](renderer::RenderPipelineBase& lp,
                                  PipelineObject& out) {
        lp.BuildPipeline(&out, blend_state, rasterizer_state,
                         depth_stencil_state,
                         Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                         {target_format}, Diligent::TEX_FORMAT_UNKNOWN,
                         default_sample);
      };

      make_anime4k_pso(loader->anime4k_enhance, anime4k_enhance);
    }

    {  // CAS - no scissor - no depth
      Diligent::BlendStateDesc blend_state;

      Diligent::DepthStencilStateDesc depth_stencil_state =
          GetDefaultDepthStencilState(false);

      Diligent::RasterizerStateDesc rasterizer_state = Get2DRasterizerState();
      rasterizer_state.ScissorEnable = Diligent::False;

      loader->cas.BuildPipeline(
          &cas, blend_state, rasterizer_state, depth_stencil_state,
          Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, {target_format},
          Diligent::TEX_FORMAT_UNKNOWN, default_sample);
    }

    {  // Anime4K Mode A passes - no scissor - no depth
      Diligent::BlendStateDesc blend_state;

      Diligent::DepthStencilStateDesc depth_stencil_state =
          GetDefaultDepthStencilState(false);

      Diligent::RasterizerStateDesc rasterizer_state = Get2DRasterizerState();
      rasterizer_state.ScissorEnable = Diligent::False;

      auto make_a4a = [&](renderer::RenderPipelineBase& lp,
                           PipelineObject& out) {
        lp.BuildPipeline(&out, blend_state, rasterizer_state,
                         depth_stencil_state,
                         Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                         {target_format}, Diligent::TEX_FORMAT_UNKNOWN,
                         default_sample);
      };

      make_a4a(loader->anime4k_clamp_hl_pass0, anime4k_clamp_hl_pass0);
      make_a4a(loader->anime4k_clamp_hl_pass1, anime4k_clamp_hl_pass1);
      make_a4a(loader->anime4k_clamp_hl_pass2, anime4k_clamp_hl_pass2);
      make_a4a(loader->anime4k_restore_pass0, anime4k_restore_pass0);
      make_a4a(loader->anime4k_restore_pass1, anime4k_restore_pass1);
      make_a4a(loader->anime4k_restore_pass2, anime4k_restore_pass2);
      make_a4a(loader->anime4k_restore_pass3, anime4k_restore_pass3);
      make_a4a(loader->anime4k_restore_pass4, anime4k_restore_pass4);
      make_a4a(loader->anime4k_restore_pass5, anime4k_restore_pass5);
      make_a4a(loader->anime4k_restore_pass6, anime4k_restore_pass6);
      make_a4a(loader->anime4k_restore_pass7, anime4k_restore_pass7);
      make_a4a(loader->anime4k_upscale_pass0, anime4k_upscale_pass0);
      make_a4a(loader->anime4k_upscale_pass1, anime4k_upscale_pass1);
      make_a4a(loader->anime4k_upscale_pass2, anime4k_upscale_pass2);
      make_a4a(loader->anime4k_upscale_pass3, anime4k_upscale_pass3);
      make_a4a(loader->anime4k_upscale_pass4, anime4k_upscale_pass4);
      make_a4a(loader->anime4k_upscale_pass5, anime4k_upscale_pass5);
      make_a4a(loader->anime4k_upscale_pass6, anime4k_upscale_pass6);
      make_a4a(loader->anime4k_upscale_pass7, anime4k_upscale_pass7);
      make_a4a(loader->anime4k_upscale_pass8, anime4k_upscale_pass8);
    }

    {  // Anime4K Upscale_Denoise_L passes
      Diligent::BlendStateDesc blend_state;

      Diligent::DepthStencilStateDesc depth_stencil_state =
          GetDefaultDepthStencilState(false);

      Diligent::RasterizerStateDesc rasterizer_state = Get2DRasterizerState();
      rasterizer_state.ScissorEnable = Diligent::False;

      // Intermediate textures use RGBA16F for CNN feature values
      const auto intermediate_format = Diligent::TEX_FORMAT_RGBA16_FLOAT;

      // Pass0: 1 input → MRT (2 outputs), RGBA16F intermediate
      loader->anime4k_udl_pass0.BuildPipeline(
          &anime4k_udl_pass0, blend_state, rasterizer_state,
          depth_stencil_state,
          Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
          {intermediate_format, intermediate_format},
          Diligent::TEX_FORMAT_UNKNOWN, default_sample);

      // Pass1: 2 inputs → MRT (2 outputs), RGBA16F intermediate
      loader->anime4k_udl_pass1.BuildPipeline(
          &anime4k_udl_pass1, blend_state, rasterizer_state,
          depth_stencil_state,
          Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
          {intermediate_format, intermediate_format},
          Diligent::TEX_FORMAT_UNKNOWN, default_sample);

      // Pass2: 2 inputs → MRT (2 outputs), RGBA16F intermediate
      loader->anime4k_udl_pass2.BuildPipeline(
          &anime4k_udl_pass2, blend_state, rasterizer_state,
          depth_stencil_state,
          Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
          {intermediate_format, intermediate_format},
          Diligent::TEX_FORMAT_UNKNOWN, default_sample);

      // Pass3: 3 inputs → single output, RGBA8 final
      loader->anime4k_udl_pass3.BuildPipeline(
          &anime4k_udl_pass3, blend_state, rasterizer_state,
          depth_stencil_state,
          Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
          {target_format}, Diligent::TEX_FORMAT_UNKNOWN,
          default_sample);
    }

  {  // Window (present) - with scissor - with depth
    Diligent::BlendStateDesc blend_state;
    blend_state.RenderTargets[0] = GetBlendState(BLEND_TYPE_NORMAL);

    // Enable depth test
    Diligent::DepthStencilStateDesc depth_stencil_state =
        GetDefaultDepthStencilState(true);

    Diligent::RasterizerStateDesc rasterizer_state = Get2DRasterizerState();
    rasterizer_state.ScissorEnable = Diligent::True;  // With scissor test

    // Base with blend
    loader->base.BuildPipeline(&window, blend_state, rasterizer_state,
                               depth_stencil_state, primitive_topology,
                               {target_format}, depth_stencil_format,
                               default_sample);

    // Enable stencil (Read-only)
    depth_stencil_state.StencilEnable = Diligent::True;
    depth_stencil_state.StencilWriteMask = 0x00;
    depth_stencil_state.StencilReadMask = 0xFF;

    // Pass stencil test if ref value
    depth_stencil_state.FrontFace.StencilFunc = Diligent::COMPARISON_FUNC_EQUAL;
    depth_stencil_state.BackFace = depth_stencil_state.FrontFace;

    // Stencil with blend
    loader->base.BuildPipeline(&window_with_stencil, blend_state,
                               rasterizer_state, depth_stencil_state,
                               primitive_topology, {target_format},
                               depth_stencil_format, default_sample);
  }

  {  // Color stencil write - no scissor - with depth
     // Enable depth test
    Diligent::DepthStencilStateDesc depth_stencil_state =
        GetDefaultDepthStencilState(true);

    // Enable stencil (Read-only)
    depth_stencil_state.StencilEnable = Diligent::True;
    depth_stencil_state.StencilWriteMask = 0xFF;
    depth_stencil_state.StencilReadMask = 0x00;

    // Always write stencil ref value
    depth_stencil_state.FrontFace.StencilFailOp = Diligent::STENCIL_OP_REPLACE;
    depth_stencil_state.FrontFace.StencilDepthFailOp =
        Diligent::STENCIL_OP_REPLACE;
    depth_stencil_state.FrontFace.StencilPassOp = Diligent::STENCIL_OP_REPLACE;
    depth_stencil_state.BackFace = depth_stencil_state.FrontFace;

    Diligent::RasterizerStateDesc rasterizer_state = Get2DRasterizerState();
    rasterizer_state.ScissorEnable = Diligent::True;  // With scissor test

    // No color write
    Diligent::BlendStateDesc blend_state;
    blend_state.RenderTargets[0].BlendEnable = Diligent::True;
    blend_state.RenderTargets[0].RenderTargetWriteMask =
        Diligent::COLOR_MASK_NONE;

    loader->color.BuildPipeline(&color_write_stencil, blend_state,
                                rasterizer_state, depth_stencil_state,
                                primitive_topology, {target_format},
                                depth_stencil_format, default_sample);
  }

  {  // Window (base texture render) - no scissor - no depth
    // With blend
    Diligent::BlendStateDesc blend_state;
    blend_state.RenderTargets[0] = GetBlendState(BLEND_TYPE_NORMAL);

    // Enable depth test
    Diligent::DepthStencilStateDesc depth_stencil_state =
        GetDefaultDepthStencilState(false);

    Diligent::RasterizerStateDesc rasterizer_state = Get2DRasterizerState();
    rasterizer_state.ScissorEnable = Diligent::False;  // No scissor test

    loader->viewport.BuildPipeline(
        &window2.viewport, blend_state, rasterizer_state, depth_stencil_state,
        primitive_topology, {target_format}, Diligent::TEX_FORMAT_UNKNOWN,
        default_sample);
    loader->base.BuildPipeline(&window2.base, blend_state, rasterizer_state,
                               depth_stencil_state, primitive_topology,
                               {target_format}, Diligent::TEX_FORMAT_UNKNOWN,
                               default_sample);

    // Keep alpha
    blend_state.RenderTargets[0] = GetBlendState(BLEND_TYPE_KEEP_ALPHA);

    loader->viewport.BuildPipeline(
        &window2.viewport_alpha, blend_state, rasterizer_state,
        depth_stencil_state, primitive_topology, {target_format},
        Diligent::TEX_FORMAT_UNKNOWN, default_sample);
  }
}

}  // namespace content
