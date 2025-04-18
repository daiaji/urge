// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_PIPELINE_BUILTIN_HLSL_H_
#define RENDERER_PIPELINE_BUILTIN_HLSL_H_

#include <string>

namespace renderer {

///
// type:
//   basis shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { float4x4, float4x4 }
//   { Texture2D }
///
extern const std::string kHLSL_BaseRender_VertexShader;
extern const std::string kHLSL_BaseRender_PixelShader;

///
// type:
//   color shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { float4x4, float4x4 }
///
extern const std::string kHLSL_ColorRender_VertexShader;
extern const std::string kHLSL_ColorRender_PixelShader;

///
// type:
//   flat shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { float4x4, float4x4 }
//   { Texture2D }
//   { float4, float4 }
///
extern const std::string kHLSL_FlatRender_VertexShader;
extern const std::string kHLSL_FlatRender_PixelShader;

///
// type:
//   sprite shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { float4x4, float4x4 }
//   { Texture2D }
//   < { float4, float2 } >
//   < { float4, float4, float2, float2, float2, float, float, float, float } >
///
extern const std::string kHLSL_SpriteRender_VertexShader;
extern const std::string kHLSL_SpriteRender_PixelShader;

///
// type:
//   alpha transition shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// bind:
//   { Texture2D, Texture2D }
///
extern const std::string kHLSL_AlphaTransitionRender_VertexShader;
extern const std::string kHLSL_AlphaTransitionRender_PixelShader;

///
// type:
//   mapping transition shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// bind:
//   { Texture2D, Texture2D, Texture2D }
///
extern const std::string kHLSL_MappingTransitionRender_VertexShader;
extern const std::string kHLSL_MappingTransitionRender_PixelShader;

///
// type:
//   tilemap shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// bind:
//   { float4x4, float4x4 }
//   { Texture2D }
//   { float2, float2, float, float }
///
extern const std::string kHLSL_TilemapRender_VertexShader;
extern const std::string kHLSL_TilemapRender_PixelShader;

///
// type:
//   tilemap2 shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// bind:
//   { float4x4, float4x4 }
//   { Texture2D }
//   { float2, float2, float2, float }
///
extern const std::string kHLSL_Tilemap2Render_VertexShader;
extern const std::string kHLSL_Tilemap2Render_PixelShader;

///
// type:
//   present shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { float4x4, float4x4 }
//   { Texture2D }
//
// defination:
//   CONVERT_PS_OUTPUT_TO_GAMMA
///
extern const std::string kHLSL_PresentRender_VertexShader;
extern const std::string kHLSL_PresentRender_PixelShader;

}  // namespace renderer

#endif  // !RENDERER_PIPELINE_BUILTIN_HLSL_H_
