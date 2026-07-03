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
static float4 _47;

float _11(float4 _10)
{
    return dot(float4(0.2989999949932098388671875f, 0.58700001239776611328125f, 0.114000000059604644775390625f, 0.0f), _10);
}

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  _31 = PSIn.UV;
  float2 uv = PSIn.UV;
  {
      float4 _34 = u_Texture.Sample(u_Texture_sampler, uv);
      float _23 = _11(_34);
      float _36 = min(_23, u_Texture.Sample(u_Texture_sampler, uv).x);
      PSOut.Color = u_Texture.Sample(u_Texture_sampler, uv) - (_23 - _36).xxxx;
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
      float4 _40 = mul(_12(_61, _62), float4x4(-0.099919863045215606689453125, -0.3437488079071044921875, 0.0860722362995147705078125, 0.0, 0.13782341778278350830078125, 0.05450952053070068359375, 0.0449883937835693359375, 0.0, -0.03125168383121490478515625, 0.343478024005889892578125, 0.13717900216579437255859375, 0.0, -0.06356842815876007080078125, 0.4633537232875823974609375, 0.17976908385753631591796875, 0.0));
      float _81 = -1.0f;
      float _82 = 0.0f;
      _40 += mul(_12(_81, _82), float4x4(-0.0242124237120151519775390625, -0.13254678249359130859375, -0.063756786286830902099609375, 0.0, -0.0927850902080535888671875, 0.113105185329914093017578125, 0.009184114634990692138671875, 0.0, -0.0004090775619260966777801513671875, 0.0056679458357393741607666015625, 0.11551873385906219482421875, 0.0, 0.345522940158843994140625, -0.00036919137346558272838592529296875, -0.115506775677204132080078125, 0.0));
      float _104 = -1.0f;
      float _105 = 1.0f;
      _40 += mul(_12(_104, _105), float4x4(-0.14101827144622802734375, -0.443488419055938720703125, -0.1588039398193359375, 0.0, 0.02352349273860454559326171875, -0.08818876743316650390625, -0.013732858002185821533203125, 0.0, 0.0440945662558078765869140625, -0.402614891529083251953125, -0.02075113542377948760986328125, 0.0, -0.01927174627780914306640625, -0.21995794773101806640625, 0.0127191506326198577880859375, 0.0));
      float _126 = 0.0f;
      float _127 = -1.0f;
      _40 += mul(_12(_126, _127), float4x4(0.013001821003854274749755859375, 0.24760444462299346923828125, -0.0581328757107257843017578125, 0.0, -0.3450350463390350341796875, -0.01617340184748172760009765625, 0.01678439788520336151123046875, 0.0, 0.3921913802623748779296875, 0.10154511034488677978515625, -0.058085389435291290283203125, 0.0, 0.1879212558269500732421875, 0.154530823230743408203125, -0.110399149358272552490234375, 0.0));
      float _148 = 0.0f;
      float _149 = 0.0f;
      _40 += mul(_12(_148, _149), float4x4(0.37024533748626708984375, 0.1955559551715850830078125, 0.2122814655303955078125, 0.0, 0.0414408631622791290283203125, 0.20855538547039031982421875, -0.029534600675106048583984375, 0.0, -0.3374567925930023193359375, -0.279740750789642333984375, -0.567000567913055419921875, 0.0, -0.4499428570270538330078125, -0.53726279735565185546875, 0.03004282154142856597900390625, 0.0));
      float _170 = 0.0f;
      float _171 = 1.0f;
      _40 += mul(_12(_170, _171), float4x4(-0.12940631806850433349609375, -0.13704006373882293701171875, -0.06166251003742218017578125, 0.0, 0.057525999844074249267578125, -0.047685407102108001708984375, -0.0188351906836032867431640625, 0.0, 0.09068204462528228759765625, 0.4461567401885986328125, 0.203223705291748046875, 0.0, -0.069850333034992218017578125, -0.4805660545825958251953125, -0.113287605345249176025390625, 0.0));
      float _192 = 1.0f;
      float _193 = -1.0f;
      _40 += mul(_12(_192, _193), float4x4(0.0108566693961620330810546875, -0.039673030376434326171875, 0.015120559372007846832275390625, 0.0, -0.3582073748111724853515625, 0.03870557248592376708984375, -0.15314877033233642578125, 0.0, 0.16757218539714813232421875, 0.3265285491943359375, 0.2344200909137725830078125, 0.0, 0.0826198756694793701171875, -0.0120300166308879852294921875, 0.097679220139980316162109375, 0.0));
      float _214 = 1.0f;
      float _215 = 0.0f;
      _40 += mul(_12(_214, _215), float4x4(-0.046272672712802886962890625, 0.58619463443756103515625, -0.17025311291217803955078125, 0.0, -0.1775230467319488525390625, -0.0609034635126590728759765625, 0.0513699315488338470458984375, 0.0, 0.0820182859897613525390625, -0.02279359661042690277099609375, 0.0293832980096340179443359375, 0.0, -0.251282393932342529296875, 0.077803514897823333740234375, -0.15475408732891082763671875, 0.0));
      float _236 = 1.0f;
      float _237 = 1.0f;
      _40 += mul(_12(_236, _237), float4x4(-0.1121202409267425537109375, -0.111762188374996185302734375, 0.1332683861255645751953125, 0.0, 0.13378004729747772216796875, -0.048429377377033233642578125, 0.04306270182132720947265625, 0.0, -0.2027488052845001220703125, -0.083963863551616668701171875, 0.0513623766601085662841796875, 0.0, 0.080564208328723907470703125, 0.105078287422657012939453125, 0.06482754647731781005859375, 0.0));
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
      float4 _62 = mul(_12(_86, _87), float4x4(-0.16410656273365020751953125, 0.105412475764751434326171875, 0.12558138370513916015625, -0.142203509807586669921875, -0.40521824359893798828125, -0.0604012720286846160888671875, -0.02086146734654903411865234375, 0.20736892521381378173828125, 0.13121907413005828857421875, -0.04306347668170928955078125, 0.0303705148398876190185546875, 0.00332156405784189701080322265625, -0.0231459699571132659912109375, -0.13933973014354705810546875, 0.13178016245365142822265625, -0.2924171388149261474609375));
      float _111 = -1.0f;
      float _112 = 0.0f;
      _62 += mul(_12(_111, _112), float4x4(0.185173213481903076171875, 0.0255270116031169891357421875, 0.12750522792339324951171875, 0.083992101252079010009765625, 0.291629850864410400390625, -0.0673192441463470458984375, -0.09143595397472381591796875, 0.101866178214550018310546875, -0.2678339481353759765625, 0.0550041757524013519287109375, 0.138188421726226806640625, -0.17237375676631927490234375, 0.039760686457157135009765625, 0.04891656339168548583984375, 0.367042243480682373046875, 0.1328241825103759765625));
      float _139 = -1.0f;
      float _140 = 1.0f;
      _62 += mul(_12(_139, _140), float4x4(-0.16578869521617889404296875, -0.12756164371967315673828125, 0.15870757400989532470703125, 0.070999659597873687744140625, 0.013132513500750064849853515625, -0.08437298238277435302734375, -0.013529402203857898712158203125, -0.02406363189220428466796875, -0.17222486436367034912109375, -0.2905299663543701171875, -0.0581752993166446685791015625, 0.3183484375476837158203125, 0.091398894786834716796875, 0.3269337117671966552734375, 0.11802370846271514892578125, -0.111838586628437042236328125));
      float _166 = 0.0f;
      float _167 = -1.0f;
      _62 += mul(_12(_166, _167), float4x4(0.4603688716888427734375, 0.105554141104221343994140625, 0.028316497802734375, 0.0495397411286830902099609375, -0.076546229422092437744140625, -0.117430426180362701416015625, 0.1368434131145477294921875, -0.3134221732616424560546875, 0.22923062741756439208984375, 0.12406776845455169677734375, 0.00966408662497997283935546875, -0.610313117504119873046875, 0.174638211727142333984375, -0.011399491690099239349365234375, 0.20226590335369110107421875, -0.13605757057666778564453125));
      float _193 = 0.0f;
      float _194 = 0.0f;
      _62 += mul(_12(_193, _194), float4x4(0.0340695492923259735107421875, -0.02932107262313365936279296875, 0.11464084684848785400390625, -0.560991585254669189453125, -0.398193657398223876953125, 0.466194927692413330078125, -0.109314523637294769287109375, 0.3182623386383056640625, 0.61176002025604248046875, 0.3670018613338470458984375, -0.09154021739959716796875, -0.011012659408152103424072265625, -0.4680945575237274169921875, 0.0228856094181537628173828125, 0.073341466486454010009765625, -0.467195451259613037109375));
      float _220 = 0.0f;
      float _221 = 1.0f;
      _62 += mul(_12(_220, _221), float4x4(-0.056855045258998870849609375, -0.0681611597537994384765625, 0.09954045712947845458984375, 0.3825241029262542724609375, 0.2703702747821807861328125, -0.2298661172389984130859375, -0.053741760551929473876953125, -0.16098870337009429931640625, -0.092696957290172576904296875, 0.086931668221950531005859375, 0.0071916827000677585601806640625, 0.0552047677338123321533203125, -0.563571989536285400390625, -0.1624610126018524169921875, -0.17886920273303985595703125, 0.1021306812763214111328125));
      float _247 = 1.0f;
      float _248 = -1.0f;
      _62 += mul(_12(_247, _248), float4x4(0.064662598073482513427734375, -0.23337309062480926513671875, -0.13473303616046905517578125, -0.042647518217563629150390625, 0.102358795702457427978515625, 0.12633001804351806640625, 0.05379046499729156494140625, -0.02974073775112628936767578125, -0.4505582153797149658203125, -0.19299198687076568603515625, -0.100611932575702667236328125, -0.078652851283550262451171875, 0.20557902753353118896484375, -0.15085731446743011474609375, -0.13393497467041015625, 0.20883278548717498779296875));
      float _274 = 1.0f;
      float _275 = 0.0f;
      _62 += mul(_12(_274, _275), float4x4(0.010471527464687824249267578125, 0.23226471245288848876953125, 0.013839962892234325408935546875, 0.3377230465412139892578125, -0.0332184731960296630859375, -0.0593433268368244171142578125, 0.159303247928619384765625, 0.40261495113372802734375, -0.4615744650363922119140625, -0.14395959675312042236328125, 0.0437423549592494964599609375, -0.0835129320621490478515625, 0.0048665828071534633636474609375, 0.13619647920131683349609375, 0.1746732294559478759765625, 0.18129359185695648193359375));
      float _301 = 1.0f;
      float _302 = 1.0f;
      _62 += mul(_12(_301, _302), float4x4(-0.124934338033199310302734375, -0.0371426157653331756591796875, 0.0071619413793087005615234375, 0.14917047321796417236328125, -0.18751339614391326904296875, 0.16670019924640655517578125, 0.0034872111864387989044189453125, -0.16310586035251617431640625, -0.07494379580020904541015625, 0.16665546596050262451171875, 0.12031896412372589111328125, 0.072317369282245635986328125, -0.00317016057670116424560546875, -0.01124812662601470947265625, -0.096255786716938018798828125, 0.30447328090667724609375));
      float _328 = -1.0f;
      float _329 = -1.0f;
      _62 += mul(_16(_328, _329), float4x4(0.093798615038394927978515625, 0.118534855544567108154296875, -0.3413709700107574462890625, 0.162208616733551025390625, 0.17074613273143768310546875, 0.02750877849757671356201171875, 0.3200031220912933349609375, 0.108993016183376312255859375, -0.087806783616542816162109375, -0.2778477966785430908203125, -0.220271587371826171875, 0.1407052576541900634765625, -0.01252020709216594696044921875, -0.19509242475032806396484375, 0.33751499652862548828125, 0.127842843532562255859375));
      float _355 = -1.0f;
      float _356 = 0.0f;
      _62 += mul(_16(_355, _356), float4x4(-0.14325632154941558837890625, 0.118210829794406890869140625, -0.067666478455066680908203125, 0.131892502307891845703125, -0.1467452943325042724609375, -0.012266484089195728302001953125, 0.58165013790130615234375, -0.04346276819705963134765625, -0.27502357959747314453125, -0.21005479991436004638671875, -0.2512278854846954345703125, 0.15454484522342681884765625, 0.093708373606204986572265625, 0.47075021266937255859375, -0.3378375470638275146484375, 0.0445000566542148590087890625));
      float _382 = -1.0f;
      float _383 = 1.0f;
      _62 += mul(_16(_382, _383), float4x4(-0.0568320713937282562255859375, -0.507638633251190185546875, 0.0227095149457454681396484375, 0.014044850133359432220458984375, 0.005194646306335926055908203125, 0.0073084421455860137939453125, 0.2945230007171630859375, 0.03128270804882049560546875, -0.108000524342060089111328125, 0.85424041748046875, -0.3822472095489501953125, -0.2675681412220001220703125, 0.1013320386409759521484375, 0.28387355804443359375, 0.661664068698883056640625, -0.123147785663604736328125));
      float _409 = 0.0f;
      float _410 = -1.0f;
      _62 += mul(_16(_409, _410), float4x4(-0.3645517826080322265625, -0.1580249369144439697265625, 0.13042800128459930419921875, 0.15804816782474517822265625, 0.347055494785308837890625, -0.0019141496159136295318603515625, 0.0395427308976650238037109375, 0.1255171298980712890625, -0.0453030876815319061279296875, -0.2593958675861358642578125, -0.17985536158084869384765625, 0.2837197482585906982421875, -0.0317076407372951507568359375, -0.23875342309474945068359375, 0.10514594614505767822265625, -0.085748516023159027099609375));
      float _436 = 0.0f;
      float _437 = 0.0f;
      _62 += mul(_16(_436, _437), float4x4(0.0060625462792813777923583984375, -0.09584514796733856201171875, 0.1294201314449310302734375, 0.1049964427947998046875, 0.24439239501953125, -0.012805371545255184173583984375, 0.4178554713726043701171875, -0.2056601345539093017578125, -0.01769225858151912689208984375, -0.1394222676753997802734375, 0.0460715629160404205322265625, -0.0313212759792804718017578125, -0.20214004814624786376953125, 0.16143198311328887939453125, 0.70300257205963134765625, 0.27830326557159423828125));
      float _463 = 0.0f;
      float _464 = 1.0f;
      _62 += mul(_16(_463, _464), float4x4(-0.081274963915348052978515625, 0.012910989113152027130126953125, -0.22015951573848724365234375, -0.316453039646148681640625, -0.14562319219112396240234375, 0.0242013968527317047119140625, -0.4416075646877288818359375, 0.15469242632389068603515625, 0.272005259990692138671875, 0.04816257953643798828125, -0.0560353733599185943603515625, 0.053187452256679534912109375, -0.20491313934326171875, 0.21297328174114227294921875, 0.3382441699504852294921875, -0.2098944485187530517578125));
      float _490 = 1.0f;
      float _491 = -1.0f;
      _62 += mul(_16(_490, _491), float4x4(-0.0465503670275211334228515625, 0.23520171642303466796875, -0.07058717310428619384765625, -0.055425353348255157470703125, 0.0331854037940502166748046875, -0.059092141687870025634765625, -0.11759936809539794921875, -0.12506316602230072021484375, 0.3333724439144134521484375, 0.08613680303096771240234375, -0.1859404742717742919921875, 0.15729053318500518798828125, 0.12853644788265228271484375, 0.10706329345703125, 0.080006264150142669677734375, -0.09150040149688720703125));
      float _517 = 1.0f;
      float _518 = 0.0f;
      _62 += mul(_16(_517, _518), float4x4(0.0425164066255092620849609375, -0.06554169952869415283203125, 0.1285592615604400634765625, -0.2446714937686920166015625, 0.14844788610935211181640625, -0.0572563968598842620849609375, 0.01421927474439144134521484375, -0.4008074104785919189453125, 0.16533111035823822021484375, 0.07671372592449188232421875, 0.0517613850533962249755859375, 0.19603717327117919921875, 0.13502933084964752197265625, -0.23448966443538665771484375, 0.053433082997798919677734375, -0.1796950995922088623046875));
      float _544 = 1.0f;
      float _545 = 1.0f;
      _62 += mul(_16(_544, _545), float4x4(0.147778034210205078125, 0.1921064555644989013671875, -0.18074385821819305419921875, -0.383769810199737548828125, 0.15524907410144805908203125, -0.21443639695644378662109375, -0.21639029681682586669921875, -0.00226614973507821559906005859375, 0.0431586168706417083740234375, -0.470207870006561279296875, 0.00307549652643501758575439453125, -0.37276732921600341796875, -0.069968760013580322265625, -0.420790612697601318359375, 0.367999732494354248046875, -0.289349973201751708984375));
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
      float4 _62 = mul(_12(_86, _87), float4x4(0.315431773662567138671875, 0.00362250395119190216064453125, -0.05296458303928375244140625, 0.13107763230800628662109375, 0.23095236718654632568359375, 0.1794884204864501953125, -0.1555115878582000732421875, 0.11369179189205169677734375, -0.06692610681056976318359375, -0.14627707004547119140625, 0.0564478598535060882568359375, -0.094529949128627777099609375, -0.586776316165924072265625, 0.174501597881317138671875, -0.0126651637256145477294921875, -0.119734026491641998291015625));
      float _111 = -1.0f;
      float _112 = 0.0f;
      _62 += mul(_12(_111, _112), float4x4(-0.269466102123260498046875, -0.2551148235797882080078125, -0.02261786349117755889892578125, -0.0133466534316539764404296875, -0.115382134914398193359375, -0.13922207057476043701171875, 0.20333401858806610107421875, -0.0990953743457794189453125, 0.307326793670654296875, 0.367582142353057861328125, -0.111258886754512786865234375, -0.2510061562061309814453125, -0.0672284662723541259765625, -0.18821828067302703857421875, 0.35522449016571044921875, 0.3552175462245941162109375));
      float _139 = -1.0f;
      float _140 = 1.0f;
      _62 += mul(_12(_139, _140), float4x4(0.0110124088823795318603515625, -0.231846749782562255859375, -0.1646140515804290771484375, -0.1903336346149444580078125, -0.1367508471012115478515625, 0.18012201786041259765625, 0.03817708790302276611328125, 0.074691779911518096923828125, 0.2564199864864349365234375, 0.576541364192962646484375, 0.1234095990657806396484375, -0.017948545515537261962890625, -0.3485120832920074462890625, 0.1031735241413116455078125, 0.013202029280364513397216796875, 0.15287701785564422607421875));
      float _166 = 0.0f;
      float _167 = -1.0f;
      _62 += mul(_12(_166, _167), float4x4(-0.05340532958507537841796875, -0.121811740100383758544921875, -0.1151945292949676513671875, 0.02221863158047199249267578125, 0.237974822521209716796875, -0.23363493382930755615234375, 0.13842065632343292236328125, 0.0312387235462665557861328125, 0.203513920307159423828125, -0.2069660723209381103515625, -0.106878317892551422119140625, 0.2685182094573974609375, -0.0533335097134113311767578125, 0.1099410355091094970703125, 0.2904000580310821533203125, 0.1530006825923919677734375));
      float _193 = 0.0f;
      float _194 = 0.0f;
      _62 += mul(_12(_193, _194), float4x4(0.2298531830310821533203125, -0.11690287292003631591796875, -0.063354738056659698486328125, -0.1677628457546234130859375, -0.3103801906108856201171875, -0.19474880397319793701171875, -0.007870727218687534332275390625, -0.006570436991751194000244140625, -0.2291641533374786376953125, 0.118020534515380859375, 0.07610632479190826416015625, -0.2958958446979522705078125, 0.252388060092926025390625, 0.07814262807369232177734375, 0.094677485525608062744140625, 0.4141350686550140380859375));
      float _220 = 0.0f;
      float _221 = 1.0f;
      _62 += mul(_12(_220, _221), float4x4(0.43607962131500244140625, -0.091190874576568603515625, 0.1235634386539459228515625, -0.2349030673503875732421875, -0.3645643293857574462890625, 0.13035081326961517333984375, -0.00861617736518383026123046875, 0.3013122975826263427734375, -0.123776875436305999755859375, 0.2862796783447265625, 0.095998160541057586669921875, 0.1415315568447113037109375, -0.16634953022003173828125, 0.27249968051910400390625, -0.0061445571482181549072265625, 0.21837277710437774658203125));
      float _247 = 1.0f;
      float _248 = -1.0f;
      _62 += mul(_12(_247, _248), float4x4(0.0603645853698253631591796875, -0.08991022408008575439453125, 0.03693449497222900390625, 0.1348964869976043701171875, 0.3786022365093231201171875, -0.06817696988582611083984375, -0.07826615869998931884765625, 0.0623766295611858367919921875, 0.0391824133694171905517578125, -0.26842749118804931640625, 0.065599761903285980224609375, 0.1263760030269622802734375, -0.2280542552471160888671875, -0.12528502941131591796875, -0.0825364589691162109375, 0.21194183826446533203125));
      float _274 = 1.0f;
      float _275 = 0.0f;
      _62 += mul(_12(_274, _275), float4x4(-0.12534816563129425048828125, -0.0069575770758092403411865234375, 0.23081482946872711181640625, -0.2904095947742462158203125, 0.21225188672542572021484375, -0.0251058526337146759033203125, 0.18027560412883758544921875, -0.2529282271862030029296875, -0.2781804502010345458984375, 0.121009238064289093017578125, -0.18995638191699981689453125, -0.21834068000316619873046875, -0.3070442974567413330078125, -0.069164521992206573486328125, 0.1660301387310028076171875, 0.13719652593135833740234375));
      float _301 = 1.0f;
      float _302 = 1.0f;
      _62 += mul(_12(_301, _302), float4x4(0.01720965467393398284912109375, 0.104677163064479827880859375, 0.21004720032215118408203125, -0.06983841955661773681640625, 0.107571370899677276611328125, -0.21848909556865692138671875, -0.2576854526996612548828125, -0.1038548648357391357421875, 0.21414296329021453857421875, 0.100061476230621337890625, -0.22329919040203094482421875, -0.051384352147579193115234375, -0.30885982513427734375, -0.15275280177593231201171875, -0.2915342748165130615234375, 0.14629121124744415283203125));
      float _328 = -1.0f;
      float _329 = -1.0f;
      _62 += mul(_16(_328, _329), float4x4(0.0059623294509947299957275390625, 0.09783084690570831298828125, -0.0335082821547985076904296875, 0.10504043102264404296875, -0.260608017444610595703125, -0.15865178406238555908203125, 0.174803912639617919921875, -0.061056859791278839111328125, 0.3211581707000732421875, 0.14730210602283477783203125, -0.091310136020183563232421875, 0.013493488542735576629638671875, 0.0210255049169063568115234375, -0.2497730255126953125, 0.09870876371860504150390625, -0.112788550555706024169921875));
      float _355 = -1.0f;
      float _356 = 0.0f;
      _62 += mul(_16(_355, _356), float4x4(0.1487524807453155517578125, 0.101288855075836181640625, -0.0373923368752002716064453125, -0.0593755580484867095947265625, -0.1485941410064697265625, -0.111368201673030853271484375, 0.085396908223628997802734375, 0.02766367234289646148681640625, 0.1937706172466278076171875, -0.489446461200714111328125, 0.1751306056976318359375, 0.051804013550281524658203125, -0.17456068098545074462890625, 0.1018564999103546142578125, -0.15428723394870758056640625, -0.0498132221400737762451171875));
      float _382 = -1.0f;
      float _383 = 1.0f;
      _62 += mul(_16(_382, _383), float4x4(0.1188465654850006103515625, -0.116625271737575531005859375, -0.0519916675984859466552734375, 0.057021595537662506103515625, -0.19869871437549591064453125, -0.4381835162639617919921875, 0.2100829184055328369140625, 0.094605267047882080078125, -0.037388257682323455810546875, -0.093285344541072845458984375, 0.107923649251461029052734375, 0.001655128784477710723876953125, 0.0845672786235809326171875, 0.0385072045028209686279296875, 0.20209239423274993896484375, -0.001595706329680979251861572265625));
      float _409 = 0.0f;
      float _410 = -1.0f;
      _62 += mul(_16(_409, _410), float4x4(0.110621742904186248779296875, -0.0505452118813991546630859375, 0.028986752033233642578125, 0.03468765318393707275390625, -0.2639231979846954345703125, 0.309895575046539306640625, 0.0374294035136699676513671875, -0.095991350710391998291015625, -0.0602954663336277008056640625, 0.3090613186359405517578125, 0.20855663716793060302734375, -0.062504939734935760498046875, -0.3217330873012542724609375, 0.03032327257096767425537109375, -0.1984894275665283203125, -0.1321586668491363525390625));
      float _436 = 0.0f;
      float _437 = 0.0f;
      _62 += mul(_16(_436, _437), float4x4(-0.0103911459445953369140625, 0.007593150250613689422607421875, -0.15452717244625091552734375, 0.006791184656322002410888671875, 0.07657845318317413330078125, 0.4263265430927276611328125, -0.1461341083049774169921875, 0.05750115215778350830078125, 0.4449125826358795166015625, 0.47022533416748046875, -0.4523106515407562255859375, 0.098769791424274444580078125, 0.0435906015336513519287109375, 0.3473743498325347900390625, 0.120944090187549591064453125, 0.04494644701480865478515625));
      float _463 = 0.0f;
      float _464 = 1.0f;
      _62 += mul(_16(_463, _464), float4x4(-0.156074345111846923828125, -0.1528245508670806884765625, -0.055801592767238616943359375, 0.123009622097015380859375, 0.229305803775787353515625, 0.264377176761627197265625, -0.01677872799336910247802734375, -0.13235826790332794189453125, -0.0952033102512359619140625, -0.16854770481586456298828125, -0.34478986263275146484375, -0.13987202942371368408203125, 0.012836731970310211181640625, -0.1321112215518951416015625, -0.2322830855846405029296875, -0.16550971567630767822265625));
      float _490 = 1.0f;
      float _491 = -1.0f;
      _62 += mul(_16(_490, _491), float4x4(0.13161735236644744873046875, 0.15148849785327911376953125, 0.090661935508251190185546875, -0.3265170753002166748046875, -0.090393461287021636962890625, 0.20977421104907989501953125, 0.152880609035491943359375, 0.18825398385524749755859375, -0.03347547352313995361328125, 0.0314319543540477752685546875, -0.0331658311188220977783203125, -0.1577723920345306396484375, -0.2368669807910919189453125, -0.004922610707581043243408203125, 0.096465729176998138427734375, 0.1757270395755767822265625));
      float _517 = 1.0f;
      float _518 = 0.0f;
      _62 += mul(_16(_517, _518), float4x4(0.11215722560882568359375, -0.14686782658100128173828125, -0.315301120281219482421875, 0.18053482472896575927734375, -0.087128780782222747802734375, 0.28682422637939453125, -0.2700583040714263916015625, 0.166533410549163818359375, 0.234531819820404052734375, -0.086443506181240081787109375, -0.0602895207703113555908203125, 0.252151966094970703125, 0.104387700557708740234375, 0.0594570524990558624267578125, -0.070416875183582305908203125, 0.061915852129459381103515625));
      float _544 = 1.0f;
      float _545 = 1.0f;
      _62 += mul(_16(_544, _545), float4x4(-0.20122241973876953125, -0.3543668687343597412109375, 0.104252420365810394287109375, 0.01630274951457977294921875, 0.076313145458698272705078125, 0.3762327134609222412109375, -0.17087407410144805908203125, 0.24247427284717559814453125, -0.09884829819202423095703125, -0.078095577657222747802734375, 0.03030149638652801513671875, -0.0064744767732918262481689453125, 0.094337783753871917724609375, 0.30558478832244873046875, -0.13911743462085723876953125, 0.0384264104068279266357421875));
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
      float4 _62 = mul(_12(_86, _87), float4x4(-0.22377209365367889404296875, 0.015353088267147541046142578125, -0.074562691152095794677734375, -0.14183366298675537109375, -0.0064096362330019474029541015625, 0.23983319103717803955078125, 0.093151815235614776611328125, 0.064010448753833770751953125, -0.3180842697620391845703125, 0.14967978000640869140625, -0.14331085979938507080078125, -0.22044073045253753662109375, 0.73477733135223388671875, -0.3492022454738616943359375, -0.24586205184459686279296875, 0.2993227541446685791015625));
      float _111 = -1.0f;
      float _112 = 0.0f;
      _62 += mul(_12(_111, _112), float4x4(-0.07968509197235107421875, 0.4095855057239532470703125, 0.29812729358673095703125, -0.06524765491485595703125, -0.3349145948886871337890625, -0.1712070405483245849609375, 0.22123689949512481689453125, -0.15255849063396453857421875, 0.1652912795543670654296875, 0.17425705492496490478515625, 0.1039238870143890380859375, 0.13094437122344970703125, 0.084434993565082550048828125, 0.15298946201801300048828125, -0.287754535675048828125, 0.1868521869182586669921875));
      float _139 = -1.0f;
      float _140 = 1.0f;
      _62 += mul(_12(_139, _140), float4x4(0.0157067365944385528564453125, -0.158767879009246826171875, -0.0233209617435932159423828125, -0.18880949914455413818359375, -0.17755036056041717529296875, -0.384669959545135498046875, -0.3145248889923095703125, -0.04637010395526885986328125, 0.2622525990009307861328125, -0.3370084464550018310546875, -0.21223734319210052490234375, 0.09000895917415618896484375, 0.112057305872440338134765625, -0.03171174228191375732421875, -0.13145959377288818359375, -0.0046378844417631626129150390625));
      float _166 = 0.0f;
      float _167 = -1.0f;
      _62 += mul(_12(_166, _167), float4x4(-0.311275064945220947265625, -0.02985105477273464202880859375, 0.18019931018352508544921875, -0.013932473957538604736328125, 0.31304323673248291015625, 0.0580137707293033599853515625, 0.1441551148891448974609375, -0.0464549474418163299560546875, -0.03965751826763153076171875, 0.00040150844142772257328033447265625, -0.0984523594379425048828125, -0.3403935134410858154296875, 0.036490179598331451416015625, -0.0442206896841526031494140625, 0.21895433962345123291015625, -0.006705288775265216827392578125));
      float _193 = 0.0f;
      float _194 = 0.0f;
      _62 += mul(_12(_193, _194), float4x4(-0.34878647327423095703125, 0.20644618570804595947265625, 0.2444942295551300048828125, -0.269146978855133056640625, -0.512928307056427001953125, 0.08732272684574127197265625, 0.4410338699817657470703125, -0.2130998671054840087890625, 0.06025095283985137939453125, -0.24118888378143310546875, 0.2245592772960662841796875, 0.083864860236644744873046875, -0.16354133188724517822265625, 0.244550645351409912109375, 0.25738942623138427734375, 0.02148481644690036773681640625));
      float _220 = 0.0f;
      float _221 = 1.0f;
      _62 += mul(_12(_220, _221), float4x4(-0.0574549026787281036376953125, 0.0333140790462493896484375, 0.24326409399509429931640625, -0.193350613117218017578125, -0.4121921956539154052734375, 0.050440080463886260986328125, 0.07690669596195220947265625, 0.092174507677555084228515625, 0.02266154624521732330322265625, 0.0432437099516391754150390625, -0.20858038961887359619140625, 0.19683690369129180908203125, 0.371782720088958740234375, 0.20727942883968353271484375, 0.012439015321433544158935546875, -0.19435833394527435302734375));
      float _247 = 1.0f;
      float _248 = -1.0f;
      _62 += mul(_12(_247, _248), float4x4(-0.169604957103729248046875, -0.011531225405633449554443359375, 0.008601217530667781829833984375, -0.2912061214447021484375, 0.2461616694927215576171875, -0.113121427595615386962890625, -0.3564490973949432373046875, 0.23756824433803558349609375, 0.3797747790813446044921875, -0.18141078948974609375, -0.126394808292388916015625, 0.1803569495677947998046875, 0.14324574172496795654296875, -0.2384393215179443359375, 0.0097992978990077972412109375, -0.087133996188640594482421875));
      float _274 = 1.0f;
      float _275 = 0.0f;
      _62 += mul(_12(_274, _275), float4x4(-0.10081239044666290283203125, 0.00899775885045528411865234375, -0.11627764999866485595703125, 0.050861470401287078857421875, 0.29191493988037109375, 0.10475623607635498046875, 0.0236932225525379180908203125, 0.18498174846172332763671875, 0.104346930980682373046875, 0.0396410860121250152587890625, -0.3080175817012786865234375, 0.1559543907642364501953125, 0.089706361293792724609375, 0.02323888055980205535888671875, -0.120208986103534698486328125, -0.098773062229156494140625));
      float _301 = 1.0f;
      float _302 = 1.0f;
      _62 += mul(_12(_301, _302), float4x4(0.101321674883365631103515625, -0.0407393686473369598388671875, 0.0192773304879665374755859375, -0.054297395050525665283203125, -0.2929975986480712890625, 0.03011070378124713897705078125, 0.15335668623447418212890625, -0.07752205431461334228515625, 0.3881041705608367919921875, -0.18147061765193939208984375, -0.15384073555469512939453125, 0.079183690249919891357421875, 0.5605375766754150390625, -0.098339520394802093505859375, -0.110595054924488067626953125, -0.06848062574863433837890625));
      float _328 = -1.0f;
      float _329 = -1.0f;
      _62 += mul(_16(_328, _329), float4x4(0.23263514041900634765625, -0.020222447812557220458984375, 0.1252970397472381591796875, 0.1835050284862518310546875, -0.117192320525646209716796875, -0.177901566028594970703125, 0.255488574504852294921875, -0.295935332775115966796875, 0.2903209030628204345703125, -0.1560076177120208740234375, -0.045854471623897552490234375, 0.086893297731876373291015625, -0.0075037949718534946441650390625, -0.0874177515506744384765625, -0.102550327777862548828125, 0.0270047374069690704345703125));
      float _355 = -1.0f;
      float _356 = 0.0f;
      _62 += mul(_16(_355, _356), float4x4(-0.14958654344081878662109375, -0.1705780327320098876953125, 0.0596714206039905548095703125, -0.085967786610126495361328125, -0.00623883493244647979736328125, 0.12524141371250152587890625, -0.077908180654048919677734375, 0.078753583133220672607421875, -0.2928948104381561279296875, 0.1397826373577117919921875, -0.58938181400299072265625, -0.033166669309139251708984375, 0.198855698108673095703125, -0.019280292093753814697265625, -0.02284571342170238494873046875, -0.436928212642669677734375));
      float _382 = -1.0f;
      float _383 = 1.0f;
      _62 += mul(_16(_382, _383), float4x4(0.19195687770843505859375, 0.0908333957195281982421875, -0.05017118155956268310546875, 0.53920543193817138671875, -0.0608836822211742401123046875, 0.00342288310639560222625732421875, 0.02286216802895069122314453125, -0.102527759969234466552734375, -0.2589782774448394775390625, 0.1095341742038726806640625, -0.2701129913330078125, -0.09180748462677001953125, 0.070633240044116973876953125, 0.03118087351322174072265625, -0.057831235229969024658203125, 0.0042943428270518779754638671875));
      float _409 = 0.0f;
      float _410 = -1.0f;
      _62 += mul(_16(_409, _410), float4x4(-0.184942424297332763671875, 0.15568028390407562255859375, -0.15292496979236602783203125, 0.0016430220566689968109130859375, -0.119284816086292266845703125, -0.2854858934879302978515625, 0.21895618736743927001953125, -0.02617698721587657928466796875, 0.382189691066741943359375, -0.22441281378269195556640625, -0.095677755773067474365234375, 0.0484630763530731201171875, 0.077779792249202728271484375, -0.04915587604045867919921875, 0.15210424363613128662109375, -0.4824008941650390625));
      float _436 = 0.0f;
      float _437 = 0.0f;
      _62 += mul(_16(_436, _437), float4x4(0.00721512921154499053955078125, -0.17180430889129638671875, -0.082942970097064971923828125, 0.4092667996883392333984375, 0.17074333131313323974609375, -0.15163862705230712890625, -0.24580220878124237060546875, 0.06288687884807586669921875, 0.0539300739765167236328125, -0.0012122131884098052978515625, -0.465528666973114013671875, -0.1602188050746917724609375, -0.02701481617987155914306640625, -0.189342558383941650390625, -0.2792322337627410888671875, -0.00308768451213836669921875));
      float _463 = 0.0f;
      float _464 = 1.0f;
      _62 += mul(_16(_463, _464), float4x4(0.11187088489532470703125, -0.0510413087904453277587890625, 0.054937921464443206787109375, 0.49054706096649169921875, 0.0331714488565921783447265625, 0.13979794085025787353515625, -0.1497578322887420654296875, 0.1828818619251251220703125, 0.141552984714508056640625, 0.0189668349921703338623046875, -0.102932371199131011962890625, -0.2692582607269287109375, 0.20328505337238311767578125, -0.072385109961032867431640625, -0.21985305845737457275390625, 0.3584593236446380615234375));
      float _490 = 1.0f;
      float _491 = -1.0f;
      _62 += mul(_16(_490, _491), float4x4(0.3747799098491668701171875, -0.174211680889129638671875, -0.20580358803272247314453125, 0.2835056781768798828125, -0.09674848616123199462890625, -0.01846181787550449371337890625, 0.5618965625762939453125, -0.214860141277313232421875, -0.1713974177837371826171875, 0.097471617162227630615234375, 0.17151354253292083740234375, -0.4433092772960662841796875, 0.2528985440731048583984375, 0.01660534925758838653564453125, -0.26347768306732177734375, -0.008981036953628063201904296875));
      float _517 = 1.0f;
      float _518 = 0.0f;
      _62 += mul(_16(_517, _518), float4x4(0.101699851453304290771484375, -0.094687856733798980712890625, 0.106301121413707733154296875, 0.21764881908893585205078125, -0.18244017660617828369140625, -0.02421847544610500335693359375, 0.367717802524566650390625, 0.078915797173976898193359375, 0.0476073585450649261474609375, 0.103733874857425689697265625, -0.104170955717563629150390625, -0.22041337192058563232421875, 0.4101764261722564697265625, -0.22540338337421417236328125, 0.0573174469172954559326171875, 0.15065215528011322021484375));
      float _544 = 1.0f;
      float _545 = 1.0f;
      _62 += mul(_16(_544, _545), float4x4(0.1163399517536163330078125, 0.058413274586200714111328125, 0.13760869204998016357421875, -0.08733968436717987060546875, -0.008195114322006702423095703125, 0.055995367467403411865234375, 0.04031978547573089599609375, 0.14120729267597198486328125, -0.1450153291225433349609375, 0.09362144768238067626953125, 0.0388950444757938385009765625, -0.17166458070278167724609375, 0.071680247783660888671875, -0.13827963173389434814453125, 0.2675252854824066162109375, -0.23129940032958984375));
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
      float4 _62 = mul(_12(_86, _87), float4x4(-0.290129840373992919921875, -0.0502898655831813812255859375, 0.0603073872625827789306640625, -0.07999418675899505615234375, -0.1315014660358428955078125, 0.14845313131809234619140625, -0.041604518890380859375, 0.118182837963104248046875, 0.3101561367511749267578125, -0.096088983118534088134765625, 0.035932682454586029052734375, -0.2751228809356689453125, 0.059922911226749420166015625, 0.2791330814361572265625, -0.0813756287097930908203125, 0.21948812901973724365234375));
      float _111 = -1.0f;
      float _112 = 0.0f;
      _62 += mul(_12(_111, _112), float4x4(0.129160583019256591796875, 0.0534702427685260772705078125, -0.016891010105609893798828125, -0.2069201171398162841796875, -0.21759961545467376708984375, 0.141242504119873046875, -0.2623834908008575439453125, -0.167786300182342529296875, -0.338685333728790283203125, 0.0433953963220119476318359375, 0.010809152387082576751708984375, -0.2331385910511016845703125, 0.0216366611421108245849609375, -0.2675105631351470947265625, 0.06296281516551971435546875, -0.17402614653110504150390625));
      float _139 = -1.0f;
      float _140 = 1.0f;
      _62 += mul(_12(_139, _140), float4x4(-0.082041122019290924072265625, -0.0566929243505001068115234375, 0.14137582480907440185546875, 0.15734316408634185791015625, -0.2367208302021026611328125, -0.02708657085895538330078125, 0.15404348075389862060546875, 0.1656242311000823974609375, -0.0064437394030392169952392578125, 0.12536962330341339111328125, -0.105753876268863677978515625, -0.010160828940570354461669921875, -0.13200695812702178955078125, 0.0044289189390838146209716796875, 0.0479574538767337799072265625, -0.06602983176708221435546875));
      float _166 = 0.0f;
      float _167 = -1.0f;
      _62 += mul(_12(_166, _167), float4x4(0.02565399743616580963134765625, -0.360051929950714111328125, 0.4663994014263153076171875, -0.3577422797679901123046875, -0.10877774655818939208984375, 0.18163569271564483642578125, 0.006518651731312274932861328125, -0.04136605560779571533203125, -0.3125890791416168212890625, -0.345376431941986083984375, 0.08109033107757568359375, -0.3785277307033538818359375, 0.18841636180877685546875, -0.074108697474002838134765625, 0.2976773083209991455078125, 0.0505656562745571136474609375));
      float _193 = 0.0f;
      float _194 = 0.0f;
      _62 += mul(_12(_193, _194), float4x4(0.0439231283962726593017578125, -0.16512739658355712890625, -0.115393899381160736083984375, -0.068682558834552764892578125, 0.11316680908203125, -0.565620899200439453125, 0.1682985126972198486328125, -0.5693595409393310546875, -0.1442138850688934326171875, -0.124100483953952789306640625, 0.202561199665069580078125, -0.122279606759548187255859375, 0.1798566877841949462890625, 0.4277405440807342529296875, 0.05400745570659637451171875, 0.1768886148929595947265625));
      float _220 = 0.0f;
      float _221 = 1.0f;
      _62 += mul(_12(_220, _221), float4x4(0.3404099941253662109375, -0.2732667028903961181640625, 0.26090228557586669921875, 0.04928840696811676025390625, 0.499000012874603271484375, -0.049950934946537017822265625, 0.0164384543895721435546875, -0.3112630546092987060546875, 0.15234196186065673828125, 0.03550811111927032470703125, -0.2987463176250457763671875, 0.0292355120182037353515625, 0.21353457868099212646484375, -0.210516870021820068359375, 0.379941284656524658203125, -0.0122560150921344757080078125));
      float _247 = 1.0f;
      float _248 = -1.0f;
      _62 += mul(_12(_247, _248), float4x4(-0.0046853204257786273956298828125, -0.08137620985507965087890625, -0.066420383751392364501953125, -0.062607727944850921630859375, 0.1539137363433837890625, 0.359055578708648681640625, 0.02960065566003322601318359375, 0.04634220898151397705078125, -0.04068966209888458251953125, 0.2373384535312652587890625, -0.3142104446887969970703125, -0.1094849109649658203125, 0.20186872780323028564453125, 0.2179479300975799560546875, -0.0507738627493381500244140625, -0.0454989336431026458740234375));
      float _274 = 1.0f;
      float _275 = 0.0f;
      _62 += mul(_12(_274, _275), float4x4(-0.082952998578548431396484375, 0.275063991546630859375, -0.4382666051387786865234375, 0.56957280635833740234375, -0.02583706378936767578125, 0.0779361724853515625, -0.293218195438385009765625, 0.20719237625598907470703125, -0.099283032119274139404296875, 0.22240887582302093505859375, -0.272431671619415283203125, 0.557592689990997314453125, -0.14300231635570526123046875, 0.06637834012508392333984375, -0.1422118246555328369140625, 0.4081688225269317626953125));
      float _301 = 1.0f;
      float _302 = 1.0f;
      _62 += mul(_12(_301, _302), float4x4(-0.1851092875003814697265625, 0.01638700067996978759765625, -0.2898727357387542724609375, 0.2954347431659698486328125, -0.1505216658115386962890625, 0.2031003534793853759765625, -0.119426049292087554931640625, -0.0428309030830860137939453125, 0.2527721226215362548828125, 0.29032289981842041015625, 0.013498960994184017181396484375, -0.018111206591129302978515625, 0.068044610321521759033203125, -0.061587698757648468017578125, 0.3184151947498321533203125, -0.13263674080371856689453125));
      float _328 = -1.0f;
      float _329 = -1.0f;
      _62 += mul(_16(_328, _329), float4x4(0.2574908733367919921875, -0.094091184437274932861328125, -0.06290300190448760986328125, -0.073379933834075927734375, 0.0053866603411734104156494140625, -0.074196331202983856201171875, -0.02042240090668201446533203125, 0.0522019863128662109375, -0.0939116179943084716796875, 0.001385861076414585113525390625, -0.121133126318454742431640625, 0.3586457669734954833984375, -0.0612952895462512969970703125, 0.012000353075563907623291015625, 0.01794255711138248443603515625, 0.02356440387666225433349609375));
      float _355 = -1.0f;
      float _356 = 0.0f;
      _62 += mul(_16(_355, _356), float4x4(0.100115694105625152587890625, -0.1247077882289886474609375, -0.151593387126922607421875, 0.1973570287227630615234375, 0.1945135891437530517578125, 0.0027281935326755046844482421875, 0.18457151949405670166015625, 0.0732674300670623779296875, 0.23252093791961669921875, -0.1748857200145721435546875, 0.05771298706531524658203125, -0.28563106060028076171875, 0.1950680911540985107421875, -0.0187219642102718353271484375, -0.08191494643688201904296875, 0.016428150236606597900390625));
      float _382 = -1.0f;
      float _383 = 1.0f;
      _62 += mul(_16(_382, _383), float4x4(0.0680625140666961669921875, 0.2872502505779266357421875, -0.16676305234432220458984375, 0.084470875561237335205078125, 0.283566653728485107421875, -0.130452930927276611328125, -0.2555944919586181640625, 0.06460686028003692626953125, 0.073778979480266571044921875, -0.17525704205036163330078125, -0.100784219801425933837890625, 0.138243615627288818359375, 0.427769720554351806640625, -0.0588559098541736602783203125, -0.05303287506103515625, -0.052313528954982757568359375));
      float _409 = 0.0f;
      float _410 = -1.0f;
      _62 += mul(_16(_409, _410), float4x4(0.2263782918453216552734375, 0.0380170531570911407470703125, -0.38211119174957275390625, 0.224161446094512939453125, -0.0289692543447017669677734375, -0.008854481391608715057373046875, 0.11085270345211029052734375, -0.03149211406707763671875, 0.19682539999485015869140625, -0.20316390693187713623046875, -0.110299326479434967041015625, -0.1914430558681488037109375, -0.13331995904445648193359375, 0.092370890080928802490234375, -0.245420277118682861328125, -0.099627099931240081787109375));
      float _436 = 0.0f;
      float _437 = 0.0f;
      _62 += mul(_16(_436, _437), float4x4(0.107767440378665924072265625, -0.06642015278339385986328125, 0.12506575882434844970703125, 0.01706348918378353118896484375, 0.1636344492435455322265625, 0.56165492534637451171875, -0.1532903611660003662109375, 0.181370198726654052734375, 0.1465650498867034912109375, -0.008412252180278301239013671875, 0.03753824532032012939453125, 0.03565178811550140380859375, -0.3737814128398895263671875, -0.3726684749126434326171875, -0.1081025898456573486328125, -0.012786579318344593048095703125));
      float _463 = 0.0f;
      float _464 = 1.0f;
      _62 += mul(_16(_463, _464), float4x4(-0.402333796024322509765625, 0.2610736191272735595703125, -0.399586021900177001953125, 0.3617881834506988525390625, -0.20986139774322509765625, 0.041306912899017333984375, -0.2122933864593505859375, 0.2093491256237030029296875, -0.18285121023654937744140625, -0.0365155041217803955078125, -0.021053291857242584228515625, 0.15008519589900970458984375, -0.02727653086185455322265625, -0.045217297971248626708984375, -0.134275019168853759765625, 0.26345539093017578125));
      float _490 = 1.0f;
      float _491 = -1.0f;
      _62 += mul(_16(_490, _491), float4x4(0.07794611155986785888671875, 0.094220124185085296630859375, -0.080684490501880645751953125, 0.23963861167430877685546875, -0.259375870227813720703125, 0.2158884704113006591796875, -0.3136669695377349853515625, 0.137155354022979736328125, -0.068225286900997161865234375, -0.0455217994749546051025390625, 0.07799637317657470703125, 0.01032934524118900299072265625, -0.0563361346721649169921875, -0.10968329012393951416015625, 0.24252681434154510498046875, 0.0909430086612701416015625));
      float _517 = 1.0f;
      float _518 = 0.0f;
      _62 += mul(_16(_517, _518), float4x4(-0.2097571790218353271484375, -0.071530677378177642822265625, 0.3720407187938690185546875, 0.25719821453094482421875, -0.12550137937068939208984375, 0.32499980926513671875, 0.170183360576629638671875, -0.2725863158702850341796875, 0.14453573524951934814453125, -0.056577377021312713623046875, 0.375289499759674072265625, -0.2597100436687469482421875, -0.00208786316215991973876953125, 0.18166828155517578125, 0.321785867214202880859375, -0.4053600728511810302734375));
      float _544 = 1.0f;
      float _545 = 1.0f;
      _62 += mul(_16(_544, _545), float4x4(-0.324390709400177001953125, 0.14906860888004302978515625, 0.12964856624603271484375, 0.0580797083675861358642578125, -0.06300620734691619873046875, 0.0615377835929393768310546875, 0.09979093074798583984375, -0.0563712455332279205322265625, -0.093984358012676239013671875, -0.055284477770328521728515625, -0.18101589381694793701171875, 0.080725543200969696044921875, -0.1954918801784515380859375, 0.112817279994487762451171875, -0.4104282855987548828125, 0.184790074825286865234375));
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
      float4 _62 = mul(_12(_86, _87), float4x4(0.15332128107547760009765625, 0.1702123582363128662109375, 0.518261849880218505859375, 0.1990320980548858642578125, 0.0272582583129405975341796875, -0.510460436344146728515625, -0.34817993640899658203125, -0.04997922480106353759765625, 0.1490050256252288818359375, -0.1528727114200592041015625, 0.004513166844844818115234375, 0.113919891417026519775390625, -0.1598279476165771484375, -0.0581673271954059600830078125, 0.0539576895534992218017578125, -0.16062729060649871826171875));
      float _111 = -1.0f;
      float _112 = 0.0f;
      _62 += mul(_12(_111, _112), float4x4(0.033682905137538909912109375, 0.258576810359954833984375, -0.11462070047855377197265625, 0.254976928234100341796875, 0.01972888596355915069580078125, -0.21245719492435455322265625, -0.2396624982357025146484375, 0.11692859232425689697265625, 0.19931755959987640380859375, -0.014632458798587322235107421875, 0.089602768421173095703125, -0.14207516610622406005859375, 0.1738192737102508544921875, 0.3977989256381988525390625, 0.3834529817104339599609375, 0.12667973339557647705078125));
      float _139 = -1.0f;
      float _140 = 1.0f;
      _62 += mul(_12(_139, _140), float4x4(-0.1491125524044036865234375, 0.24204038083553314208984375, -0.002135685645043849945068359375, 0.0451775826513767242431640625, 0.089107058942317962646484375, -0.0360714904963970184326171875, 0.008858780376613140106201171875, 0.111206062138080596923828125, 0.16136817634105682373046875, -0.4571109116077423095703125, 0.22297303378582000732421875, -0.00997190363705158233642578125, 0.0391456596553325653076171875, 0.10802461206912994384765625, 0.2367230951786041259765625, -0.05926239490509033203125));
      float _166 = 0.0f;
      float _167 = -1.0f;
      _62 += mul(_12(_166, _167), float4x4(0.24565999209880828857421875, -0.109230518341064453125, 0.3544572889804840087890625, 0.1876021921634674072265625, -0.22613839805126190185546875, 0.0390273146331310272216796875, -0.546857774257659912109375, -0.1908200085163116455078125, 0.473732054233551025390625, -0.4270740449428558349609375, -0.27599155902862548828125, 0.03056546859443187713623046875, 0.02461341209709644317626953125, -0.3783372938632965087890625, -0.09455917775630950927734375, 0.20589156448841094970703125));
      float _193 = 0.0f;
      float _194 = 0.0f;
      _62 += mul(_12(_193, _194), float4x4(0.197319805622100830078125, 0.18195949494838714599609375, -0.0426320470869541168212890625, -0.1956300437450408935546875, -0.034338630735874176025390625, -0.14460869133472442626953125, -0.118429668247699737548828125, 0.02742596901953220367431640625, 0.05996048450469970703125, 0.1286174952983856201171875, -0.1122444570064544677734375, 0.24056376516819000244140625, 0.04564286768436431884765625, 0.20675750076770782470703125, -0.187647759914398193359375, 0.594964921474456787109375));
      float _220 = 0.0f;
      float _221 = 1.0f;
      _62 += mul(_12(_220, _221), float4x4(0.0550276823341846466064453125, 0.4588985145092010498046875, -0.001210132963024079799652099609375, -0.049561135470867156982421875, 0.163315951824188232421875, 0.0364290885627269744873046875, -0.05765141546726226806640625, 0.275098860263824462890625, -0.2608588039875030517578125, 0.22187738120555877685546875, -0.0611990429461002349853515625, 0.13778673112392425537109375, 0.12545955181121826171875, 0.451907336711883544921875, 0.119354762136936187744140625, -0.12491403520107269287109375));
      float _247 = 1.0f;
      float _248 = -1.0f;
      _62 += mul(_12(_247, _248), float4x4(-0.02257459051907062530517578125, 0.0598237402737140655517578125, -0.101555280387401580810546875, 0.329570710659027099609375, 0.2770510613918304443359375, -0.2824302017688751220703125, 0.16182267665863037109375, -0.50616395473480224609375, 0.044165275990962982177734375, 0.3171142041683197021484375, -0.091831468045711517333984375, -0.03696404397487640380859375, -0.2652123272418975830078125, 0.084305606782436370849609375, -0.19447176158428192138671875, 0.23166708648204803466796875));
      float _274 = 1.0f;
      float _275 = 0.0f;
      _62 += mul(_12(_274, _275), float4x4(-0.0232341997325420379638671875, -0.10830597579479217529296875, -0.2648853361606597900390625, -0.3309468328952789306640625, 0.072997987270355224609375, 0.15024791657924652099609375, 0.19481427967548370361328125, 0.2415511608123779296875, -0.1803807914257049560546875, -0.1953192651271820068359375, 0.10737945139408111572265625, -0.0985033214092254638671875, -0.13672702014446258544921875, 0.08709789812564849853515625, -0.14573483169078826904296875, 0.2797003090381622314453125));
      float _301 = 1.0f;
      float _302 = 1.0f;
      _62 += mul(_12(_301, _302), float4x4(-0.24089853465557098388671875, 0.36212956905364990234375, -0.077959708869457244873046875, -0.2589303553104400634765625, 0.1950659453868865966796875, -0.44844806194305419921875, -0.00338619272224605083465576171875, 0.23793478310108184814453125, 0.4799155890941619873046875, 0.23864488303661346435546875, -0.1121616363525390625, -0.1576942503452301025390625, -0.0583131127059459686279296875, 0.15477742254734039306640625, 0.0334545634686946868896484375, -0.00033481256105005741119384765625));
      float _328 = -1.0f;
      float _329 = -1.0f;
      _62 += mul(_16(_328, _329), float4x4(0.0577250681817531585693359375, -0.0243999660015106201171875, 0.00036956605617888271808624267578125, 0.02665150165557861328125, -0.16402530670166015625, 0.14966167509555816650390625, -0.24236615002155303955078125, 0.3901919424533843994140625, -0.13499663770198822021484375, -0.090857334434986114501953125, -0.053542695939540863037109375, -0.2742246091365814208984375, -0.2046035826206207275390625, -0.03967775404453277587890625, -0.004954411648213863372802734375, -0.0612423233687877655029296875));
      float _355 = -1.0f;
      float _356 = 0.0f;
      _62 += mul(_16(_355, _356), float4x4(-0.01632327400147914886474609375, -0.00016685205628164112567901611328125, 0.13242749869823455810546875, -0.027705170214176177978515625, -0.036179907619953155517578125, -0.29573023319244384765625, -0.18442131578922271728515625, 0.2845299541950225830078125, 0.0299659185111522674560546875, 0.17996422946453094482421875, -0.2461815178394317626953125, 0.3980409801006317138671875, 0.11151491105556488037109375, -0.2014543712139129638671875, 0.0617804266512393951416015625, -0.11743889749050140380859375));
      float _382 = -1.0f;
      float _383 = 1.0f;
      _62 += mul(_16(_382, _383), float4x4(-0.02506884746253490447998046875, -0.098662041127681732177734375, -0.13319958746433258056640625, -0.050626449286937713623046875, -0.053328387439250946044921875, 0.0576772131025791168212890625, -0.144111812114715576171875, -0.036771543323993682861328125, -0.270537853240966796875, 0.01850111968815326690673828125, -0.2635524272918701171875, 0.132944166660308837890625, 0.2686645686626434326171875, -0.18014706671237945556640625, -0.022209353744983673095703125, -0.1845855712890625));
      float _409 = 0.0f;
      float _410 = -1.0f;
      _62 += mul(_16(_409, _410), float4x4(-0.046194963157176971435546875, 0.110311232507228851318359375, -0.5279924869537353515625, 0.3159375488758087158203125, 0.0382304377853870391845703125, -0.165049076080322265625, 0.126866817474365234375, 0.0273280926048755645751953125, -0.08993043005466461181640625, -0.095170356333255767822265625, -0.057261250913143157958984375, 0.001839602016843855381011962890625, -0.07236354053020477294921875, -0.16459833085536956787109375, 0.0553616769611835479736328125, 0.3058166205883026123046875));
      float _436 = 0.0f;
      float _437 = 0.0f;
      _62 += mul(_16(_436, _437), float4x4(0.0860867798328399658203125, -0.12689830362796783447265625, 0.018839336931705474853515625, -0.08723390102386474609375, 0.0316843688488006591796875, 0.1339586079120635986328125, -0.0498210750520229339599609375, 0.4709666669368743896484375, 0.0077133770100772380828857421875, -0.069848835468292236328125, -0.21461345255374908447265625, 0.0225125066936016082763671875, -0.2614029347896575927734375, -0.24080403149127960205078125, -0.1416830122470855712890625, 0.14860631525516510009765625));
      float _463 = 0.0f;
      float _464 = 1.0f;
      _62 += mul(_16(_463, _464), float4x4(0.062936730682849884033203125, 0.18227446079254150390625, -0.01219026930630207061767578125, -0.385364711284637451171875, 0.22462968528270721435546875, -0.2956554889678955078125, 0.241982996463775634765625, 0.108171097934246063232421875, 0.0454949848353862762451171875, 0.080105431377887725830078125, -0.046537093818187713623046875, -0.16926057636737823486328125, 0.02167354337871074676513671875, -0.01919729076325893402099609375, -0.4009456634521484375, 0.16138376295566558837890625));
      float _490 = 1.0f;
      float _491 = -1.0f;
      _62 += mul(_16(_490, _491), float4x4(-0.1485458910465240478515625, 0.09997196495532989501953125, 0.05435852706432342529296875, -0.16914124786853790283203125, -0.17625804245471954345703125, 0.139015734195709228515625, -0.103517048060894012451171875, 0.12729422748088836669921875, -0.10849075019359588623046875, 0.2946414649486541748046875, -0.00629142858088016510009765625, -0.183774530887603759765625, 0.22154299914836883544921875, 0.02006852626800537109375, 0.24127025902271270751953125, -0.645237505435943603515625));
      float _517 = 1.0f;
      float _518 = 0.0f;
      _62 += mul(_16(_517, _518), float4x4(0.1260339319705963134765625, -0.13619254529476165771484375, 0.19077192246913909912109375, -0.015576320700347423553466796875, -0.1098609268665313720703125, -0.093490727245807647705078125, 0.0525007955729961395263671875, 0.082549072802066802978515625, 0.2314102947711944580078125, 0.20594225823879241943359375, 0.071856446564197540283203125, -0.550174295902252197265625, 0.1691504418849945068359375, -0.34507083892822265625, 0.029082737863063812255859375, -0.3849584758281707763671875));
      float _544 = 1.0f;
      float _545 = 1.0f;
      _62 += mul(_16(_544, _545), float4x4(0.093007959425449371337890625, 0.06321121752262115478515625, 0.0969714820384979248046875, 0.18379296362400054931640625, -0.079218305647373199462890625, 0.16234867274761199951171875, 0.2345752418041229248046875, 0.17770062386989593505859375, 0.46825134754180908203125, 0.042932413518428802490234375, 0.194174826145172119140625, -0.0502349995076656341552734375, -0.087356247007846832275390625, -0.0130574218928813934326171875, -0.16804663836956024169921875, -0.05967660248279571533203125));
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
      float4 _62 = mul(_12(_86, _87), float4x4(-0.22753362357616424560546875, -0.18788953125476837158203125, 0.05455936491489410400390625, 0.14884404838085174560546875, -0.086120732128620147705078125, -0.0565791167318820953369140625, 0.1503159701824188232421875, -0.069429099559783935546875, 0.3314069211483001708984375, -0.12905196845531463623046875, -0.134303629398345947265625, 0.2614941298961639404296875, 0.08699528872966766357421875, -0.066946208477020263671875, 0.02164602465927600860595703125, 0.112705029547214508056640625));
      float _111 = -1.0f;
      float _112 = 0.0f;
      _62 += mul(_12(_111, _112), float4x4(0.178767621517181396484375, 0.131718695163726806640625, 0.287607371807098388671875, -0.16811855137348175048828125, -0.096378482878208160400390625, -0.0361626856029033660888671875, -0.1250514090061187744140625, -0.1634070873260498046875, 0.112853229045867919921875, 0.17958368360996246337890625, 0.12760694324970245361328125, 0.13278298079967498779296875, 0.20048929750919342041015625, -0.069624997675418853759765625, 0.0477179549634456634521484375, -0.0840395390987396240234375));
      float _139 = -1.0f;
      float _140 = 1.0f;
      _62 += mul(_12(_139, _140), float4x4(-0.21917523443698883056640625, 0.03001488931477069854736328125, -0.013841082341969013214111328125, -0.22184245288372039794921875, 0.079711854457855224609375, -0.0147729180753231048583984375, 0.17034237086772918701171875, -0.59067356586456298828125, -0.2864253520965576171875, -0.3487395942211151123046875, 0.108102820813655853271484375, 0.441133975982666015625, 0.2822416126728057861328125, 0.105971448123455047607421875, -0.080896951258182525634765625, 0.13045649230480194091796875));
      float _166 = 0.0f;
      float _167 = -1.0f;
      _62 += mul(_12(_166, _167), float4x4(-0.2990693151950836181640625, -0.13953633606433868408203125, 0.10563714802265167236328125, -0.37824213504791259765625, 0.01392374932765960693359375, 0.080034546554088592529296875, 0.310331165790557861328125, -0.14506383240222930908203125, 0.2031123936176300048828125, -0.101644940674304962158203125, -0.0759035050868988037109375, 0.11866700649261474609375, -0.118466876447200775146484375, -0.2121855914592742919921875, 0.0473109073936939239501953125, -0.2138448655605316162109375));
      float _193 = 0.0f;
      float _194 = 0.0f;
      _62 += mul(_12(_193, _194), float4x4(-0.13538490235805511474609375, 0.2724498212337493896484375, 0.18514315783977508544921875, -0.0577326230704784393310546875, 0.19258606433868408203125, 0.16653059422969818115234375, -0.17840464413166046142578125, 0.421667039394378662109375, 0.063908584415912628173828125, -0.2935789525508880615234375, 0.20986096560955047607421875, -0.231820642948150634765625, -0.20437879860401153564453125, -0.22441709041595458984375, 0.1435105502605438232421875, -0.49572479724884033203125));
      float _220 = 0.0f;
      float _221 = 1.0f;
      _62 += mul(_12(_220, _221), float4x4(-0.3483012616634368896484375, -0.122909180819988250732421875, 0.230472624301910400390625, 0.30666649341583251953125, 0.1090667545795440673828125, 0.0429165102541446685791015625, 0.093989737331867218017578125, -0.540769994258880615234375, -0.282858669757843017578125, -0.0474841855466365814208984375, 0.02246710844337940216064453125, 0.0577718727290630340576171875, -0.048280067741870880126953125, -0.037025950849056243896484375, 0.08271034061908721923828125, 0.231940925121307373046875));
      float _247 = 1.0f;
      float _248 = -1.0f;
      _62 += mul(_12(_247, _248), float4x4(-0.17731948196887969970703125, -0.16433562338352203369140625, -0.14827461540699005126953125, 0.1698997914791107177734375, -0.3175927102565765380859375, -0.0183365307748317718505859375, 0.18544113636016845703125, -0.20985202491283416748046875, 0.14527280628681182861328125, -0.22345604002475738525390625, -0.1554412543773651123046875, 0.163915336132049560546875, 0.093967862427234649658203125, -0.0416119284927845001220703125, -0.061790071427822113037109375, -0.0944726765155792236328125));
      float _274 = 1.0f;
      float _275 = 0.0f;
      _62 += mul(_12(_274, _275), float4x4(-0.0538788624107837677001953125, 0.3164721429347991943359375, -0.21446131169795989990234375, 0.1210909783840179443359375, -0.210346162319183349609375, 0.012653477489948272705078125, 0.067189045250415802001953125, 0.22009392082691192626953125, 0.023831523954868316650390625, -0.191308438777923583984375, 0.091174490749835968017578125, -0.392466485500335693359375, 0.19772215187549591064453125, -0.0492821075022220611572265625, -0.255487740039825439453125, -0.13340388238430023193359375));
      float _301 = 1.0f;
      float _302 = 1.0f;
      _62 += mul(_12(_301, _302), float4x4(-0.1609668433666229248046875, -0.00183497997932136058807373046875, 0.043269135057926177978515625, 0.115557633340358734130859375, -0.18495404720306396484375, -0.044303037226200103759765625, 0.069244809448719024658203125, -0.20292861759662628173828125, 0.10410177707672119140625, -0.062745355069637298583984375, -0.21367405354976654052734375, 0.57995569705963134765625, 0.001567303319461643695831298828125, -0.090802393853664398193359375, -0.14619028568267822265625, 0.14739845693111419677734375));
      float _328 = -1.0f;
      float _329 = -1.0f;
      _62 += mul(_16(_328, _329), float4x4(-0.210302770137786865234375, 0.12995781004428863525390625, 0.15550352632999420166015625, -0.076946206390857696533203125, -0.09578801691532135009765625, 0.404310524463653564453125, -0.0440230108797550201416015625, -0.0535230748355388641357421875, 0.013482288457453250885009765625, -0.3347856104373931884765625, 0.4603779017925262451171875, -0.1960732638835906982421875, -0.2148433625698089599609375, -0.18183486163616180419921875, 0.148743569850921630859375, -0.108507417142391204833984375));
      float _355 = -1.0f;
      float _356 = 0.0f;
      _62 += mul(_16(_355, _356), float4x4(-0.2347210943698883056640625, 0.1723145544528961181640625, -0.23357500135898590087890625, 0.02293519861996173858642578125, 0.2697403132915496826171875, 0.24999184906482696533203125, 0.529503643512725830078125, 0.19369156658649444580078125, -0.063479401171207427978515625, -0.520853579044342041015625, 0.003806318156421184539794921875, 0.1458655297756195068359375, -0.1792598664760589599609375, -0.104918278753757476806640625, -0.1380037963390350341796875, 0.1938703954219818115234375));
      float _382 = -1.0f;
      float _383 = 1.0f;
      _62 += mul(_16(_382, _383), float4x4(-0.10245223343372344970703125, 0.559777081012725830078125, 0.17046789824962615966796875, -0.05699561536312103271484375, 0.3415019214153289794921875, 0.1145108640193939208984375, -0.23335956037044525146484375, 0.24153493344783782958984375, 0.25862157344818115234375, -0.1225265562534332275390625, -0.16771887242794036865234375, -0.080824293196201324462890625, -0.201655089855194091796875, -0.0401097498834133148193359375, -0.0378345511853694915771484375, -0.2421093285083770751953125));
      float _409 = 0.0f;
      float _410 = -1.0f;
      _62 += mul(_16(_409, _410), float4x4(-0.10346652567386627197265625, 0.103505425155162811279296875, -0.091803111135959625244140625, 0.0752877891063690185546875, 0.1527834832668304443359375, 0.1586279571056365966796875, -0.12505088746547698974609375, -0.096360862255096435546875, -0.3052616417407989501953125, 0.1469652354717254638671875, 0.2805254161357879638671875, -0.103696167469024658203125, -0.08075569570064544677734375, -0.00835807621479034423828125, -0.135515630245208740234375, 0.23656134307384490966796875));
      float _436 = 0.0f;
      float _437 = 0.0f;
      _62 += mul(_16(_436, _437), float4x4(-0.257528364658355712890625, 0.02350901626050472259521484375, 0.088015384972095489501953125, 0.180962979793548583984375, 0.09943975508213043212890625, 0.23106367886066436767578125, 0.2699559628963470458984375, -0.100688554346561431884765625, -0.3071634769439697265625, 0.0527712516486644744873046875, 0.13906450569629669189453125, 0.549204885959625244140625, 0.0350777246057987213134765625, 0.3491046428680419921875, -0.40671825408935546875, 0.2482101023197174072265625));
      float _463 = 0.0f;
      float _464 = 1.0f;
      _62 += mul(_16(_463, _464), float4x4(0.414117753505706787109375, 0.2713774740695953369140625, -0.0316612087190151214599609375, -0.36956536769866943359375, -0.107200555503368377685546875, 0.0631361901760101318359375, -0.341568291187286376953125, 0.17912900447845458984375, -0.138134777545928955078125, -0.085229672491550445556640625, -0.522419989109039306640625, -0.09742935001850128173828125, 0.13768874108791351318359375, 0.0321830213069915771484375, -0.17418129742145538330078125, -0.1169661581516265869140625));
      float _490 = 1.0f;
      float _491 = -1.0f;
      _62 += mul(_16(_490, _491), float4x4(-0.0797550380229949951171875, 0.14309953153133392333984375, 0.3461274802684783935546875, 0.185225188732147216796875, 0.17964838445186614990234375, 0.29473078250885009765625, -0.3387472927570343017578125, -0.21297298371791839599609375, 0.3712253272533416748046875, 0.09263910353183746337890625, 0.0077308523468673229217529296875, 0.1149397790431976318359375, 0.16064764559268951416015625, -0.22333665192127227783203125, -0.07239449024200439453125, 0.1611781418323516845703125));
      float _517 = 1.0f;
      float _518 = 0.0f;
      _62 += mul(_16(_517, _518), float4x4(-0.17402778565883636474609375, 0.18713302910327911376953125, -0.2010295093059539794921875, -0.13732676208019256591796875, 0.1002314388751983642578125, 0.087362952530384063720703125, -0.010721134953200817108154296875, -0.4025804698467254638671875, 0.117122061550617218017578125, 0.013007052242755889892578125, -0.2562521994113922119140625, 0.25824391841888427734375, 0.0319717340171337127685546875, -0.069431386888027191162109375, 0.3487745821475982666015625, 0.1572063863277435302734375));
      float _544 = 1.0f;
      float _545 = 1.0f;
      _62 += mul(_16(_544, _545), float4x4(0.0444943048059940338134765625, 0.38839244842529296875, -0.562663733959197998046875, -0.18491415679454803466796875, 0.3296107947826385498046875, 0.400158584117889404296875, 0.251377999782562255859375, -0.0468869991600513458251953125, 0.00176038523204624652862548828125, -0.1339519917964935302734375, 0.50057888031005859375, 0.06779767572879791259765625, 0.0936228930950164794921875, -0.04452185332775115966796875, -0.1310605704784393310546875, -0.14694957435131072998046875));
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
      float4 _9 = mul(max(u_Texture1.Sample(u_Texture1_sampler, uv), 0.0f.xxxx), float4x4(-0.088371627032756805419921875, 0.02140550129115581512451171875, 0.053288631141185760498046875, -0.12216047942638397216796875, -0.06523473560810089111328125, 0.013663728721439838409423828125, 0.035803340375423431396484375, 0.02254789136350154876708984375, -0.03470431268215179443359375, 0.01924959383904933929443359375, 0.0464575923979282379150390625, 0.0164008252322673797607421875, 0.0, 0.0, 0.0, 0.0));
      _9 += mul(max(-u_Texture1.Sample(u_Texture1_sampler, uv), 0.0f.xxxx), float4x4(0.0619964636862277984619140625, -0.005013109184801578521728515625, 0.01648160256445407867431640625, 0.02003588713705539703369140625, 0.0563146583735942840576171875, -0.0044589997269213199615478515625, 0.1372105777263641357421875, -0.072500027716159820556640625, 0.068084068596363067626953125, -0.0323677957057952880859375, 0.1492464840412139892578125, -0.08034037053585052490234375, 0.0, 0.0, 0.0, 0.0));
      _9 += mul(max(u_Texture2.Sample(u_Texture2_sampler, uv), 0.0f.xxxx), float4x4(0.24078513681888580322265625, -0.009353794157505035400390625, -0.1407109797000885009765625, -0.1489841938018798828125, 0.081361524760723114013671875, -0.0510771162807941436767578125, 0.01035965979099273681640625, -0.067118167877197265625, 0.05342070758342742919921875, -0.0580077469348907470703125, 0.0053089489229023456573486328125, -0.055529259145259857177734375, 0.0, 0.0, 0.0, 0.0));
      _9 += mul(max(-u_Texture2.Sample(u_Texture2_sampler, uv), 0.0f.xxxx), float4x4(-0.1300237476825714111328125, 0.177674829959869384765625, 0.12804912030696868896484375, 0.170445144176483154296875, 0.012733756564557552337646484375, 0.20204603672027587890625, 0.073814533650875091552734375, 0.07301451265811920166015625, 0.0178219862282276153564453125, 0.1751779019832611083984375, 0.056559108197689056396484375, 0.065239779651165008544921875, 0.0, 0.0, 0.0, 0.0));
      _9 += mul(max(u_Texture3.Sample(u_Texture3_sampler, uv), 0.0f.xxxx), float4x4(-0.1170985996723175048828125, -0.16645707190036773681640625, -0.041431181132793426513671875, -0.084318704903125762939453125, -0.0513037107884883880615234375, -0.121526904404163360595703125, 0.02669376693665981292724609375, -0.064990036189556121826171875, -0.02793991379439830780029296875, -0.0947136580944061279296875, 0.0346154458820819854736328125, -0.054324172437191009521484375, 0.0, 0.0, 0.0, 0.0));
      _9 += mul(max(-u_Texture3.Sample(u_Texture3_sampler, uv), 0.0f.xxxx), float4x4(0.120945237576961517333984375, 0.0622163824737071990966796875, 0.07279710471630096435546875, 0.1207190454006195068359375, 0.0951840877532958984375, 0.053228355944156646728515625, 0.02625816501677036285400390625, 0.07328115403652191162109375, 0.07387219369411468505859375, 0.0313723348081111907958984375, 0.009804672561585903167724609375, 0.056623302400112152099609375, 0.0, 0.0, 0.0, 0.0));
      _9 += mul(max(u_Texture4.Sample(u_Texture4_sampler, uv), 0.0f.xxxx), float4x4(-0.111414946615695953369140625, -0.065189503133296966552734375, -0.0327464751899242401123046875, -0.02465570531785488128662109375, -0.11566288769245147705078125, -0.06820690631866455078125, -0.008849683217704296112060546875, -0.0487788580358028411865234375, -0.1039872467517852783203125, -0.054204143583774566650390625, -0.0076102218590676784515380859375, -0.0411447547376155853271484375, 0.0, 0.0, 0.0, 0.0));
      _9 += mul(max(-u_Texture4.Sample(u_Texture4_sampler, uv), 0.0f.xxxx), float4x4(0.05809019505977630615234375, 0.044788487255573272705078125, 0.0489286594092845916748046875, -0.011864113621413707733154296875, 0.07538767158985137939453125, 0.0421274192631244659423828125, 0.015416751615703105926513671875, -0.007475279271602630615234375, 0.05972291529178619384765625, 0.027502588927745819091796875, 0.008312418125569820404052734375, -0.0060824654065072536468505859375, 0.0, 0.0, 0.0, 0.0));
      _9 += mul(max(u_Texture5.Sample(u_Texture5_sampler, uv), 0.0f.xxxx), float4x4(0.0434465520083904266357421875, -0.0637915432453155517578125, 0.01630773581564426422119140625, 0.0414453446865081787109375, 0.06197130680084228515625, -0.0537582449615001678466796875, 0.03423424065113067626953125, 0.0384377203881740570068359375, 0.0575808584690093994140625, -0.0472042150795459747314453125, 0.030179083347320556640625, 0.033059112727642059326171875, 0.0, 0.0, 0.0, 0.0));
      _9 += mul(max(-u_Texture5.Sample(u_Texture5_sampler, uv), 0.0f.xxxx), float4x4(-0.00380354397930204868316650390625, 0.102071285247802734375, -0.074306003749370574951171875, -0.03070421516895294189453125, 0.0008906116127036511898040771484375, 0.114852242171764373779296875, -0.08803550899028778076171875, -0.021514274179935455322265625, -0.0005958531401120126247406005859375, 0.100072540342807769775390625, -0.0797232091426849365234375, -0.009049375541508197784423828125, 0.0, 0.0, 0.0, 0.0));
      _9 += mul(max(u_Texture6.Sample(u_Texture6_sampler, uv), 0.0f.xxxx), float4x4(0.006605808623135089874267578125, -0.039164729416370391845703125, -0.0315344594419002532958984375, 0.113516055047512054443359375, 0.0011408007703721523284912109375, -0.0429292656481266021728515625, -0.0394135080277919769287109375, 0.12577052414417266845703125, 0.001619900576770305633544921875, -0.0401841811835765838623046875, -0.0347672365605831146240234375, 0.11333562433719635009765625, 0.0, 0.0, 0.0, 0.0));
      _9 += mul(max(-u_Texture6.Sample(u_Texture6_sampler, uv), 0.0f.xxxx), float4x4(0.026559479534626007080078125, 0.0484714247286319732666015625, 0.120928131043910980224609375, -0.00235085375607013702392578125, 0.0419053025543689727783203125, 0.049788586795330047607421875, 0.13564217090606689453125, 0.001282897428609430789947509765625, 0.0386173687875270843505859375, 0.0504475347697734832763671875, 0.126132488250732421875, 0.002873095683753490447998046875, 0.0, 0.0, 0.0, 0.0));
      _9 += mul(max(u_Texture7.Sample(u_Texture7_sampler, uv), 0.0f.xxxx), float4x4(0.008475848473608493804931640625, -0.0561236031353473663330078125, -0.081793963909149169921875, -0.04402355849742889404296875, 0.008800082840025424957275390625, -0.06610845029354095458984375, -0.1016386449337005615234375, -0.04177539050579071044921875, 0.008206044323742389678955078125, -0.0603207834064960479736328125, -0.09669901430606842041015625, -0.0382964499294757843017578125, 0.0, 0.0, 0.0, 0.0));
      _9 += mul(max(-u_Texture7.Sample(u_Texture7_sampler, uv), 0.0f.xxxx), float4x4(0.10676299035549163818359375, -0.0588025189936161041259765625, 0.019221924245357513427734375, -0.075125277042388916015625, 0.11840951442718505859375, -0.064883671700954437255859375, 0.0176027975976467132568359375, -0.080483615398406982421875, 0.106184780597686767578125, -0.064326949417591094970703125, 0.0174139775335788726806640625, -0.066218294203281402587890625, 0.0, 0.0, 0.0, 0.0));
      _9 += float4(-0.01047893427312374114990234375f, -0.00836478359997272491455078125f, -0.010246551595628261566162109375f, 0.0f);
      PSOut.Color = _9 + u_Texture.Sample(u_Texture_sampler, uv);
}
})";

extern const std::string kHLSL_Anime4K_Upscale_CNN_x2_S_Pass0_Pixel = R"(// Mode A Pass 11

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
      float4 _40 = mul(_12(_61, _62), float4x4(-0.005732293240725994110107421875, -0.0306272991001605987548828125, 0.001870986190624535083770751953125, 0.0, 0.12928207218647003173828125, 0.2560246288776397705078125, 0.02284823171794414520263671875, 0.0, -0.0568487457931041717529296875, 0.053723163902759552001953125, -0.041055269539356231689453125, 0.0, 0.1868011653423309326171875, 0.204193413257598876953125, 0.10169033706188201904296875, 0.0));
      float _81 = -1.0f;
      float _82 = 0.0f;
      _40 += mul(_12(_81, _82), float4x4(0.009471417404711246490478515625, 0.0002160195144824683666229248046875, 0.02176210097968578338623046875, 0.0, -0.1295780241489410400390625, -0.229976832866668701171875, 0.00478635542094707489013671875, 0.0, 0.09601442515850067138671875, 0.2366625368595123291015625, 0.008233427070081233978271484375, 0.0, 0.21836183965206146240234375, 0.411923348903656005859375, 0.1085147857666015625, 0.0));
      float _104 = -1.0f;
      float _105 = 1.0f;
      _40 += mul(_12(_104, _105), float4x4(-0.011563760228455066680908203125, 0.01050635986030101776123046875, -0.02771867625415325164794921875, 0.0, -0.18988978862762451171875, -0.2642633616924285888671875, -0.1420233547687530517578125, 0.0, 0.046147048473358154296875, 0.23741047084331512451171875, -0.01665028743445873260498046875, 0.0, -0.0447672270238399505615234375, 0.00276366085745394229888916015625, -0.06637124717235565185546875, 0.0));
      float _126 = 0.0f;
      float _127 = -1.0f;
      _40 += mul(_12(_126, _127), float4x4(0.05780923366546630859375, 0.13880665600299835205078125, 0.0032683121971786022186279296875, 0.0, -0.11033858358860015869140625, -0.1871033608913421630859375, -0.0264370739459991455078125, 0.0, 0.0565335340797901153564453125, 0.2441031038761138916015625, 0.0023248852230608463287353515625, 0.0, -0.062924660742282867431640625, -0.253262460231781005859375, 7.6407661254052072763442993164062e-05, 0.0));
      float _148 = 0.0f;
      float _149 = 0.0f;
      _40 += mul(_12(_148, _149), float4x4(-0.4911060333251953125, -0.877382934093475341796875, -0.18409836292266845703125, 0.0, 0.4429003894329071044921875, 0.780846774578094482421875, 0.18513800203800201416015625, 0.0, -0.4401546418666839599609375, -1.09293651580810546875, -0.11773224174976348876953125, 0.0, -0.4117483794689178466796875, -0.596990764141082763671875, -0.170972764492034912109375, 0.0));
      float _170 = 0.0f;
      float _171 = 1.0f;
      _40 += mul(_12(_170, _171), float4x4(0.105809591710567474365234375, 0.14862583577632904052734375, 0.0355938710272312164306640625, 0.0, -0.0559479035437107086181640625, -0.15393938124179840087890625, -0.003990826196968555450439453125, 0.0, -0.0343123711645603179931640625, -0.18872876465320587158203125, 0.0212985686957836151123046875, 0.0, -0.080236494541168212890625, -0.3170680999755859375, 0.012844483368098735809326171875, 0.0));
      float _192 = 1.0f;
      float _193 = -1.0f;
      _40 += mul(_12(_192, _193), float4x4(-0.0407155863940715789794921875, -0.15790502727031707763671875, 0.003413123078644275665283203125, 0.0, -0.257811129093170166015625, -0.54010903835296630859375, -0.10835732519626617431640625, 0.0, 0.088967137038707733154296875, 0.2958860695362091064453125, 0.011287034489214420318603515625, 0.0, -0.122587896883487701416015625, 0.104010589420795440673828125, -0.11888621747493743896484375, 0.0));
      float _214 = 1.0f;
      float _215 = 0.0f;
      _40 += mul(_12(_214, _215), float4x4(0.0049315444193780422210693359375, -0.041512914001941680908203125, -0.01724783517420291900634765625, 0.0, 0.02376201935112476348876953125, -0.0279943086206912994384765625, 0.0056576863862574100494384765625, 0.0, -0.082247711718082427978515625, -0.585987985134124755859375, 0.04319012165069580078125, 0.0, 0.121118225157260894775390625, -0.069672115147113800048828125, 0.0550035052001476287841796875, 0.0));
      float _236 = 1.0f;
      float _237 = 1.0f;
      _40 += mul(_12(_236, _237), float4x4(0.375213921070098876953125, 0.812032520771026611328125, 0.16570656001567840576171875, 0.0, 0.1591608226299285888671875, 0.3834386765956878662109375, 0.069576866924762725830078125, 0.0, 0.0597089640796184539794921875, 0.343657791614532470703125, 0.0140225924551486968994140625, 0.0, 0.19046007096767425537109375, 0.528795778751373291015625, 0.074799835681915283203125, 0.0));
      _40 += float4(-0.0105096399784088134765625f, -0.00939480960369110107421875f, 0.17684458196163177490234375f, 0.02736674249172210693359375f);
      PSOut.Color = _40;
}
})";

extern const std::string kHLSL_Anime4K_Upscale_CNN_x2_S_Pass1_Pixel = R"(// Mode A Pass 12

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
      float4 _62 = mul(_12(_86, _87), float4x4(-0.011029495857656002044677734375, -0.02248887903988361358642578125, 0.084668017923831939697265625, 0.13580276072025299072265625, 0.0586606301367282867431640625, 0.18384216725826263427734375, 0.1066748797893524169921875, 0.0009743825648911297321319580078125, -0.094606459140777587890625, -0.00397663004696369171142578125, 8.0212535976897925138473510742188e-05, 0.121765218675136566162109375, -0.0176647417247295379638671875, -0.064733065664768218994140625, 0.090886898338794708251953125, -0.08218465745449066162109375));
      float _111 = -1.0f;
      float _112 = 0.0f;
      _62 += mul(_12(_111, _112), float4x4(0.16062797605991363525390625, -0.0096842311322689056396484375, 0.19675050675868988037109375, -0.21377412974834442138671875, -0.10190267860889434814453125, -0.084643073379993438720703125, -0.1450099050998687744140625, 0.100792907178401947021484375, 0.032806821167469024658203125, 0.1705830097198486328125, 0.093607284128665924072265625, -0.174152195453643798828125, 0.056219160556793212890625, -0.096469186246395111083984375, -0.2824014723300933837890625, 0.1733057498931884765625));
      float _139 = -1.0f;
      float _140 = 1.0f;
      _62 += mul(_12(_139, _140), float4x4(-0.06016047298908233642578125, 0.1385172903537750244140625, 0.007052837871015071868896484375, -0.07380388677120208740234375, 0.06316997110843658447265625, 0.068307019770145416259765625, -0.035768859088420867919921875, -0.09369824826717376708984375, 0.0046929032541811466217041015625, -0.0586871989071369171142578125, -0.1112616360187530517578125, 0.0447115600109100341796875, -0.0494059659540653228759765625, -0.040827132761478424072265625, 0.0391553156077861785888671875, 0.096784867346286773681640625));
      float _166 = 0.0f;
      float _167 = -1.0f;
      _62 += mul(_12(_166, _167), float4x4(-0.3668361604213714599609375, 0.19335322082042694091796875, 0.652913033962249755859375, -0.074232466518878936767578125, -0.0359506048262119293212890625, -0.0992535054683685302734375, -0.2559942305088043212890625, -0.06896768510341644287109375, -0.24414362013339996337890625, 0.075083903968334197998046875, 0.19827641546726226806640625, 0.005055452696979045867919921875, -0.009159743785858154296875, -0.000766955432482063770294189453125, 0.06589953601360321044921875, -0.0602728240191936492919921875));
      float _193 = 0.0f;
      float _194 = 0.0f;
      _62 += mul(_12(_193, _194), float4x4(-0.0206884853541851043701171875, 0.13655476272106170654296875, 0.44552695751190185546875, 0.19366647303104400634765625, -0.831782758235931396484375, 0.376750469207763671875, 0.92510306835174560546875, -0.06996066868305206298828125, 0.111048780381679534912109375, -0.222192287445068359375, 0.160632610321044921875, -0.25048410892486572265625, 0.26454412937164306640625, -0.01751934923231601715087890625, -0.620110452175140380859375, 0.00803722999989986419677734375));
      float _220 = 0.0f;
      float _221 = 1.0f;
      _62 += mul(_12(_220, _221), float4x4(0.00515370070934295654296875, -0.044470988214015960693359375, -0.2621257305145263671875, -0.079490609467029571533203125, -0.057168535888195037841796875, 0.1199735105037689208984375, -0.21970181167125701904296875, -0.06480823457241058349609375, -0.1611058712005615234375, 0.1480810344219207763671875, 0.272440493106842041015625, -0.2120827734470367431640625, 0.2523259818553924560546875, -0.3444356620311737060546875, 0.2105081081390380859375, -0.0042361654341220855712890625));
      float _247 = 1.0f;
      float _248 = -1.0f;
      _62 += mul(_12(_247, _248), float4x4(-0.08889520168304443359375, -0.013283102773129940032958984375, -0.065454445779323577880859375, 0.09219782054424285888671875, -0.201694488525390625, 0.07552997767925262451171875, -0.0161232836544513702392578125, 0.131181657314300537109375, 0.19144904613494873046875, -0.24686802923679351806640625, -0.47316181659698486328125, 0.074736095964908599853515625, -0.01688286103308200836181640625, 0.01245321333408355712890625, 0.07092602550983428955078125, 0.007791052572429180145263671875));
      float _274 = 1.0f;
      float _275 = 0.0f;
      _62 += mul(_12(_274, _275), float4x4(0.583215415477752685546875, -0.25497996807098388671875, -0.3989150524139404296875, 0.0205905549228191375732421875, 0.113806903362274169921875, 0.001399313914589583873748779296875, -0.19094778597354888916015625, -0.001249017775990068912506103515625, -0.0397656224668025970458984375, 0.3928508758544921875, -0.082146175205707550048828125, -0.4398621022701263427734375, 0.3182784020900726318359375, -0.4851152598857879638671875, -0.20826934278011322021484375, 0.14377014338970184326171875));
      float _301 = 1.0f;
      float _302 = 1.0f;
      _62 += mul(_12(_301, _302), float4x4(0.21917395293712615966796875, 0.015720672905445098876953125, -0.23638041317462921142578125, -0.033572576940059661865234375, 3.4314656659262254834175109863281e-05, 0.2676126956939697265625, -0.005023303441703319549560546875, -0.084505878388881683349609375, 0.2573486268520355224609375, -0.068072967231273651123046875, -0.13666133582592010498046875, -0.23341487348079681396484375, -0.34333050251007080078125, 0.15040148794651031494140625, 0.4542110860347747802734375, 0.0534908473491668701171875));
      float _328 = -1.0f;
      float _329 = -1.0f;
      _62 += mul(_16(_328, _329), float4x4(-0.17482174932956695556640625, -0.17188489437103271484375, -0.109038688242435455322265625, -0.036696054041385650634765625, 0.0576471351087093353271484375, -0.085412301123142242431640625, -0.1900730133056640625, 0.078444458544254302978515625, 0.331354439258575439453125, 0.036795794963836669921875, -0.0606433413922786712646484375, 0.0125231854617595672607421875, 0.085075102746486663818359375, -0.1387496888637542724609375, -0.03786031901836395263671875, -0.015629060566425323486328125));
      float _355 = -1.0f;
      float _356 = 0.0f;
      _62 += mul(_16(_355, _356), float4x4(-0.04411996901035308837890625, 0.07431592047214508056640625, -0.101925872266292572021484375, 0.17881326377391815185546875, -0.103318192064762115478515625, 0.30109691619873046875, 0.078212551772594451904296875, -0.139142811298370361328125, 0.100501932203769683837890625, -0.17511665821075439453125, -0.224150955677032470703125, 0.109979234635829925537109375, 0.12406484782695770263671875, -0.13263563811779022216796875, 0.2555244266986846923828125, -0.001646357937715947628021240234375));
      float _382 = -1.0f;
      float _383 = 1.0f;
      _62 += mul(_16(_382, _383), float4x4(-0.01911644078791141510009765625, 0.003751750104129314422607421875, -0.0156779848039150238037109375, -0.095373727381229400634765625, -0.154125273227691650390625, 0.081109531223773956298828125, 0.065042279660701751708984375, 0.02890508808195590972900390625, 0.0289031229913234710693359375, 0.074919395148754119873046875, 0.088178180158138275146484375, -0.0512884743511676788330078125, 0.208318173885345458984375, -0.175816237926483154296875, -0.12518326938152313232421875, 0.0543340779840946197509765625));
      float _409 = 0.0f;
      float _410 = -1.0f;
      _62 += mul(_16(_409, _410), float4x4(0.285277903079986572265625, -0.2833647429943084716796875, -0.16090054810047149658203125, 0.15883244574069976806640625, -0.28924024105072021484375, 0.167966306209564208984375, 0.1287612020969390869140625, 0.005302629433572292327880859375, 0.368051230907440185546875, -0.086411409080028533935546875, -0.1591012477874755859375, 0.080674745142459869384765625, 0.2107930481433868408203125, -0.106994070112705230712890625, 0.05734755098819732666015625, 0.05051369965076446533203125));
      float _436 = 0.0f;
      float _437 = 0.0f;
      _62 += mul(_16(_436, _437), float4x4(0.1763906180858612060546875, 0.2619738280773162841796875, -0.083081148564815521240234375, -0.136566102504730224609375, 0.3790121972560882568359375, 0.090147681534290313720703125, -0.332794845104217529296875, -0.1309436261653900146484375, -0.19588692486286163330078125, 0.1969682276248931884765625, -0.2252878248691558837890625, -0.005086558870971202850341796875, -0.02031428180634975433349609375, -0.410254180431365966796875, 0.0617243908345699310302734375, 0.089024484157562255859375));
      float _463 = 0.0f;
      float _464 = 1.0f;
      _62 += mul(_16(_463, _464), float4x4(0.0526299290359020233154296875, 0.121267013251781463623046875, 0.094976954162120819091796875, -0.20738880336284637451171875, 0.0006296958890743553638458251953125, 0.0615432448685169219970703125, 0.094161570072174072265625, 0.0572733767330646514892578125, 0.165772497653961181640625, -0.1052684783935546875, -0.22019256651401519775390625, 0.1955828368663787841796875, -0.3259192407131195068359375, 0.0415839366614818572998046875, -0.058390073478221893310546875, 0.004208021797239780426025390625));
      float _490 = 1.0f;
      float _491 = -1.0f;
      _62 += mul(_16(_490, _491), float4x4(0.3000573813915252685546875, -0.01648812182247638702392578125, 0.095958076417446136474609375, -0.1876434385776519775390625, 0.18478931486606597900390625, 0.09963430464267730712890625, 0.001377468812279403209686279296875, -0.261944472789764404296875, -0.233429431915283203125, 0.316208362579345703125, 0.4827329814434051513671875, -0.117942251265048980712890625, 0.22455732524394989013671875, -0.15731157362461090087890625, -0.0702793598175048828125, -0.012173601426184177398681640625));
      float _517 = 1.0f;
      float _518 = 0.0f;
      _62 += mul(_16(_517, _518), float4x4(0.117986746132373809814453125, 0.55011641979217529296875, 0.13050971925258636474609375, -0.229655325412750244140625, -0.13846518099308013916015625, 0.3408611118793487548828125, 0.051776595413684844970703125, -0.0533673278987407684326171875, -0.0196148119866847991943359375, -0.40090847015380859375, 0.207929432392120361328125, 0.3911586105823516845703125, -0.30111920833587646484375, 0.15706886351108551025390625, 0.2338970601558685302734375, -0.0329885967075824737548828125));
      float _544 = 1.0f;
      float _545 = 1.0f;
      _62 += mul(_16(_544, _545), float4x4(0.054753623902797698974609375, 0.13657845556735992431640625, 0.12736235558986663818359375, -0.144235789775848388671875, -0.0084857307374477386474609375, 0.01048043556511402130126953125, 0.13804523646831512451171875, 0.084030888974666595458984375, -0.245167195796966552734375, 0.07651422917842864990234375, 0.12529011070728302001953125, 0.24335162341594696044921875, 0.1752812862396240234375, -0.4331683218479156494140625, -0.309462368488311767578125, 0.05728803575038909912109375));
      _62 += float4(0.012077211402356624603271484375f, 0.01304588280618190765380859375f, 0.0380778014659881591796875f, -0.029088579118251800537109375f);
      PSOut.Color = _62;
}
})";

extern const std::string kHLSL_Anime4K_Upscale_CNN_x2_S_Pass2_Pixel = R"(// Mode A Pass 13

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
      float4 _62 = mul(_12(_86, _87), float4x4(-0.0361151956021785736083984375, 0.12120111286640167236328125, 0.02963575534522533416748046875, 0.1017058193683624267578125, -0.069718949496746063232421875, 0.24536025524139404296875, -0.1542718708515167236328125, 0.0779192149639129638671875, -0.075089417397975921630859375, 0.0447555072605609893798828125, 0.02714899368584156036376953125, 0.660630166530609130859375, 0.01603616774082183837890625, -0.20663575828075408935546875, -0.20795093476772308349609375, -0.4632968008518218994140625));
      float _111 = -1.0f;
      float _112 = 0.0f;
      _62 += mul(_12(_111, _112), float4x4(-0.0052889925427734851837158203125, -0.080979757010936737060546875, -0.16293914616107940673828125, 0.090949840843677520751953125, -0.01906090788543224334716796875, -0.015142803080379962921142578125, -0.20099808275699615478515625, 0.248722493648529052734375, -0.08660142123699188232421875, -0.18552722036838531494140625, -0.083708219230175018310546875, 0.24338845908641815185546875, -0.0220952071249485015869140625, -0.0784935057163238525390625, 0.3701389133930206298828125, 0.0440038330852985382080078125));
      float _139 = -1.0f;
      float _140 = 1.0f;
      _62 += mul(_12(_139, _140), float4x4(-0.06140649318695068359375, 0.04069982469081878662109375, 0.072095520794391632080078125, 0.15809004008769989013671875, -0.01723279245197772979736328125, -0.01929434575140476226806640625, 0.01606993563473224639892578125, 0.102702297270298004150390625, -0.10917423665523529052734375, 0.08495366573333740234375, 0.1780555546283721923828125, 0.150446712970733642578125, 0.112033188343048095703125, -0.01813359558582305908203125, -0.08953781425952911376953125, -0.15530107915401458740234375));
      float _166 = 0.0f;
      float _167 = -1.0f;
      _62 += mul(_12(_166, _167), float4x4(0.09486760199069976806640625, -0.075477771461009979248046875, -0.20805130898952484130859375, -0.2942969799041748046875, -0.0403056927025318145751953125, 0.0566065721213817596435546875, -0.099587254226207733154296875, -0.098986208438873291015625, -0.00559162907302379608154296875, 0.021390207111835479736328125, 0.02961316891014575958251953125, 0.4447088539600372314453125, -0.0480484031140804290771484375, 0.3260056674480438232421875, 0.009212960489094257354736328125, -0.894873440265655517578125));
      float _193 = 0.0f;
      float _194 = 0.0f;
      _62 += mul(_12(_193, _194), float4x4(-0.1222598850727081298828125, -0.15539920330047607421875, -0.3460996448993682861328125, 0.2129391133785247802734375, 0.1144587695598602294921875, -0.1658740937709808349609375, 0.11169157922267913818359375, 0.09640371799468994140625, 0.066669069230556488037109375, 0.2988137900829315185546875, -0.4187775552272796630859375, -0.12754213809967041015625, 0.18694280087947845458984375, -0.5774662494659423828125, 0.3807563483715057373046875, -0.08026103675365447998046875));
      float _220 = 0.0f;
      float _221 = 1.0f;
      _62 += mul(_12(_220, _221), float4x4(0.15128807723522186279296875, 0.00445712171494960784912109375, 0.19601224362850189208984375, 0.184417247772216796875, 0.05008779466152191162109375, -0.0460194051265716552734375, 0.0466791689395904541015625, 0.068453960120677947998046875, 0.09219755232334136962890625, -0.12899219989776611328125, 0.17465586960315704345703125, 0.11288584768772125244140625, -0.18080945312976837158203125, 0.20305426418781280517578125, 0.02767266519367694854736328125, -0.23283863067626953125));
      float _247 = 1.0f;
      float _248 = -1.0f;
      _62 += mul(_12(_247, _248), float4x4(-0.07296200096607208251953125, 0.10396070778369903564453125, 0.066283442080020904541015625, 0.2237655818462371826171875, -0.066394470632076263427734375, 0.081877768039703369140625, 0.03779740631580352783203125, 0.3624307811260223388671875, 0.0493474937975406646728515625, -0.04280745983123779296875, 0.02188580296933650970458984375, 0.12874890863895416259765625, -0.138640105724334716796875, 0.07390891015529632568359375, -0.0131474025547504425048828125, -0.0023783943615853786468505859375));
      float _274 = 1.0f;
      float _275 = 0.0f;
      _62 += mul(_12(_274, _275), float4x4(0.07494528591632843017578125, 0.05476008355617523193359375, 0.118173070251941680908203125, -0.0523904226720333099365234375, 0.1604559123516082763671875, -0.095626175403594970703125, 0.037452436983585357666015625, 0.113735117018222808837890625, -0.1179834902286529541015625, -0.047832094132900238037109375, -0.14301221072673797607421875, 0.076867751777172088623046875, 0.12910711765289306640625, 0.0349391214549541473388671875, -0.02735678851604461669921875, 0.010008693672716617584228515625));
      float _301 = 1.0f;
      float _302 = 1.0f;
      _62 += mul(_12(_301, _302), float4x4(-0.0239991731941699981689453125, 0.00656335055828094482421875, 0.02646580524742603302001953125, 0.07345540821552276611328125, -0.091900624334812164306640625, -0.0337167568504810333251953125, -0.07517130672931671142578125, 0.046037502586841583251953125, 0.02388156950473785400390625, -0.11983239650726318359375, -0.07760597765445709228515625, 0.21101558208465576171875, 0.0317387282848358154296875, 0.1205776631832122802734375, 0.0604630969464778900146484375, -0.267854630947113037109375));
      float _328 = -1.0f;
      float _329 = -1.0f;
      _62 += mul(_16(_328, _329), float4x4(0.15544603765010833740234375, -0.069135896861553192138671875, -0.2653980255126953125, -0.105860494077205657958984375, -0.039028249680995941162109375, 0.07476507127285003662109375, -0.3958477079868316650390625, -0.0039968038909137248992919921875, 0.0463038384914398193359375, 0.009071253240108489990234375, -0.22155670821666717529296875, -0.04481588304042816162109375, -0.251736164093017578125, 0.08996419608592987060546875, 0.20735882222652435302734375, 0.3954462707042694091796875));
      float _355 = -1.0f;
      float _356 = 0.0f;
      _62 += mul(_16(_355, _356), float4x4(0.61697089672088623046875, 0.02016982622444629669189453125, -0.3914998471736907958984375, 0.0326582491397857666015625, 0.23717613518238067626953125, -0.3071883618831634521484375, -0.068435631692409515380859375, -0.151377260684967041015625, -0.37884676456451416015625, 1.0965588092803955078125, -0.0652290880680084228515625, 0.12837898731231689453125, -0.748486697673797607421875, -0.20711036026477813720703125, 0.10380585491657257080078125, -0.012949219904839992523193359375));
      float _382 = -1.0f;
      float _383 = 1.0f;
      _62 += mul(_16(_382, _383), float4x4(-0.23638196289539337158203125, 0.10690008103847503662109375, -0.16509796679019927978515625, 0.001189062255434691905975341796875, -0.4560866057872772216796875, 0.007835960946977138519287109375, 0.075027354061603546142578125, 0.052157886326313018798828125, -0.1194868385791778564453125, 0.11864341795444488525390625, 0.081229977309703826904296875, 0.0837240517139434814453125, -0.14641439914703369140625, -0.13101322948932647705078125, 0.13451206684112548828125, -0.070850379765033721923828125));
      float _409 = 0.0f;
      float _410 = -1.0f;
      _62 += mul(_16(_409, _410), float4x4(-0.21997725963592529296875, 0.14932109415531158447265625, 0.18699000775814056396484375, 0.125075995922088623046875, -0.164886474609375, 0.02749429829418659210205078125, 0.1472863256931304931640625, -0.1471455395221710205078125, -0.0291316993534564971923828125, 0.003461322747170925140380859375, -0.0428951345384120941162109375, -0.034800089895725250244140625, 0.1799747645854949951171875, -0.320772707462310791015625, -0.076120428740978240966796875, -0.2275397479534149169921875));
      float _436 = 0.0f;
      float _437 = 0.0f;
      _62 += mul(_16(_436, _437), float4x4(-0.5342686176300048828125, 0.4605320394039154052734375, 0.519857883453369140625, 0.068107940256595611572265625, -0.742610514163970947265625, 0.786787927150726318359375, -0.521940410137176513671875, -0.15122683346271514892578125, -0.382945835590362548828125, 0.10623480379581451416015625, 0.14809475839138031005859375, -0.0474090017378330230712890625, 0.42549991607666015625, -0.0411630980670452117919921875, -0.4180237352848052978515625, 0.131783425807952880859375));
      float _463 = 0.0f;
      float _464 = 1.0f;
      _62 += mul(_16(_463, _464), float4x4(-0.50428164005279541015625, 0.0315581299364566802978515625, -0.0538555085659027099609375, 0.008557827211916446685791015625, 0.18220625817775726318359375, 0.01928426325321197509765625, 0.17803181707859039306640625, 0.08401449024677276611328125, 0.3551070392131805419921875, 0.0032388572581112384796142578125, -0.26206362247467041015625, -0.02759889326989650726318359375, -0.081787474453449249267578125, -0.20513348281383514404296875, 0.28703749179840087890625, -0.010791234672069549560546875));
      float _490 = 1.0f;
      float _491 = -1.0f;
      _62 += mul(_16(_490, _491), float4x4(0.1665741503238677978515625, -0.10017001628875732421875, -0.009178675711154937744140625, -0.11212347447872161865234375, 0.067647464573383331298828125, 0.0022367141209542751312255859375, -0.01967359520494937896728515625, -0.110799707472324371337890625, 0.0930769741535186767578125, 0.0325093604624271392822265625, -0.001669706660322844982147216796875, 0.01198711059987545013427734375, -0.144384860992431640625, -0.0527945458889007568359375, -0.15424625575542449951171875, -0.11747758090496063232421875));
      float _517 = 1.0f;
      float _518 = 0.0f;
      _62 += mul(_16(_517, _518), float4x4(-0.02302179671823978424072265625, -0.1313044130802154541015625, -0.078231580555438995361328125, -0.14002072811126708984375, -0.05870342254638671875, 0.048656322062015533447265625, 0.1478529274463653564453125, 0.073952518403530120849609375, -0.0379783548414707183837890625, 0.0568393729627132415771484375, 0.0585550777614116668701171875, 0.098268873989582061767578125, -0.06243391335010528564453125, 0.109036915004253387451171875, -0.116790346801280975341796875, -0.067104637622833251953125));
      float _544 = 1.0f;
      float _545 = 1.0f;
      _62 += mul(_16(_544, _545), float4x4(0.14906375110149383544921875, -0.1619530022144317626953125, -0.00949121825397014617919921875, -0.073147796094417572021484375, 0.03000119514763355255126953125, -0.1368281543254852294921875, 0.06737101078033447265625, 0.0076706199906766414642333984375, -0.103382147848606109619140625, 0.095631420612335205078125, -0.13933889567852020263671875, 0.02867521159350872039794921875, 0.06629680097103118896484375, 0.009514228440821170806884765625, 0.15231515467166900634765625, 0.014213087968528270721435546875));
      _62 += float4(0.0187367312610149383544921875f, -0.00260390737093985080718994140625f, 0.05013002455234527587890625f, -0.0553642250597476959228515625f);
      PSOut.Color = _62;
}
})";

extern const std::string kHLSL_Anime4K_Upscale_CNN_x2_S_Pass3_Pixel = R"(// Mode A Pass 14

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
      float4 _62 = mul(_12(_86, _87), float4x4(0.01910067535936832427978515625, 0.106731094419956207275390625, 0.0040967264212667942047119140625, -0.033909045159816741943359375, -0.0142415650188922882080078125, 0.02609966136515140533447265625, -0.0046263360418379306793212890625, -0.08590595424175262451171875, 0.0046670357696712017059326171875, 0.014594410546123981475830078125, 0.0064695081673562526702880859375, 0.078613780438899993896484375, -0.0386506207287311553955078125, -0.011881356127560138702392578125, 0.01087530516088008880615234375, 0.019452631473541259765625));
      float _111 = -1.0f;
      float _112 = 0.0f;
      _62 += mul(_12(_111, _112), float4x4(0.207776546478271484375, -0.17397616803646087646484375, 0.010251239873468875885009765625, 0.21943406760692596435546875, -0.0603549741208553314208984375, 0.0192934572696685791015625, -0.01738238148391246795654296875, -0.1155749857425689697265625, 0.0023840065114200115203857421875, -0.097071826457977294921875, 0.008661792613565921783447265625, 0.1447159349918365478515625, -0.06412160396575927734375, 0.0806419849395751953125, -0.010995664633810520172119140625, -0.068836234509944915771484375));
      float _139 = -1.0f;
      float _140 = 1.0f;
      _62 += mul(_12(_139, _140), float4x4(0.0579428859055042266845703125, -0.020731754601001739501953125, 0.030419670045375823974609375, 0.02943861484527587890625, -0.063117541372776031494140625, 0.00787715055048465728759765625, -0.02513754181563854217529296875, -0.01550687290728092193603515625, 0.22533960640430450439453125, 0.0415258146822452545166015625, 0.02436417900025844573974609375, 0.081685997545719146728515625, -0.041592918336391448974609375, 0.0252786912024021148681640625, -0.0245435275137424468994140625, -0.07812221348285675048828125));
      float _166 = 0.0f;
      float _167 = -1.0f;
      _62 += mul(_12(_166, _167), float4x4(0.0542375147342681884765625, -0.10032303631305694580078125, 0.0784220993518829345703125, 0.01563869416713714599609375, 0.0676093995571136474609375, -0.02049862779676914215087890625, 0.01794596202671527862548828125, -0.1000154316425323486328125, -0.0047708177007734775543212890625, 0.04240585863590240478515625, -0.022310398519039154052734375, 0.104303099215030670166015625, 0.004346723668277263641357421875, 0.07272253930568695068359375, -0.013134622015058994293212890625, 0.0589883811771869659423828125));
      float _193 = 0.0f;
      float _194 = 0.0f;
      _62 += mul(_12(_193, _194), float4x4(-0.0216525085270404815673828125, 0.1545495092868804931640625, -0.0397455208003520965576171875, -0.097054541110992431640625, 0.35796642303466796875, -0.100172348320484161376953125, 0.04529368877410888671875, 0.56230258941650390625, 0.0594977773725986480712890625, -0.19072173535823822021484375, 0.2220743596553802490234375, -0.3354105055332183837890625, 0.23948468267917633056640625, -0.448125362396240234375, 0.026222564280033111572265625, -0.0172785557806491851806640625));
      float _220 = 0.0f;
      float _221 = 1.0f;
      _62 += mul(_12(_220, _221), float4x4(-0.05368244647979736328125, -0.074629999697208404541015625, -0.07731454074382781982421875, -0.15015421807765960693359375, -0.034112371504306793212890625, -0.0420207269489765167236328125, 0.044114403426647186279296875, 0.001804067636840045452117919921875, -0.093999363481998443603515625, 0.00317839276976883411407470703125, -0.23085598647594451904296875, -0.18684981763362884521484375, 0.1512882411479949951171875, 0.134819567203521728515625, 0.0604442022740840911865234375, 0.2812511026859283447265625));
      float _247 = 1.0f;
      float _248 = -1.0f;
      _62 += mul(_12(_247, _248), float4x4(0.00293299159966409206390380859375, -0.0487694181501865386962890625, 0.0155181773006916046142578125, -0.02763803862035274505615234375, 0.00159601797349750995635986328125, -0.0527240894734859466552734375, 0.11368592083454132080078125, 0.107719294726848602294921875, 0.00075122411362826824188232421875, 0.03788469731807708740234375, -0.038157768547534942626953125, -0.0411578714847564697265625, 0.016544111073017120361328125, 0.049948208034038543701171875, -0.013149977661669254302978515625, 0.02745413966476917266845703125));
      float _274 = 1.0f;
      float _275 = 0.0f;
      _62 += mul(_12(_274, _275), float4x4(0.016691081225872039794921875, 0.0332582890987396240234375, -0.11098420619964599609375, 0.015375060029327869415283203125, 0.010204118676483631134033203125, 0.011482405476272106170654296875, -0.21041764318943023681640625, -0.20591349899768829345703125, 0.040788538753986358642578125, -0.01728691160678863525390625, 0.00895435549318790435791015625, 0.0290740169584751129150390625, 0.01613336987793445587158203125, -0.0728412568569183349609375, 0.18986733257770538330078125, 0.013117442838847637176513671875));
      float _301 = 1.0f;
      float _302 = 1.0f;
      _62 += mul(_12(_301, _302), float4x4(0.013965926133096218109130859375, 0.0221203267574310302734375, 0.08032371103763580322265625, 0.063053913414478302001953125, 0.029871881008148193359375, -0.0068748262710869312286376953125, 0.05080926418304443359375, -0.02195894531905651092529296875, 0.00344990356825292110443115234375, 0.0093243420124053955078125, 0.035050742328166961669921875, 0.03856916725635528564453125, -0.011343668214976787567138671875, -0.0390810035169124603271484375, -0.20328469574451446533203125, -0.22465245425701141357421875));
      float _328 = -1.0f;
      float _329 = -1.0f;
      _62 += mul(_16(_328, _329), float4x4(0.0463077239692211151123046875, 0.01104241423308849334716796875, -0.1324029862880706787109375, -0.02840627916157245635986328125, -0.012419472448527812957763671875, 0.01699425093829631805419921875, 0.0176843106746673583984375, 0.0044142031110823154449462890625, 0.0076738628558814525604248046875, -0.01816640608012676239013671875, -0.02760764770209789276123046875, 0.004961841739714145660400390625, -0.042344845831394195556640625, -0.01695573143661022186279296875, 0.069992698729038238525390625, 0.011084678582847118377685546875));
      float _355 = -1.0f;
      float _356 = 0.0f;
      _62 += mul(_16(_355, _356), float4x4(-0.11995415389537811279296875, 0.007706596516072750091552734375, 0.10273124277591705322265625, -0.06823575496673583984375, -0.007455482147634029388427734375, 0.01660344935953617095947265625, 0.15565730631351470947265625, 0.0561896003782749176025390625, -0.031108133494853973388671875, 0.032943665981292724609375, -0.2464384138584136962890625, -0.0104672014713287353515625, -0.00994644872844219207763671875, 0.0163765847682952880859375, 0.107307843863964080810546875, 0.0426933430135250091552734375));
      float _382 = -1.0f;
      float _383 = 1.0f;
      _62 += mul(_16(_382, _383), float4x4(-0.01634600944817066192626953125, -0.034602515399456024169921875, -0.03962551057338714599609375, -0.0587772093713283538818359375, 0.041953749954700469970703125, -0.00344192632474005222320556640625, -0.03003136813640594482421875, 0.01650488190352916717529296875, -0.10401894152164459228515625, -0.01045785844326019287109375, 0.1603631675243377685546875, -0.15523467957973480224609375, 0.0476419441401958465576171875, 0.015194474719464778900146484375, 0.01928356848657131195068359375, 0.01816161163151264190673828125));
      float _409 = 0.0f;
      float _410 = -1.0f;
      _62 += mul(_16(_409, _410), float4x4(-0.080839909613132476806640625, 0.01133221946656703948974609375, 0.12493048608303070068359375, -0.01264925114810466766357421875, 0.0024665035307407379150390625, -0.01467695645987987518310546875, 0.066257737576961517333984375, -0.07388083636760711669921875, -0.04937337338924407958984375, 0.011646688915789127349853515625, -0.045840866863727569580078125, 0.008790074847638607025146484375, 0.0303713567554950714111328125, -0.011426669545471668243408203125, -0.00969303585588932037353515625, 0.0013844533823430538177490234375));
      float _436 = 0.0f;
      float _437 = 0.0f;
      _62 += mul(_16(_436, _437), float4x4(-0.339418351650238037109375, 0.2093601524829864501953125, 0.0596508122980594635009765625, 0.064327768981456756591796875, -0.2763476073741912841796875, 0.13731300830841064453125, -0.3697310388088226318359375, -0.18283031880855560302734375, -0.1183114349842071533203125, 0.13443593680858612060546875, 0.16444234549999237060546875, -0.04448960721492767333984375, -0.063535265624523162841796875, 0.0746443271636962890625, -0.3708287179470062255859375, -0.13895285129547119140625));
      float _463 = 0.0f;
      float _464 = 1.0f;
      _62 += mul(_16(_463, _464), float4x4(0.13533665239810943603515625, 0.01862549968063831329345703125, 0.047663025557994842529296875, 0.084002040326595306396484375, 0.08268915116786956787109375, -0.05051691830158233642578125, -0.08818845450878143310546875, -0.007377772592008113861083984375, -0.0367572717368602752685546875, 0.0567029528319835662841796875, 0.23254345357418060302734375, 0.220206797122955322265625, -0.1434865891933441162109375, 0.0061717894859611988067626953125, -0.01401546411216259002685546875, -0.123660780489444732666015625));
      float _490 = 1.0f;
      float _491 = -1.0f;
      _62 += mul(_16(_490, _491), float4x4(0.0436100400984287261962890625, -0.02260218746960163116455078125, -0.01981479860842227935791015625, 0.010125854052603244781494140625, 0.046543695032596588134765625, 0.018138997256755828857421875, -0.1725017130374908447265625, 0.02066074870526790618896484375, 0.0064863073639571666717529296875, -0.011071863584220409393310546875, 0.04082326591014862060546875, 0.00204350356943905353546142578125, -0.0335814617574214935302734375, 0.010244091041386127471923828125, -0.040131986141204833984375, -0.010819303803145885467529296875));
      float _517 = 1.0f;
      float _518 = 0.0f;
      _62 += mul(_16(_517, _518), float4x4(-0.0048101930879056453704833984375, -0.0239504277706146240234375, 0.02298696525394916534423828125, 0.031645469367504119873046875, -0.112860739231109619140625, 0.0361451245844364166259765625, 0.2642074525356292724609375, 0.1565215289592742919921875, 0.0519858337938785552978515625, -0.038203828036785125732421875, -0.0607691705226898193359375, -0.0379340015351772308349609375, 0.0478863082826137542724609375, 0.0524013079702854156494140625, -0.092529989778995513916015625, -0.0035418556071817874908447265625));
      float _544 = 1.0f;
      float _545 = 1.0f;
      _62 += mul(_16(_544, _545), float4x4(0.0335836596786975860595703125, -0.02941682003438472747802734375, -0.107232265174388885498046875, -0.047238290309906005859375, -0.005219481885433197021484375, 0.009660559706389904022216796875, -0.034056760370731353759765625, -0.02589829079806804656982421875, 0.0070608821697533130645751953125, -0.0153679884970188140869140625, -0.0443401150405406951904296875, 0.004563231952488422393798828125, -0.0656911432743072509765625, 0.0199054181575775146484375, 0.2689283192157745361328125, 0.19318114221096038818359375));
      _62 += float4(-0.00346730998717248439788818359375f, -0.0046263863332569599151611328125f, -0.00462715514004230499267578125f, -0.0057769152335822582244873046875f);
      PSOut.Color = _62;
}
})";

extern const std::string kHLSL_Anime4K_Upscale_CNN_x2_S_Pass4_Pixel = R"(// Mode A Pass 15

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
})";


}  // namespace renderer
