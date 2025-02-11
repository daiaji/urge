// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/pipeline/builtin_wgsl.h"

namespace renderer {

const std::string kBaseRenderWGSL = R"(

struct WorldMatrix {
  projMat: mat4x4<f32>,
  transMat: mat4x4<f32>,
};

struct VertexOutput {
  @builtin(position) pos: vec4<f32>,
  @location(0) uv: vec2<f32>,
  @location(1) color: vec4<f32>,
};

@group(0) @binding(0) var<uniform> u_transform: WorldMatrix;
@group(1) @binding(0) var u_texture: texture_2d<f32>;
@group(1) @binding(1) var u_sampler: sampler;
@group(1) @binding(2) var<uniform> u_texSize: vec2<f32>;

@vertex fn vertexMain(
    @location(0) pos: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) color: vec4<f32>) -> VertexOutput {
  var result: VertexOutput;
  result.pos = u_transform.projMat * pos;
  result.pos = u_transform.transMat * result.pos;
  result.uv = u_texSize * uv;
  result.color = color;
  return result;
}

@fragment fn fragmentMain(vertex: VertexOutput) -> @location(0) vec4f {
  var tex = textureSample(u_texture, u_sampler, vertex.uv);
  tex.a *= vertex.color.a;
  return tex;
}

)";

const std::string kColorRenderWGSL = R"(

struct WorldMatrix {
  projMat: mat4x4<f32>,
  transMat: mat4x4<f32>,
};

struct VertexOutput {
  @builtin(position) pos: vec4<f32>,
  @location(0) color: vec4<f32>,
};

@group(0) @binding(0) var<uniform> u_transform: WorldMatrix;

@vertex fn vertexMain(
    @location(0) pos: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) color: vec4<f32>) -> VertexOutput {
  var result: VertexOutput;
  result.pos = u_transform.projMat * pos;
  result.pos = u_transform.transMat * result.pos;
  result.color = color;
  return result;
}

@fragment fn fragmentMain(vertex: VertexOutput) -> @location(0) vec4f {
  return vertex.color;
}

)";

const std::string kViewportBaseRenderWGSL = R"(

struct WorldMatrix {
  projMat: mat4x4<f32>,
  transMat: mat4x4<f32>,
};

struct EffectParams {
  color: vec4<f32>,
  tone: vec4<f32>,
};

struct VertexOutput {
  @builtin(position) pos: vec4<f32>,
  @location(0) uv: vec2<f32>,
  @location(1) color: vec4<f32>,
};

@group(0) @binding(0) var<uniform> u_transform: WorldMatrix;
@group(1) @binding(0) var u_texture: texture_2d<f32>;
@group(1) @binding(1) var u_sampler: sampler;
@group(1) @binding(2) var<uniform> u_texSize: vec2<f32>;
@group(2) @binding(0) var<uniform> u_effect: EffectParams;

const lumaF: vec3<f32> = vec3<f32>(0.299, 0.587, 0.114);

@vertex fn vertexMain(
    @location(0) pos: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) color: vec4<f32>) -> VertexOutput {
  var result: VertexOutput;
  result.pos = u_transform.projMat * pos;
  result.pos = u_transform.transMat * result.pos;
  result.uv = u_texSize * uv;
  result.color = color;
  return result;
}

@fragment fn fragmentMain(vertex: VertexOutput) -> @location(0) vec4f {
  var frag: vec4<f32> = textureSample(u_texture, u_sampler, vertex.uv);
  
  var luma: f32 = dot(frag.rgb, lumaF);
  frag = vec4<f32>(mix(frag.rgb, vec3<f32>(luma), u_effect.tone.w), frag.a);
  frag = vec4<f32>(frag.rgb + u_effect.tone.rgb, frag.a);
  
  frag.a *= vertex.color.a;
  frag = vec4<f32>(mix(frag.rgb, u_effect.color.rgb, u_effect.color.a), frag.a);

  return frag;
}

)";

const std::string kSpriteRenderWGSL = R"(

struct WorldMatrix {
  projMat: mat4x4<f32>,
  transMat: mat4x4<f32>,
};

struct EffectParams {
  transMat: mat4x4<f32>,
  color: vec4<f32>,
  tone: vec4<f32>,
  bushDepth: f32,
  bushOpacity: f32,
};

struct VertexOutput {
  @builtin(position) pos: vec4<f32>,
  @location(0) uv: vec2<f32>,
  @location(1) color: vec4<f32>,
};

@group(0) @binding(0) var<uniform> u_transform: WorldMatrix;
@group(1) @binding(0) var u_texture: texture_2d<f32>;
@group(1) @binding(1) var u_sampler: sampler;
@group(1) @binding(2) var<uniform> u_texSize: vec2<f32>;
@group(2) @binding(0) var<uniform> u_effect: EffectParams;

const lumaF: vec3<f32> = vec3<f32>(0.299, 0.587, 0.114);

@vertex fn vertexMain(
    @location(0) pos: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) color: vec4<f32>) -> VertexOutput {
  var result: VertexOutput;
  var sprite_pos = u_effect.transMat * pos;
  result.pos = u_transform.projMat * sprite_pos;
  result.pos = u_transform.transMat * result.pos;
  result.uv = u_texSize * uv;
  result.color = color;
  return result;
}

@fragment fn fragmentMain(vertex: VertexOutput) -> @location(0) vec4f {
  var frag: vec4<f32> = textureSample(u_texture, u_sampler, vertex.uv);
  
  var luma: f32 = dot(frag.rgb, lumaF);
  frag = vec4<f32>(mix(frag.rgb, vec3<f32>(luma), u_effect.tone.w), frag.a);
  frag = vec4<f32>(frag.rgb + u_effect.tone.rgb, frag.a);
  
  frag.a *= vertex.color.a;
  frag = vec4<f32>(mix(frag.rgb, u_effect.color.rgb, u_effect.color.a), frag.a);

  let currentPos = vertex.uv.y / u_texSize.y;
  let underBush = select(0.0, 1.0, u_effect.bushDepth > currentPos);
  frag.a *= clamp(u_effect.bushOpacity + underBush, 0.0, 1.0);

  return frag;
}

)";

const std::string kAlphaTransitionRenderWGSL = R"(

struct VertexOutput {
  @builtin(position) pos: vec4<f32>,
  @location(0) uv: vec2<f32>,
  @location(1) color: vec4<f32>,
};

@group(0) @binding(0) var u_frozenTexture: texture_2d<f32>;
@group(0) @binding(1) var u_currentTexture: texture_2d<f32>;
@group(0) @binding(2) var u_sampler: sampler;

@vertex fn vertexMain(
    @location(0) pos: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) color: vec4<f32>) -> VertexOutput {
  var result: VertexOutput;
  result.pos = pos;
  result.uv = uv;
  result.color = color;
  return result;
}

@fragment fn fragmentMain(vertex: VertexOutput) -> @location(0) vec4f {
  var frozenFrag = textureSample(u_frozenTexture, u_sampler, vertex.uv);
  var currentFrag = textureSample(u_currentTexture, u_sampler, vertex.uv);
  return mix(frozenFrag, currentFrag, vertex.color.a);
}

)";

const std::string kMappedTransitionRenderWGSL = R"(

struct VertexOutput {
  @builtin(position) pos: vec4<f32>,
  @location(0) uv: vec2<f32>,
  @location(1) color: vec4<f32>,
};

@group(0) @binding(0) var u_frozenTexture: texture_2d<f32>;
@group(0) @binding(1) var u_currentTexture: texture_2d<f32>;
@group(0) @binding(2) var u_transTexture: texture_2d<f32>;
@group(0) @binding(3) var u_sampler: sampler;

@vertex fn vertexMain(
    @location(0) pos: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) color: vec4<f32>) -> VertexOutput {
  var result: VertexOutput;
  result.pos = pos;
  result.uv = uv;
  result.color = color;
  return result;
}

@fragment fn fragmentMain(vertex: VertexOutput) -> @location(0) vec4f {
  var frozenFrag = textureSample(u_frozenTexture, u_sampler, vertex.uv);
  var currentFrag = textureSample(u_currentTexture, u_sampler, vertex.uv);
  var transValue = textureSample(u_transTexture, u_sampler, vertex.uv).r;

  var vague = vertex.color.r;
  var progress = vertex.color.a;

  transValue = clamp(transValue, progress, progress + vague);
  var mixAlpha = (transValue - progress) / vague;

  return mix(currentFrag, frozenFrag, mixAlpha);
}

)";

}  // namespace renderer
