// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/pipeline/builtin_hlsl.h"

namespace renderer {

///
// type:
//   basis shader
///
// entry:
//   vertex: VSMain
//   pixel: PSMain
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { float4x4, float4x4 }
//   { Texture2D }
///

const std::string kHLSL_BaseRender_Vertex = R"(
struct WorldMatrix {
  float4x4 ProjMat;
  float4x4 TransMat;
};

cbuffer WorldMatrixBuffer {
  WorldMatrix u_Transform;
};

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

void VSMain(in VSInput VSIn, out PSInput PSIn) {
  PSIn.Pos = mul(u_Transform.TransMat, VSIn.Pos);
  PSIn.Pos = mul(u_Transform.ProjMat, PSIn.Pos);
  
  PSIn.UV = VSIn.UV;
  PSIn.Color = VSIn.Color;
}
)";

const std::string kHLSL_BaseRender_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

struct PSOutput {
  float4 Color : SV_TARGET;
};

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float4 frag = u_Texture.Sample(u_Texture_sampler, PSIn.UV);
  frag.a *= PSIn.Color.a;
  PSOut.Color = frag;
}
)";

///
// type:
//   bitmapblt shader
///
// entry:
//   vertex: VSMain
//   pixel: PSMain
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { float4x4, float4x4 }
//   { Texture2D }
//   { Texture2D }
///

const std::string kHLSL_BitmapBltRender_Vertex = R"(
struct WorldMatrix {
  float4x4 ProjMat;
  float4x4 TransMat;
};

cbuffer WorldMatrixBuffer {
  WorldMatrix u_Transform;
};

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : TEXCOORD1;
  float Alpha : NORMAL0;
};

void VSMain(in VSInput VSIn, out PSInput PSIn) {
  PSIn.Pos = mul(u_Transform.TransMat, VSIn.Pos);
  PSIn.Pos = mul(u_Transform.ProjMat, PSIn.Pos);
  
  PSIn.UV = VSIn.UV;
  PSIn.Color = VSIn.Color;
  PSIn.Alpha = VSIn.Color.w;
}
)";

const std::string kHLSL_BitmapBltRender_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : TEXCOORD1;
  float Alpha : NORMAL0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

Texture2D u_DstTexture;
SamplerState u_DstTexture_sampler;

struct PSOutput {
  float4 Color : SV_TARGET;
};

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float4 srcFrag = u_Texture.Sample(u_Texture_sampler, PSIn.UV);
  float4 dstFrag = u_DstTexture.Sample(u_DstTexture_sampler, PSIn.Color.xy);

  float srcAlpha = srcFrag.a * PSIn.Alpha;
  float3 resultRGB = (srcFrag.rgb * srcAlpha) + (dstFrag.rgb * dstFrag.a * (1.0 - srcAlpha));
  float resultAlpha = srcAlpha + (dstFrag.a * (1.0 - srcAlpha));
  
  resultRGB = (resultAlpha <= 0.0 ? srcFrag.rgb : resultRGB / resultAlpha);

  PSOut.Color.rgb = resultRGB;
  PSOut.Color.a = resultAlpha;
}
)";

///
// type:
//   bitmapclipblt shader
///
// entry:
//   vertex: VSMain
//   pixel: PSMain
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { float4x4, float4x4 }
//   { Texture2D }
//   { Texture2D }
//   { Texture2D }
///

const std::string kHLSL_BitmapClipBltRender_Vertex = R"(
struct WorldMatrix {
  float4x4 ProjMat;
  float4x4 TransMat;
};

cbuffer WorldMatrixBuffer {
  WorldMatrix u_Transform;
};

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;

  uint VertexIdx : SV_VertexID;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float2 ClipUV : TEXCOORD1;
  float4 Color : TEXCOORD2;
};

void VSMain(in VSInput VSIn, out PSInput PSIn) {
  PSIn.Pos = mul(u_Transform.TransMat, VSIn.Pos);
  PSIn.Pos = mul(u_Transform.ProjMat, PSIn.Pos);
  
  float2 posUV[4];
  posUV[0] = float2(0.0, 0.0); // LT
  posUV[1] = float2(1.0, 0.0); // RT
  posUV[2] = float2(1.0, 1.0); // RB
  posUV[3] = float2(0.0, 1.0); // LB

  PSIn.UV = VSIn.UV;
  PSIn.ClipUV = posUV[VSIn.VertexIdx];
  PSIn.Color = VSIn.Color;
}
)";

const std::string kHLSL_BitmapClipBltRender_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float2 ClipUV : TEXCOORD1;
  float4 Color : TEXCOORD2;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

Texture2D u_ClipTexture;
SamplerState u_ClipTexture_sampler;

Texture2D u_DstTexture;
SamplerState u_DstTexture_sampler;

struct PSOutput {
  float4 Color : SV_TARGET;
};

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float4 srcFrag = u_Texture.Sample(u_Texture_sampler, PSIn.UV);
  float4 clipFrag = u_ClipTexture.Sample(u_ClipTexture_sampler, PSIn.ClipUV);
  float4 dstFrag = u_DstTexture.Sample(u_DstTexture_sampler, PSIn.Color.xy);

  float srcAlpha = srcFrag.a * clipFrag.a;
  float3 resultRGB = (srcFrag.rgb * srcAlpha) + (dstFrag.rgb * dstFrag.a * (1.0 - srcAlpha));
  float resultAlpha = srcAlpha + (dstFrag.a * (1.0 - srcAlpha));
  
  resultRGB = (resultAlpha <= 0.0 ? srcFrag.rgb : resultRGB / resultAlpha);

  PSOut.Color.rgb = resultRGB;
  PSOut.Color.a = resultAlpha;
}
)";

///
// type:
//   color shader
///
// entry:
//   vertex: VSMain
//   pixel: PSMain
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { float4x4, float4x4 }
///

const std::string kHLSL_ColorRender_Vertex = R"(
struct WorldMatrix {
  float4x4 ProjMat;
  float4x4 TransMat;
};

cbuffer WorldMatrixBuffer {
  WorldMatrix u_Transform;
};

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

void VSMain(in VSInput VSIn, out PSInput PSIn) {
  PSIn.Pos = mul(u_Transform.TransMat, VSIn.Pos);
  PSIn.Pos = mul(u_Transform.ProjMat, PSIn.Pos);
  
  PSIn.UV = VSIn.UV;
  PSIn.Color = VSIn.Color;
}
)";

const std::string kHLSL_ColorRender_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  PSOut.Color = PSIn.Color;
}
)";

///
// type:
//   flat shader
///
// entry:
//   vertex: VSMain
//   pixel: PSMain
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { float4x4, float4x4 }
//   { Texture2D }
//   { float4, float4 }
///

const std::string kHLSL_FlatRender_Vertex = R"(
struct WorldMatrix {
  float4x4 ProjMat;
  float4x4 TransMat;
};

cbuffer WorldMatrixBuffer {
  WorldMatrix u_Transform;
};

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

void VSMain(in VSInput VSIn, out PSInput PSIn) {
  PSIn.Pos = mul(u_Transform.TransMat, VSIn.Pos);
  PSIn.Pos = mul(u_Transform.ProjMat, PSIn.Pos);
  
  PSIn.UV = VSIn.UV;
  PSIn.Color = VSIn.Color;
}
)";

const std::string kHLSL_FlatRender_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

struct FlatParams {
  float4 Color;
  float4 Tone;
};

cbuffer FlatUniformConstants {
  FlatParams u_Effect;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

struct PSOutput {
  float4 Color : SV_TARGET;
};

static const float3 lumaF = float3(0.299, 0.587, 0.114);

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float4 frag = u_Texture.Sample(u_Texture_sampler, PSIn.UV);

  // Tone
  float luma = dot(frag.rgb, lumaF);
  frag.rgb = lerp(frag.rgb, float3(luma, luma, luma), u_Effect.Tone.w);
  frag.rgb += u_Effect.Tone.rgb;

  // Opacity
  frag.a *= PSIn.Color.a;

  // Color
  frag.rgb = lerp(frag.rgb, u_Effect.Color.rgb, u_Effect.Color.a);

  PSOut.Color = frag;
}
)";

///
// type:
//   sprite shader
///
// entry:
//   vertex: VSMain
//   pixel: PSMain
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { float4x4, float4x4 }
//   { Texture2D }
//   < { float4, float2 } >
//   < { float2, float2, float2, float, float4, float4, float, float, float } >
///

const std::string kHLSL_SpriteRender_Vertex = R"(
struct WorldMatrix {
  float4x4 ProjMat;
  float4x4 TransMat;
};

cbuffer WorldMatrixBuffer {
  WorldMatrix u_Transform;
};

struct SpriteParam {
  float4 Color;
  float4 Tone;
  float4 Position;
  float4 Origin;
  float4 Scale;
  float4 Rotation;
  float4 Opacity;
  float4 BushDepthAndOpacity;
};

#if STORAGE_BUFFER_SUPPORT
StructuredBuffer<SpriteParam> u_Params;
#else
cbuffer SpriteUniformConstants {
  SpriteParam u_Effect;
};
#endif // STORAGE_BUFFER_SUPPORT

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;

  uint VertexIdx : SV_VertexID;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : NORMAL0;
  float4 Tone : NORMAL1;
  float Opacity : NORMAL2;
  float BushDepth : NORMAL3;
  float BushOpacity : NORMAL4;
};

void VSMain(in VSInput VSIn, out PSInput PSIn) {
#if STORAGE_BUFFER_SUPPORT
#if defined(GLSL)
  int vertex_id = int(VSIn.VertexIdx);
#else
  uint vertex_id = VSIn.VertexIdx;
#endif // GLSL
  SpriteParam effect = u_Params[vertex_id / 4];
#else
  SpriteParam effect = u_Effect;
#endif // STORAGE_BUFFER_SUPPORT

  float sine = sin(effect.Rotation.x);
  float cosine = cos(effect.Rotation.x);
  float sxs = effect.Scale.x * sine;
  float sxc = effect.Scale.x * cosine;
  float sys = effect.Scale.y * sine;
  float syc = effect.Scale.y * cosine;
  float tx = -effect.Origin.x * sxc - effect.Origin.y * sys + effect.Position.x;
  float ty = effect.Origin.x * sxs - effect.Origin.y * syc + effect.Position.y;

  float4 transPos;
  transPos.x = VSIn.Pos.x * sxc + VSIn.Pos.y * sys + tx;
  transPos.y = -VSIn.Pos.x * sxs + VSIn.Pos.y * syc + ty;
  transPos.z = VSIn.Pos.z;
  transPos.w = VSIn.Pos.w;

  PSIn.Pos = mul(u_Transform.TransMat, transPos);
  PSIn.Pos = mul(u_Transform.ProjMat, PSIn.Pos);
  
  PSIn.UV = VSIn.UV;
  PSIn.Color = effect.Color;
  PSIn.Tone = effect.Tone;
  PSIn.Opacity = effect.Opacity.x;
  PSIn.BushDepth = effect.BushDepthAndOpacity.x;
  PSIn.BushOpacity = effect.BushDepthAndOpacity.y;
}
)";

const std::string kHLSL_SpriteRender_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : NORMAL0;
  float4 Tone : NORMAL1;
  float Opacity : NORMAL2;
  float BushDepth : NORMAL3;
  float BushOpacity : NORMAL4;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

struct PSOutput {
  float4 Color : SV_TARGET;
};

static const float3 lumaF = float3(0.299, 0.587, 0.114);

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float4 frag = u_Texture.Sample(u_Texture_sampler, PSIn.UV);
  
  // Tone
  float luma = dot(frag.rgb, lumaF);
  frag.rgb = lerp(frag.rgb, float3(luma, luma, luma), PSIn.Tone.w);
  frag.rgb += PSIn.Tone.rgb;
    
  // Color
  frag.a *= PSIn.Opacity;
  frag.rgb = lerp(frag.rgb, PSIn.Color.rgb, PSIn.Color.a);
    
  // Bush
  float currentPos = PSIn.UV.y;
  float underBush = (currentPos > PSIn.BushDepth) ? 0.0 : 1.0;
  frag.a *= clamp(PSIn.BushOpacity + underBush, 0.0, 1.0);

  PSOut.Color = frag;
}
)";

///
// type:
//   alpha transition shader
///
// entry:
//   vertex: VSMain
//   pixel: PSMain
///
// vertex:
//   <float4, float2, float4>
///
// bind:
//   { Texture2D, Texture2D }
///

const std::string kHLSL_AlphaTransitionRender_Vertex = R"(
struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

void VSMain(in VSInput VSIn, out PSInput PSIn) {
  PSIn.Pos = VSIn.Pos;
  
  PSIn.UV = VSIn.UV;
  PSIn.Color = VSIn.Color;
}
)";

const std::string kHLSL_AlphaTransitionRender_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

Texture2D u_FrozenTexture;
SamplerState u_FrozenTexture_sampler;

Texture2D u_CurrentTexture;
SamplerState u_CurrentTexture_sampler;

struct PSOutput {
  float4 Color : SV_TARGET;
};

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float4 frozenFrag = u_FrozenTexture.Sample(u_FrozenTexture_sampler, PSIn.UV);
  float4 currentFrag = u_CurrentTexture.Sample(u_CurrentTexture_sampler, PSIn.UV);
  PSOut.Color = lerp(frozenFrag, currentFrag, PSIn.Color.a);
}
)";

///
// type:
//   mapping transition shader
///
// entry:
//   vertex: VSMain
//   pixel: PSMain
///
// vertex:
//   <float4, float2, float4>
///
// bind:
//   { Texture2D, Texture2D, Texture2D }
///

const std::string kHLSL_MappingTransitionRender_Vertex = R"(
struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

void VSMain(in VSInput VSIn, out PSInput PSIn) {
  PSIn.Pos = VSIn.Pos;
  
  PSIn.UV = VSIn.UV;
  PSIn.Color = VSIn.Color;
}
)";

const std::string kHLSL_MappingTransitionRender_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

Texture2D u_FrozenTexture;
SamplerState u_FrozenTexture_sampler;

Texture2D u_CurrentTexture;
SamplerState u_CurrentTexture_sampler;

Texture2D u_TransTexture;
SamplerState u_TransTexture_sampler;

struct PSOutput {
  float4 Color : SV_TARGET;
};

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float4 frozenFrag = u_FrozenTexture.Sample(u_FrozenTexture_sampler, PSIn.UV);
  float4 currentFrag = u_CurrentTexture.Sample(u_CurrentTexture_sampler, PSIn.UV);
  float transSample = u_TransTexture.Sample(u_TransTexture_sampler, PSIn.UV).r;

  float vague = PSIn.Color.r;
  float progress = PSIn.Color.a;

  transSample = clamp(transSample, progress, progress + vague);
  float mixAlpha = (transSample - progress) / vague;

  PSOut.Color = lerp(currentFrag, frozenFrag, mixAlpha);
}
)";

///
// type:
//   tilemap shader
///
// entry:
//   vertex: VSMain
//   pixel: PSMain
///
// vertex:
//   <float4, float2, float4>
///
// bind:
//   { float4x4, float4x4 }
//   { Texture2D }
//   { float2, float2, float, float }
///

const std::string kHLSL_TilemapRender_Vertex = R"(
struct WorldMatrix {
  float4x4 ProjMat;
  float4x4 TransMat;
};

cbuffer WorldMatrixBuffer {
  WorldMatrix u_Transform;
};

struct TilemapParams {
  float4 OffsetAndTexSize;
  float4 AnimateIndexAndTileSizeAndFlashAlpha;
};

cbuffer TilemapUniformBuffer {
  TilemapParams u_Params;
};

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

void VSMain(in VSInput VSIn, out PSInput PSIn) {
  float4 transPos = VSIn.Pos;
  float2 transUV = VSIn.UV;
  float frameCount = VSIn.Color.a;

  // Apply offset
  transPos.x += u_Params.OffsetAndTexSize.x;
  transPos.y += u_Params.OffsetAndTexSize.y;

  // Animated area
  float tile_size = u_Params.AnimateIndexAndTileSizeAndFlashAlpha.y;
  transUV.x += 3.0 * tile_size * fmod(u_Params.AnimateIndexAndTileSizeAndFlashAlpha.x, frameCount);

  // Setup pixel shader params
  PSIn.Pos = mul(u_Transform.TransMat, transPos);
  PSIn.Pos = mul(u_Transform.ProjMat, PSIn.Pos);
  
  PSIn.UV = float2(transUV.x * u_Params.OffsetAndTexSize.z,
                   transUV.y * u_Params.OffsetAndTexSize.w);
  PSIn.Color = VSIn.Color;
  PSIn.Color.a = ((PSIn.Color.r == 0.0) && (PSIn.Color.g == 0.0) && (PSIn.Color.b == 0.0))
                 ? 0.0
                 : u_Params.AnimateIndexAndTileSizeAndFlashAlpha.z;
}
)";

const std::string kHLSL_TilemapRender_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

struct PSOutput {
  float4 Color : SV_TARGET;
};

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float4 frag = u_Texture.Sample(u_Texture_sampler, PSIn.UV);
  PSOut.Color.rgb = lerp(frag.rgb, PSIn.Color.rgb, PSIn.Color.a);
  PSOut.Color.a = frag.a;
}
)";

///
// type:
//   tilemap2 shader
///
// entry:
//   vertex: VSMain
//   pixel: PSMain
///
// vertex:
//   <float4, float2, float4>
///
// bind:
//   { float4x4, float4x4 }
//   { Texture2D }
//   { float2, float2, float2, float }
///

const std::string kHLSL_Tilemap2Render_Vertex = R"(
struct WorldMatrix {
  float4x4 ProjMat;
  float4x4 TransMat;
};

cbuffer WorldMatrixBuffer {
  WorldMatrix u_Transform;
};

struct Tilemap2Params {
  float4 OffsetAndTexSize;
  float4 AnimationOffsetAndTileSizeAndFlashAlpha;
};

cbuffer Tilemap2UniformBuffer {
  Tilemap2Params u_Params;
};

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

float PosInArea(float2 pos, float4 area) {
  return (pos.x >= area.x && pos.y >= area.y && pos.x <= (area.x + area.z) &&
          pos.y <= (area.y + area.w))
             ? 1.0
             : 0.0;
}

static const float2 kRegularArea = float2(12.0, 12.0);
static const float4 kWaterfallArea = float4(12.0, 0.0, 4.0, 12.0);
static const float4 kWaterfallAutotileArea = float4(12.0, 0.0, 2.0, 6.0);

void VSMain(in VSInput VSIn, out PSInput PSIn) {
  float4 transPos = VSIn.Pos;
  float2 transUV = VSIn.UV;

  // Apply offset
  transPos.x += u_Params.OffsetAndTexSize.x;
  transPos.y += u_Params.OffsetAndTexSize.y;

  // Regular area
  float tile_size = u_Params.AnimationOffsetAndTileSizeAndFlashAlpha.z;
	float addition1 = (transUV.x <= kRegularArea.x * tile_size &&
                     transUV.y <= kRegularArea.y * tile_size)
                        ? 1.0
                        : 0.0;
	transUV.x += u_Params.AnimationOffsetAndTileSizeAndFlashAlpha.x * addition1;

	// Waterfall area
	float addition2 = PosInArea(transUV, kWaterfallArea * tile_size) -
                    PosInArea(transUV, kWaterfallAutotileArea * tile_size);
	transUV.y += u_Params.AnimationOffsetAndTileSizeAndFlashAlpha.y * addition2;

  // Setup pixel shader params
  PSIn.Pos = mul(u_Transform.TransMat, transPos);
  PSIn.Pos = mul(u_Transform.ProjMat, PSIn.Pos);
  
  PSIn.UV = float2(transUV.x * u_Params.OffsetAndTexSize.z,
                   transUV.y * u_Params.OffsetAndTexSize.w);
  PSIn.Color = VSIn.Color;
  PSIn.Color.a = ((PSIn.Color.r == 0.0) && (PSIn.Color.g == 0.0) && (PSIn.Color.b == 0.0))
                 ? 0.0
                 : u_Params.AnimationOffsetAndTileSizeAndFlashAlpha.w;
}
)";

const std::string kHLSL_Tilemap2Render_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

struct PSOutput {
  float4 Color : SV_TARGET;
};

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float4 frag = u_Texture.Sample(u_Texture_sampler, PSIn.UV);
  PSOut.Color.rgb = lerp(frag.rgb, PSIn.Color.rgb, PSIn.Color.a);
  PSOut.Color.a = frag.a;
}
)";

///
// type:
//   bitmap hue shader
///
// entry:
//   vertex: VSMain
//   pixel: PSMain
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { Texture2D }
///

const std::string kHLSL_BitmapHueRender_Vertex = R"(
struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

void VSMain(in VSInput VSIn, out PSInput PSIn) {
  PSIn.Pos = VSIn.Pos;
  
  PSIn.UV = VSIn.UV;
  PSIn.Color = VSIn.Color;
}
)";

const std::string kHLSL_BitmapHueRender_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

struct PSOutput {
  float4 Color : SV_TARGET;
};

float3 rgb2hsv(float3 c) {
  float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
  float4 p = lerp(float4(c.b, c.g, K.w, K.z), float4(c.g, c.b, K.x, K.y), step(c.b, c.g));
  float4 q = lerp(float4(p.x, p.y, p.w, c.r), float4(c.r, p.y, p.z, p.x), step(p.x, c.r));

  float d = q.x - min(q.w, q.y);
  float e = 1e-10;

  float h = abs(q.z + (q.w - q.y) / (6.0 * d + e));
  float s = d / (q.x + e);
  float v = q.x;

  return float3(h, s, v);
}

float3 hsv2rgb(float3 c) {
  float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  float3 t = float3(c.x, c.x, c.x) + K.xyz;
  float3 p = abs(frac(t) * 6.0 - float3(K.w, K.w, K.w));

  return c.z * lerp(float3(K.x, K.x, K.x), clamp(p - float3(K.x, K.x, K.x), 0.0, 1.0), c.y);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float4 frag = u_Texture.Sample(u_Texture_sampler, PSIn.UV);
  float3 hsv = rgb2hsv(frag.rgb);
  hsv.x += PSIn.Color.a;
  PSOut.Color = float4(hsv2rgb(hsv), frag.a);
}
)";

///
// type:
//   yuv shader
///
// entry:
//   vertex: VSMain
//   pixel: PSMain
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { Texture2D x 3 }
///

const std::string kHLSL_YUVRender_Vertex = R"(
struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

void VSMain(in VSInput VSIn, out PSInput PSIn) {
  PSIn.Pos = VSIn.Pos;
  
  PSIn.UV = VSIn.UV;
}
)";

const std::string kHLSL_YUVRender_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_TextureY;
SamplerState u_TextureY_sampler;

Texture2D u_TextureU;
SamplerState u_TextureU_sampler;

Texture2D u_TextureV;
SamplerState u_TextureV_sampler;

struct PSOutput {
  float4 Color : SV_TARGET;
};

static const float3 kYUVOffset = float3(0.0, -0.501960814, -0.501960814);
static const float3x3 kYUVMatrix = float3x3(1,       1,        1,
                                            0,      -0.3441,   1.772,
                                            1.402,  -0.7141,   0);

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float3 yuv;
  yuv.x = u_TextureY.Sample(u_TextureY_sampler, PSIn.UV).r;
  yuv.y = u_TextureU.Sample(u_TextureU_sampler, PSIn.UV).r;
  yuv.z = u_TextureV.Sample(u_TextureV_sampler, PSIn.UV).r;
  yuv += kYUVOffset;
  float3 rgb =
#if defined(GLSL)
    mul(kYUVMatrix, yuv);
#else
    mul(yuv, kYUVMatrix);
#endif
  PSOut.Color = float4(rgb.x, rgb.y, rgb.z, 1.0);
}
)";

///
// type:
//   upscale pass shader
///
// entry:
//   vertex: VSMain (SV_VertexID, no vertex buffer needed)
//   pixel: PSMain
///
// resource:
//   { Texture2D }
//   { ScalingParamsBuffer }
///
// algorithm modes (u_Mode in Upscale PS):
//   0 = Bilinear
//   1 = Nearest
//   2 = Lanczos3
//   3 = Bicubic
// (Sobel blend mode select in Anime4K Enhance PS: 0=DoG, 1=Sobel adaptive)

const std::string kHLSL_UpscalePass_Vertex = R"(
struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

void VSMain(in VSInput VSIn, out PSInput PSIn) {
  PSIn.Pos = VSIn.Pos;
  PSIn.UV = VSIn.UV;
}
)";

const std::string kHLSL_UpscalePass_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
  uint   u_Mode;
  float  u_ARStrength;
  float  u_BicubicB;
  float  u_BicubicC;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

#define MODE_BILINEAR  0
#define MODE_NEAREST   1
#define MODE_LANCZOS3  2
#define MODE_BICUBIC   3

// ---- Lanczos3 helpers ----
#define FIX(c) max(abs(c), 1e-5)
#define min4(a, b, c, d) min(min(a, b), min(c, d))
#define max4(a, b, c, d) max(max(a, b), max(c, d))

float3 LanczosWeight3(float x) {
  float rcpRadius = 1.0 / 3.0;
  float3 s = FIX(6.28318530718 * float3(x - 1.5, x - 0.5, x + 0.5));
  return sin(s) * sin(s * rcpRadius) / (s * s);
}

float3 SampleLanczos3(float2 uv) {
  float2 pos = uv * u_InputSize;
  float2 input_pt = u_InputPt;

  float2 f = frac(pos.xy + 0.5);
  float3 linetaps1 = LanczosWeight3(0.5 - f.x * 0.5);
  float3 linetaps2 = LanczosWeight3(1.0 - f.x * 0.5);
  float3 columntaps1 = LanczosWeight3(0.5 - f.y * 0.5);
  float3 columntaps2 = LanczosWeight3(1.0 - f.y * 0.5);

  float suml = dot(linetaps1, float3(1, 1, 1)) + dot(linetaps2, float3(1, 1, 1));
  float sumc = dot(columntaps1, float3(1, 1, 1)) + dot(columntaps2, float3(1, 1, 1));
  linetaps1 /= suml;
  linetaps2 /= suml;
  columntaps1 /= sumc;
  columntaps2 /= sumc;

  pos -= f + 1.5;

  float3 src[6][6];
  uint i, j;

  [unroll] for (i = 0; i <= 4; i += 2) {
    [unroll] for (j = 0; j <= 4; j += 2) {
      float2 tpos = (pos + uint2(i, j)) * input_pt;
      const float4 sr = u_Texture.GatherRed(u_Texture_sampler, tpos);
      const float4 sg = u_Texture.GatherGreen(u_Texture_sampler, tpos);
      const float4 sb = u_Texture.GatherBlue(u_Texture_sampler, tpos);

#ifdef URGE_GLSL_GATHER
      src[i][j].x = sr.x;
      src[i][j].y = sg.x;
      src[i][j].z = sb.x;
      src[i][j + 1].x = sr.w;
      src[i][j + 1].y = sg.w;
      src[i][j + 1].z = sb.w;
      src[i + 1][j].x = sr.y;
      src[i + 1][j].y = sg.y;
      src[i + 1][j].z = sb.y;
      src[i + 1][j + 1].x = sr.z;
      src[i + 1][j + 1].y = sg.z;
      src[i + 1][j + 1].z = sb.z;
#else
      src[i][j].x = sr.w;
      src[i][j].y = sg.w;
      src[i][j].z = sb.w;
      src[i][j + 1].x = sr.x;
      src[i][j + 1].y = sg.x;
      src[i][j + 1].z = sb.x;
      src[i + 1][j].x = sr.z;
      src[i + 1][j].y = sg.z;
      src[i + 1][j].z = sb.z;
      src[i + 1][j + 1].x = sr.y;
      src[i + 1][j + 1].y = sg.y;
      src[i + 1][j + 1].z = sb.y;
#endif
    }
  }

  float3 color = float3(0, 0, 0);
  [unroll] for (i = 0; i <= 4; i += 2) {
    color += (mul(linetaps1, float3x3(src[0][i], src[2][i], src[4][i])) +
              mul(linetaps2, float3x3(src[1][i], src[3][i], src[5][i]))) *
             columntaps1[i / 2] +
             (mul(linetaps1, float3x3(src[0][i + 1], src[2][i + 1], src[4][i + 1])) +
              mul(linetaps2, float3x3(src[1][i + 1], src[3][i + 1], src[5][i + 1]))) *
             columntaps2[i / 2];
  }

  float3 min_sample = min4(src[2][2], src[3][2], src[2][3], src[3][3]);
  float3 max_sample = max4(src[2][2], src[3][2], src[2][3], src[3][3]);
  color = lerp(color, clamp(color, min_sample, max_sample), u_ARStrength);

  return color;
}

// ---- Bicubic (Mitchell-Netravali) helpers ----
float MitchellWeight(float x) {
  float B = u_BicubicB;
  float C = u_BicubicC;
  float ax = abs(x);
  if (ax < 1.0) {
    float ax2 = ax * ax;
    float ax3 = ax2 * ax;
    return ((12.0 - 9.0 * B - 6.0 * C) * ax3 +
            (-18.0 + 12.0 * B + 6.0 * C) * ax2 +
            (6.0 - 2.0 * B)) / 6.0;
  } else if (ax < 2.0) {
    float ax2 = ax * ax;
    float ax3 = ax2 * ax;
    return ((-B - 6.0 * C) * ax3 +
            (6.0 * B + 30.0 * C) * ax2 +
            (-12.0 * B - 48.0 * C) * ax +
            (8.0 * B + 24.0 * C)) / 6.0;
  }
  return 0.0;
}

float3 SampleBicubic(float2 uv) {
  float2 pos = uv * u_InputSize;
  float2 input_pt = u_InputPt;

  float2 pos1 = floor(pos - 0.5) + 0.5;
  float2 f = pos - pos1;

  float wx[4], wy[4];
  float swx = 0, swy = 0;
  [unroll] for (int k = 0; k < 4; k++) {
    wx[k] = MitchellWeight(k - 1.0 - f.x);
    wy[k] = MitchellWeight(k - 1.0 - f.y);
    swx += wx[k];
    swy += wy[k];
  }

  float2 uv_base = pos1 * input_pt - input_pt;

  float3 color = float3(0.0, 0.0, 0.0);
  [unroll] for (int i = 0; i < 4; i++) {
    float3 row = float3(0.0, 0.0, 0.0);
    [unroll] for (int j = 0; j < 4; j++) {
      float2 sp = uv_base + float2(j, i) * input_pt;
      float4 _s = u_Texture.Sample(u_Texture_sampler, sp);
      row += _s.rgb * wx[j];
    }
    color += row * wy[i];
  }

  return color;
}

float3 SampleLanczos3_GL(float2 uv) {
  float2 pos = uv * u_InputSize;
  float2 input_pt = u_InputPt;
  float2 f = frac(pos.xy + 0.5);
  float3 wx_even = LanczosWeight3(0.5 - f.x * 0.5);
  float3 wx_odd  = LanczosWeight3(1.0 - f.x * 0.5);
  float3 wy_even = LanczosWeight3(0.5 - f.y * 0.5);
  float3 wy_odd  = LanczosWeight3(1.0 - f.y * 0.5);
  float suml = dot(wx_even, float3(1, 1, 1)) + dot(wx_odd, float3(1, 1, 1));
  float sumc = dot(wy_even, float3(1, 1, 1)) + dot(wy_odd, float3(1, 1, 1));
  wx_even /= suml; wx_odd /= suml;
  wy_even /= sumc; wy_odd /= sumc;
  pos -= f + 1.5;

  float3 src[6][6];
  [unroll] for (uint i = 0; i <= 4; i += 2) {
    [unroll] for (uint j = 0; j <= 4; j += 2) {
      float2 tpos = (pos + uint2(i, j)) * input_pt;
      const float4 sr = u_Texture.GatherRed(u_Texture_sampler, tpos);
      const float4 sg = u_Texture.GatherGreen(u_Texture_sampler, tpos);
      const float4 sb = u_Texture.GatherBlue(u_Texture_sampler, tpos);
      src[i][j].x = sr.w; src[i][j].y = sg.w; src[i][j].z = sb.w;
      src[i][j+1].x = sr.x; src[i][j+1].y = sg.x; src[i][j+1].z = sb.x;
      src[i+1][j].x = sr.z; src[i+1][j].y = sg.z; src[i+1][j].z = sb.z;
      src[i+1][j+1].x = sr.y; src[i+1][j+1].y = sg.y; src[i+1][j+1].z = sb.y;
    }
  }

  float3 color = float3(0, 0, 0);
  [unroll] for (uint i = 0; i <= 4; i += 2) {
    float3 h0 = wx_even.x * src[0][i] + wx_even.y * src[2][i] + wx_even.z * src[4][i]
              + wx_odd.x  * src[1][i] + wx_odd.y  * src[3][i] + wx_odd.z  * src[5][i];
    float3 h1 = wx_even.x * src[0][i+1] + wx_even.y * src[2][i+1] + wx_even.z * src[4][i+1]
              + wx_odd.x  * src[1][i+1] + wx_odd.y  * src[3][i+1] + wx_odd.z  * src[5][i+1];
    color += h0 * wy_even[i/2] + h1 * wy_odd[i/2];
  }
  float3 mn = min(min(src[2][2], src[3][2]), min(src[2][3], src[3][3]));
  float3 mx = max(max(src[2][2], src[3][2]), max(src[2][3], src[3][3]));
  color = lerp(color, clamp(color, mn, mx), u_ARStrength);
  return saturate(color);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float2 uv = PSIn.UV;

  if (u_Mode == MODE_NEAREST) {
    float2 texel = floor(uv * u_InputSize) + 0.5;
    uv = texel * u_InputPt;
    PSOut.Color = u_Texture.Sample(u_Texture_sampler, uv);
  } else if (u_Mode == MODE_LANCZOS3) {
#ifdef URGE_GLSL_GATHER
    PSOut.Color = float4(SampleLanczos3_GL(uv), 1.0);
#else
    PSOut.Color = float4(SampleLanczos3(uv), 1.0);
#endif
  } else if (u_Mode == MODE_BICUBIC) {
    PSOut.Color = float4(SampleBicubic(uv), 1.0);
  } else {
    PSOut.Color = u_Texture.Sample(u_Texture_sampler, uv);
  }
}
)";

// ---- Anime4K DoG (single-pass, inline 7x7 gauss + min/max clamp) ----
const std::string kHLSL_Anime4K_Enhance_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
  uint   u_Mode;
  float  u_ARStrength;
  float  u_BicubicB;
  float  u_BicubicC;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static const float kWeight[3] = { 0.38774, 0.24477, 0.06136 };

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float2 uv = PSIn.UV;
  float2 pt = u_InputPt;

  float4 screen = u_Texture.Sample(u_Texture_sampler, uv);
  float center_luma = dot(screen, float4(0.299, 0.587, 0.114, 0.0));

  float gauss = 0.0;
  float mn = 1.0, mx = 0.0;
  float luma33[3][3];
  [unroll] for (int i = -2; i <= 2; i++) {
    [unroll] for (int j = -2; j <= 2; j++) {
      float w = kWeight[abs(i)] * kWeight[abs(j)];
      float2 s = uv + float2(i * pt.x, j * pt.y);
      float l = dot(u_Texture.Sample(u_Texture_sampler, s),
                    float4(0.299, 0.587, 0.114, 0.0));
      gauss += l * w;
      mn = min(mn, l);
      mx = max(mx, l);
      if (i >= -1 && i <= 1 && j >= -1 && j <= 1)
        luma33[i + 1][j + 1] = l;
    }
  }

  float c = (center_luma - gauss) * 0.8;
  float cc = clamp(c + center_luma, mn, mx) - center_luma;
  float4 enhanced = saturate(screen + cc);

  // Line darken (applies to both mode 0 and 1)
  float dark = min(center_luma - gauss, 0.0) * u_BicubicC;
  enhanced = saturate(enhanced + dark);

  // Sobel adaptive blend (mode 1) + thin warp
  float4 result = enhanced;
  if (u_Mode == 1) {
    float gx = (-luma33[0][0] + luma33[0][2]
                - 2.0 * luma33[1][0] + 2.0 * luma33[1][2]
                - luma33[2][0] + luma33[2][2]) / 4.0;
    float gy = (-luma33[0][0] - 2.0 * luma33[0][1] - luma33[0][2]
                + luma33[2][0] + 2.0 * luma33[2][1] + luma33[2][2]) / 4.0;
    float norm = saturate(sqrt(gx * gx + gy * gy) * 2.0);
    float dval = pow(norm, u_ARStrength);
    result = lerp(screen, enhanced, dval);

    // Thin warp (u_BicubicB = warp strength)
    float warp = u_BicubicB;
    if (warp > 0.0) {
      float len = max(abs(gx) + abs(gy), 1e-6);
      float2 dir = float2(gx, gy) / len;
      float2 wuv = uv - dir * dval * warp * pt;
      float4 warped = u_Texture.Sample(u_Texture_sampler, wuv);
      result = lerp(result, warped, dval * saturate(warp * 2.0));
    }
  }

  PSOut.Color = result;
}
)";

// ---- CAS (Contrast Adaptive Sharpening) ----
// Ported from AMD FidelityFX-CAS (ffx_cas.h)
// 9-tap 3x3 min/max with 5-tap cross filter, green-channel uniform weight
const std::string kHLSL_CAS_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
  uint   u_Mode;
  float  u_ARStrength;
  float  u_BicubicB;
  float  u_BicubicC;
  float  u_CASSharpness;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float2 uv = PSIn.UV;
  float2 pt = u_OutputPt;

  // 3x3 full neighborhood for accurate min/max
  // a b c
  // d e f
  // g h i
  float3 a = u_Texture.Sample(u_Texture_sampler, uv + float2(-pt.x, -pt.y)).rgb;
  float3 b = u_Texture.Sample(u_Texture_sampler, uv + float2(0, -pt.y)).rgb;
  float3 c = u_Texture.Sample(u_Texture_sampler, uv + float2(pt.x, -pt.y)).rgb;
  float3 d = u_Texture.Sample(u_Texture_sampler, uv + float2(-pt.x, 0)).rgb;
  float3 e = u_Texture.Sample(u_Texture_sampler, uv).rgb;
  float3 f = u_Texture.Sample(u_Texture_sampler, uv + float2(pt.x, 0)).rgb;
  float3 g = u_Texture.Sample(u_Texture_sampler, uv + float2(-pt.x, pt.y)).rgb;
  float3 h = u_Texture.Sample(u_Texture_sampler, uv + float2(0, pt.y)).rgb;
  float3 i = u_Texture.Sample(u_Texture_sampler, uv + float2(pt.x, pt.y)).rgb;

  // Soft min and max over full 3x3
  float mnR = min(min(min(a.r, b.r), min(c.r, d.r)),
                  min(min(e.r, f.r), min(g.r, min(h.r, i.r))));
  float mnG = min(min(min(a.g, b.g), min(c.g, d.g)),
                  min(min(e.g, f.g), min(g.g, min(h.g, i.g))));
  float mnB = min(min(min(a.b, b.b), min(c.b, d.b)),
                  min(min(e.b, f.b), min(g.b, min(h.b, i.b))));
  float mxR = max(max(max(a.r, b.r), max(c.r, d.r)),
                  max(max(e.r, f.r), max(g.r, max(h.r, i.r))));
  float mxG = max(max(max(a.g, b.g), max(c.g, d.g)),
                  max(max(e.g, f.g), max(g.g, max(h.g, i.g))));
  float mxB = max(max(max(a.b, b.b), max(c.b, d.b)),
                  max(max(e.b, f.b), max(g.b, max(h.b, i.b))));

  // Smooth minimum distance to signal limit divided by smooth max
  float ampR = sqrt(saturate(min(mnR, 1.0 - mxR) / mxR));
  float ampG = sqrt(saturate(min(mnG, 1.0 - mxG) / mxG));
  float ampB = sqrt(saturate(min(mnB, 1.0 - mxB) / mxB));

  // Filter shape (green channel weight used uniformly)
  float peak = -1.0 / lerp(8.0, 5.0, u_CASSharpness);
  float wG = ampG * peak;
  float rcpWeight = 1.0 / (1.0 + 4.0 * wG);

  // 5-tap cross filter (matching AMD reference)
  PSOut.Color.r = saturate((b.r * wG + d.r * wG + f.r * wG + h.r * wG + e.r) * rcpWeight);
  PSOut.Color.g = saturate((b.g * wG + d.g * wG + f.g * wG + h.g * wG + e.g) * rcpWeight);
  PSOut.Color.b = saturate((b.b * wG + d.b * wG + f.b * wG + h.b * wG + e.b) * rcpWeight);
  PSOut.Color.a = 1.0;
}
)";
extern const std::string kHLSL_Anime4K_Clamp_Highlights_Pass0_Pixel = R"(// Mode A Pass 0

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _45;
static float _70;

float _11(float4 _10)
{
    return dot(float4(0.2989999949932098388671875f, 0.58700001239776611328125f, 0.114000000059604644775390625f, 0.0f), _10);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _45 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float _23 = 0.0f;
      for (int _26 = 0; _26 < 5; _26++)
      {
          float4 _61 = u_Texture.Sample(u_Texture_sampler, uv + (float2(float(_26 - 2), 0.0f) * u_InputPt));
          float _37 = _11(_61);
          _23 = max(_37, _23);
      }
      PSOut.Color = _23;
}
})";

extern const std::string kHLSL_Anime4K_Clamp_Highlights_Pass1_Pixel = R"(// Mode A Pass 1

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _31;
static float _58;

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _31 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float _8 = 0.0f;
      for (int _12 = 0; _12 < 5; _12++)
      {
          float _23 = u_Texture.Sample(u_Texture_sampler, uv + (float2(0.0f, float(_12 - 2)) * u_InputPt)).x;
          _8 = max(_23, _8);
      }
      PSOut.Color = _8;
}
})";

extern const std::string kHLSL_Anime4K_Clamp_Highlights_Pass2_Pixel = R"(// Mode A Pass 2

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;
Texture2D u_Texture1;
SamplerState u_Texture1_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

float get_luma(float4 rgba)
{
    return dot(float4(0.2989999949932098388671875f, 0.58700001239776611328125f, 0.114000000059604644775390625f, 0.0f), rgba);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float2 uv = PSIn.UV;
  {
      float4 hooked_color = u_Texture1.Sample(u_Texture1_sampler, uv);
      float current_luma = get_luma(hooked_color);
      float new_luma = min(current_luma, u_Texture.Sample(u_Texture_sampler, uv).x);
      PSOut.Color = hooked_color - (current_luma - new_luma).xxxx;
}
})";

extern const std::string kHLSL_Anime4K_Restore_CNN_M_Pass0_Pixel = R"(// Mode A Pass 3

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _21;
static float4 _250;

float4 _12(float _10, float _11)
{
    return u_Texture.Sample(u_Texture_sampler, _21 + (float2(_10, _11) * u_InputPt));
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _21 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float _61 = -1.0f;
      float _62 = -1.0f;
      float4 _40 = mul(_12(_61, _62), float4x4(-0.099919863045215606689453125, 0.13782341778278350830078125, -0.03125168383121490478515625, -0.06356842815876007080078125, -0.3437488079071044921875, 0.05450952053070068359375, 0.343478024005889892578125, 0.4633537232875823974609375, 0.0860722362995147705078125, 0.0449883937835693359375, 0.13717900216579437255859375, 0.17976908385753631591796875, 0.0, 0.0, 0.0, 0.0));
      float _81 = -1.0f;
      float _82 = 0.0f;
      _40 += mul(_12(_81, _82), float4x4(-0.0242124237120151519775390625, -0.0927850902080535888671875, -0.0004090775619260966777801513671875, 0.345522940158843994140625, -0.13254678249359130859375, 0.113105185329914093017578125, 0.0056679458357393741607666015625, -0.00036919137346558272838592529296875, -0.063756786286830902099609375, 0.009184114634990692138671875, 0.11551873385906219482421875, -0.115506775677204132080078125, 0.0, 0.0, 0.0, 0.0));
      float _104 = -1.0f;
      float _105 = 1.0f;
      _40 += mul(_12(_104, _105), float4x4(-0.14101827144622802734375, 0.02352349273860454559326171875, 0.0440945662558078765869140625, -0.01927174627780914306640625, -0.443488419055938720703125, -0.08818876743316650390625, -0.402614891529083251953125, -0.21995794773101806640625, -0.1588039398193359375, -0.013732858002185821533203125, -0.02075113542377948760986328125, 0.0127191506326198577880859375, 0.0, 0.0, 0.0, 0.0));
      float _126 = 0.0f;
      float _127 = -1.0f;
      _40 += mul(_12(_126, _127), float4x4(0.013001821003854274749755859375, -0.3450350463390350341796875, 0.3921913802623748779296875, 0.1879212558269500732421875, 0.24760444462299346923828125, -0.01617340184748172760009765625, 0.10154511034488677978515625, 0.154530823230743408203125, -0.0581328757107257843017578125, 0.01678439788520336151123046875, -0.058085389435291290283203125, -0.110399149358272552490234375, 0.0, 0.0, 0.0, 0.0));
      float _148 = 0.0f;
      float _149 = 0.0f;
      _40 += mul(_12(_148, _149), float4x4(0.37024533748626708984375, 0.0414408631622791290283203125, -0.3374567925930023193359375, -0.4499428570270538330078125, 0.1955559551715850830078125, 0.20855538547039031982421875, -0.279740750789642333984375, -0.53726279735565185546875, 0.2122814655303955078125, -0.029534600675106048583984375, -0.567000567913055419921875, 0.03004282154142856597900390625, 0.0, 0.0, 0.0, 0.0));
      float _170 = 0.0f;
      float _171 = 1.0f;
      _40 += mul(_12(_170, _171), float4x4(-0.12940631806850433349609375, 0.057525999844074249267578125, 0.09068204462528228759765625, -0.069850333034992218017578125, -0.13704006373882293701171875, -0.047685407102108001708984375, 0.4461567401885986328125, -0.4805660545825958251953125, -0.06166251003742218017578125, -0.0188351906836032867431640625, 0.203223705291748046875, -0.113287605345249176025390625, 0.0, 0.0, 0.0, 0.0));
      float _192 = 1.0f;
      float _193 = -1.0f;
      _40 += mul(_12(_192, _193), float4x4(0.0108566693961620330810546875, -0.3582073748111724853515625, 0.16757218539714813232421875, 0.0826198756694793701171875, -0.039673030376434326171875, 0.03870557248592376708984375, 0.3265285491943359375, -0.0120300166308879852294921875, 0.015120559372007846832275390625, -0.15314877033233642578125, 0.2344200909137725830078125, 0.097679220139980316162109375, 0.0, 0.0, 0.0, 0.0));
      float _214 = 1.0f;
      float _215 = 0.0f;
      _40 += mul(_12(_214, _215), float4x4(-0.046272672712802886962890625, -0.1775230467319488525390625, 0.0820182859897613525390625, -0.251282393932342529296875, 0.58619463443756103515625, -0.0609034635126590728759765625, -0.02279359661042690277099609375, 0.077803514897823333740234375, -0.17025311291217803955078125, 0.0513699315488338470458984375, 0.0293832980096340179443359375, -0.15475408732891082763671875, 0.0, 0.0, 0.0, 0.0));
      float _236 = 1.0f;
      float _237 = 1.0f;
      _40 += mul(_12(_236, _237), float4x4(-0.1121202409267425537109375, 0.13378004729747772216796875, -0.2027488052845001220703125, 0.080564208328723907470703125, -0.111762188374996185302734375, -0.048429377377033233642578125, -0.083963863551616668701171875, 0.105078287422657012939453125, 0.1332683861255645751953125, 0.04306270182132720947265625, 0.0513623766601085662841796875, 0.06482754647731781005859375, 0.0, 0.0, 0.0, 0.0));
      _40 += float4(-0.0612334720790386199951171875f, 0.3922264575958251953125f, 0.02970497868955135345458984375f, 0.02586827985942363739013671875f);
      PSOut.Color = _40;
}
})";

extern const std::string kHLSL_Anime4K_Restore_CNN_M_Pass1_Pixel = R"(// Mode A Pass 4

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _25;
static float4 _558;

float4 _12(float _10, float _11)
{
    return max(u_Texture.Sample(u_Texture_sampler, _25 + (float2(_10, _11) * u_InputPt)), 0.0f.xxxx);
}
float4 _16(float _14, float _15)
{
    return max(-u_Texture.Sample(u_Texture_sampler, _25 + (float2(_14, _15) * u_InputPt)), 0.0f.xxxx);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _25 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float _86 = -1.0f;
      float _87 = -1.0f;
      float4 _62 = mul(_12(_86, _87), float4x4(-0.16410656273365020751953125, -0.40521824359893798828125, 0.13121907413005828857421875, -0.0231459699571132659912109375, 0.105412475764751434326171875, -0.0604012720286846160888671875, -0.04306347668170928955078125, -0.13933973014354705810546875, 0.12558138370513916015625, -0.02086146734654903411865234375, 0.0303705148398876190185546875, 0.13178016245365142822265625, -0.142203509807586669921875, 0.20736892521381378173828125, 0.00332156405784189701080322265625, -0.2924171388149261474609375));
      float _111 = -1.0f;
      float _112 = 0.0f;
      _62 += mul(_12(_111, _112), float4x4(0.185173213481903076171875, 0.291629850864410400390625, -0.2678339481353759765625, 0.039760686457157135009765625, 0.0255270116031169891357421875, -0.0673192441463470458984375, 0.0550041757524013519287109375, 0.04891656339168548583984375, 0.12750522792339324951171875, -0.09143595397472381591796875, 0.138188421726226806640625, 0.367042243480682373046875, 0.083992101252079010009765625, 0.101866178214550018310546875, -0.17237375676631927490234375, 0.1328241825103759765625));
      float _139 = -1.0f;
      float _140 = 1.0f;
      _62 += mul(_12(_139, _140), float4x4(-0.16578869521617889404296875, 0.013132513500750064849853515625, -0.17222486436367034912109375, 0.091398894786834716796875, -0.12756164371967315673828125, -0.08437298238277435302734375, -0.2905299663543701171875, 0.3269337117671966552734375, 0.15870757400989532470703125, -0.013529402203857898712158203125, -0.0581752993166446685791015625, 0.11802370846271514892578125, 0.070999659597873687744140625, -0.02406363189220428466796875, 0.3183484375476837158203125, -0.111838586628437042236328125));
      float _166 = 0.0f;
      float _167 = -1.0f;
      _62 += mul(_12(_166, _167), float4x4(0.4603688716888427734375, -0.076546229422092437744140625, 0.22923062741756439208984375, 0.174638211727142333984375, 0.105554141104221343994140625, -0.117430426180362701416015625, 0.12406776845455169677734375, -0.011399491690099239349365234375, 0.028316497802734375, 0.1368434131145477294921875, 0.00966408662497997283935546875, 0.20226590335369110107421875, 0.0495397411286830902099609375, -0.3134221732616424560546875, -0.610313117504119873046875, -0.13605757057666778564453125));
      float _193 = 0.0f;
      float _194 = 0.0f;
      _62 += mul(_12(_193, _194), float4x4(0.0340695492923259735107421875, -0.398193657398223876953125, 0.61176002025604248046875, -0.4680945575237274169921875, -0.02932107262313365936279296875, 0.466194927692413330078125, 0.3670018613338470458984375, 0.0228856094181537628173828125, 0.11464084684848785400390625, -0.109314523637294769287109375, -0.09154021739959716796875, 0.073341466486454010009765625, -0.560991585254669189453125, 0.3182623386383056640625, -0.011012659408152103424072265625, -0.467195451259613037109375));
      float _220 = 0.0f;
      float _221 = 1.0f;
      _62 += mul(_12(_220, _221), float4x4(-0.056855045258998870849609375, 0.2703702747821807861328125, -0.092696957290172576904296875, -0.563571989536285400390625, -0.0681611597537994384765625, -0.2298661172389984130859375, 0.086931668221950531005859375, -0.1624610126018524169921875, 0.09954045712947845458984375, -0.053741760551929473876953125, 0.0071916827000677585601806640625, -0.17886920273303985595703125, 0.3825241029262542724609375, -0.16098870337009429931640625, 0.0552047677338123321533203125, 0.1021306812763214111328125));
      float _247 = 1.0f;
      float _248 = -1.0f;
      _62 += mul(_12(_247, _248), float4x4(0.064662598073482513427734375, 0.102358795702457427978515625, -0.4505582153797149658203125, 0.20557902753353118896484375, -0.23337309062480926513671875, 0.12633001804351806640625, -0.19299198687076568603515625, -0.15085731446743011474609375, -0.13473303616046905517578125, 0.05379046499729156494140625, -0.100611932575702667236328125, -0.13393497467041015625, -0.042647518217563629150390625, -0.02974073775112628936767578125, -0.078652851283550262451171875, 0.20883278548717498779296875));
      float _274 = 1.0f;
      float _275 = 0.0f;
      _62 += mul(_12(_274, _275), float4x4(0.010471527464687824249267578125, -0.0332184731960296630859375, -0.4615744650363922119140625, 0.0048665828071534633636474609375, 0.23226471245288848876953125, -0.0593433268368244171142578125, -0.14395959675312042236328125, 0.13619647920131683349609375, 0.013839962892234325408935546875, 0.159303247928619384765625, 0.0437423549592494964599609375, 0.1746732294559478759765625, 0.3377230465412139892578125, 0.40261495113372802734375, -0.0835129320621490478515625, 0.18129359185695648193359375));
      float _301 = 1.0f;
      float _302 = 1.0f;
      _62 += mul(_12(_301, _302), float4x4(-0.124934338033199310302734375, -0.18751339614391326904296875, -0.07494379580020904541015625, -0.00317016057670116424560546875, -0.0371426157653331756591796875, 0.16670019924640655517578125, 0.16665546596050262451171875, -0.01124812662601470947265625, 0.0071619413793087005615234375, 0.0034872111864387989044189453125, 0.12031896412372589111328125, -0.096255786716938018798828125, 0.14917047321796417236328125, -0.16310586035251617431640625, 0.072317369282245635986328125, 0.30447328090667724609375));
      float _328 = -1.0f;
      float _329 = -1.0f;
      _62 += mul(_16(_328, _329), float4x4(0.093798615038394927978515625, 0.17074613273143768310546875, -0.087806783616542816162109375, -0.01252020709216594696044921875, 0.118534855544567108154296875, 0.02750877849757671356201171875, -0.2778477966785430908203125, -0.19509242475032806396484375, -0.3413709700107574462890625, 0.3200031220912933349609375, -0.220271587371826171875, 0.33751499652862548828125, 0.162208616733551025390625, 0.108993016183376312255859375, 0.1407052576541900634765625, 0.127842843532562255859375));
      float _355 = -1.0f;
      float _356 = 0.0f;
      _62 += mul(_16(_355, _356), float4x4(-0.14325632154941558837890625, -0.1467452943325042724609375, -0.27502357959747314453125, 0.093708373606204986572265625, 0.118210829794406890869140625, -0.012266484089195728302001953125, -0.21005479991436004638671875, 0.47075021266937255859375, -0.067666478455066680908203125, 0.58165013790130615234375, -0.2512278854846954345703125, -0.3378375470638275146484375, 0.131892502307891845703125, -0.04346276819705963134765625, 0.15454484522342681884765625, 0.0445000566542148590087890625));
      float _382 = -1.0f;
      float _383 = 1.0f;
      _62 += mul(_16(_382, _383), float4x4(-0.0568320713937282562255859375, 0.005194646306335926055908203125, -0.108000524342060089111328125, 0.1013320386409759521484375, -0.507638633251190185546875, 0.0073084421455860137939453125, 0.85424041748046875, 0.28387355804443359375, 0.0227095149457454681396484375, 0.2945230007171630859375, -0.3822472095489501953125, 0.661664068698883056640625, 0.014044850133359432220458984375, 0.03128270804882049560546875, -0.2675681412220001220703125, -0.123147785663604736328125));
      float _409 = 0.0f;
      float _410 = -1.0f;
      _62 += mul(_16(_409, _410), float4x4(-0.3645517826080322265625, 0.347055494785308837890625, -0.0453030876815319061279296875, -0.0317076407372951507568359375, -0.1580249369144439697265625, -0.0019141496159136295318603515625, -0.2593958675861358642578125, -0.23875342309474945068359375, 0.13042800128459930419921875, 0.0395427308976650238037109375, -0.17985536158084869384765625, 0.10514594614505767822265625, 0.15804816782474517822265625, 0.1255171298980712890625, 0.2837197482585906982421875, -0.085748516023159027099609375));
      float _436 = 0.0f;
      float _437 = 0.0f;
      _62 += mul(_16(_436, _437), float4x4(0.0060625462792813777923583984375, 0.24439239501953125, -0.01769225858151912689208984375, -0.20214004814624786376953125, -0.09584514796733856201171875, -0.012805371545255184173583984375, -0.1394222676753997802734375, 0.16143198311328887939453125, 0.1294201314449310302734375, 0.4178554713726043701171875, 0.0460715629160404205322265625, 0.70300257205963134765625, 0.1049964427947998046875, -0.2056601345539093017578125, -0.0313212759792804718017578125, 0.27830326557159423828125));
      float _463 = 0.0f;
      float _464 = 1.0f;
      _62 += mul(_16(_463, _464), float4x4(-0.081274963915348052978515625, -0.14562319219112396240234375, 0.272005259990692138671875, -0.20491313934326171875, 0.012910989113152027130126953125, 0.0242013968527317047119140625, 0.04816257953643798828125, 0.21297328174114227294921875, -0.22015951573848724365234375, -0.4416075646877288818359375, -0.0560353733599185943603515625, 0.3382441699504852294921875, -0.316453039646148681640625, 0.15469242632389068603515625, 0.053187452256679534912109375, -0.2098944485187530517578125));
      float _490 = 1.0f;
      float _491 = -1.0f;
      _62 += mul(_16(_490, _491), float4x4(-0.0465503670275211334228515625, 0.0331854037940502166748046875, 0.3333724439144134521484375, 0.12853644788265228271484375, 0.23520171642303466796875, -0.059092141687870025634765625, 0.08613680303096771240234375, 0.10706329345703125, -0.07058717310428619384765625, -0.11759936809539794921875, -0.1859404742717742919921875, 0.080006264150142669677734375, -0.055425353348255157470703125, -0.12506316602230072021484375, 0.15729053318500518798828125, -0.09150040149688720703125));
      float _517 = 1.0f;
      float _518 = 0.0f;
      _62 += mul(_16(_517, _518), float4x4(0.0425164066255092620849609375, 0.14844788610935211181640625, 0.16533111035823822021484375, 0.13502933084964752197265625, -0.06554169952869415283203125, -0.0572563968598842620849609375, 0.07671372592449188232421875, -0.23448966443538665771484375, 0.1285592615604400634765625, 0.01421927474439144134521484375, 0.0517613850533962249755859375, 0.053433082997798919677734375, -0.2446714937686920166015625, -0.4008074104785919189453125, 0.19603717327117919921875, -0.1796950995922088623046875));
      float _544 = 1.0f;
      float _545 = 1.0f;
      _62 += mul(_16(_544, _545), float4x4(0.147778034210205078125, 0.15524907410144805908203125, 0.0431586168706417083740234375, -0.069968760013580322265625, 0.1921064555644989013671875, -0.21443639695644378662109375, -0.470207870006561279296875, -0.420790612697601318359375, -0.18074385821819305419921875, -0.21639029681682586669921875, 0.00307549652643501758575439453125, 0.367999732494354248046875, -0.383769810199737548828125, -0.00226614973507821559906005859375, -0.37276732921600341796875, -0.289349973201751708984375));
      _62 += float4(-0.01829734630882740020751953125f, -0.08095182478427886962890625f, -0.0621630661189556121826171875f, -0.08050014078617095947265625f);
      PSOut.Color = _62;
}
})";

extern const std::string kHLSL_Anime4K_Restore_CNN_M_Pass2_Pixel = R"(// Mode A Pass 5

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _25;
static float4 _558;

float4 _12(float _10, float _11)
{
    return max(u_Texture.Sample(u_Texture_sampler, _25 + (float2(_10, _11) * u_InputPt)), 0.0f.xxxx);
}
float4 _16(float _14, float _15)
{
    return max(-u_Texture.Sample(u_Texture_sampler, _25 + (float2(_14, _15) * u_InputPt)), 0.0f.xxxx);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _25 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float _86 = -1.0f;
      float _87 = -1.0f;
      float4 _62 = mul(_12(_86, _87), float4x4(0.315431773662567138671875, 0.23095236718654632568359375, -0.06692610681056976318359375, -0.586776316165924072265625, 0.00362250395119190216064453125, 0.1794884204864501953125, -0.14627707004547119140625, 0.174501597881317138671875, -0.05296458303928375244140625, -0.1555115878582000732421875, 0.0564478598535060882568359375, -0.0126651637256145477294921875, 0.13107763230800628662109375, 0.11369179189205169677734375, -0.094529949128627777099609375, -0.119734026491641998291015625));
      float _111 = -1.0f;
      float _112 = 0.0f;
      _62 += mul(_12(_111, _112), float4x4(-0.269466102123260498046875, -0.115382134914398193359375, 0.307326793670654296875, -0.0672284662723541259765625, -0.2551148235797882080078125, -0.13922207057476043701171875, 0.367582142353057861328125, -0.18821828067302703857421875, -0.02261786349117755889892578125, 0.20333401858806610107421875, -0.111258886754512786865234375, 0.35522449016571044921875, -0.0133466534316539764404296875, -0.0990953743457794189453125, -0.2510061562061309814453125, 0.3552175462245941162109375));
      float _139 = -1.0f;
      float _140 = 1.0f;
      _62 += mul(_12(_139, _140), float4x4(0.0110124088823795318603515625, -0.1367508471012115478515625, 0.2564199864864349365234375, -0.3485120832920074462890625, -0.231846749782562255859375, 0.18012201786041259765625, 0.576541364192962646484375, 0.1031735241413116455078125, -0.1646140515804290771484375, 0.03817708790302276611328125, 0.1234095990657806396484375, 0.013202029280364513397216796875, -0.1903336346149444580078125, 0.074691779911518096923828125, -0.017948545515537261962890625, 0.15287701785564422607421875));
      float _166 = 0.0f;
      float _167 = -1.0f;
      _62 += mul(_12(_166, _167), float4x4(-0.05340532958507537841796875, 0.237974822521209716796875, 0.203513920307159423828125, -0.0533335097134113311767578125, -0.121811740100383758544921875, -0.23363493382930755615234375, -0.2069660723209381103515625, 0.1099410355091094970703125, -0.1151945292949676513671875, 0.13842065632343292236328125, -0.106878317892551422119140625, 0.2904000580310821533203125, 0.02221863158047199249267578125, 0.0312387235462665557861328125, 0.2685182094573974609375, 0.1530006825923919677734375));
      float _193 = 0.0f;
      float _194 = 0.0f;
      _62 += mul(_12(_193, _194), float4x4(0.2298531830310821533203125, -0.3103801906108856201171875, -0.2291641533374786376953125, 0.252388060092926025390625, -0.11690287292003631591796875, -0.19474880397319793701171875, 0.118020534515380859375, 0.07814262807369232177734375, -0.063354738056659698486328125, -0.007870727218687534332275390625, 0.07610632479190826416015625, 0.094677485525608062744140625, -0.1677628457546234130859375, -0.006570436991751194000244140625, -0.2958958446979522705078125, 0.4141350686550140380859375));
      float _220 = 0.0f;
      float _221 = 1.0f;
      _62 += mul(_12(_220, _221), float4x4(0.43607962131500244140625, -0.3645643293857574462890625, -0.123776875436305999755859375, -0.16634953022003173828125, -0.091190874576568603515625, 0.13035081326961517333984375, 0.2862796783447265625, 0.27249968051910400390625, 0.1235634386539459228515625, -0.00861617736518383026123046875, 0.095998160541057586669921875, -0.0061445571482181549072265625, -0.2349030673503875732421875, 0.3013122975826263427734375, 0.1415315568447113037109375, 0.21837277710437774658203125));
      float _247 = 1.0f;
      float _248 = -1.0f;
      _62 += mul(_12(_247, _248), float4x4(0.0603645853698253631591796875, 0.3786022365093231201171875, 0.0391824133694171905517578125, -0.2280542552471160888671875, -0.08991022408008575439453125, -0.06817696988582611083984375, -0.26842749118804931640625, -0.12528502941131591796875, 0.03693449497222900390625, -0.07826615869998931884765625, 0.065599761903285980224609375, -0.0825364589691162109375, 0.1348964869976043701171875, 0.0623766295611858367919921875, 0.1263760030269622802734375, 0.21194183826446533203125));
      float _274 = 1.0f;
      float _275 = 0.0f;
      _62 += mul(_12(_274, _275), float4x4(-0.12534816563129425048828125, 0.21225188672542572021484375, -0.2781804502010345458984375, -0.3070442974567413330078125, -0.0069575770758092403411865234375, -0.0251058526337146759033203125, 0.121009238064289093017578125, -0.069164521992206573486328125, 0.23081482946872711181640625, 0.18027560412883758544921875, -0.18995638191699981689453125, 0.1660301387310028076171875, -0.2904095947742462158203125, -0.2529282271862030029296875, -0.21834068000316619873046875, 0.13719652593135833740234375));
      float _301 = 1.0f;
      float _302 = 1.0f;
      _62 += mul(_12(_301, _302), float4x4(0.01720965467393398284912109375, 0.107571370899677276611328125, 0.21414296329021453857421875, -0.30885982513427734375, 0.104677163064479827880859375, -0.21848909556865692138671875, 0.100061476230621337890625, -0.15275280177593231201171875, 0.21004720032215118408203125, -0.2576854526996612548828125, -0.22329919040203094482421875, -0.2915342748165130615234375, -0.06983841955661773681640625, -0.1038548648357391357421875, -0.051384352147579193115234375, 0.14629121124744415283203125));
      float _328 = -1.0f;
      float _329 = -1.0f;
      _62 += mul(_16(_328, _329), float4x4(0.0059623294509947299957275390625, -0.260608017444610595703125, 0.3211581707000732421875, 0.0210255049169063568115234375, 0.09783084690570831298828125, -0.15865178406238555908203125, 0.14730210602283477783203125, -0.2497730255126953125, -0.0335082821547985076904296875, 0.174803912639617919921875, -0.091310136020183563232421875, 0.09870876371860504150390625, 0.10504043102264404296875, -0.061056859791278839111328125, 0.013493488542735576629638671875, -0.112788550555706024169921875));
      float _355 = -1.0f;
      float _356 = 0.0f;
      _62 += mul(_16(_355, _356), float4x4(0.1487524807453155517578125, -0.1485941410064697265625, 0.1937706172466278076171875, -0.17456068098545074462890625, 0.101288855075836181640625, -0.111368201673030853271484375, -0.489446461200714111328125, 0.1018564999103546142578125, -0.0373923368752002716064453125, 0.085396908223628997802734375, 0.1751306056976318359375, -0.15428723394870758056640625, -0.0593755580484867095947265625, 0.02766367234289646148681640625, 0.051804013550281524658203125, -0.0498132221400737762451171875));
      float _382 = -1.0f;
      float _383 = 1.0f;
      _62 += mul(_16(_382, _383), float4x4(0.1188465654850006103515625, -0.19869871437549591064453125, -0.037388257682323455810546875, 0.0845672786235809326171875, -0.116625271737575531005859375, -0.4381835162639617919921875, -0.093285344541072845458984375, 0.0385072045028209686279296875, -0.0519916675984859466552734375, 0.2100829184055328369140625, 0.107923649251461029052734375, 0.20209239423274993896484375, 0.057021595537662506103515625, 0.094605267047882080078125, 0.001655128784477710723876953125, -0.001595706329680979251861572265625));
      float _409 = 0.0f;
      float _410 = -1.0f;
      _62 += mul(_16(_409, _410), float4x4(0.110621742904186248779296875, -0.2639231979846954345703125, -0.0602954663336277008056640625, -0.3217330873012542724609375, -0.0505452118813991546630859375, 0.309895575046539306640625, 0.3090613186359405517578125, 0.03032327257096767425537109375, 0.028986752033233642578125, 0.0374294035136699676513671875, 0.20855663716793060302734375, -0.1984894275665283203125, 0.03468765318393707275390625, -0.095991350710391998291015625, -0.062504939734935760498046875, -0.1321586668491363525390625));
      float _436 = 0.0f;
      float _437 = 0.0f;
      _62 += mul(_16(_436, _437), float4x4(-0.0103911459445953369140625, 0.07657845318317413330078125, 0.4449125826358795166015625, 0.0435906015336513519287109375, 0.007593150250613689422607421875, 0.4263265430927276611328125, 0.47022533416748046875, 0.3473743498325347900390625, -0.15452717244625091552734375, -0.1461341083049774169921875, -0.4523106515407562255859375, 0.120944090187549591064453125, 0.006791184656322002410888671875, 0.05750115215778350830078125, 0.098769791424274444580078125, 0.04494644701480865478515625));
      float _463 = 0.0f;
      float _464 = 1.0f;
      _62 += mul(_16(_463, _464), float4x4(-0.156074345111846923828125, 0.229305803775787353515625, -0.0952033102512359619140625, 0.012836731970310211181640625, -0.1528245508670806884765625, 0.264377176761627197265625, -0.16854770481586456298828125, -0.1321112215518951416015625, -0.055801592767238616943359375, -0.01677872799336910247802734375, -0.34478986263275146484375, -0.2322830855846405029296875, 0.123009622097015380859375, -0.13235826790332794189453125, -0.13987202942371368408203125, -0.16550971567630767822265625));
      float _490 = 1.0f;
      float _491 = -1.0f;
      _62 += mul(_16(_490, _491), float4x4(0.13161735236644744873046875, -0.090393461287021636962890625, -0.03347547352313995361328125, -0.2368669807910919189453125, 0.15148849785327911376953125, 0.20977421104907989501953125, 0.0314319543540477752685546875, -0.004922610707581043243408203125, 0.090661935508251190185546875, 0.152880609035491943359375, -0.0331658311188220977783203125, 0.096465729176998138427734375, -0.3265170753002166748046875, 0.18825398385524749755859375, -0.1577723920345306396484375, 0.1757270395755767822265625));
      float _517 = 1.0f;
      float _518 = 0.0f;
      _62 += mul(_16(_517, _518), float4x4(0.11215722560882568359375, -0.087128780782222747802734375, 0.234531819820404052734375, 0.104387700557708740234375, -0.14686782658100128173828125, 0.28682422637939453125, -0.086443506181240081787109375, 0.0594570524990558624267578125, -0.315301120281219482421875, -0.2700583040714263916015625, -0.0602895207703113555908203125, -0.070416875183582305908203125, 0.18053482472896575927734375, 0.166533410549163818359375, 0.252151966094970703125, 0.061915852129459381103515625));
      float _544 = 1.0f;
      float _545 = 1.0f;
      _62 += mul(_16(_544, _545), float4x4(-0.20122241973876953125, 0.076313145458698272705078125, -0.09884829819202423095703125, 0.094337783753871917724609375, -0.3543668687343597412109375, 0.3762327134609222412109375, -0.078095577657222747802734375, 0.30558478832244873046875, 0.104252420365810394287109375, -0.17087407410144805908203125, 0.03030149638652801513671875, -0.13911743462085723876953125, 0.01630274951457977294921875, 0.24247427284717559814453125, -0.0064744767732918262481689453125, 0.0384264104068279266357421875));
      _62 += float4(-0.00895284675061702728271484375f, -0.0058945752680301666259765625f, -0.080972291529178619384765625f, 0.02096859179437160491943359375f);
      PSOut.Color = _62;
}
})";

extern const std::string kHLSL_Anime4K_Restore_CNN_M_Pass3_Pixel = R"(// Mode A Pass 6

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _25;
static float4 _558;

float4 _12(float _10, float _11)
{
    return max(u_Texture.Sample(u_Texture_sampler, _25 + (float2(_10, _11) * u_InputPt)), 0.0f.xxxx);
}
float4 _16(float _14, float _15)
{
    return max(-u_Texture.Sample(u_Texture_sampler, _25 + (float2(_14, _15) * u_InputPt)), 0.0f.xxxx);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _25 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float _86 = -1.0f;
      float _87 = -1.0f;
      float4 _62 = mul(_12(_86, _87), float4x4(-0.22377209365367889404296875, -0.0064096362330019474029541015625, -0.3180842697620391845703125, 0.73477733135223388671875, 0.015353088267147541046142578125, 0.23983319103717803955078125, 0.14967978000640869140625, -0.3492022454738616943359375, -0.074562691152095794677734375, 0.093151815235614776611328125, -0.14331085979938507080078125, -0.24586205184459686279296875, -0.14183366298675537109375, 0.064010448753833770751953125, -0.22044073045253753662109375, 0.2993227541446685791015625));
      float _111 = -1.0f;
      float _112 = 0.0f;
      _62 += mul(_12(_111, _112), float4x4(-0.07968509197235107421875, -0.3349145948886871337890625, 0.1652912795543670654296875, 0.084434993565082550048828125, 0.4095855057239532470703125, -0.1712070405483245849609375, 0.17425705492496490478515625, 0.15298946201801300048828125, 0.29812729358673095703125, 0.22123689949512481689453125, 0.1039238870143890380859375, -0.287754535675048828125, -0.06524765491485595703125, -0.15255849063396453857421875, 0.13094437122344970703125, 0.1868521869182586669921875));
      float _139 = -1.0f;
      float _140 = 1.0f;
      _62 += mul(_12(_139, _140), float4x4(0.0157067365944385528564453125, -0.17755036056041717529296875, 0.2622525990009307861328125, 0.112057305872440338134765625, -0.158767879009246826171875, -0.384669959545135498046875, -0.3370084464550018310546875, -0.03171174228191375732421875, -0.0233209617435932159423828125, -0.3145248889923095703125, -0.21223734319210052490234375, -0.13145959377288818359375, -0.18880949914455413818359375, -0.04637010395526885986328125, 0.09000895917415618896484375, -0.0046378844417631626129150390625));
      float _166 = 0.0f;
      float _167 = -1.0f;
      _62 += mul(_12(_166, _167), float4x4(-0.311275064945220947265625, 0.31304323673248291015625, -0.03965751826763153076171875, 0.036490179598331451416015625, -0.02985105477273464202880859375, 0.0580137707293033599853515625, 0.00040150844142772257328033447265625, -0.0442206896841526031494140625, 0.18019931018352508544921875, 0.1441551148891448974609375, -0.0984523594379425048828125, 0.21895433962345123291015625, -0.013932473957538604736328125, -0.0464549474418163299560546875, -0.3403935134410858154296875, -0.006705288775265216827392578125));
      float _193 = 0.0f;
      float _194 = 0.0f;
      _62 += mul(_12(_193, _194), float4x4(-0.34878647327423095703125, -0.512928307056427001953125, 0.06025095283985137939453125, -0.16354133188724517822265625, 0.20644618570804595947265625, 0.08732272684574127197265625, -0.24118888378143310546875, 0.244550645351409912109375, 0.2444942295551300048828125, 0.4410338699817657470703125, 0.2245592772960662841796875, 0.25738942623138427734375, -0.269146978855133056640625, -0.2130998671054840087890625, 0.083864860236644744873046875, 0.02148481644690036773681640625));
      float _220 = 0.0f;
      float _221 = 1.0f;
      _62 += mul(_12(_220, _221), float4x4(-0.0574549026787281036376953125, -0.4121921956539154052734375, 0.02266154624521732330322265625, 0.371782720088958740234375, 0.0333140790462493896484375, 0.050440080463886260986328125, 0.0432437099516391754150390625, 0.20727942883968353271484375, 0.24326409399509429931640625, 0.07690669596195220947265625, -0.20858038961887359619140625, 0.012439015321433544158935546875, -0.193350613117218017578125, 0.092174507677555084228515625, 0.19683690369129180908203125, -0.19435833394527435302734375));
      float _247 = 1.0f;
      float _248 = -1.0f;
      _62 += mul(_12(_247, _248), float4x4(-0.169604957103729248046875, 0.2461616694927215576171875, 0.3797747790813446044921875, 0.14324574172496795654296875, -0.011531225405633449554443359375, -0.113121427595615386962890625, -0.18141078948974609375, -0.2384393215179443359375, 0.008601217530667781829833984375, -0.3564490973949432373046875, -0.126394808292388916015625, 0.0097992978990077972412109375, -0.2912061214447021484375, 0.23756824433803558349609375, 0.1803569495677947998046875, -0.087133996188640594482421875));
      float _274 = 1.0f;
      float _275 = 0.0f;
      _62 += mul(_12(_274, _275), float4x4(-0.10081239044666290283203125, 0.29191493988037109375, 0.104346930980682373046875, 0.089706361293792724609375, 0.00899775885045528411865234375, 0.10475623607635498046875, 0.0396410860121250152587890625, 0.02323888055980205535888671875, -0.11627764999866485595703125, 0.0236932225525379180908203125, -0.3080175817012786865234375, -0.120208986103534698486328125, 0.050861470401287078857421875, 0.18498174846172332763671875, 0.1559543907642364501953125, -0.098773062229156494140625));
      float _301 = 1.0f;
      float _302 = 1.0f;
      _62 += mul(_12(_301, _302), float4x4(0.101321674883365631103515625, -0.2929975986480712890625, 0.3881041705608367919921875, 0.5605375766754150390625, -0.0407393686473369598388671875, 0.03011070378124713897705078125, -0.18147061765193939208984375, -0.098339520394802093505859375, 0.0192773304879665374755859375, 0.15335668623447418212890625, -0.15384073555469512939453125, -0.110595054924488067626953125, -0.054297395050525665283203125, -0.07752205431461334228515625, 0.079183690249919891357421875, -0.06848062574863433837890625));
      float _328 = -1.0f;
      float _329 = -1.0f;
      _62 += mul(_16(_328, _329), float4x4(0.23263514041900634765625, -0.117192320525646209716796875, 0.2903209030628204345703125, -0.0075037949718534946441650390625, -0.020222447812557220458984375, -0.177901566028594970703125, -0.1560076177120208740234375, -0.0874177515506744384765625, 0.1252970397472381591796875, 0.255488574504852294921875, -0.045854471623897552490234375, -0.102550327777862548828125, 0.1835050284862518310546875, -0.295935332775115966796875, 0.086893297731876373291015625, 0.0270047374069690704345703125));
      float _355 = -1.0f;
      float _356 = 0.0f;
      _62 += mul(_16(_355, _356), float4x4(-0.14958654344081878662109375, -0.00623883493244647979736328125, -0.2928948104381561279296875, 0.198855698108673095703125, -0.1705780327320098876953125, 0.12524141371250152587890625, 0.1397826373577117919921875, -0.019280292093753814697265625, 0.0596714206039905548095703125, -0.077908180654048919677734375, -0.58938181400299072265625, -0.02284571342170238494873046875, -0.085967786610126495361328125, 0.078753583133220672607421875, -0.033166669309139251708984375, -0.436928212642669677734375));
      float _382 = -1.0f;
      float _383 = 1.0f;
      _62 += mul(_16(_382, _383), float4x4(0.19195687770843505859375, -0.0608836822211742401123046875, -0.2589782774448394775390625, 0.070633240044116973876953125, 0.0908333957195281982421875, 0.00342288310639560222625732421875, 0.1095341742038726806640625, 0.03118087351322174072265625, -0.05017118155956268310546875, 0.02286216802895069122314453125, -0.2701129913330078125, -0.057831235229969024658203125, 0.53920543193817138671875, -0.102527759969234466552734375, -0.09180748462677001953125, 0.0042943428270518779754638671875));
      float _409 = 0.0f;
      float _410 = -1.0f;
      _62 += mul(_16(_409, _410), float4x4(-0.184942424297332763671875, -0.119284816086292266845703125, 0.382189691066741943359375, 0.077779792249202728271484375, 0.15568028390407562255859375, -0.2854858934879302978515625, -0.22441281378269195556640625, -0.04915587604045867919921875, -0.15292496979236602783203125, 0.21895618736743927001953125, -0.095677755773067474365234375, 0.15210424363613128662109375, 0.0016430220566689968109130859375, -0.02617698721587657928466796875, 0.0484630763530731201171875, -0.4824008941650390625));
      float _436 = 0.0f;
      float _437 = 0.0f;
      _62 += mul(_16(_436, _437), float4x4(0.00721512921154499053955078125, 0.17074333131313323974609375, 0.0539300739765167236328125, -0.02701481617987155914306640625, -0.17180430889129638671875, -0.15163862705230712890625, -0.0012122131884098052978515625, -0.189342558383941650390625, -0.082942970097064971923828125, -0.24580220878124237060546875, -0.465528666973114013671875, -0.2792322337627410888671875, 0.4092667996883392333984375, 0.06288687884807586669921875, -0.1602188050746917724609375, -0.00308768451213836669921875));
      float _463 = 0.0f;
      float _464 = 1.0f;
      _62 += mul(_16(_463, _464), float4x4(0.11187088489532470703125, 0.0331714488565921783447265625, 0.141552984714508056640625, 0.20328505337238311767578125, -0.0510413087904453277587890625, 0.13979794085025787353515625, 0.0189668349921703338623046875, -0.072385109961032867431640625, 0.054937921464443206787109375, -0.1497578322887420654296875, -0.102932371199131011962890625, -0.21985305845737457275390625, 0.49054706096649169921875, 0.1828818619251251220703125, -0.2692582607269287109375, 0.3584593236446380615234375));
      float _490 = 1.0f;
      float _491 = -1.0f;
      _62 += mul(_16(_490, _491), float4x4(0.3747799098491668701171875, -0.09674848616123199462890625, -0.1713974177837371826171875, 0.2528985440731048583984375, -0.174211680889129638671875, -0.01846181787550449371337890625, 0.097471617162227630615234375, 0.01660534925758838653564453125, -0.20580358803272247314453125, 0.5618965625762939453125, 0.17151354253292083740234375, -0.26347768306732177734375, 0.2835056781768798828125, -0.214860141277313232421875, -0.4433092772960662841796875, -0.008981036953628063201904296875));
      float _517 = 1.0f;
      float _518 = 0.0f;
      _62 += mul(_16(_517, _518), float4x4(0.101699851453304290771484375, -0.18244017660617828369140625, 0.0476073585450649261474609375, 0.4101764261722564697265625, -0.094687856733798980712890625, -0.02421847544610500335693359375, 0.103733874857425689697265625, -0.22540338337421417236328125, 0.106301121413707733154296875, 0.367717802524566650390625, -0.104170955717563629150390625, 0.0573174469172954559326171875, 0.21764881908893585205078125, 0.078915797173976898193359375, -0.22041337192058563232421875, 0.15065215528011322021484375));
      float _544 = 1.0f;
      float _545 = 1.0f;
      _62 += mul(_16(_544, _545), float4x4(0.1163399517536163330078125, -0.008195114322006702423095703125, -0.1450153291225433349609375, 0.071680247783660888671875, 0.058413274586200714111328125, 0.055995367467403411865234375, 0.09362144768238067626953125, -0.13827963173389434814453125, 0.13760869204998016357421875, 0.04031978547573089599609375, 0.0388950444757938385009765625, 0.2675252854824066162109375, -0.08733968436717987060546875, 0.14120729267597198486328125, -0.17166458070278167724609375, -0.23129940032958984375));
      _62 += float4(-0.0593773536384105682373046875f, -0.020553410053253173828125f, 0.072348691523075103759765625f, -0.015452985651791095733642578125f);
      PSOut.Color = _62;
}
})";

extern const std::string kHLSL_Anime4K_Restore_CNN_M_Pass4_Pixel = R"(// Mode A Pass 7

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _25;
static float4 _558;

float4 _12(float _10, float _11)
{
    return max(u_Texture.Sample(u_Texture_sampler, _25 + (float2(_10, _11) * u_InputPt)), 0.0f.xxxx);
}
float4 _16(float _14, float _15)
{
    return max(-u_Texture.Sample(u_Texture_sampler, _25 + (float2(_14, _15) * u_InputPt)), 0.0f.xxxx);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _25 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float _86 = -1.0f;
      float _87 = -1.0f;
      float4 _62 = mul(_12(_86, _87), float4x4(-0.290129840373992919921875, -0.1315014660358428955078125, 0.3101561367511749267578125, 0.059922911226749420166015625, -0.0502898655831813812255859375, 0.14845313131809234619140625, -0.096088983118534088134765625, 0.2791330814361572265625, 0.0603073872625827789306640625, -0.041604518890380859375, 0.035932682454586029052734375, -0.0813756287097930908203125, -0.07999418675899505615234375, 0.118182837963104248046875, -0.2751228809356689453125, 0.21948812901973724365234375));
      float _111 = -1.0f;
      float _112 = 0.0f;
      _62 += mul(_12(_111, _112), float4x4(0.129160583019256591796875, -0.21759961545467376708984375, -0.338685333728790283203125, 0.0216366611421108245849609375, 0.0534702427685260772705078125, 0.141242504119873046875, 0.0433953963220119476318359375, -0.2675105631351470947265625, -0.016891010105609893798828125, -0.2623834908008575439453125, 0.010809152387082576751708984375, 0.06296281516551971435546875, -0.2069201171398162841796875, -0.167786300182342529296875, -0.2331385910511016845703125, -0.17402614653110504150390625));
      float _139 = -1.0f;
      float _140 = 1.0f;
      _62 += mul(_12(_139, _140), float4x4(-0.082041122019290924072265625, -0.2367208302021026611328125, -0.0064437394030392169952392578125, -0.13200695812702178955078125, -0.0566929243505001068115234375, -0.02708657085895538330078125, 0.12536962330341339111328125, 0.0044289189390838146209716796875, 0.14137582480907440185546875, 0.15404348075389862060546875, -0.105753876268863677978515625, 0.0479574538767337799072265625, 0.15734316408634185791015625, 0.1656242311000823974609375, -0.010160828940570354461669921875, -0.06602983176708221435546875));
      float _166 = 0.0f;
      float _167 = -1.0f;
      _62 += mul(_12(_166, _167), float4x4(0.02565399743616580963134765625, -0.10877774655818939208984375, -0.3125890791416168212890625, 0.18841636180877685546875, -0.360051929950714111328125, 0.18163569271564483642578125, -0.345376431941986083984375, -0.074108697474002838134765625, 0.4663994014263153076171875, 0.006518651731312274932861328125, 0.08109033107757568359375, 0.2976773083209991455078125, -0.3577422797679901123046875, -0.04136605560779571533203125, -0.3785277307033538818359375, 0.0505656562745571136474609375));
      float _193 = 0.0f;
      float _194 = 0.0f;
      _62 += mul(_12(_193, _194), float4x4(0.0439231283962726593017578125, 0.11316680908203125, -0.1442138850688934326171875, 0.1798566877841949462890625, -0.16512739658355712890625, -0.565620899200439453125, -0.124100483953952789306640625, 0.4277405440807342529296875, -0.115393899381160736083984375, 0.1682985126972198486328125, 0.202561199665069580078125, 0.05400745570659637451171875, -0.068682558834552764892578125, -0.5693595409393310546875, -0.122279606759548187255859375, 0.1768886148929595947265625));
      float _220 = 0.0f;
      float _221 = 1.0f;
      _62 += mul(_12(_220, _221), float4x4(0.3404099941253662109375, 0.499000012874603271484375, 0.15234196186065673828125, 0.21353457868099212646484375, -0.2732667028903961181640625, -0.049950934946537017822265625, 0.03550811111927032470703125, -0.210516870021820068359375, 0.26090228557586669921875, 0.0164384543895721435546875, -0.2987463176250457763671875, 0.379941284656524658203125, 0.04928840696811676025390625, -0.3112630546092987060546875, 0.0292355120182037353515625, -0.0122560150921344757080078125));
      float _247 = 1.0f;
      float _248 = -1.0f;
      _62 += mul(_12(_247, _248), float4x4(-0.0046853204257786273956298828125, 0.1539137363433837890625, -0.04068966209888458251953125, 0.20186872780323028564453125, -0.08137620985507965087890625, 0.359055578708648681640625, 0.2373384535312652587890625, 0.2179479300975799560546875, -0.066420383751392364501953125, 0.02960065566003322601318359375, -0.3142104446887969970703125, -0.0507738627493381500244140625, -0.062607727944850921630859375, 0.04634220898151397705078125, -0.1094849109649658203125, -0.0454989336431026458740234375));
      float _274 = 1.0f;
      float _275 = 0.0f;
      _62 += mul(_12(_274, _275), float4x4(-0.082952998578548431396484375, -0.02583706378936767578125, -0.099283032119274139404296875, -0.14300231635570526123046875, 0.275063991546630859375, 0.0779361724853515625, 0.22240887582302093505859375, 0.06637834012508392333984375, -0.4382666051387786865234375, -0.293218195438385009765625, -0.272431671619415283203125, -0.1422118246555328369140625, 0.56957280635833740234375, 0.20719237625598907470703125, 0.557592689990997314453125, 0.4081688225269317626953125));
      float _301 = 1.0f;
      float _302 = 1.0f;
      _62 += mul(_12(_301, _302), float4x4(-0.1851092875003814697265625, -0.1505216658115386962890625, 0.2527721226215362548828125, 0.068044610321521759033203125, 0.01638700067996978759765625, 0.2031003534793853759765625, 0.29032289981842041015625, -0.061587698757648468017578125, -0.2898727357387542724609375, -0.119426049292087554931640625, 0.013498960994184017181396484375, 0.3184151947498321533203125, 0.2954347431659698486328125, -0.0428309030830860137939453125, -0.018111206591129302978515625, -0.13263674080371856689453125));
      float _328 = -1.0f;
      float _329 = -1.0f;
      _62 += mul(_16(_328, _329), float4x4(0.2574908733367919921875, 0.0053866603411734104156494140625, -0.0939116179943084716796875, -0.0612952895462512969970703125, -0.094091184437274932861328125, -0.074196331202983856201171875, 0.001385861076414585113525390625, 0.012000353075563907623291015625, -0.06290300190448760986328125, -0.02042240090668201446533203125, -0.121133126318454742431640625, 0.01794255711138248443603515625, -0.073379933834075927734375, 0.0522019863128662109375, 0.3586457669734954833984375, 0.02356440387666225433349609375));
      float _355 = -1.0f;
      float _356 = 0.0f;
      _62 += mul(_16(_355, _356), float4x4(0.100115694105625152587890625, 0.1945135891437530517578125, 0.23252093791961669921875, 0.1950680911540985107421875, -0.1247077882289886474609375, 0.0027281935326755046844482421875, -0.1748857200145721435546875, -0.0187219642102718353271484375, -0.151593387126922607421875, 0.18457151949405670166015625, 0.05771298706531524658203125, -0.08191494643688201904296875, 0.1973570287227630615234375, 0.0732674300670623779296875, -0.28563106060028076171875, 0.016428150236606597900390625));
      float _382 = -1.0f;
      float _383 = 1.0f;
      _62 += mul(_16(_382, _383), float4x4(0.0680625140666961669921875, 0.283566653728485107421875, 0.073778979480266571044921875, 0.427769720554351806640625, 0.2872502505779266357421875, -0.130452930927276611328125, -0.17525704205036163330078125, -0.0588559098541736602783203125, -0.16676305234432220458984375, -0.2555944919586181640625, -0.100784219801425933837890625, -0.05303287506103515625, 0.084470875561237335205078125, 0.06460686028003692626953125, 0.138243615627288818359375, -0.052313528954982757568359375));
      float _409 = 0.0f;
      float _410 = -1.0f;
      _62 += mul(_16(_409, _410), float4x4(0.2263782918453216552734375, -0.0289692543447017669677734375, 0.19682539999485015869140625, -0.13331995904445648193359375, 0.0380170531570911407470703125, -0.008854481391608715057373046875, -0.20316390693187713623046875, 0.092370890080928802490234375, -0.38211119174957275390625, 0.11085270345211029052734375, -0.110299326479434967041015625, -0.245420277118682861328125, 0.224161446094512939453125, -0.03149211406707763671875, -0.1914430558681488037109375, -0.099627099931240081787109375));
      float _436 = 0.0f;
      float _437 = 0.0f;
      _62 += mul(_16(_436, _437), float4x4(0.107767440378665924072265625, 0.1636344492435455322265625, 0.1465650498867034912109375, -0.3737814128398895263671875, -0.06642015278339385986328125, 0.56165492534637451171875, -0.008412252180278301239013671875, -0.3726684749126434326171875, 0.12506575882434844970703125, -0.1532903611660003662109375, 0.03753824532032012939453125, -0.1081025898456573486328125, 0.01706348918378353118896484375, 0.181370198726654052734375, 0.03565178811550140380859375, -0.012786579318344593048095703125));
      float _463 = 0.0f;
      float _464 = 1.0f;
      _62 += mul(_16(_463, _464), float4x4(-0.402333796024322509765625, -0.20986139774322509765625, -0.18285121023654937744140625, -0.02727653086185455322265625, 0.2610736191272735595703125, 0.041306912899017333984375, -0.0365155041217803955078125, -0.045217297971248626708984375, -0.399586021900177001953125, -0.2122933864593505859375, -0.021053291857242584228515625, -0.134275019168853759765625, 0.3617881834506988525390625, 0.2093491256237030029296875, 0.15008519589900970458984375, 0.26345539093017578125));
      float _490 = 1.0f;
      float _491 = -1.0f;
      _62 += mul(_16(_490, _491), float4x4(0.07794611155986785888671875, -0.259375870227813720703125, -0.068225286900997161865234375, -0.0563361346721649169921875, 0.094220124185085296630859375, 0.2158884704113006591796875, -0.0455217994749546051025390625, -0.10968329012393951416015625, -0.080684490501880645751953125, -0.3136669695377349853515625, 0.07799637317657470703125, 0.24252681434154510498046875, 0.23963861167430877685546875, 0.137155354022979736328125, 0.01032934524118900299072265625, 0.0909430086612701416015625));
      float _517 = 1.0f;
      float _518 = 0.0f;
      _62 += mul(_16(_517, _518), float4x4(-0.2097571790218353271484375, -0.12550137937068939208984375, 0.14453573524951934814453125, -0.00208786316215991973876953125, -0.071530677378177642822265625, 0.32499980926513671875, -0.056577377021312713623046875, 0.18166828155517578125, 0.3720407187938690185546875, 0.170183360576629638671875, 0.375289499759674072265625, 0.321785867214202880859375, 0.25719821453094482421875, -0.2725863158702850341796875, -0.2597100436687469482421875, -0.4053600728511810302734375));
      float _544 = 1.0f;
      float _545 = 1.0f;
      _62 += mul(_16(_544, _545), float4x4(-0.324390709400177001953125, -0.06300620734691619873046875, -0.093984358012676239013671875, -0.1954918801784515380859375, 0.14906860888004302978515625, 0.0615377835929393768310546875, -0.055284477770328521728515625, 0.112817279994487762451171875, 0.12964856624603271484375, 0.09979093074798583984375, -0.18101589381694793701171875, -0.4104282855987548828125, 0.0580797083675861358642578125, -0.0563712455332279205322265625, 0.080725543200969696044921875, 0.184790074825286865234375));
      _62 += float4(-0.0488884635269641876220703125f, -0.0561433993279933929443359375f, 0.0306909121572971343994140625f, -0.03049668483436107635498046875f);
      PSOut.Color = _62;
}
})";

extern const std::string kHLSL_Anime4K_Restore_CNN_M_Pass5_Pixel = R"(// Mode A Pass 8

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _25;
static float4 _558;

float4 _12(float _10, float _11)
{
    return max(u_Texture.Sample(u_Texture_sampler, _25 + (float2(_10, _11) * u_InputPt)), 0.0f.xxxx);
}
float4 _16(float _14, float _15)
{
    return max(-u_Texture.Sample(u_Texture_sampler, _25 + (float2(_14, _15) * u_InputPt)), 0.0f.xxxx);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _25 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float _86 = -1.0f;
      float _87 = -1.0f;
      float4 _62 = mul(_12(_86, _87), float4x4(0.15332128107547760009765625, 0.0272582583129405975341796875, 0.1490050256252288818359375, -0.1598279476165771484375, 0.1702123582363128662109375, -0.510460436344146728515625, -0.1528727114200592041015625, -0.0581673271954059600830078125, 0.518261849880218505859375, -0.34817993640899658203125, 0.004513166844844818115234375, 0.0539576895534992218017578125, 0.1990320980548858642578125, -0.04997922480106353759765625, 0.113919891417026519775390625, -0.16062729060649871826171875));
      float _111 = -1.0f;
      float _112 = 0.0f;
      _62 += mul(_12(_111, _112), float4x4(0.033682905137538909912109375, 0.01972888596355915069580078125, 0.19931755959987640380859375, 0.1738192737102508544921875, 0.258576810359954833984375, -0.21245719492435455322265625, -0.014632458798587322235107421875, 0.3977989256381988525390625, -0.11462070047855377197265625, -0.2396624982357025146484375, 0.089602768421173095703125, 0.3834529817104339599609375, 0.254976928234100341796875, 0.11692859232425689697265625, -0.14207516610622406005859375, 0.12667973339557647705078125));
      float _139 = -1.0f;
      float _140 = 1.0f;
      _62 += mul(_12(_139, _140), float4x4(-0.1491125524044036865234375, 0.089107058942317962646484375, 0.16136817634105682373046875, 0.0391456596553325653076171875, 0.24204038083553314208984375, -0.0360714904963970184326171875, -0.4571109116077423095703125, 0.10802461206912994384765625, -0.002135685645043849945068359375, 0.008858780376613140106201171875, 0.22297303378582000732421875, 0.2367230951786041259765625, 0.0451775826513767242431640625, 0.111206062138080596923828125, -0.00997190363705158233642578125, -0.05926239490509033203125));
      float _166 = 0.0f;
      float _167 = -1.0f;
      _62 += mul(_12(_166, _167), float4x4(0.24565999209880828857421875, -0.22613839805126190185546875, 0.473732054233551025390625, 0.02461341209709644317626953125, -0.109230518341064453125, 0.0390273146331310272216796875, -0.4270740449428558349609375, -0.3783372938632965087890625, 0.3544572889804840087890625, -0.546857774257659912109375, -0.27599155902862548828125, -0.09455917775630950927734375, 0.1876021921634674072265625, -0.1908200085163116455078125, 0.03056546859443187713623046875, 0.20589156448841094970703125));
      float _193 = 0.0f;
      float _194 = 0.0f;
      _62 += mul(_12(_193, _194), float4x4(0.197319805622100830078125, -0.034338630735874176025390625, 0.05996048450469970703125, 0.04564286768436431884765625, 0.18195949494838714599609375, -0.14460869133472442626953125, 0.1286174952983856201171875, 0.20675750076770782470703125, -0.0426320470869541168212890625, -0.118429668247699737548828125, -0.1122444570064544677734375, -0.187647759914398193359375, -0.1956300437450408935546875, 0.02742596901953220367431640625, 0.24056376516819000244140625, 0.594964921474456787109375));
      float _220 = 0.0f;
      float _221 = 1.0f;
      _62 += mul(_12(_220, _221), float4x4(0.0550276823341846466064453125, 0.163315951824188232421875, -0.2608588039875030517578125, 0.12545955181121826171875, 0.4588985145092010498046875, 0.0364290885627269744873046875, 0.22187738120555877685546875, 0.451907336711883544921875, -0.001210132963024079799652099609375, -0.05765141546726226806640625, -0.0611990429461002349853515625, 0.119354762136936187744140625, -0.049561135470867156982421875, 0.275098860263824462890625, 0.13778673112392425537109375, -0.12491403520107269287109375));
      float _247 = 1.0f;
      float _248 = -1.0f;
      _62 += mul(_12(_247, _248), float4x4(-0.02257459051907062530517578125, 0.2770510613918304443359375, 0.044165275990962982177734375, -0.2652123272418975830078125, 0.0598237402737140655517578125, -0.2824302017688751220703125, 0.3171142041683197021484375, 0.084305606782436370849609375, -0.101555280387401580810546875, 0.16182267665863037109375, -0.091831468045711517333984375, -0.19447176158428192138671875, 0.329570710659027099609375, -0.50616395473480224609375, -0.03696404397487640380859375, 0.23166708648204803466796875));
      float _274 = 1.0f;
      float _275 = 0.0f;
      _62 += mul(_12(_274, _275), float4x4(-0.0232341997325420379638671875, 0.072997987270355224609375, -0.1803807914257049560546875, -0.13672702014446258544921875, -0.10830597579479217529296875, 0.15024791657924652099609375, -0.1953192651271820068359375, 0.08709789812564849853515625, -0.2648853361606597900390625, 0.19481427967548370361328125, 0.10737945139408111572265625, -0.14573483169078826904296875, -0.3309468328952789306640625, 0.2415511608123779296875, -0.0985033214092254638671875, 0.2797003090381622314453125));
      float _301 = 1.0f;
      float _302 = 1.0f;
      _62 += mul(_12(_301, _302), float4x4(-0.24089853465557098388671875, 0.1950659453868865966796875, 0.4799155890941619873046875, -0.0583131127059459686279296875, 0.36212956905364990234375, -0.44844806194305419921875, 0.23864488303661346435546875, 0.15477742254734039306640625, -0.077959708869457244873046875, -0.00338619272224605083465576171875, -0.1121616363525390625, 0.0334545634686946868896484375, -0.2589303553104400634765625, 0.23793478310108184814453125, -0.1576942503452301025390625, -0.00033481256105005741119384765625));
      float _328 = -1.0f;
      float _329 = -1.0f;
      _62 += mul(_16(_328, _329), float4x4(0.0577250681817531585693359375, -0.16402530670166015625, -0.13499663770198822021484375, -0.2046035826206207275390625, -0.0243999660015106201171875, 0.14966167509555816650390625, -0.090857334434986114501953125, -0.03967775404453277587890625, 0.00036956605617888271808624267578125, -0.24236615002155303955078125, -0.053542695939540863037109375, -0.004954411648213863372802734375, 0.02665150165557861328125, 0.3901919424533843994140625, -0.2742246091365814208984375, -0.0612423233687877655029296875));
      float _355 = -1.0f;
      float _356 = 0.0f;
      _62 += mul(_16(_355, _356), float4x4(-0.01632327400147914886474609375, -0.036179907619953155517578125, 0.0299659185111522674560546875, 0.11151491105556488037109375, -0.00016685205628164112567901611328125, -0.29573023319244384765625, 0.17996422946453094482421875, -0.2014543712139129638671875, 0.13242749869823455810546875, -0.18442131578922271728515625, -0.2461815178394317626953125, 0.0617804266512393951416015625, -0.027705170214176177978515625, 0.2845299541950225830078125, 0.3980409801006317138671875, -0.11743889749050140380859375));
      float _382 = -1.0f;
      float _383 = 1.0f;
      _62 += mul(_16(_382, _383), float4x4(-0.02506884746253490447998046875, -0.053328387439250946044921875, -0.270537853240966796875, 0.2686645686626434326171875, -0.098662041127681732177734375, 0.0576772131025791168212890625, 0.01850111968815326690673828125, -0.18014706671237945556640625, -0.13319958746433258056640625, -0.144111812114715576171875, -0.2635524272918701171875, -0.022209353744983673095703125, -0.050626449286937713623046875, -0.036771543323993682861328125, 0.132944166660308837890625, -0.1845855712890625));
      float _409 = 0.0f;
      float _410 = -1.0f;
      _62 += mul(_16(_409, _410), float4x4(-0.046194963157176971435546875, 0.0382304377853870391845703125, -0.08993043005466461181640625, -0.07236354053020477294921875, 0.110311232507228851318359375, -0.165049076080322265625, -0.095170356333255767822265625, -0.16459833085536956787109375, -0.5279924869537353515625, 0.126866817474365234375, -0.057261250913143157958984375, 0.0553616769611835479736328125, 0.3159375488758087158203125, 0.0273280926048755645751953125, 0.001839602016843855381011962890625, 0.3058166205883026123046875));
      float _436 = 0.0f;
      float _437 = 0.0f;
      _62 += mul(_16(_436, _437), float4x4(0.0860867798328399658203125, 0.0316843688488006591796875, 0.0077133770100772380828857421875, -0.2614029347896575927734375, -0.12689830362796783447265625, 0.1339586079120635986328125, -0.069848835468292236328125, -0.24080403149127960205078125, 0.018839336931705474853515625, -0.0498210750520229339599609375, -0.21461345255374908447265625, -0.1416830122470855712890625, -0.08723390102386474609375, 0.4709666669368743896484375, 0.0225125066936016082763671875, 0.14860631525516510009765625));
      float _463 = 0.0f;
      float _464 = 1.0f;
      _62 += mul(_16(_463, _464), float4x4(0.062936730682849884033203125, 0.22462968528270721435546875, 0.0454949848353862762451171875, 0.02167354337871074676513671875, 0.18227446079254150390625, -0.2956554889678955078125, 0.080105431377887725830078125, -0.01919729076325893402099609375, -0.01219026930630207061767578125, 0.241982996463775634765625, -0.046537093818187713623046875, -0.4009456634521484375, -0.385364711284637451171875, 0.108171097934246063232421875, -0.16926057636737823486328125, 0.16138376295566558837890625));
      float _490 = 1.0f;
      float _491 = -1.0f;
      _62 += mul(_16(_490, _491), float4x4(-0.1485458910465240478515625, -0.17625804245471954345703125, -0.10849075019359588623046875, 0.22154299914836883544921875, 0.09997196495532989501953125, 0.139015734195709228515625, 0.2946414649486541748046875, 0.02006852626800537109375, 0.05435852706432342529296875, -0.103517048060894012451171875, -0.00629142858088016510009765625, 0.24127025902271270751953125, -0.16914124786853790283203125, 0.12729422748088836669921875, -0.183774530887603759765625, -0.645237505435943603515625));
      float _517 = 1.0f;
      float _518 = 0.0f;
      _62 += mul(_16(_517, _518), float4x4(0.1260339319705963134765625, -0.1098609268665313720703125, 0.2314102947711944580078125, 0.1691504418849945068359375, -0.13619254529476165771484375, -0.093490727245807647705078125, 0.20594225823879241943359375, -0.34507083892822265625, 0.19077192246913909912109375, 0.0525007955729961395263671875, 0.071856446564197540283203125, 0.029082737863063812255859375, -0.015576320700347423553466796875, 0.082549072802066802978515625, -0.550174295902252197265625, -0.3849584758281707763671875));
      float _544 = 1.0f;
      float _545 = 1.0f;
      _62 += mul(_16(_544, _545), float4x4(0.093007959425449371337890625, -0.079218305647373199462890625, 0.46825134754180908203125, -0.087356247007846832275390625, 0.06321121752262115478515625, 0.16234867274761199951171875, 0.042932413518428802490234375, -0.0130574218928813934326171875, 0.0969714820384979248046875, 0.2345752418041229248046875, 0.194174826145172119140625, -0.16804663836956024169921875, 0.18379296362400054931640625, 0.17770062386989593505859375, -0.0502349995076656341552734375, -0.05967660248279571533203125));
      _62 += float4(0.01116949133574962615966796875f, 0.0323995463550090789794921875f, 0.13809899985790252685546875f, 0.02385707199573516845703125f);
      PSOut.Color = _62;
}
})";

extern const std::string kHLSL_Anime4K_Restore_CNN_M_Pass6_Pixel = R"(// Mode A Pass 9

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _25;
static float4 _558;

float4 _12(float _10, float _11)
{
    return max(u_Texture.Sample(u_Texture_sampler, _25 + (float2(_10, _11) * u_InputPt)), 0.0f.xxxx);
}
float4 _16(float _14, float _15)
{
    return max(-u_Texture.Sample(u_Texture_sampler, _25 + (float2(_14, _15) * u_InputPt)), 0.0f.xxxx);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _25 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float _86 = -1.0f;
      float _87 = -1.0f;
      float4 _62 = mul(_12(_86, _87), float4x4(-0.22753362357616424560546875, -0.086120732128620147705078125, 0.3314069211483001708984375, 0.08699528872966766357421875, -0.18788953125476837158203125, -0.0565791167318820953369140625, -0.12905196845531463623046875, -0.066946208477020263671875, 0.05455936491489410400390625, 0.1503159701824188232421875, -0.134303629398345947265625, 0.02164602465927600860595703125, 0.14884404838085174560546875, -0.069429099559783935546875, 0.2614941298961639404296875, 0.112705029547214508056640625));
      float _111 = -1.0f;
      float _112 = 0.0f;
      _62 += mul(_12(_111, _112), float4x4(0.178767621517181396484375, -0.096378482878208160400390625, 0.112853229045867919921875, 0.20048929750919342041015625, 0.131718695163726806640625, -0.0361626856029033660888671875, 0.17958368360996246337890625, -0.069624997675418853759765625, 0.287607371807098388671875, -0.1250514090061187744140625, 0.12760694324970245361328125, 0.0477179549634456634521484375, -0.16811855137348175048828125, -0.1634070873260498046875, 0.13278298079967498779296875, -0.0840395390987396240234375));
      float _139 = -1.0f;
      float _140 = 1.0f;
      _62 += mul(_12(_139, _140), float4x4(-0.21917523443698883056640625, 0.079711854457855224609375, -0.2864253520965576171875, 0.2822416126728057861328125, 0.03001488931477069854736328125, -0.0147729180753231048583984375, -0.3487395942211151123046875, 0.105971448123455047607421875, -0.013841082341969013214111328125, 0.17034237086772918701171875, 0.108102820813655853271484375, -0.080896951258182525634765625, -0.22184245288372039794921875, -0.59067356586456298828125, 0.441133975982666015625, 0.13045649230480194091796875));
      float _166 = 0.0f;
      float _167 = -1.0f;
      _62 += mul(_12(_166, _167), float4x4(-0.2990693151950836181640625, 0.01392374932765960693359375, 0.2031123936176300048828125, -0.118466876447200775146484375, -0.13953633606433868408203125, 0.080034546554088592529296875, -0.101644940674304962158203125, -0.2121855914592742919921875, 0.10563714802265167236328125, 0.310331165790557861328125, -0.0759035050868988037109375, 0.0473109073936939239501953125, -0.37824213504791259765625, -0.14506383240222930908203125, 0.11866700649261474609375, -0.2138448655605316162109375));
      float _193 = 0.0f;
      float _194 = 0.0f;
      _62 += mul(_12(_193, _194), float4x4(-0.13538490235805511474609375, 0.19258606433868408203125, 0.063908584415912628173828125, -0.20437879860401153564453125, 0.2724498212337493896484375, 0.16653059422969818115234375, -0.2935789525508880615234375, -0.22441709041595458984375, 0.18514315783977508544921875, -0.17840464413166046142578125, 0.20986096560955047607421875, 0.1435105502605438232421875, -0.0577326230704784393310546875, 0.421667039394378662109375, -0.231820642948150634765625, -0.49572479724884033203125));
      float _220 = 0.0f;
      float _221 = 1.0f;
      _62 += mul(_12(_220, _221), float4x4(-0.3483012616634368896484375, 0.1090667545795440673828125, -0.282858669757843017578125, -0.048280067741870880126953125, -0.122909180819988250732421875, 0.0429165102541446685791015625, -0.0474841855466365814208984375, -0.037025950849056243896484375, 0.230472624301910400390625, 0.093989737331867218017578125, 0.02246710844337940216064453125, 0.08271034061908721923828125, 0.30666649341583251953125, -0.540769994258880615234375, 0.0577718727290630340576171875, 0.231940925121307373046875));
      float _247 = 1.0f;
      float _248 = -1.0f;
      _62 += mul(_12(_247, _248), float4x4(-0.17731948196887969970703125, -0.3175927102565765380859375, 0.14527280628681182861328125, 0.093967862427234649658203125, -0.16433562338352203369140625, -0.0183365307748317718505859375, -0.22345604002475738525390625, -0.0416119284927845001220703125, -0.14827461540699005126953125, 0.18544113636016845703125, -0.1554412543773651123046875, -0.061790071427822113037109375, 0.1698997914791107177734375, -0.20985202491283416748046875, 0.163915336132049560546875, -0.0944726765155792236328125));
      float _274 = 1.0f;
      float _275 = 0.0f;
      _62 += mul(_12(_274, _275), float4x4(-0.0538788624107837677001953125, -0.210346162319183349609375, 0.023831523954868316650390625, 0.19772215187549591064453125, 0.3164721429347991943359375, 0.012653477489948272705078125, -0.191308438777923583984375, -0.0492821075022220611572265625, -0.21446131169795989990234375, 0.067189045250415802001953125, 0.091174490749835968017578125, -0.255487740039825439453125, 0.1210909783840179443359375, 0.22009392082691192626953125, -0.392466485500335693359375, -0.13340388238430023193359375));
      float _301 = 1.0f;
      float _302 = 1.0f;
      _62 += mul(_12(_301, _302), float4x4(-0.1609668433666229248046875, -0.18495404720306396484375, 0.10410177707672119140625, 0.001567303319461643695831298828125, -0.00183497997932136058807373046875, -0.044303037226200103759765625, -0.062745355069637298583984375, -0.090802393853664398193359375, 0.043269135057926177978515625, 0.069244809448719024658203125, -0.21367405354976654052734375, -0.14619028568267822265625, 0.115557633340358734130859375, -0.20292861759662628173828125, 0.57995569705963134765625, 0.14739845693111419677734375));
      float _328 = -1.0f;
      float _329 = -1.0f;
      _62 += mul(_16(_328, _329), float4x4(-0.210302770137786865234375, -0.09578801691532135009765625, 0.013482288457453250885009765625, -0.2148433625698089599609375, 0.12995781004428863525390625, 0.404310524463653564453125, -0.3347856104373931884765625, -0.18183486163616180419921875, 0.15550352632999420166015625, -0.0440230108797550201416015625, 0.4603779017925262451171875, 0.148743569850921630859375, -0.076946206390857696533203125, -0.0535230748355388641357421875, -0.1960732638835906982421875, -0.108507417142391204833984375));
      float _355 = -1.0f;
      float _356 = 0.0f;
      _62 += mul(_16(_355, _356), float4x4(-0.2347210943698883056640625, 0.2697403132915496826171875, -0.063479401171207427978515625, -0.1792598664760589599609375, 0.1723145544528961181640625, 0.24999184906482696533203125, -0.520853579044342041015625, -0.104918278753757476806640625, -0.23357500135898590087890625, 0.529503643512725830078125, 0.003806318156421184539794921875, -0.1380037963390350341796875, 0.02293519861996173858642578125, 0.19369156658649444580078125, 0.1458655297756195068359375, 0.1938703954219818115234375));
      float _382 = -1.0f;
      float _383 = 1.0f;
      _62 += mul(_16(_382, _383), float4x4(-0.10245223343372344970703125, 0.3415019214153289794921875, 0.25862157344818115234375, -0.201655089855194091796875, 0.559777081012725830078125, 0.1145108640193939208984375, -0.1225265562534332275390625, -0.0401097498834133148193359375, 0.17046789824962615966796875, -0.23335956037044525146484375, -0.16771887242794036865234375, -0.0378345511853694915771484375, -0.05699561536312103271484375, 0.24153493344783782958984375, -0.080824293196201324462890625, -0.2421093285083770751953125));
      float _409 = 0.0f;
      float _410 = -1.0f;
      _62 += mul(_16(_409, _410), float4x4(-0.10346652567386627197265625, 0.1527834832668304443359375, -0.3052616417407989501953125, -0.08075569570064544677734375, 0.103505425155162811279296875, 0.1586279571056365966796875, 0.1469652354717254638671875, -0.00835807621479034423828125, -0.091803111135959625244140625, -0.12505088746547698974609375, 0.2805254161357879638671875, -0.135515630245208740234375, 0.0752877891063690185546875, -0.096360862255096435546875, -0.103696167469024658203125, 0.23656134307384490966796875));
      float _436 = 0.0f;
      float _437 = 0.0f;
      _62 += mul(_16(_436, _437), float4x4(-0.257528364658355712890625, 0.09943975508213043212890625, -0.3071634769439697265625, 0.0350777246057987213134765625, 0.02350901626050472259521484375, 0.23106367886066436767578125, 0.0527712516486644744873046875, 0.3491046428680419921875, 0.088015384972095489501953125, 0.2699559628963470458984375, 0.13906450569629669189453125, -0.40671825408935546875, 0.180962979793548583984375, -0.100688554346561431884765625, 0.549204885959625244140625, 0.2482101023197174072265625));
      float _463 = 0.0f;
      float _464 = 1.0f;
      _62 += mul(_16(_463, _464), float4x4(0.414117753505706787109375, -0.107200555503368377685546875, -0.138134777545928955078125, 0.13768874108791351318359375, 0.2713774740695953369140625, 0.0631361901760101318359375, -0.085229672491550445556640625, 0.0321830213069915771484375, -0.0316612087190151214599609375, -0.341568291187286376953125, -0.522419989109039306640625, -0.17418129742145538330078125, -0.36956536769866943359375, 0.17912900447845458984375, -0.09742935001850128173828125, -0.1169661581516265869140625));
      float _490 = 1.0f;
      float _491 = -1.0f;
      _62 += mul(_16(_490, _491), float4x4(-0.0797550380229949951171875, 0.17964838445186614990234375, 0.3712253272533416748046875, 0.16064764559268951416015625, 0.14309953153133392333984375, 0.29473078250885009765625, 0.09263910353183746337890625, -0.22333665192127227783203125, 0.3461274802684783935546875, -0.3387472927570343017578125, 0.0077308523468673229217529296875, -0.07239449024200439453125, 0.185225188732147216796875, -0.21297298371791839599609375, 0.1149397790431976318359375, 0.1611781418323516845703125));
      float _517 = 1.0f;
      float _518 = 0.0f;
      _62 += mul(_16(_517, _518), float4x4(-0.17402778565883636474609375, 0.1002314388751983642578125, 0.117122061550617218017578125, 0.0319717340171337127685546875, 0.18713302910327911376953125, 0.087362952530384063720703125, 0.013007052242755889892578125, -0.069431386888027191162109375, -0.2010295093059539794921875, -0.010721134953200817108154296875, -0.2562521994113922119140625, 0.3487745821475982666015625, -0.13732676208019256591796875, -0.4025804698467254638671875, 0.25824391841888427734375, 0.1572063863277435302734375));
      float _544 = 1.0f;
      float _545 = 1.0f;
      _62 += mul(_16(_544, _545), float4x4(0.0444943048059940338134765625, 0.3296107947826385498046875, 0.00176038523204624652862548828125, 0.0936228930950164794921875, 0.38839244842529296875, 0.400158584117889404296875, -0.1339519917964935302734375, -0.04452185332775115966796875, -0.562663733959197998046875, 0.251377999782562255859375, 0.50057888031005859375, -0.1310605704784393310546875, -0.18491415679454803466796875, -0.0468869991600513458251953125, 0.06779767572879791259765625, -0.14694957435131072998046875));
      _62 += float4(0.01368753425776958465576171875f, -0.081851638853549957275390625f, -0.047554381191730499267578125f, 0.290178000926971435546875f);
      PSOut.Color = _62;
}
})";

extern const std::string kHLSL_Anime4K_Restore_CNN_M_Pass7_Pixel = R"(// Mode A Pass 10

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;
Texture2D u_Texture1;
SamplerState u_Texture1_sampler;
Texture2D u_Texture2;
SamplerState u_Texture2_sampler;
Texture2D u_Texture3;
SamplerState u_Texture3_sampler;
Texture2D u_Texture4;
SamplerState u_Texture4_sampler;
Texture2D u_Texture5;
SamplerState u_Texture5_sampler;
Texture2D u_Texture6;
SamplerState u_Texture6_sampler;
Texture2D u_Texture7;
SamplerState u_Texture7_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _36;

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _36 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float4 _9 = mul(max(u_Texture1.Sample(u_Texture1_sampler, uv), 0.0f.xxxx), float4x4(-0.088371627032756805419921875, -0.06523473560810089111328125, -0.03470431268215179443359375, 0.0, 0.02140550129115581512451171875, 0.013663728721439838409423828125, 0.01924959383904933929443359375, 0.0, 0.053288631141185760498046875, 0.035803340375423431396484375, 0.0464575923979282379150390625, 0.0, -0.12216047942638397216796875, 0.02254789136350154876708984375, 0.0164008252322673797607421875, 0.0));
      _9 += mul(max(-u_Texture1.Sample(u_Texture1_sampler, uv), 0.0f.xxxx), float4x4(0.0619964636862277984619140625, 0.0563146583735942840576171875, 0.068084068596363067626953125, 0.0, -0.005013109184801578521728515625, -0.0044589997269213199615478515625, -0.0323677957057952880859375, 0.0, 0.01648160256445407867431640625, 0.1372105777263641357421875, 0.1492464840412139892578125, 0.0, 0.02003588713705539703369140625, -0.072500027716159820556640625, -0.08034037053585052490234375, 0.0));
      _9 += mul(max(u_Texture2.Sample(u_Texture2_sampler, uv), 0.0f.xxxx), float4x4(0.24078513681888580322265625, 0.081361524760723114013671875, 0.05342070758342742919921875, 0.0, -0.009353794157505035400390625, -0.0510771162807941436767578125, -0.0580077469348907470703125, 0.0, -0.1407109797000885009765625, 0.01035965979099273681640625, 0.0053089489229023456573486328125, 0.0, -0.1489841938018798828125, -0.067118167877197265625, -0.055529259145259857177734375, 0.0));
      _9 += mul(max(-u_Texture2.Sample(u_Texture2_sampler, uv), 0.0f.xxxx), float4x4(-0.1300237476825714111328125, 0.012733756564557552337646484375, 0.0178219862282276153564453125, 0.0, 0.177674829959869384765625, 0.20204603672027587890625, 0.1751779019832611083984375, 0.0, 0.12804912030696868896484375, 0.073814533650875091552734375, 0.056559108197689056396484375, 0.0, 0.170445144176483154296875, 0.07301451265811920166015625, 0.065239779651165008544921875, 0.0));
      _9 += mul(max(u_Texture3.Sample(u_Texture3_sampler, uv), 0.0f.xxxx), float4x4(-0.1170985996723175048828125, -0.0513037107884883880615234375, -0.02793991379439830780029296875, 0.0, -0.16645707190036773681640625, -0.121526904404163360595703125, -0.0947136580944061279296875, 0.0, -0.041431181132793426513671875, 0.02669376693665981292724609375, 0.0346154458820819854736328125, 0.0, -0.084318704903125762939453125, -0.064990036189556121826171875, -0.054324172437191009521484375, 0.0));
      _9 += mul(max(-u_Texture3.Sample(u_Texture3_sampler, uv), 0.0f.xxxx), float4x4(0.120945237576961517333984375, 0.0951840877532958984375, 0.07387219369411468505859375, 0.0, 0.0622163824737071990966796875, 0.053228355944156646728515625, 0.0313723348081111907958984375, 0.0, 0.07279710471630096435546875, 0.02625816501677036285400390625, 0.009804672561585903167724609375, 0.0, 0.1207190454006195068359375, 0.07328115403652191162109375, 0.056623302400112152099609375, 0.0));
      _9 += mul(max(u_Texture4.Sample(u_Texture4_sampler, uv), 0.0f.xxxx), float4x4(-0.111414946615695953369140625, -0.11566288769245147705078125, -0.1039872467517852783203125, 0.0, -0.065189503133296966552734375, -0.06820690631866455078125, -0.054204143583774566650390625, 0.0, -0.0327464751899242401123046875, -0.008849683217704296112060546875, -0.0076102218590676784515380859375, 0.0, -0.02465570531785488128662109375, -0.0487788580358028411865234375, -0.0411447547376155853271484375, 0.0));
      _9 += mul(max(-u_Texture4.Sample(u_Texture4_sampler, uv), 0.0f.xxxx), float4x4(0.05809019505977630615234375, 0.07538767158985137939453125, 0.05972291529178619384765625, 0.0, 0.044788487255573272705078125, 0.0421274192631244659423828125, 0.027502588927745819091796875, 0.0, 0.0489286594092845916748046875, 0.015416751615703105926513671875, 0.008312418125569820404052734375, 0.0, -0.011864113621413707733154296875, -0.007475279271602630615234375, -0.0060824654065072536468505859375, 0.0));
      _9 += mul(max(u_Texture5.Sample(u_Texture5_sampler, uv), 0.0f.xxxx), float4x4(0.0434465520083904266357421875, 0.06197130680084228515625, 0.0575808584690093994140625, 0.0, -0.0637915432453155517578125, -0.0537582449615001678466796875, -0.0472042150795459747314453125, 0.0, 0.01630773581564426422119140625, 0.03423424065113067626953125, 0.030179083347320556640625, 0.0, 0.0414453446865081787109375, 0.0384377203881740570068359375, 0.033059112727642059326171875, 0.0));
      _9 += mul(max(-u_Texture5.Sample(u_Texture5_sampler, uv), 0.0f.xxxx), float4x4(-0.00380354397930204868316650390625, 0.0008906116127036511898040771484375, -0.0005958531401120126247406005859375, 0.0, 0.102071285247802734375, 0.114852242171764373779296875, 0.100072540342807769775390625, 0.0, -0.074306003749370574951171875, -0.08803550899028778076171875, -0.0797232091426849365234375, 0.0, -0.03070421516895294189453125, -0.021514274179935455322265625, -0.009049375541508197784423828125, 0.0));
      _9 += mul(max(u_Texture6.Sample(u_Texture6_sampler, uv), 0.0f.xxxx), float4x4(0.006605808623135089874267578125, 0.0011408007703721523284912109375, 0.001619900576770305633544921875, 0.0, -0.039164729416370391845703125, -0.0429292656481266021728515625, -0.0401841811835765838623046875, 0.0, -0.0315344594419002532958984375, -0.0394135080277919769287109375, -0.0347672365605831146240234375, 0.0, 0.113516055047512054443359375, 0.12577052414417266845703125, 0.11333562433719635009765625, 0.0));
      _9 += mul(max(-u_Texture6.Sample(u_Texture6_sampler, uv), 0.0f.xxxx), float4x4(0.026559479534626007080078125, 0.0419053025543689727783203125, 0.0386173687875270843505859375, 0.0, 0.0484714247286319732666015625, 0.049788586795330047607421875, 0.0504475347697734832763671875, 0.0, 0.120928131043910980224609375, 0.13564217090606689453125, 0.126132488250732421875, 0.0, -0.00235085375607013702392578125, 0.001282897428609430789947509765625, 0.002873095683753490447998046875, 0.0));
      _9 += mul(max(u_Texture7.Sample(u_Texture7_sampler, uv), 0.0f.xxxx), float4x4(0.008475848473608493804931640625, 0.008800082840025424957275390625, 0.008206044323742389678955078125, 0.0, -0.0561236031353473663330078125, -0.06610845029354095458984375, -0.0603207834064960479736328125, 0.0, -0.081793963909149169921875, -0.1016386449337005615234375, -0.09669901430606842041015625, 0.0, -0.04402355849742889404296875, -0.04177539050579071044921875, -0.0382964499294757843017578125, 0.0));
      _9 += mul(max(-u_Texture7.Sample(u_Texture7_sampler, uv), 0.0f.xxxx), float4x4(0.10676299035549163818359375, 0.11840951442718505859375, 0.106184780597686767578125, 0.0, -0.0588025189936161041259765625, -0.064883671700954437255859375, -0.064326949417591094970703125, 0.0, 0.019221924245357513427734375, 0.0176027975976467132568359375, 0.0174139775335788726806640625, 0.0, -0.075125277042388916015625, -0.080483615398406982421875, -0.066218294203281402587890625, 0.0));
      _9 += float4(-0.01047893427312374114990234375f, -0.00836478359997272491455078125f, -0.010246551595628261566162109375f, 0.0f);
      PSOut.Color = _9 + u_Texture.Sample(u_Texture_sampler, uv);
}
})";

extern const std::string kHLSL_Anime4K_Upscale_CNN_x2_M_Pass0_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _21;
static float4 _250;

float4 _12(float _10, float _11)
{
    return u_Texture.Sample(u_Texture_sampler, _21 + (float2(_10, _11) * u_InputPt));
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _21 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float4 _40 = mul(_12(-1.0f, -1.0f), float4x4(-0.010995803, 0.077095956, -0.043992598, 0.06048717, 0.1164834, -0.11689607, 0.072985925, -0.078805886, 0.01182932, 0.054985743, -0.09018186, 0.044907484, 0.0, 0.0, 0.0, 0.0));
      _40 += mul(_12(-1.0f, 0.0f), float4x4(0.1813623, -0.14752422, 0.025720436, -0.17639883, 0.15697388, 0.10445984, -0.1843076, 0.5264643, 0.047516696, -0.097305484, 0.09740847, -0.29619336, 0.0, 0.0, 0.0, 0.0));
      _40 += mul(_12(-1.0f, 1.0f), float4x4(-0.014534763, 0.09486465, 0.046173926, 0.039391946, 0.09609376, -0.060574662, 0.042200956, -0.3269777, 0.051006425, 0.059818447, 0.04366627, 0.17699827, 0.0, 0.0, 0.0, 0.0));
      _40 += mul(_12(0.0f, -1.0f), float4x4(0.04268535, -0.08152529, 0.10577459, -0.036936995, -0.051562306, 0.054872766, 0.09194519, 0.0025066638, -0.01073954, 0.00064474024, 0.10038221, 0.02131141, 0.0, 0.0, 0.0, 0.0));
      _40 += mul(_12(0.0f, 0.0f), float4x4(-0.51751363, -0.40028602, 0.3469574, 0.5933738, -0.91357684, -0.67692596, 0.57815677, 0.39809322, -0.16341521, -0.27169713, 0.12232366, 0.4318641, 0.0, 0.0, 0.0, 0.0));
      _40 += mul(_12(0.0f, 1.0f), float4x4(0.12601124, -0.06263236, -0.45907676, -0.41514075, 0.3330334, -0.1929565, -0.6333532, -0.6552794, -0.045809917, 0.046351526, -0.26173338, -0.30252662, 0.0, 0.0, 0.0, 0.0));
      _40 += mul(_12(1.0f, -1.0f), float4x4(0.0030332592, 0.012103107, 0.010537323, -0.02038607, 0.095558085, 0.097704545, 0.083433494, 0.026790185, 0.01943357, -0.061712462, -0.00015703632, -0.032268334, 0.0, 0.0, 0.0, 0.0));
      _40 += mul(_12(1.0f, 0.0f), float4x4(0.016870102, 0.5215812, -0.11525501, 0.027527615, -0.09045733, 0.61310345, -0.1575268, 0.1905386, 0.020172214, 0.3503187, -0.08209157, -0.051328037, 0.0, 0.0, 0.0, 0.0));
      _40 += mul(_12(1.0f, 1.0f), float4x4(0.005494087, -0.010656317, 0.07682753, -0.08116042, -0.03934524, 0.16589017, 0.101483546, -0.066603065, 0.03494657, -0.07885597, 0.074227594, 0.0016264897, 0.0, 0.0, 0.0, 0.0));
      _40 += float4(0.014463938, -0.0031906287, 0.007015422, -0.003888468);
      PSOut.Color = _40;
  }
}
)";
extern const std::string kHLSL_Anime4K_Upscale_CNN_x2_M_Pass1_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _25;
static float4 _558;

float4 _12(float _10, float _11)
{
    return max(u_Texture.Sample(u_Texture_sampler, _25 + (float2(_10, _11) * u_InputPt)), 0.0f.xxxx);
}
float4 _16(float _14, float _15)
{
    return max(-u_Texture.Sample(u_Texture_sampler, _25 + (float2(_14, _15) * u_InputPt)), 0.0f.xxxx);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _25 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float4 _62 = mul(_12(-1.0f, -1.0f), float4x4(-0.08532478, -0.14302494, -0.017921071, -0.0032664281, -0.09841952, 0.024187077, 0.10701477, 0.14110753, -0.05714981, -0.10897174, 0.073803626, 0.103992954, 0.07914382, 0.032193683, -0.18346278, -0.09723936));
      _62 += mul(_12(-1.0f, 0.0f), float4x4(-0.034482613, -0.10742312, -0.047286414, -0.08641124, -0.33896688, -0.036533825, -0.48337597, 0.034040943, -0.13598205, -0.080917805, 0.08540263, -0.012667689, -0.009171425, -0.120026454, -0.20536867, -0.032149274));
      _62 += mul(_12(-1.0f, 1.0f), float4x4(0.18687321, 0.066278316, 0.024327392, 0.08816582, -0.08017908, 0.09488853, 0.26018232, -0.101504356, 0.17487666, 0.31057635, 0.14785016, -0.09622089, -0.07537452, -0.13844088, -0.05810814, 0.09907489));
      _62 += mul(_12(0.0f, -1.0f), float4x4(-0.04183032, 0.15207712, 0.005002397, 0.32277516, -0.16169126, -0.119836345, -0.04068436, -0.096728764, 0.11943901, 0.1789597, -0.20412198, 0.19009817, 0.36630696, 0.06946421, -0.5254373, -0.11896399));
      _62 += mul(_12(0.0f, 0.0f), float4x4(-0.31916487, -0.98911583, 1.0728644, -0.39280394, 0.33458877, -0.17325239, -0.645045, -0.28524077, -0.14512783, 0.24996442, -0.09837877, 0.05468934, 0.31559715, -0.020504637, -0.026724018, 0.24507573));
      _62 += mul(_12(0.0f, 1.0f), float4x4(-0.23759829, -0.08530173, -0.16665787, -0.22463752, 0.109896734, 0.13446991, -0.049552456, -0.02385489, -0.01245375, 0.3833208, 0.05758832, 0.1528937, 0.0501858, -0.19651426, 0.0076587177, -0.03297025));
      _62 += mul(_12(1.0f, -1.0f), float4x4(0.14554465, -0.01826686, 0.10284085, -0.19152659, -0.017585073, -0.05511482, 0.06362406, 0.023924058, -0.0018977845, -0.103172876, 0.03287086, -0.20085956, 0.36062446, 0.10749464, -0.20984372, 0.018256644));
      _62 += mul(_12(1.0f, 0.0f), float4x4(-0.005534592, 0.3709197, -0.18287498, 0.1720451, 0.030155553, -0.023265475, 0.0058617783, -0.031765483, 0.037328955, -0.2730994, 0.35090837, -0.3269043, -0.028477207, 0.32756507, -0.15989502, 0.12158258));
      _62 += mul(_12(1.0f, 1.0f), float4x4(0.10873739, 0.19583772, 0.060394943, 0.09410379, -0.04739245, 0.026561242, 0.022990001, 0.1093272, -0.01071349, -0.022938967, -0.046423864, 0.2385325, -0.0319821, 0.046962265, 0.09081178, -0.11001857));
      _62 += mul(_16(-1.0f, -1.0f), float4x4(0.13012704, 0.112289295, 0.030790284, -0.050499484, 0.11784853, 0.08107028, -0.07556717, -0.15643, 0.015249331, 0.015299608, 0.07748125, 0.054485757, 0.044857923, 0.12161275, -0.048292994, -0.033995003));
      _62 += mul(_16(-1.0f, 0.0f), float4x4(0.12931514, 0.15114146, 0.070513315, 0.11246343, 0.4142387, 0.213479, -0.5439916, 0.07776645, 0.13109331, 0.2021147, 0.25932786, -0.22157331, 0.02377734, -0.014970623, -0.1943276, 0.18440372));
      _62 += mul(_16(-1.0f, 1.0f), float4x4(-0.22365458, -0.19829084, -0.06881161, -0.06468993, 0.17202774, 0.0048758537, -0.09235021, 0.18941896, 0.064125344, -0.09067088, 0.09748182, 0.13561936, -0.05876288, -0.0122420965, -0.054380875, -0.17743628));
      _62 += mul(_16(0.0f, -1.0f), float4x4(0.18582906, -0.09263032, -0.08210888, -0.20515606, 0.11484005, 0.08557595, 0.0009253741, -0.051202174, -0.18535301, -0.1529345, -0.13092944, 0.03770747, -0.020947013, 0.19187425, -0.15494856, -0.048979875));
      _62 += mul(_16(0.0f, 0.0f), float4x4(-0.38131633, 0.4278787, 0.19763695, 0.27655518, -0.08711912, 0.07374453, -0.064803004, 0.5983854, 0.2361923, -0.057221692, -0.37138999, -0.24259573, 0.13890724, 0.25706333, -0.54021406, 0.08095518));
      _62 += mul(_16(0.0f, 1.0f), float4x4(0.0991328, -0.022651536, -0.029148921, -0.009812537, -0.09523686, -0.15704902, 0.052389514, 0.21561539, 0.1950314, -0.08572602, 0.0016523858, 0.14125621, -0.030999828, 0.12009709, 0.0373512, -0.105043754));
      _62 += mul(_16(1.0f, -1.0f), float4x4(-0.11251988, 0.12106985, 0.011923068, 0.3662747, 0.004800994, 0.017972551, 0.004761366, -0.07934206, -0.13755941, -0.022852683, 0.1502225, 0.009758547, -0.16964264, 0.00984782, 0.07855833, 0.035730787));
      _62 += mul(_16(1.0f, 0.0f), float4x4(0.01964957, -0.27226487, 0.033933397, -0.117632054, -0.009058229, 0.047830686, -0.01125145, 0.136628, 0.0056388285, 0.3028781, -0.12286517, 0.23498532, -0.009319075, -0.444048, 0.16174883, -0.06367683));
      _62 += mul(_16(1.0f, 1.0f), float4x4(0.02343933, -0.010915871, -0.058680378, -0.21886891, -0.010750894, -0.06671997, 0.0602906, -0.07903071, 0.066891186, 0.06650588, 0.14362891, -0.101870626, 0.02264628, -0.06940821, -0.077616625, 0.110911585));
      _62 += float4(0.032014452, -0.020821465, 0.0826416, -0.002838458);
      PSOut.Color = _62;
  }
}
)";
extern const std::string kHLSL_Anime4K_Upscale_CNN_x2_M_Pass2_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _25;
static float4 _558;

float4 _12(float _10, float _11)
{
    return max(u_Texture.Sample(u_Texture_sampler, _25 + (float2(_10, _11) * u_InputPt)), 0.0f.xxxx);
}
float4 _16(float _14, float _15)
{
    return max(-u_Texture.Sample(u_Texture_sampler, _25 + (float2(_14, _15) * u_InputPt)), 0.0f.xxxx);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _25 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float4 _62 = mul(_12(-1.0f, -1.0f), float4x4(-0.06963679, -0.07560548, -0.069522075, 0.0038078027, -0.08002613, 0.13671301, 0.084461786, -0.039376218, 0.19136548, -0.123174496, 0.26566333, -0.16583005, -0.18664864, -0.023539122, -0.21928434, -0.026818147));
      _62 += mul(_12(-1.0f, 0.0f), float4x4(0.16660932, -0.18558703, 0.37230486, 0.118128106, -0.14098641, 0.14659132, -0.22217897, 0.12952235, -0.4139033, -0.04308319, 0.12885277, -0.17986743, -0.23556231, -0.08351981, -0.43240538, 0.019033253));
      _62 += mul(_12(-1.0f, 1.0f), float4x4(-0.18008037, -0.04448665, 0.011906908, -0.023056917, 0.18136618, -0.04723555, -0.0050158803, -0.14823224, -0.2105281, 0.023047728, -0.14040631, -0.03178526, -0.13477588, -0.01820428, 0.058358394, 0.23792502));
      _62 += mul(_12(0.0f, -1.0f), float4x4(0.07363309, -0.061728477, 0.03573137, -0.0050971056, -0.012813505, -0.17236637, 0.1697835, 0.055788577, -0.22263195, 0.10324512, 0.58971673, -0.4872246, -0.1555681, 0.032747746, -0.096495196, 0.070196226));
      _62 += mul(_12(0.0f, 0.0f), float4x4(0.14174286, 0.099460006, -0.088765986, 0.58350676, -0.025177564, -0.46004987, 0.37007022, -0.11437029, -0.5164534, -0.60465246, 0.38859612, -0.32846406, 0.050266482, -0.20334712, 0.18316261, -0.19327633));
      _62 += mul(_12(0.0f, 1.0f), float4x4(-0.09377763, -0.0012762006, -0.028991895, -0.26523829, 0.20173682, 0.037923716, -0.03174243, 0.07103378, -0.10764164, -0.30752546, 0.20556998, -0.1892279, 0.08115748, -0.023550175, -0.07627362, 0.11746628));
      _62 += mul(_12(1.0f, -1.0f), float4x4(-0.06998859, -0.017997518, 0.069938794, -0.14943017, -0.14179112, 0.16643842, -0.110231474, 0.08895815, -0.24074875, 0.3277253, -0.07435203, -0.23452802, 0.039962552, -0.07145652, -0.022511544, -0.04571222));
      _62 += mul(_12(1.0f, 0.0f), float4x4(-0.059785757, -0.23771374, -0.030571314, 0.25222278, 0.106601834, 0.34398326, 0.14511436, -0.03867526, -0.38982397, -0.11944689, 0.12997924, -0.13079585, 0.005729482, 0.012653905, -0.063693404, 0.09632285));
      _62 += mul(_12(1.0f, 1.0f), float4x4(-0.04933823, 0.0547175, 0.050636575, -0.10060694, 0.1344485, 0.19752938, -0.100068115, -0.028829506, -0.14096203, -0.079092234, 0.092109434, 0.011606209, -0.04052607, -0.008347507, 0.06956573, -0.028109524));
      _62 += mul(_16(-1.0f, -1.0f), float4x4(0.21918017, -0.11115073, 0.2262453, -0.06889667, -0.11256312, -0.07438075, -0.088454485, 0.13672407, -0.06905764, 0.08128395, 0.016103368, 0.050190717, 0.09691516, 0.05845721, 0.4886816, 0.041121427));
      _62 += mul(_16(-1.0f, 0.0f), float4x4(-0.3449472, 0.09711974, -0.13881907, -0.018265123, 0.27855873, -0.07030004, 0.29545054, 0.37216932, 0.08657718, 0.099066615, -0.10574013, -0.17667885, -0.14855732, -0.11351448, 0.66945946, 0.11312157));
      _62 += mul(_16(-1.0f, 1.0f), float4x4(0.2526151, -0.04594331, -0.06606611, 0.09104881, 0.06857995, -0.075284235, -0.17664689, 0.21578754, 0.0696524, 0.09142951, 0.080997564, -0.0682772, -0.0011445724, -0.11736295, 0.2519232, -0.101926275));
      _62 += mul(_16(0.0f, -1.0f), float4x4(-0.12913518, 0.058357026, 0.195421, -0.15651494, 0.2877076, 0.0033844314, -0.07831594, 0.052855384, -0.031295884, 0.03301088, -0.18408822, 0.06732994, 0.23742151, -0.12568143, 0.22810535, -0.11545694));
      _62 += mul(_16(0.0f, 0.0f), float4x4(-0.49203303, -0.22656603, 0.1723193, -0.51250046, -0.09742038, 0.758559, -0.3387505, -0.6193586, 0.14136684, 0.27679884, -0.050113205, 0.31041816, -0.36475047, -0.48746544, 0.3233227, 0.4579754));
      _62 += mul(_16(0.0f, 1.0f), float4x4(0.46636763, 0.1507748, -0.2581362, 0.15413165, -0.17160143, 0.14256273, -0.074575804, -0.099299066, -0.0017214464, 0.13778336, -0.07378213, -0.15489665, -0.10533715, -0.0011083825, 0.39584312, 0.0023906573));
      _62 += mul(_16(1.0f, -1.0f), float4x4(0.026959421, -0.06391859, 0.0034752619, 0.14521928, -0.0010877338, -0.032619733, 0.005375293, -0.018952755, 0.03381545, -0.007652831, 0.034141563, 0.046016496, 0.11219674, 0.030913852, 0.077403754, 0.17192438));
      _62 += mul(_16(1.0f, 0.0f), float4x4(0.040326044, 0.17290725, -0.1220239, -0.09594783, -0.025229257, 0.17913155, -0.26623353, -0.033396784, -0.03075146, 0.009143897, -0.0136083895, -0.13886899, 0.075683735, -0.11584183, 0.22182357, 0.19350322));
      _62 += mul(_16(1.0f, 1.0f), float4x4(0.15726025, -0.10215694, -0.060057458, 0.26487043, -0.04075552, -0.016496127, 0.0015382086, 0.108562306, 0.026795091, 0.0441233, -0.08754318, -0.0460157, 0.048422016, 0.14107347, 0.07986661, 0.1047697));
      _62 += float4(0.0766796, 0.08115133, -0.05703058, 0.14025708);
      PSOut.Color = _62;
  }
}
)";
extern const std::string kHLSL_Anime4K_Upscale_CNN_x2_M_Pass3_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _25;
static float4 _558;

float4 _12(float _10, float _11)
{
    return max(u_Texture.Sample(u_Texture_sampler, _25 + (float2(_10, _11) * u_InputPt)), 0.0f.xxxx);
}
float4 _16(float _14, float _15)
{
    return max(-u_Texture.Sample(u_Texture_sampler, _25 + (float2(_14, _15) * u_InputPt)), 0.0f.xxxx);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _25 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float4 _62 = mul(_12(-1.0f, -1.0f), float4x4(-0.18038331, 0.21830973, -0.10019419, -0.022745568, -0.14944611, -0.15669158, 0.46361133, -0.07289843, 0.02976627, -0.09000817, 0.113060996, 0.05635241, 0.012762965, -0.022688959, 0.01629751, 0.061114635));
      _62 += mul(_12(-1.0f, 0.0f), float4x4(0.024338024, -0.10004009, -0.13709056, -0.0851965, 0.23927099, -0.024349794, -0.16574804, 0.084686354, -0.047885604, 0.09688507, -0.12733915, 0.06980246, 0.11480734, 0.014669346, -0.07505829, 0.04676309));
      _62 += mul(_12(-1.0f, 1.0f), float4x4(0.054203495, 0.011881634, -0.036115017, -0.0686298, -0.13682245, -0.15678032, 0.057050128, -0.03368558, 0.13011025, 0.033391044, -0.09841339, -0.027057761, -0.18701133, 0.20852546, -0.13660902, 0.0005817616));
      _62 += mul(_12(0.0f, -1.0f), float4x4(-0.08077834, 0.35952288, -0.07647382, -0.0033230998, 0.13929126, -0.09155619, 0.14128102, 0.16005981, 0.18161216, -0.09485738, 0.0029118075, 0.052682754, 0.03242074, 0.08299826, 0.073796146, -0.06446532));
      _62 += mul(_12(0.0f, 0.0f), float4x4(-0.36655015, 0.4606936, 0.19073649, 0.31655258, -0.006838053, -0.579939, 0.089126326, -0.14021218, -0.3437716, 0.16714323, 0.17705944, -0.22418492, -0.3883696, -0.2302651, 0.2581861, 0.21983066));
      _62 += mul(_12(0.0f, 1.0f), float4x4(0.0992383, -0.014257871, -0.023896435, 0.19868234, 0.0408007, 0.07995299, 0.16102871, -0.11668251, 0.22458278, -0.05587917, 0.19373615, -0.016202094, -0.25106144, 0.15634494, 0.11624891, -0.2930768));
      _62 += mul(_12(1.0f, -1.0f), float4x4(0.024616942, 0.36248252, -0.14779098, -0.019894283, -0.007111256, 0.010641561, -0.09541178, 0.21236233, 0.009501827, 0.08132797, -0.13983901, 0.027207611, 0.038444366, -0.013995817, -0.16242191, 0.03294123));
      _62 += mul(_12(1.0f, 0.0f), float4x4(0.0131698875, -0.18124102, -0.13503514, -0.06099072, 0.07422735, -0.20906176, -0.049005672, 0.08739405, -0.031758767, -0.1978915, 0.23094437, 0.54512614, 0.21338555, -0.011205669, -0.23727885, -0.29533875));
      _62 += mul(_12(1.0f, 1.0f), float4x4(-0.0010255767, -0.07168225, -0.033568826, 0.22161655, -0.087293416, 0.11350447, 0.13653576, 0.061226424, -0.13074352, 0.058425818, 0.038460605, 0.2749964, -0.012814839, 0.085885845, -0.038151987, -0.17960808));
      _62 += mul(_16(-1.0f, -1.0f), float4x4(0.19728905, -0.040724937, -0.18270236, 0.046735186, 0.03507326, 0.119867206, -0.12691991, 0.18119748, -0.052895024, 0.11348764, -0.043787055, 0.004703516, 0.006752757, -0.06939761, -0.009801806, -0.075640485));
      _62 += mul(_16(-1.0f, 0.0f), float4x4(0.051735226, 0.1732299, -0.10672899, 0.0320877, -0.4913656, 0.2102274, 0.43920282, 0.059108034, 0.08349019, -0.16517872, 0.15436842, -0.1075667, 0.022741623, -0.26693836, 0.3645307, 0.017874828));
      _62 += mul(_16(-1.0f, 1.0f), float4x4(0.034464058, 0.014929155, 0.054227423, 0.14167373, -0.0023630706, -0.08904212, 0.11918041, -0.034539603, 0.06048089, -0.06807333, 0.14447778, 0.035260547, 0.09979546, -0.1924939, 0.14596114, -0.12069667));
      _62 += mul(_16(0.0f, -1.0f), float4x4(-0.04427228, -0.23673469, 0.010357103, -0.2907043, -0.06845721, -0.078984015, 0.06867713, -0.058163825, -0.12154615, 0.08430951, 0.1922373, 0.030108064, -0.43081748, -0.38715646, -0.022240646, -0.15403675));
      _62 += mul(_16(0.0f, 0.0f), float4x4(0.46885306, -0.33421394, -0.6695223, -0.41841158, 0.30317923, 0.24244753, -0.1047785, -0.18656285, 0.06261881, -0.4405616, 0.24233986, 0.40070608, 0.81440526, 0.11305212, -0.8826317, -0.023478031));
      _62 += mul(_16(0.0f, 1.0f), float4x4(-0.07879348, -0.024378026, -0.041883785, -0.17030984, 0.23229122, -0.011237109, 0.12058088, 0.20766267, -0.36519575, 0.09599417, -0.1271098, 0.06990154, 0.21161246, 0.041002538, -0.36046275, 0.007304667));
      _62 += mul(_16(1.0f, -1.0f), float4x4(0.10873893, 0.003872542, -0.13476561, -0.036068805, -0.054637462, 0.02304618, 0.04707738, -0.2856381, 0.07124422, 0.010866545, 0.20484549, -0.008342406, -0.43660247, -0.041055538, 0.33536008, -0.060022205));
      _62 += mul(_16(1.0f, 0.0f), float4x4(0.1966458, 0.0016302796, -0.25712642, -0.09639119, -0.006955351, 0.10882133, 0.1107341, 0.062697805, -0.1074494, 0.17361663, 0.6429869, -0.39846307, -0.26302996, 0.048710946, 0.40387508, 0.4299715));
      _62 += mul(_16(1.0f, 1.0f), float4x4(0.18948616, 0.24086732, -0.064474985, -0.11069709, 0.1279659, -0.13438123, -0.028438117, 0.125883, 0.018153818, -0.21942288, 0.020390838, -0.22797634, -0.10821287, -0.17175092, 0.122016855, 0.20699544));
      _62 += float4(-0.05101961, -0.060740646, -0.024465766, 0.058471628);
      PSOut.Color = _62;
  }
}
)";
extern const std::string kHLSL_Anime4K_Upscale_CNN_x2_M_Pass4_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _25;
static float4 _558;

float4 _12(float _10, float _11)
{
    return max(u_Texture.Sample(u_Texture_sampler, _25 + (float2(_10, _11) * u_InputPt)), 0.0f.xxxx);
}
float4 _16(float _14, float _15)
{
    return max(-u_Texture.Sample(u_Texture_sampler, _25 + (float2(_14, _15) * u_InputPt)), 0.0f.xxxx);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _25 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float4 _62 = mul(_12(-1.0f, -1.0f), float4x4(-0.14533128, 0.07266841, 0.13238011, -0.23328504, 0.031516243, 0.058471266, -0.06394412, 0.090752736, -0.0042359144, 0.12357294, -0.04377495, 0.0011743477, 0.05412243, -0.08146249, 0.04002749, -0.032876283));
      _62 += mul(_12(-1.0f, 0.0f), float4x4(-0.036972385, -0.15238069, -0.3453321, -0.36025128, 0.07597202, -0.02368151, -0.3889606, 0.34607083, 0.3133179, -0.21712309, -0.4210954, 0.21450534, 0.15226828, 0.25326282, 0.45327064, -0.3350824));
      _62 += mul(_12(-1.0f, 1.0f), float4x4(0.019018406, -0.33060563, -0.092601225, 0.14970545, 0.1441509, -0.19228427, -0.032771986, 0.26331595, 0.052981265, -0.06627376, -0.08634131, 0.038706224, 0.13403937, -4.4842476e-05, 0.049002815, -0.12719193));
      _62 += mul(_12(0.0f, -1.0f), float4x4(0.17527401, -0.0035254909, -0.047959115, -0.4526988, -0.07510284, 0.0013256798, -0.07539148, 0.24220634, -0.08708839, -0.14494033, -0.17085724, -0.099797316, 0.0068515535, -0.08918779, 0.27164719, -0.1702649));
      _62 += mul(_12(0.0f, 0.0f), float4x4(0.31848368, 0.48983255, -0.44140294, -0.65174145, -0.004199057, 0.19494705, 0.5196497, -0.027118586, 0.032509074, -0.23900363, -0.14489244, 0.36314297, -0.23168536, -0.20960593, 0.61471456, 0.12401275));
      _62 += mul(_12(0.0f, 1.0f), float4x4(-0.24317405, 0.21560913, 0.15564032, 0.11606844, -0.15039803, -0.59578896, 0.14100945, -0.026194477, 0.37237462, -0.49472088, -0.15215331, -0.38820064, -0.25089455, -0.29643852, -0.09513793, 0.019779462));
      _62 += mul(_12(1.0f, -1.0f), float4x4(0.12498539, 0.0710632, -0.25012368, -0.2272255, -0.08647026, 0.12277892, 0.011025097, -0.12168395, -0.13489573, 0.016708186, -0.15583871, -0.057124946, 0.1216943, 0.019803725, 0.06952334, -0.032985855));
      _62 += mul(_12(1.0f, 0.0f), float4x4(0.28794885, 0.33783793, -0.14469545, -0.081780486, -0.50320613, -0.067601606, -0.06847453, -0.021648854, -0.34295765, 0.15071863, -0.06619896, -0.084465064, 0.31909832, 0.015414661, 0.14930317, -0.11295768));
      _62 += mul(_12(1.0f, 1.0f), float4x4(0.24530606, 0.25526014, 0.09971985, -0.07749641, -0.2361951, -0.07997673, 0.03617294, 0.02959561, -0.4498983, -0.014073485, -0.20587012, 0.06396779, 0.1262825, 0.027433183, 0.14469334, 0.011538011));
      _62 += mul(_16(-1.0f, -1.0f), float4x4(-0.038572453, -0.023108613, -0.039481267, -0.012160024, -0.004521989, -0.028665857, 0.04295255, 0.10580258, 0.05439479, -0.072261885, 0.11030243, 0.08934696, 0.09133867, 0.017547369, 0.097613186, 0.05491059));
      _62 += mul(_16(-1.0f, 0.0f), float4x4(-0.09972817, 0.057730395, 0.12665828, 0.32861367, -0.16186063, 0.0745509, 0.2394045, -0.08687853, -0.034404907, -0.05843572, 0.0684561, -0.1355754, 0.19248672, -0.60372186, 0.12583947, 0.4388962));
      _62 += mul(_16(-1.0f, 1.0f), float4x4(0.10341107, 0.061113223, 0.08773817, -0.082504354, -0.16612078, 0.2681751, 0.019737698, -0.17122322, -0.135949, 0.3048101, 0.087803006, 0.11373851, 0.013192192, -0.27022064, 0.35529897, -0.15321451));
      _62 += mul(_16(0.0f, -1.0f), float4x4(-0.032835662, 0.11123062, -0.11322452, -0.17300649, 0.04680824, 0.12849288, 0.17269878, -0.048671383, 0.05189037, -0.009078046, 0.22105052, 0.013008137, -0.009738674, 0.15391739, 0.20969556, 0.14189166));
      _62 += mul(_16(0.0f, 0.0f), float4x4(-0.47377753, 0.3038031, 0.18604809, 0.1931698, -0.2964668, -0.12287907, -0.7107761, 0.26619422, -0.33923018, 0.19200724, 0.013786281, -0.17496964, 0.079325035, -0.3694445, 0.0054486147, -0.33018264));
      _62 += mul(_16(0.0f, 1.0f), float4x4(0.14903802, -0.028043179, 1.5238678e-05, 0.021232028, 0.16025065, 0.14746875, -0.22831628, -0.12177345, 0.038778774, 0.32188168, -0.042017702, 0.27155936, 0.17920609, 0.04099755, 0.28527525, 0.074623376));
      _62 += mul(_16(1.0f, -1.0f), float4x4(0.057019282, -0.112741895, 0.030361209, 0.14567861, 0.056265317, -0.01573537, -0.06707608, 0.016657263, 0.09829025, -0.026795063, 0.023042196, 0.09438241, -0.025483066, -0.052787006, 0.19730279, 0.021218104));
      _62 += mul(_16(1.0f, 0.0f), float4x4(0.19868211, -0.01531125, 0.108596824, -0.035456363, 0.0033609823, 0.057961613, -0.013726211, 0.101742364, 0.33357215, 0.14468077, 0.29711527, -0.24662566, -0.119014986, -0.1899639, 0.11246697, -0.0035374009));
      _62 += mul(_16(1.0f, 1.0f), float4x4(-0.05602109, -0.15539522, 0.010730943, 0.057116497, -0.02037749, 0.084210664, -0.028235348, 0.10574697, 0.056925274, 0.07922333, -0.090088, 0.1615985, -0.0044301567, -0.089945644, 0.024176618, -0.041844133));
      _62 += float4(0.0015292584, -0.043625206, -0.09429898, -0.06280405);
      PSOut.Color = _62;
  }
}
)";
extern const std::string kHLSL_Anime4K_Upscale_CNN_x2_M_Pass5_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _25;
static float4 _558;

float4 _12(float _10, float _11)
{
    return max(u_Texture.Sample(u_Texture_sampler, _25 + (float2(_10, _11) * u_InputPt)), 0.0f.xxxx);
}
float4 _16(float _14, float _15)
{
    return max(-u_Texture.Sample(u_Texture_sampler, _25 + (float2(_14, _15) * u_InputPt)), 0.0f.xxxx);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _25 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float4 _62 = mul(_12(-1.0f, -1.0f), float4x4(0.06051604, -0.028152643, -0.21418124, 0.13032125, 0.42565975, -0.09571944, -0.34494513, 0.30004, -0.073245734, -0.028659137, 0.0032105136, -0.05009555, -0.048971225, 0.04814533, 0.002843805, -0.046224426));
      _62 += mul(_12(-1.0f, 0.0f), float4x4(-0.07495975, 0.018714864, 0.21229684, -0.13614887, 0.79988647, -0.0697328, 0.38232988, 0.24165109, 0.25947478, -0.0009418982, -0.17369923, 0.10007766, 0.024117598, 0.028611807, 0.15090801, -0.06344829));
      _62 += mul(_12(-1.0f, 1.0f), float4x4(-0.07982219, 0.0900347, 0.007609254, -0.0034791247, 0.013611781, -0.13560618, 0.09685799, 0.06276075, 0.134693, -0.14370437, -0.25175703, -0.0016138123, -0.0075672898, -0.13325731, -0.061100446, 0.0059743375));
      _62 += mul(_12(0.0f, -1.0f), float4x4(-0.039018434, -0.19668463, -0.43018532, 0.31886247, 0.4965479, 0.114569925, 0.19110382, 0.27343535, 0.0707728, -0.11877004, -0.25827697, 0.37012872, 0.1474777, 0.07056952, -0.14965728, 0.061595406));
      _62 += mul(_12(0.0f, 0.0f), float4x4(0.506543, -0.16268773, 0.455319, -0.0702646, 0.70102173, -0.14041683, 0.70184857, 0.4817842, -0.3389246, -0.14463086, 0.13763213, -1.1259074, 0.47722015, 0.38352612, -0.04293366, -0.5604627));
      _62 += mul(_12(0.0f, 1.0f), float4x4(0.17606944, 0.15897374, 0.13499324, 0.29241478, -0.032824475, 0.11128662, -0.22204424, -0.051803727, 0.013195331, -0.42040786, -0.3950585, 0.70745844, 0.38646924, -0.19080774, -0.15171832, -0.10742828));
      _62 += mul(_12(1.0f, -1.0f), float4x4(-0.039278325, 0.18421806, -0.044948544, 0.07902063, -0.2149251, 0.09913459, -0.09743655, -0.26899317, -0.002695496, -0.07554527, -0.22373366, 0.17830558, -0.047994815, -0.06789183, -0.06755918, -0.104452066));
      _62 += mul(_12(1.0f, 0.0f), float4x4(-0.0493473, -0.30411786, -0.056439694, -0.06582185, -0.21309847, 0.100670904, -0.22966193, -0.045954112, 0.12728062, -0.25081897, -0.094699375, -0.4036555, 0.060854495, -0.64373237, -0.21522263, -0.6683476));
      _62 += mul(_12(1.0f, 1.0f), float4x4(0.063481025, 0.11744312, -0.043330096, 0.33817932, -0.06679828, -0.23207302, -0.10188898, -0.10590511, 0.058780864, 0.047292337, -0.11834696, 0.10076128, -0.036641665, 0.30200714, -0.0002892557, -0.10303763));
      _62 += mul(_16(-1.0f, -1.0f), float4x4(-0.10842604, 0.042055763, 0.29702973, -0.07409644, -0.030164458, -0.012098744, -0.06396587, -0.08787527, 0.051854923, 0.12997511, 0.11468497, 0.15022379, 0.007814715, 0.014517445, 0.025484756, 0.01078619));
      _62 += mul(_16(-1.0f, 0.0f), float4x4(-0.29229385, 0.040265664, -0.15376821, 0.075579196, -0.05593569, -0.045405343, 0.12099204, 0.1571252, 0.17841713, 0.04673325, 0.14550509, 0.08603346, -0.049786013, 0.06121843, -0.16273825, -0.13857752));
      _62 += mul(_16(-1.0f, 1.0f), float4x4(0.06903744, 0.2628764, -0.13582836, -0.35678583, -0.13821034, -0.019381443, -0.19570538, -0.09298511, 0.08965436, 0.09745909, 0.20055099, 0.024967568, 0.08144204, 0.004633625, 0.12809834, -0.009431525));
      _62 += mul(_16(0.0f, -1.0f), float4x4(0.09784006, 0.010729353, 0.046643205, -0.110926524, -0.21556224, 0.00016300633, 0.122175336, 0.15004392, 0.013864355, 0.24767809, 0.13865592, 0.0155424485, -0.1450483, -0.15688781, -0.06195043, -0.13745981));
      _62 += mul(_16(0.0f, 0.0f), float4x4(0.018991318, 0.55401963, 0.11709872, -0.028442185, -0.46035343, -0.10215539, -0.60193926, 0.47882316, -0.23346989, 0.037200127, 0.22814943, -0.08231696, -0.36430013, -0.011152757, 0.48752213, 0.29796222));
      _62 += mul(_16(0.0f, 1.0f), float4x4(-0.07258066, -0.023222538, 0.23230423, -0.30317304, 0.03942911, -0.06899803, 0.23778579, 0.07418621, -0.17443737, 0.33387753, 0.007354842, -0.123447575, -0.1745315, 0.11071779, -0.11949625, -0.22832453));
      _62 += mul(_16(1.0f, -1.0f), float4x4(-0.024909232, -0.0308135, 0.12170621, -0.13298757, 0.045828197, -0.1532345, -0.06633672, 0.23591088, 0.04964077, 0.14091493, 0.038343724, -0.029780807, 0.05762822, -0.048930667, -0.02434709, 0.07109019));
      _62 += mul(_16(1.0f, 0.0f), float4x4(-0.16039175, 0.3004474, -0.17278233, 0.13677922, 0.18838613, 0.15054552, 0.32901475, -0.1288333, 0.26378244, -0.05119892, 0.34533516, 0.25180495, 0.19452183, 0.0843233, -0.08029368, 0.39877903));
      _62 += mul(_16(1.0f, 1.0f), float4x4(-0.07097129, -0.26492423, -0.055032317, -0.093516104, -0.11795062, 0.04086253, -0.07989471, 0.059686553, 0.09378249, 0.45851848, 0.2510942, 0.19599153, 0.019765077, -0.02920918, -0.04125142, -0.13859107));
      _62 += float4(0.04400571, -0.04015565, 0.0140529545, 0.05474095);
      PSOut.Color = _62;
  }
}
)";
extern const std::string kHLSL_Anime4K_Upscale_CNN_x2_M_Pass6_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _25;
static float4 _558;

float4 _12(float _10, float _11)
{
    return max(u_Texture.Sample(u_Texture_sampler, _25 + (float2(_10, _11) * u_InputPt)), 0.0f.xxxx);
}
float4 _16(float _14, float _15)
{
    return max(-u_Texture.Sample(u_Texture_sampler, _25 + (float2(_14, _15) * u_InputPt)), 0.0f.xxxx);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _25 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float4 _62 = mul(_12(-1.0f, -1.0f), float4x4(-0.014236042, -0.0031431736, -0.1551387, 0.12515116, -0.28528872, 0.36161992, 0.15750743, -0.17111474, 0.13792591, -0.0657419, -0.17471549, 0.14650472, 0.034169197, -0.019157575, 0.23520657, -0.20358163));
      _62 += mul(_12(-1.0f, 0.0f), float4x4(0.02015035, 0.12993371, 0.11199667, -0.09854378, 0.5001741, 0.03462961, 0.24919736, 0.08505297, -0.20902094, -0.24141377, -0.15360375, 0.049974803, -0.037157424, -0.048510186, 0.20106035, -0.118480384));
      _62 += mul(_12(-1.0f, 1.0f), float4x4(0.086798504, -0.009607818, 0.034812123, -0.005187592, 0.0351509, 0.021755, -0.04996161, -0.041231696, 0.0020545553, 0.015730752, -0.07507172, 0.018597523, -0.02393343, 0.07624775, 0.03892451, -0.0025574185));
      _62 += mul(_12(0.0f, -1.0f), float4x4(0.035725456, 0.06809103, 0.51926994, -0.39983147, -0.16402833, -0.1243394, -0.25922915, 0.28285915, 0.15959994, -0.2351732, 0.2650535, -0.30193794, -0.11468332, 0.050777763, -0.51894253, 0.4408367));
      _62 += mul(_12(0.0f, 0.0f), float4x4(-0.27042082, 0.22243942, 0.14902467, 0.38428563, 0.46612173, 0.5169912, -0.22330502, -0.11300288, -0.36141354, 0.0668681, 0.2984152, 0.1275798, -0.24121419, 0.2952039, -0.45109174, -0.3822957));
      _62 += mul(_12(0.0f, 1.0f), float4x4(0.26543504, -0.05742226, -0.052103903, -0.013124308, -0.14358385, -0.04024543, 0.07665455, -0.012301872, -0.18752757, -0.03913891, 0.038205814, -0.006583095, -0.25550908, -0.25725332, -0.12454206, -0.0058936924));
      _62 += mul(_12(1.0f, -1.0f), float4x4(-0.0018946569, 0.019746022, -0.13080788, 0.11450627, -0.013743845, -0.027179785, -0.14425103, 0.07109661, 0.023703793, 0.086905524, 0.03151253, 0.0132474145, 0.041018624, 0.04548913, 0.2718715, -0.20008296));
      _62 += mul(_12(1.0f, 0.0f), float4x4(-0.076830454, 0.11652955, 0.5068201, -0.3082819, 0.058615055, -0.006765798, -0.057522714, 0.049981344, -0.006897243, -0.21763432, 0.16896053, -0.21176189, -0.061227098, 0.03566485, 0.08901554, -0.050980624));
      _62 += mul(_12(1.0f, 1.0f), float4x4(0.02327798, 0.07662976, 0.034811985, -0.03238033, -0.0021881019, -0.030997375, -0.069672935, 0.04040273, -0.1217442, 0.104173124, 0.09862539, 0.020557549, -0.022286594, 0.10287763, -0.021694934, 0.07542515));
      _62 += mul(_16(-1.0f, -1.0f), float4x4(0.124069154, -0.08579466, -0.07816314, 0.11332851, -0.034682628, -0.11038275, 0.04750615, -0.096100725, 0.039588403, -0.15149672, -0.05529172, 0.034304325, -0.022520235, -0.05023852, -0.2674731, 0.21886522));
      _62 += mul(_16(-1.0f, 0.0f), float4x4(-0.1948599, -0.14946899, -0.39548838, 0.18042913, -0.007919619, 0.19826505, 0.23789087, 0.009140256, 0.11857748, 0.18215668, 0.13606293, -0.09209675, -0.080678545, -0.020431137, -0.07728839, -0.051353537));
      _62 += mul(_16(-1.0f, 1.0f), float4x4(-0.07616472, -0.0032800382, -0.045657665, -0.039144326, -0.37786487, -0.08877774, 0.053579114, -0.070886396, 0.011311804, 0.107276045, 0.013236154, 0.009832061, 0.08292063, 0.12258811, 0.0005569043, -0.009806432));
      _62 += mul(_16(0.0f, -1.0f), float4x4(-0.28062925, 0.15946878, -0.1021801, -0.06471589, -0.26999477, 0.21230288, -0.14243907, 0.2555922, -0.09608517, 0.26339412, 0.20891234, -0.23538485, 0.33958244, -0.12569186, 0.43289876, -0.33462036));
      _62 += mul(_16(0.0f, 0.0f), float4x4(0.16265294, 0.2625464, -0.34452894, 0.2233622, 0.13850005, -0.42999864, -0.5385177, -0.11035979, 0.51662, -0.78238726, -0.09422375, 0.83759475, 0.44468537, 0.14301361, 0.108906105, 1.1596143));
      _62 += mul(_16(0.0f, 1.0f), float4x4(-0.73757625, -0.12369605, 0.23523071, 0.006587637, -0.15445381, 0.22757277, 0.052819528, 0.10183905, -0.07912228, -0.16998893, -0.13360223, 0.014348178, -0.17778571, -0.41047302, 0.10241381, -0.08526306));
      _62 += mul(_16(1.0f, -1.0f), float4x4(0.14712952, 0.048995696, 0.05299946, -0.06817572, 0.1498064, -0.079825334, 0.40354064, -0.31789717, -0.1998377, 0.00955295, -0.32318407, 0.30898204, -0.039571725, -0.026203401, -0.16292085, 0.08574385));
      _62 += mul(_16(1.0f, 0.0f), float4x4(-0.6353329, -0.56000775, -0.17279743, 0.18198174, -0.19555812, 0.056538377, 0.34365895, -0.07799055, 0.19011354, -0.13952748, 0.029196098, -0.19596763, -0.069196045, -0.17402656, 0.07948411, -0.016226962));
      _62 += mul(_16(1.0f, 1.0f), float4x4(0.25592864, 0.083498634, -0.28515807, 0.10789751, 0.0043962947, 0.07085363, 0.048724182, -0.025131436, -0.0049440865, -0.033094388, -0.032935806, 0.04266025, 0.20026933, 0.0927841, -0.006839351, -0.013012285));
      _62 += float4(0.02021373, 0.0014037411, 0.0012718709, 0.017278494);
      PSOut.Color = _62;
  }
}
)";
extern const std::string kHLSL_Anime4K_Upscale_CNN_x2_M_Pass7_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture1;
SamplerState u_Texture1_sampler;
Texture2D u_Texture2;
SamplerState u_Texture2_sampler;
Texture2D u_Texture3;
SamplerState u_Texture3_sampler;
Texture2D u_Texture4;
SamplerState u_Texture4_sampler;
Texture2D u_Texture5;
SamplerState u_Texture5_sampler;
Texture2D u_Texture6;
SamplerState u_Texture6_sampler;
Texture2D u_Texture7;
SamplerState u_Texture7_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _36;

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _36 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float4 _9 = mul(max(u_Texture1.Sample(u_Texture1_sampler, uv), 0.0f.xxxx), float4x4(-0.0067711817, 0.08160003, 0.0247279, 0.03084815, -0.026977416, -0.02120602, -0.025078611, -0.029852165, -0.011627478, -0.012742972, 0.022736797, -0.0028815821, -0.007515677, 0.0172887, -0.023259213, 0.009608947));
      _9 += mul(max(-u_Texture1.Sample(u_Texture1_sampler, uv), 0.0f.xxxx), float4x4(-0.028660107, -0.014015208, -0.027838672, -0.013171922, 0.0029435428, 0.027047642, -0.017478354, 0.022834882, -0.037572853, -0.0034044068, -0.0149029335, -0.013362301, 0.009827443, -0.015742151, -0.0074795415, -0.0022266617));
      _9 += mul(max(u_Texture2.Sample(u_Texture2_sampler, uv), 0.0f.xxxx), float4x4(-0.07579662, -0.039754186, -0.066026606, -0.046816852, 0.1099032, 0.043956704, 0.073109835, 0.04680284, -0.06896613, -0.008838632, -0.044584926, -0.01319039, -0.0021152915, -0.04503326, 0.027061926, -0.028334105));
      _9 += mul(max(-u_Texture2.Sample(u_Texture2_sampler, uv), 0.0f.xxxx), float4x4(0.15458213, 0.059769996, 0.09327123, -0.028782733, 0.023459995, -0.15390377, -0.13432898, -0.1127775, 0.072764635, -0.0020463336, 0.034736466, -0.0012086042, -0.05847183, -0.029952323, 0.052969377, 0.09590908));
      _9 += mul(max(u_Texture3.Sample(u_Texture3_sampler, uv), 0.0f.xxxx), float4x4(-0.07476772, -0.016574614, 0.04131183, 0.017335678, 0.009654406, 0.072183535, -0.002266456, 0.086873695, 9.310129e-05, 0.0056416965, -0.004188391, 0.023132093, -0.05183336, -0.025825873, -0.03684392, -0.0075729224));
      _9 += mul(max(-u_Texture3.Sample(u_Texture3_sampler, uv), 0.0f.xxxx), float4x4(0.00878842, 0.03869637, -0.035759524, 0.003345386, -0.064184256, -0.034568302, -0.06672922, -0.0686381, -0.06794392, -0.10685906, 0.04679947, -0.012535639, 0.006932529, -0.007783515, 0.109123886, 0.13804391));
      _9 += mul(max(u_Texture4.Sample(u_Texture4_sampler, uv), 0.0f.xxxx), float4x4(-0.03160699, 0.050473, -0.09030729, 0.0649397, 0.11466501, 0.17912874, -0.0081851315, 0.052244574, 0.051632743, 0.061941486, 0.06546816, 0.12174249, -0.05104755, -0.018193979, -0.032196652, -0.035292786));
      _9 += mul(max(-u_Texture4.Sample(u_Texture4_sampler, uv), 0.0f.xxxx), float4x4(0.013612735, -0.0024100312, -0.068611205, -0.07369285, -0.019647537, -0.066944756, -0.010012875, -0.06785739, -0.062246565, -0.087313406, -0.044278186, -0.09368995, 0.052555013, 0.13604961, 0.05645059, 0.08763303));
      _9 += mul(max(u_Texture5.Sample(u_Texture5_sampler, uv), 0.0f.xxxx), float4x4(0.04218486, -0.05028401, 0.059086576, -0.03545452, 0.027737848, 0.0043074046, 0.0011001764, -0.073026665, -0.04094988, 0.044061556, -0.009812515, 0.06841999, -0.06612581, 0.037223976, -0.07759491, -0.04356598));
      _9 += mul(max(-u_Texture5.Sample(u_Texture5_sampler, uv), 0.0f.xxxx), float4x4(-0.027558247, 0.014248466, -0.019813016, -0.058107473, -0.016717663, -0.020424338, 0.0053625097, -0.009917319, 0.013678771, 0.0113340765, 0.0061787106, -0.036083996, -0.020179711, -0.011310535, 0.054827053, -0.0008278952));
      _9 += mul(max(u_Texture6.Sample(u_Texture6_sampler, uv), 0.0f.xxxx), float4x4(0.028690035, -0.012079616, 0.11931408, -0.048533775, 0.069336995, 0.0049852817, 0.013774468, 0.035233382, -0.07384821, 0.0003354423, -0.0059171803, -0.04503906, 0.08727279, 0.005138857, -0.17724465, 0.055782065));
      _9 += mul(max(-u_Texture6.Sample(u_Texture6_sampler, uv), 0.0f.xxxx), float4x4(-0.20744391, 0.24348328, -0.3145766, 0.17026486, -0.022870807, -0.01648648, -0.05912279, -0.012555373, -0.066004686, 0.03182394, 0.16285324, -0.1221846, -0.31816196, 0.007928748, 0.43180224, -0.015949022));
      _9 += mul(max(u_Texture7.Sample(u_Texture7_sampler, uv), 0.0f.xxxx), float4x4(0.16363169, 0.14781676, -0.2377973, -0.1571377, -0.09038187, 0.0046504294, 0.033955004, -0.051421452, 0.046735536, 0.006827522, -0.121338, 0.12671822, 0.15833299, -0.1858712, -0.1942371, 0.17336044));
      _9 += mul(max(-u_Texture7.Sample(u_Texture7_sampler, uv), 0.0f.xxxx), float4x4(-0.018145572, -0.015550516, 0.044410378, 0.046016492, 0.084021375, 0.05327457, -0.008270992, -0.045435544, 0.07185879, -0.131923, 0.26721445, -0.26745328, -0.07093472, 0.042701527, 0.13793674, -0.095621444));
      _9 += float4(0.016836504, 0.010161949, 0.021351453, 0.01278978);
      PSOut.Color = _9;
  }
}
)";
extern const std::string kHLSL_Anime4K_Upscale_CNN_x2_M_Pass8_Pixel = R"(
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;
Texture2D u_Texture1;
SamplerState u_Texture1_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static float2 _11;

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _11 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float2 _9 = frac(uv * u_InputSize);
      int2 _25 = int2(_9 * 2.0f.xx);
      float _32 = u_Texture.Sample(u_Texture_sampler, ((0.5f.xx - _9) * u_InputPt) + uv)[(_25.y * 2) + _25.x];
      float _61 = _32;
      float _63 = _61;
      float _65 = _63;
      PSOut.Color = float4(_32, _61, _63, _65) + u_Texture1.Sample(u_Texture1_sampler, uv);
  }
}
)";

}  // namespace renderer
