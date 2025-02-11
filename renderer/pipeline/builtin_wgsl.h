// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_PIPELINE_BUILTIN_WGSL_H_
#define RENDERER_PIPELINE_BUILTIN_WGSL_H_

#include <string>

namespace renderer {

///
// type:
//   basis shader
///
// entry:
//   vertexMain fragmentMain
///
// vertex:
//   @<0>: vec4<f32>
//   @<1>: vec2<f32>
//   @<2>: vec4<f32>
///
// bind:
//   @<0>: mat4x4, mat4x4
//   @<1>: texture2d<f32>, sampler, vec2
///
extern const std::string kBaseRenderWGSL;

///
// type:
//   color shader
///
// entry:
//   vertexMain fragmentMain
///
// vertex:
//   @<0>: vec4<f32>
//   @<1>: vec2<f32>
//   @<2>: vec4<f32>
///
// bind:
//   @<0>: mat4x4
///
extern const std::string kColorRenderWGSL;

///
// type:
//   viewport shader
///
// entry:
//   vertexMain fragmentMain
///
// vertex:
//   @<0>: vec4<f32>
//   @<1>: vec2<f32>
//   @<2>: vec4<f32>
///
// bind:
//   @<0>: mat4x4
//   @<1>: texture2d<f32>, sampler, vec2
//   @<2>: vec4, vec4
///
extern const std::string kViewportBaseRenderWGSL;

///
// type:
//   sprite shader
///
// entry:
//   vertexMain fragmentMain
///
// vertex:
//   @<0>: vec4<f32>
//   @<1>: vec2<f32>
//   @<2>: vec4<f32>
///
// bind:
//   @<0>: mat4x4
//   @<1>: texture2d<f32>, sampler, vec2
//   @<2>: vec4, vec4, f32, f32
///
extern const std::string kSpriteRenderWGSL;

///
// type:
//   alpha transition shader
///
// entry:
//   vertexMain fragmentMain
///
// vertex:
//   @<0>: vec4<f32>
//   @<1>: vec2<f32>
//   @<2>: vec4<f32>
///
// bind:
//   @<0>: texture2d<f32>, texture2d<f32>, sampler
///
extern const std::string kAlphaTransitionRenderWGSL;

///
// type:
//   mapped transition shader
///
// entry:
//   vertexMain fragmentMain
///
// vertex:
//   @<0>: vec4<f32>
//   @<1>: vec2<f32>
//   @<2>: vec4<f32>
///
// bind:
//   @<0>: texture2d<f32>, texture2d<f32>, texture2d<f32>, sampler
///
extern const std::string kMappedTransitionRenderWGSL;

}  // namespace renderer

#endif  //! RENDERER_PIPELINE_BUILTIN_WGSL_H_
