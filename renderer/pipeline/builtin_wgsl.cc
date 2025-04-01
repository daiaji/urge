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

@vertex
fn vertexMain(
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

@fragment
fn fragmentMain(vertex: VertexOutput) -> @location(0) vec4f {
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

@vertex
fn vertexMain(
    @location(0) pos: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) color: vec4<f32>) -> VertexOutput {
  var result: VertexOutput;
  result.pos = u_transform.projMat * pos;
  result.pos = u_transform.transMat * result.pos;
  result.color = color;
  return result;
}

@fragment
fn fragmentMain(vertex: VertexOutput) -> @location(0) vec4f {
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

@vertex
fn vertexMain(
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

@fragment
fn fragmentMain(vertex: VertexOutput) -> @location(0) vec4f {
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

struct SpriteVertex {
  pos: vec4<f32>,
  uv: vec2<f32>,
  color: vec4<f32>,
};

struct EffectParams {
  position: vec2<f32>,
  origin: vec2<f32>,
  scale: vec2<f32>,
  rotation: f32,  

  color: vec4<f32>,
  tone: vec4<f32>,
  opacity: f32,
  bushDepth: f32,
  bushOpacity: f32,
};

struct VertexOutput {
  @builtin(position) pos: vec4<f32>,
  @location(0) uv: vec2<f32>,
  @location(1) color: vec4<f32>,
  @location(2) @interpolate(flat) instanceIdx: u32,
};

@group(0) @binding(0) var<uniform> u_transform: WorldMatrix;
@group(1) @binding(0) var u_texture: texture_2d<f32>;
@group(1) @binding(1) var u_sampler: sampler;
@group(1) @binding(2) var<uniform> u_texSize: vec2<f32>;
@group(2) @binding(0) var<storage, read> u_vertices: array<SpriteVertex>;
@group(2) @binding(1) var<storage, read> u_effects: array<EffectParams>;

const lumaF: vec3<f32> = vec3<f32>(0.299, 0.587, 0.114);

@vertex
fn vertexMain(
    @location(0) pos: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) color: vec4<f32>,
    @builtin(vertex_index) vertexIdx: u32,
    @builtin(instance_index) instanceIdx: u32
) -> VertexOutput {
  let effect = u_effects[instanceIdx];
  let vert = u_vertices[vertexIdx + 4 * instanceIdx];

  let sine = sin(effect.rotation);
  let cosine = cos(effect.rotation);

  let sxs = effect.scale.x * sine;
  let sys = effect.scale.y * sine;
  let sxc = effect.scale.x * cosine;
  let syc = effect.scale.y * cosine;

  let tx = -effect.origin.x * sxc - effect.origin.y * sys + effect.position.x;
  let ty = effect.origin.x * sxs - effect.origin.y * syc + effect.position.y;

  let trans_pos = vec4<f32>(
    vert.pos.x * sxc + vert.pos.y * sys + tx,
    -vert.pos.x * sxs + vert.pos.y * syc + ty,
    vert.pos.z,
    vert.pos.w
  );

  var result: VertexOutput;
  result.pos = u_transform.projMat * trans_pos;
  result.pos = u_transform.transMat * result.pos;
  result.uv = u_texSize * vert.uv;
  result.color = vert.color;
  result.instanceIdx = instanceIdx;
  return result;
}

@fragment
fn fragmentMain(vertex: VertexOutput) -> @location(0) vec4f {
  let effect = u_effects[vertex.instanceIdx];
  var frag = textureSample(u_texture, u_sampler, vertex.uv);
  
  // Tone
  let luma = dot(frag.rgb, lumaF);
  frag = vec4(mix(frag.rgb, vec3(luma), effect.tone.w), frag.a);
  frag = vec4(frag.rgb + effect.tone.rgb, frag.a);
  
  // Color
  frag.a *= effect.opacity;
  frag = vec4(mix(frag.rgb, effect.color.rgb, effect.color.a), frag.a);

  // Bush
  let currentPos = vertex.uv.y / u_texSize.y;
  let underBush = select(1.0, 0.0, currentPos > effect.bushDepth);
  frag.a *= clamp(effect.bushOpacity + underBush, 0.0, 1.0);

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

@vertex
fn vertexMain(
    @location(0) pos: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) color: vec4<f32>) -> VertexOutput {
  var result: VertexOutput;
  result.pos = pos;
  result.uv = uv;
  result.color = color;
  return result;
}

@fragment
fn fragmentMain(vertex: VertexOutput) -> @location(0) vec4f {
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

@vertex
fn vertexMain(
    @location(0) pos: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) color: vec4<f32>) -> VertexOutput {
  var result: VertexOutput;
  result.pos = pos;
  result.uv = uv;
  result.color = color;
  return result;
}

@fragment
fn fragmentMain(vertex: VertexOutput) -> @location(0) vec4f {
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

const std::string kTilemapRenderWGSL = R"(

struct WorldMatrix {
  projMat: mat4x4<f32>,
  transMat: mat4x4<f32>,
};

struct VertexOutput {
  @builtin(position) pos: vec4<f32>,
  @location(0) uv: vec2<f32>,
  @location(1) color: vec4<f32>,
};

struct EffectParams {
  offset: vec2<f32>,
  tileSize: f32,
  animateIndex: f32,
};

@group(0) @binding(0) var<uniform> u_transform: WorldMatrix;
@group(1) @binding(0) var u_texture: texture_2d<f32>;
@group(1) @binding(1) var u_sampler: sampler;
@group(1) @binding(2) var<uniform> u_texSize: vec2<f32>;
@group(2) @binding(0) var<uniform> u_effect: EffectParams;

const kAutotileArea: vec2<f32> = vec2<f32>(3.0, 28.0);

@vertex
fn vertexMain(
    @location(0) pos: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) color: vec4<f32>) -> VertexOutput {
  var trans_pos = pos;
  var tex = uv;

  // Apply tilemap offset
  trans_pos.x += u_effect.offset.x;
  trans_pos.y += u_effect.offset.y;

  // Animated area
	let addition = select(0.0, 1.0, tex.x <= kAutotileArea.x * u_effect.tileSize && tex.y <= kAutotileArea.y * u_effect.tileSize);
	tex.x += 3.0 * u_effect.tileSize * u_effect.animateIndex * addition;

  var result: VertexOutput;
  result.pos = u_transform.projMat * trans_pos;
  result.pos = u_transform.transMat * result.pos;
  result.uv = u_texSize * tex;
  result.color = color;
  return result;
}

@fragment
fn fragmentMain(vertex: VertexOutput) -> @location(0) vec4f {
  var tex = textureSample(u_texture, u_sampler, vertex.uv);
  tex = vec4(mix(tex.rgb, vertex.color.rgb, vertex.color.a), tex.a);
  return tex;
}

)";

const std::string kTilemap2RenderWGSL = R"(

struct WorldMatrix {
  projMat: mat4x4<f32>,
  transMat: mat4x4<f32>,
};

struct VertexOutput {
  @builtin(position) pos: vec4<f32>,
  @location(0) uv: vec2<f32>,
  @location(1) color: vec4<f32>,
};

struct EffectParams {
  offset: vec2<f32>,
  animationOffset: vec2<f32>,
  tileSize: f32,
};

@group(0) @binding(0) var<uniform> u_transform: WorldMatrix;
@group(1) @binding(0) var u_texture: texture_2d<f32>;
@group(1) @binding(1) var u_sampler: sampler;
@group(1) @binding(2) var<uniform> u_texSize: vec2<f32>;
@group(2) @binding(0) var<uniform> u_effect: EffectParams;

const kRegularArea: vec2<f32> = vec2<f32>(12.0, 12.0);
const kWaterfallArea: vec4<f32> = vec4<f32>(12.0, 0.0, 4.0, 12.0);
const kWaterfallAutotileArea: vec4<f32> = vec4<f32>(12.0, 0.0, 2.0, 6.0);

fn posInArea(pos: vec2<f32>, area: vec4<f32>) -> f32 {
	return select(0.0, 1.0, pos.x >= area.x && pos.y >= area.y && pos.x <= (area.x + area.z) && pos.y <= (area.y + area.w));
}

@vertex
fn vertexMain(
    @location(0) pos: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) color: vec4<f32>) -> VertexOutput {
  var trans_pos = pos;
  var tex = uv;

  // Apply tilemap offset
  trans_pos.x += u_effect.offset.x;
  trans_pos.y += u_effect.offset.y;

  // Regular area
	let addition1 = select(0.0, 1.0, tex.x <= kRegularArea.x * u_effect.tileSize && tex.y <= kRegularArea.y * u_effect.tileSize);
	tex.x += u_effect.animationOffset.x * addition1;

	// Waterfall area
	let addition2 = posInArea(tex, kWaterfallArea * u_effect.tileSize) - posInArea(tex, kWaterfallAutotileArea * u_effect.tileSize);
	tex.y += u_effect.animationOffset.y * addition2;

  var result: VertexOutput;
  result.pos = u_transform.projMat * trans_pos;
  result.pos = u_transform.transMat * result.pos;
  result.uv = u_texSize * tex;
  result.color = color;
  return result;
}

@fragment
fn fragmentMain(vertex: VertexOutput) -> @location(0) vec4f {
  var tex = textureSample(u_texture, u_sampler, vertex.uv);
  tex = vec4(mix(tex.rgb, vertex.color.rgb, vertex.color.a), tex.a);
  return tex;
}

)";

}  // namespace renderer
