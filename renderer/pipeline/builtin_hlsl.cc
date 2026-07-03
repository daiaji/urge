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

extern const std::string kHLSL_Anime4K_UDL_Pass0_Pixel = R"(
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
  float4 Color0 : SV_TARGET0;
  float4 Color1 : SV_TARGET1;
};

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float2 uv = PSIn.UV;
  float2 p = u_InputPt;

  float3 s00 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(-p.x, -p.y), 0).rgb;
  float3 s01 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(-p.x, 0), 0).rgb;
  float3 s02 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(-p.x, p.y), 0).rgb;
  float3 s10 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(0, -p.y), 0).rgb;
  float3 s11 = u_Texture.SampleLevel(u_Texture_sampler, uv, 0).rgb;
  float3 s12 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(0, p.y), 0).rgb;
  float3 s20 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(p.x, -p.y), 0).rgb;
  float3 s21 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(p.x, 0), 0).rgb;
  float3 s22 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(p.x, p.y), 0).rgb;

  float4 target1 = float4(-0.012922576, -0.11982956, 0.021963459, 0.019259451);
  target1 += mul(s00, float3x4(-0.050913796, -0.05115213, -0.0205767, -0.26266688, -0.12883802, 0.107968464, 0.03389763, -0.70179373, 0.0030511466, 0.07718592, -0.06562523, -0.060305536));
  target1 += mul(s01, float3x4(0.009235469, -0.018979615, 0.10033019, -0.20307243, 0.040932532, -0.10095427, 0.038843542, -0.28774044, -0.07829864, -0.04317961, 0.032555006, -0.05584433));
  target1 += mul(s02, float3x4(0.23774138, 0.04701499, -0.16824278, 0.025335955, 0.30246395, -0.037289508, 0.070405066, 0.03094164, -0.0075012813, 0.06881163, -0.03157643, -0.032394916));
  target1 += mul(s10, float3x4(-0.12524955, 0.18535072, -0.05323482, 0.004486272, 0.15295836, 0.3050709, 0.081431866, 0.09352846, -0.059866652, -0.029570978, 0.019920588, 0.121749535));
  target1 += mul(s11, float3x4(-0.2111615, -0.1268416, 0.45642895, 0.47401953, -0.7580866, 0.5514855, 0.96250856, 0.7827129, 0.0003978912, 0.17167407, -0.04423575, -0.04569368));
  target1 += mul(s12, float3x4(0.17050457, -0.18697786, -0.11608587, -0.038065948, 0.26542, -0.7021022, -0.33751717, 0.053689335, 0.10030526, -0.19492362, 0.069387496, 0.07228368));
  target1 += mul(s20, float3x4(0.15900351, -0.017636139, 0.01917807, 0.05584281, 0.28530255, 0.04795445, -0.104170926, 0.1192509, 0.09859251, 0.057123564, 0.025724344, -0.07723904));
  target1 += mul(s21, float3x4(-0.06581913, 0.07548721, -0.054552317, -0.08317343, 0.32851526, -0.2362575, -0.39470714, -0.073999345, 0.07246812, -0.04103072, 0.06058696, 0.09532553));
  target1 += mul(s22, float3x4(-0.12524493, 0.095179625, -0.0918538, 0.016793616, -0.48433152, 0.03702525, -0.100864686, -0.0018861603, -0.14784335, -0.048320837, -0.057494648, -0.024096634));

  float4 target2 = float4(-0.0018859988, 0.004285429, 0.5060845, -0.030093472);
  target2 += mul(s00, float3x4(0.04816902, 0.030087546, 0.019183155, -0.08234757, 0.09378316, -0.047217257, -0.04757087, -0.16541782, -0.043394983, 0.05779227, 0.018105166, 0.03222583));
  target2 += mul(s01, float3x4(0.13639967, -0.001877575, 0.049495522, 0.060094353, 0.015303669, 0.059043188, 0.090356335, -0.12654372, 0.06469071, -0.054733396, -0.013548386, -0.093697555));
  target2 += mul(s02, float3x4(-0.13214277, 0.00062924915, -0.640379, -0.052121993, -0.022532608, 0.01077454, -0.057074178, -0.103670195, -0.0017062012, 0.0035225085, -0.044859786, -0.020764757));
  target2 += mul(s10, float3x4(0.2553945, -0.08126201, 0.055215932, 0.10690791, 0.6771195, 0.09377514, -0.09488318, -0.43969935, 0.35444704, -0.10392259, 0.07595239, 0.021814484));
  target2 += mul(s11, float3x4(-0.37628967, 0.026895085, 0.035044484, -0.16414654, -0.5694931, -0.20123884, 0.14891861, 1.1822934, -0.25648627, 0.14110301, -0.057699542, 0.17731132));
  target2 += mul(s12, float3x4(0.023089241, 0.14888923, -0.2730167, 0.1330048, -0.039043408, 0.75768983, 0.07385114, 0.0138615575, -0.06565686, 0.10451973, 0.037489507, 0.021156311));
  target2 += mul(s20, float3x4(0.03965048, 0.040422294, -0.0662493, -0.043219455, 0.00834316, -0.08315282, 0.13010995, -0.11822414, -0.06811034, 0.029744523, -0.098641835, -0.063671604));
  target2 += mul(s21, float3x4(-0.077282995, -0.29400682, 0.116103284, 0.096747644, -0.47398612, -0.77101594, -0.20683232, 0.111703634, -0.08370965, -0.24218678, 0.13780457, -0.017660126));
  target2 += mul(s22, float3x4(0.08542605, 0.13080615, 0.081582755, -0.00024888176, 0.31160986, 0.17787197, -0.019935975, -0.09658498, 0.096656196, 0.064402744, -0.033331197, 0.027531069));

  PSOut.Color0 = target1;
  PSOut.Color1 = target2;
}
)";

extern const std::string kHLSL_Anime4K_UDL_Pass1_Pixel = R"(
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
  float4 Color0 : SV_TARGET0;
  float4 Color1 : SV_TARGET1;
};

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float2 uv = PSIn.UV;

  // [ a, d, g ]
  // [ b, e, h ]
  // [ c, f, i ]
  float4 a1 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(-u_InputPt.x, -u_InputPt.y), 0);
  float4 b1 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(-u_InputPt.x, 0), 0);
  float4 c1 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(-u_InputPt.x, u_InputPt.y), 0);
  float4 d1 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(0, -u_InputPt.y), 0);
  float4 e1 = u_Texture.SampleLevel(u_Texture_sampler, uv, 0);
  float4 f1 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(0, u_InputPt.y), 0);
  float4 g1 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(u_InputPt.x, -u_InputPt.y), 0);
  float4 h1 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(u_InputPt.x, 0), 0);
  float4 i1 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(u_InputPt.x, u_InputPt.y), 0);

  float4 na1 = max(-a1, 0);
  float4 nb1 = max(-b1, 0);
  float4 nc1 = max(-c1, 0);
  float4 nd1 = max(-d1, 0);
  float4 ne1 = max(-e1, 0);
  float4 nf1 = max(-f1, 0);
  float4 ng1 = max(-g1, 0);
  float4 nh1 = max(-h1, 0);
  float4 ni1 = max(-i1, 0);

  a1 = max(a1, 0);
  b1 = max(b1, 0);
  c1 = max(c1, 0);
  d1 = max(d1, 0);
  e1 = max(e1, 0);
  f1 = max(f1, 0);
  g1 = max(g1, 0);
  h1 = max(h1, 0);
  i1 = max(i1, 0);

  float4 a2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv + float2(-u_InputPt.x, -u_InputPt.y), 0);
  float4 b2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv + float2(-u_InputPt.x, 0), 0);
  float4 c2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv + float2(-u_InputPt.x, u_InputPt.y), 0);
  float4 d2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv + float2(0, -u_InputPt.y), 0);
  float4 e2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv, 0);
  float4 f2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv + float2(0, u_InputPt.y), 0);
  float4 g2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv + float2(u_InputPt.x, -u_InputPt.y), 0);
  float4 h2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv + float2(u_InputPt.x, 0), 0);
  float4 i2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv + float2(u_InputPt.x, u_InputPt.y), 0);

  float4 na2 = max(-a2, 0);
  float4 nb2 = max(-b2, 0);
  float4 nc2 = max(-c2, 0);
  float4 nd2 = max(-d2, 0);
  float4 ne2 = max(-e2, 0);
  float4 nf2 = max(-f2, 0);
  float4 ng2 = max(-g2, 0);
  float4 nh2 = max(-h2, 0);
  float4 ni2 = max(-i2, 0);

  a2 = max(a2, 0);
  b2 = max(b2, 0);
  c2 = max(c2, 0);
  d2 = max(d2, 0);
  e2 = max(e2, 0);
  f2 = max(f2, 0);
  g2 = max(g2, 0);
  h2 = max(h2, 0);
  i2 = max(i2, 0);

  float4 target1 = { 0.03144068, -0.027781913, 0.04483475, 0.037489943 };
  target1 += mul(a1, float4x4(0.34559122, 0.052896723, -0.27492252, -0.1604473, 0.4791457, 0.17956258, 0.0076199574, -0.16324736, -0.075430416, 0.019434236, -0.275363, -0.16502565, 0.05507322, -0.046572465, 0.08130956, 0.009380191));
  target1 += mul(b1, float4x4(0.1754505, 0.10862336, -0.14956018, 0.20161937, 0.16598102, -0.0033441933, 0.19303258, 0.3278992, -0.31819978, 0.14614153, 0.08434212, 0.21208692, -0.0014794758, -0.06754758, -0.06314527, 0.023496931));
  target1 += mul(c1, float4x4(0.13594365, -0.06382366, -0.40069854, -0.087743916, 0.022426397, -0.073364444, -0.19371308, 0.09916138, -0.044016927, 0.0018689828, -0.07705671, 0.15398589, -0.069929935, -0.01874144, 0.050793763, 0.06565281));
  target1 += mul(d1, float4x4(0.56292456, 0.25537506, -0.16147509, 0.029484648, 0.11898947, 0.19103922, -0.2387553, 0.13659279, -0.044804625, -0.10285909, 0.12958583, 0.21526133, 0.02727471, 0.21990417, 0.0009558564, 0.12372512));
  target1 += mul(e1, float4x4(-0.10264466, -0.13103753, -0.069214605, 0.43234769, 0.25947884, -0.18333039, -0.15585582, -0.2406589, 0.33275372, -0.19497354, -0.09758474, -0.4531396, 0.41932744, -0.043746196, 0.08315102, -0.085604236));
  target1 += mul(f1, float4x4(0.15380725, -0.06311845, -0.28896615, -0.059237756, -0.078456834, -0.11623796, 0.017248835, 0.098803006, -0.13643564, -0.0029720776, 0.425954, 0.36920592, -0.06980546, 0.05205535, -0.15787347, -0.094921984));
  target1 += mul(g1, float4x4(0.009595518, -0.12598279, -0.04322495, -0.08838463, 0.11729769, -0.062454883, 0.19743776, -0.08590505, -0.022744715, 0.00457582, -0.06070008, 0.045312855, -0.010845991, -0.02241941, 0.07252932, 0.05525124));
  target1 += mul(h1, float4x4(-0.119069465, 0.08782395, 0.17878884, 0.0068233046, -0.36698806, -0.46077076, 0.37470114, 0.006550318, 0.08622002, -0.10081386, 0.1754186, 0.078841425, 0.060330488, 0.39436886, 0.1688179, -0.10113108));
  target1 += mul(i1, float4x4(0.17160045, -0.18541232, -0.093926296, 0.0053854887, -0.07649591, -0.3053692, 0.15255369, 0.06183564, 0.105131835, 0.076607525, -0.17482935, -0.104579754, -0.4795174, 0.30223432, 0.4728322, 0.106419675));
  target1 += mul(a2, float4x4(-0.068794325, -0.019651407, 0.048906703, 0.10097784, 0.014003637, 0.08358555, -0.34008583, 0.1677446, 0.12863056, 0.010167976, 0.10771957, -0.14823496, -0.11855097, 0.024728613, -0.06394353, 0.07123295));
  target1 += mul(b2, float4x4(0.1652107, -0.056815207, 0.26562792, -0.02586732, 0.13812682, 0.3791579, -0.40067768, 0.19901459, -0.055583958, 0.06673556, -0.16258197, 0.0014027074, 0.13844898, 0.17588624, 0.0061608437, 0.014889389));
  target1 += mul(c2, float4x4(0.023591522, -0.06255483, -0.04512753, -0.07939918, 0.17603582, -0.06219873, -0.10907254, 0.012348696, -0.053350568, 0.023741387, 0.05215983, 0.117241465, 0.28173143, 0.11200327, -0.11672438, -0.13278063));
  target1 += mul(d2, float4x4(-0.15015969, -0.1145909, 0.08583166, 0.0386507, -0.17788467, 0.29311427, 0.03577728, -0.006737705, -0.020426478, 0.065881886, -0.10966947, -0.016716056, -0.0027577002, -0.20769168, 0.4357363, -0.13179652));
  target1 += mul(e2, float4x4(-0.44572783, 0.08870803, 0.42933974, -0.16602941, 0.23271243, 0.29478154, -0.53973556, -0.042550746, -0.13157314, -0.0413034, 0.12679552, 0.11579286, -0.5161936, -0.24292113, -0.10862491, 0.13528119));
  target1 += mul(f2, float4x4(-0.043000877, 0.08458555, 0.11260604, -0.5589381, -0.16010836, -0.019429926, 0.04731505, -0.12212733, 0.05655828, 0.0107375225, -0.10067243, -0.06904067, 0.07476142, -0.043922618, -0.13811466, 0.008697587));
  target1 += mul(g2, float4x4(-0.3281664, -0.104251154, 0.07188181, 0.06720938, 0.028879764, 0.07302547, 0.18261562, -0.08896491, 0.11240943, -0.1919612, -0.13059135, -0.07057044, 0.053953633, 0.17297988, -0.20344415, 0.050276734));
  target1 += mul(h2, float4x4(-0.41925356, 0.020309223, 0.2246313, -0.3418901, -0.20863962, 0.18653068, -0.04616101, 0.1236236, -0.062179156, 0.1437903, 0.1314142, 0.0699381, 0.029918872, 0.23033592, 0.09302733, -0.20570321));
  target1 += mul(i2, float4x4(0.07847491, -0.18251555, 0.0678772, -0.29089385, -0.03632992, -0.17132603, -0.04896196, 0.09839614, -0.10377483, -0.11817732, 0.03477946, 0.050376516, 0.17791937, -0.34359503, 0.030756304, 0.025246387));
  target1 += mul(na1, float4x4(-0.12972409, 0.032459006, -0.20415276, 0.31407776, -0.1743501, -0.26177478, -0.07577315, -0.104599, -0.025548192, -0.23483936, 0.40139225, 0.12898883, 0.06533049, -0.09545806, -0.032093894, 0.0032956926));
  target1 += mul(nb1, float4x4(0.22749326, -0.20613275, -0.23030083, -0.29994026, -0.18482473, -0.038720988, -0.13339107, -0.1394514, 0.36952803, -0.2709558, -0.14104684, -0.17859542, 0.09873891, 0.04330318, 0.15205383, 0.115995236));
  target1 += mul(nc1, float4x4(0.07534328, -0.13592403, 0.2224819, -0.06818886, -0.11952144, 0.004714797, 0.18252324, -0.08729513, 0.17198865, -0.00082568696, 0.33769485, -0.0920225, 0.173712, -0.038548574, -0.016980015, -0.13799237));
  target1 += mul(nd1, float4x4(-0.43659294, -0.19679698, -0.31969583, 0.24002865, -0.1064947, -0.08218358, -0.07990568, -0.028915526, -0.077836946, -0.012841249, -0.11685068, -0.2102985, 0.025435956, -0.21367492, 0.11001358, -0.09812692));
  target1 += mul(ne1, float4x4(0.28203383, 0.09570471, -0.14503846, -0.19898729, 0.18757457, 0.16626704, -0.009997161, 0.06738176, -0.18296066, 0.11583831, -0.0025225005, 0.373547, -0.24103725, 0.3553009, 0.11984093, 0.25370696));
  target1 += mul(nf1, float4x4(-0.022194814, 0.02950222, -0.121312395, 0.0040648654, 0.06509207, 0.00084966415, 0.032229617, 0.0139804585, -0.23108627, -0.004511493, -0.28217104, 0.0828633, 0.17399071, 0.2137328, 0.4731738, -0.37666738));
  target1 += mul(ng1, float4x4(-0.045961298, 0.0056297607, -0.08513672, 0.093939304, 0.07252928, -0.11458939, 0.11005008, -0.1132733, 0.10369599, 0.1636998, -0.11919379, -0.08949099, 0.080640145, 0.029493907, 0.24982096, -0.10234766));
  target1 += mul(nh1, float4x4(0.08474163, -0.24252129, -0.3065911, 0.11077523, 0.13397239, 0.14875948, -0.18212163, 0.006510455, -0.008477232, -0.3242149, 0.31507346, -0.19521071, -0.3610268, 0.25882444, -0.067812346, 0.20968717));
  target1 += mul(ni1, float4x4(0.05730163, 0.053821165, -0.10948745, 0.04090055, 0.0161064, 0.19475192, 0.09248433, -0.027268974, -0.031323943, -0.084304914, 0.28378648, 0.44910806, -0.052243132, 0.2999386, -0.26639074, -0.2529396));
  target1 += mul(na2, float4x4(0.026707547, -0.006487042, -0.044127557, -0.016287267, 0.1417188, 0.24645403, -0.32444936, 0.20339565, 0.027596464, 0.03799474, -0.029943593, 0.058569513, -0.15013286, 0.25070968, 0.08954207, -0.14304538));
  target1 += mul(nb2, float4x4(-0.22184753, -0.0732679, 0.042815078, 0.03770516, 0.22240163, -0.043244008, -0.14883384, -0.10682856, 0.16421252, 0.20890577, 0.000585579, -0.061031006, -0.551696, -0.17770186, 0.13795924, 0.101121314));
  target1 += mul(nc2, float4x4(-0.047539327, 0.11826275, 0.458172, -0.023809819, -0.0154842585, -0.015466883, 0.03837829, -0.34703115, -0.03437818, 0.12705797, -0.042713646, -0.2518409, -0.27947584, -0.020104226, -0.022687877, 0.14169087));
  target1 += mul(nd2, float4x4(0.06269709, 0.06449363, -0.02793847, 0.04407663, -0.054694284, 0.69776016, -0.32850045, 0.19365972, -0.19002354, -0.038244195, -0.20433429, -0.34071165, 0.123992935, -0.22218247, -0.30181807, -0.03031556));
  target1 += mul(ne2, float4x4(-0.06685185, -0.18313402, -0.03785641, 0.008412995, -0.017108139, 0.48937285, -0.035302214, 0.011338532, -0.08890957, 0.32343447, 0.088812076, -0.027280344, 0.40437454, -0.45940742, 0.118888274, 0.41054434));
  target1 += mul(nf2, float4x4(-0.36049488, 0.100708134, 0.331516, 0.1078647, 0.12895954, 0.13425021, -0.18602797, -0.11423174, -0.10916294, 0.061013293, 0.08984191, 0.1835112, -0.10568929, -0.046648484, 0.2127872, 0.54582083));
  target1 += mul(ng2, float4x4(0.19040897, 0.08670264, 0.12393752, -0.003475547, -0.37210098, 0.035628326, -0.29302806, 0.10709011, -0.20405664, -0.9748058, 0.39254782, 0.44914797, 0.032028764, 0.04227575, -0.25056216, 0.063437305));
  target1 += mul(nh2, float4x4(-0.07952942, -0.13263832, 0.037877183, 0.20845042, -0.026445981, -0.010450352, -0.043147005, -0.12033961, 0.20600243, -0.046332583, -0.47056386, 0.09566825, 0.18658772, -0.3381639, -0.042662457, 0.15197653));
  target1 += mul(ni2, float4x4(-0.4996296, 0.019971728, 0.10017604, 0.052051116, 0.12145858, 0.106811635, -0.056665674, -0.11708303, 0.16642408, 0.22654046, -0.04731226, -0.039967895, -0.1434505, 0.3171998, -0.19033776, -0.29952875));

  float4 target2 = { 0.049680557, 0.01432493, 0.04349397, 0.040003702 };
  target2 += mul(a1, float4x4(-0.031192884, -0.015032417, 0.25046152, 0.143142, 0.09429096, 0.2090414, -0.16252424, 0.42788, -0.005667558, 0.14787567, 0.23810932, -0.13502707, 0.0006289761, -0.014052179, -0.091041535, 0.059258565));
  target2 += mul(b1, float4x4(-0.09637771, 0.17332087, 0.123664804, 0.046110056, 0.25775972, 0.31647265, -0.1464598, 0.41624358, 0.032242253, -0.017219262, -0.35814875, 0.3348811, 0.05738627, 0.046910666, 0.014263179, -0.15797907));
  target2 += mul(c1, float4x4(-0.06782952, 0.049666278, 0.083296575, 0.19301543, -0.05964988, 0.18332662, 0.30906975, 0.03342819, 0.12226727, 0.1226969, -0.15035193, -0.003493911, -0.007647415, -0.051491078, -0.019189527, -0.009602449));
  target2 += mul(d1, float4x4(0.08838342, -0.055376932, 0.13949814, -0.12728734, -0.17266448, 0.35102528, 0.018773714, 0.050504927, -0.10556112, -0.014422574, -0.25474203, 0.31192264, -0.09063805, 0.010115312, -0.08702192, 0.08573518));
  target2 += mul(e1, float4x4(0.16521221, -0.01265248, -0.5292306, -0.17494588, -0.18994644, -0.41904125, -0.26261392, -0.42338082, 0.39478812, 0.20768805, 0.16483486, -0.22635488, 0.13576357, 0.17095351, 0.064293, 0.06416031));
  target2 += mul(f1, float4x4(-0.09107591, 0.1757355, 0.19841582, -0.25249094, 0.18083812, -0.12258315, 0.4074544, -0.17171176, -0.15881093, -0.22978021, -0.05622591, -0.09703007, -0.12538208, -0.06956953, -0.14475612, -0.066342294));
  target2 += mul(g1, float4x4(-0.029294115, -0.036292624, 0.19467807, -0.10223533, 0.086430565, -0.052809026, -0.23749635, 0.10364248, -0.22938702, 0.07210543, 0.03876035, -0.21014924, -0.11247329, -0.17755648, -0.05139757, -0.037780646));
  target2 += mul(h1, float4x4(0.12605286, 0.16123274, -0.13924524, -0.109194726, 0.033486, -0.24847955, 0.1264379, 0.28880134, -0.17594175, -0.1888256, -0.04508948, 0.047563452, -0.5476752, -0.23573762, -0.17183748, 0.14331517));
  target2 += mul(i1, float4x4(-0.006482806, 0.2289281, -0.03872587, -0.027272481, -0.09913351, -0.09453464, -0.1426349, 0.055076513, -0.025217436, -0.08307176, 0.0797406, 0.10166401, -0.294337, -0.3567936, 0.054015454, 0.068333104));
  target2 += mul(a2, float4x4(0.012300659, -0.040405195, 0.11190478, -0.07406065, -0.18364848, 0.035823543, -0.01621734, 0.07582391, 0.06704436, -0.0006620425, -0.022342965, 0.16496183, 0.11390146, 0.075079784, 0.13547076, -0.022227254));
  target2 += mul(b2, float4x4(0.23038611, -0.29141426, 0.0984085, -0.20544642, -0.18859404, 0.3620387, -0.4136066, 0.32138887, -0.0047645094, 0.11271573, 0.15377328, 0.012071895, -0.029830804, 0.14384824, 0.04148142, 0.2286753));
  target2 += mul(c2, float4x4(-0.120368056, -0.0026308578, -0.027536837, -0.13022487, 0.19286355, 0.30597997, -0.121778116, 0.29960433, -0.06231281, -0.013746478, 0.10620681, -0.02362372, -0.10042793, 0.015861828, -0.06073457, 0.11589962));
  target2 += mul(d2, float4x4(0.1148781, -0.24268909, 0.24827103, -0.17290637, -0.14397098, -0.16708367, 0.2130187, -0.18639165, -0.13702524, 0.107212365, 0.066469796, -0.14059094, 0.19621798, -0.036907773, -0.028576817, 0.19191594));
  target2 += mul(e2, float4x4(0.061653305, -0.12716687, 0.17514701, 0.003910376, -0.00651784, 0.25642744, -0.17615528, -0.03584991, -0.051342323, -0.20178711, -0.4330863, 0.15785883, -0.14388351, 0.050646614, 0.15746376, -0.17228809));
  target2 += mul(f2, float4x4(-0.32631296, -0.020115409, -0.16132942, 0.29139966, -0.18642388, -0.15140165, 0.2106485, -0.025535548, 0.08296747, 0.037819803, 0.106129125, -0.095521644, 0.312119, -0.09383011, -0.023469942, -0.035990953));
  target2 += mul(g2, float4x4(0.012878467, -0.1599543, 0.14487906, -0.083350256, 0.074949436, -0.09346481, 0.10122695, 0.08852621, 0.11138647, -0.0072039254, -0.00842464, 0.030785646, -0.04394235, 0.10987614, 0.15378197, -0.05989409));
  target2 += mul(h2, float4x4(0.41359067, -0.04985946, 0.06845964, 0.12003392, 0.0803128, 0.2420856, -0.18877462, 0.058456603, -0.02516271, 0.010639022, -0.04928307, -0.023084244, 0.06001203, 0.06881964, -0.12117699, -0.2680374));
  target2 += mul(i2, float4x4(0.09667388, 0.16247103, 0.105098106, 0.12871382, 0.063410334, 0.029997706, 0.048323907, -0.075631075, 0.034694012, -0.029085271, -0.003785678, -0.05397498, -0.1783155, -0.13680255, 0.024786513, -0.0041952017));
  target2 += mul(na1, float4x4(-0.23904142, -0.102619216, -0.21049559, -0.07428196, -0.046321787, -0.09432119, 0.08803711, -0.1660408, 0.31880215, 0.11605265, -0.086603194, 0.119239025, 0.06773056, 0.18591799, 0.0058458247, 0.05242187));
  target2 += mul(nb1, float4x4(0.12521484, -0.23739336, -0.16784379, -0.10277679, -0.18505791, 0.061825443, 0.12762548, -0.16664176, 0.20004764, -0.1400315, 0.35610282, -0.19706382, 0.046386316, -0.155162, -0.0425219, 0.0010560523));
  target2 += mul(nc1, float4x4(0.14500342, -0.0046809237, -0.1278097, 0.041527335, 0.11831141, -0.059155047, -0.17391829, 0.0059517594, -0.18033625, -0.379706, 0.11636179, -0.13310274, 0.047523372, 0.0029333998, -0.1512301, 0.1361489));
  target2 += mul(nd1, float4x4(-0.23058943, -0.08937329, 0.07061336, 0.08555644, 0.09255573, -0.15303029, 0.08891002, -0.42177418, 0.0950346, 0.20212616, 0.3866544, 0.07922501, -0.04093803, -0.10997976, -0.07189613, -0.21220057));
  target2 += mul(ne1, float4x4(-0.04484278, 0.2386453, 0.27855012, 0.011022442, 0.0409283, 0.1937425, 0.060258046, 0.2633126, -0.54181176, 0.19643608, -0.28907844, 0.04247623, -0.37548354, -0.24831985, -0.52362055, -0.4442409));
  target2 += mul(nf1, float4x4(0.014318134, 0.047169194, -0.07291308, 0.21408482, -0.01503884, 0.027093383, -0.11724912, -0.052458502, 0.1676504, 0.5505249, 0.22394833, -0.17126445, 0.13671164, -0.18371153, -0.456313, 0.14297491));
  target2 += mul(ng1, float4x4(0.00063476624, 0.16339731, -0.031160444, 0.18237135, 0.025692228, -0.04895109, 0.033651803, -0.002480504, 0.34582126, -0.039352335, -0.004698449, 0.12789944, -0.08318657, -0.007492543, -0.12888806, 0.03684109));
  target2 += mul(nh1, float4x4(-0.06481498, 0.14330916, 0.17366715, -0.028045174, 0.080571376, 0.18343642, -0.11593154, -0.077227145, 0.1973531, 0.3085006, -0.28876102, 0.06434657, 0.16654246, -0.28144804, 0.3234261, -0.026636604));
  target2 += mul(ni1, float4x4(-0.084783904, 0.03651458, 0.020044886, -0.10723048, 0.04165204, 0.04072967, 0.037039082, -0.09042298, 0.19693066, -0.21291414, -0.040890995, -0.15434273, -0.07450638, 0.27289733, 0.06332989, -0.037289053));
  target2 += mul(na2, float4x4(-0.004840926, 0.048929166, 0.015578959, 0.03571025, -0.2184971, 0.094020076, -0.17748803, 0.32877877, -0.035392962, -0.28398407, -0.13072185, -0.21858144, -0.24103665, -0.32654533, -0.063572675, -0.008728733));
  target2 += mul(nb2, float4x4(0.0060240547, 0.029166108, -0.023887299, 0.037508924, 0.04231956, 0.1503379, 0.17414866, -0.25778973, -0.14774446, -0.12541369, -0.32502824, 0.28957245, -0.030400498, 0.05351274, 0.13189505, -0.21329227));
  target2 += mul(nc2, float4x4(0.2198507, -0.49962172, -0.16456802, 0.08402717, -0.094403476, -0.1978019, -0.19233316, 0.055013265, 0.01668743, -0.117106654, -0.0745593, -0.09377295, 0.050370943, 0.07410238, 0.13543247, -0.23753798));
  target2 += mul(nd2, float4x4(0.008572295, 0.11890422, -0.047157902, -0.03717175, -0.35570037, 0.060663674, 0.109250925, -0.16135052, 0.030490266, 0.30335435, 0.38949126, 0.44852075, -0.09788441, 0.43574813, -0.30050707, 0.24572986));
  target2 += mul(ne2, float4x4(0.29497403, -0.30934516, 0.05756695, -0.15919119, -0.121505864, -0.028917443, -0.07419939, 0.13863774, -0.04398897, 0.32990414, 0.38306457, -0.030523712, 0.72267497, 0.33932966, 0.07839862, 0.11931982));
  target2 += mul(nf2, float4x4(0.26952964, -0.31019664, 0.07061176, -0.23266664, 0.14124376, 0.3597343, -0.17694736, 0.22935267, -0.12335108, -0.086614646, -0.10635, 0.22585274, -0.27139255, 0.05963002, 0.2852169, -0.3743854));
  target2 += mul(ng2, float4x4(0.0970178, -0.014084432, -0.0504985, 0.1570353, 0.091999866, 0.23429315, 0.12914294, 0.03267318, 0.5849793, 0.38205758, -0.31792474, -0.07992281, 0.022620765, 0.22215942, -0.23093775, 0.0026896205));
  target2 += mul(nh2, float4x4(-0.06753083, -0.20358866, 0.173053, 0.13768815, 0.013206715, 0.06310567, 0.17349118, -0.12714109, 0.0405548, -0.18409975, 0.3441249, -0.24606577, -0.18814458, -0.039655812, -0.15961805, 0.08212082));
  target2 += mul(ni2, float4x4(0.06746224, -0.1595078, 0.15284725, -0.057313897, -0.1229526, 0.11482664, -0.0021675595, -0.00026835455, -0.0653958, -0.0967453, -0.09400396, -0.021233113, 0.23587836, 0.2982212, -0.039116163, 0.012201323));

  PSOut.Color0 = target1;
  PSOut.Color1 = target2;
}
)";

extern const std::string kHLSL_Anime4K_UDL_Pass2_Pixel = R"(
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
  float4 Color0 : SV_TARGET0;
  float4 Color1 : SV_TARGET1;
};

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float2 uv = PSIn.UV;

  // [ a, d, g ]
  // [ b, e, h ]
  // [ c, f, i ]
  float4 a1 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(-u_InputPt.x, -u_InputPt.y), 0);
  float4 b1 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(-u_InputPt.x, 0), 0);
  float4 c1 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(-u_InputPt.x, u_InputPt.y), 0);
  float4 d1 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(0, -u_InputPt.y), 0);
  float4 e1 = u_Texture.SampleLevel(u_Texture_sampler, uv, 0);
  float4 f1 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(0, u_InputPt.y), 0);
  float4 g1 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(u_InputPt.x, -u_InputPt.y), 0);
  float4 h1 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(u_InputPt.x, 0), 0);
  float4 i1 = u_Texture.SampleLevel(u_Texture_sampler, uv + float2(u_InputPt.x, u_InputPt.y), 0);

  float4 na1 = max(-a1, 0);
  float4 nb1 = max(-b1, 0);
  float4 nc1 = max(-c1, 0);
  float4 nd1 = max(-d1, 0);
  float4 ne1 = max(-e1, 0);
  float4 nf1 = max(-f1, 0);
  float4 ng1 = max(-g1, 0);
  float4 nh1 = max(-h1, 0);
  float4 ni1 = max(-i1, 0);

  a1 = max(a1, 0);
  b1 = max(b1, 0);
  c1 = max(c1, 0);
  d1 = max(d1, 0);
  e1 = max(e1, 0);
  f1 = max(f1, 0);
  g1 = max(g1, 0);
  h1 = max(h1, 0);
  i1 = max(i1, 0);

  float4 a2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv + float2(-u_InputPt.x, -u_InputPt.y), 0);
  float4 b2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv + float2(-u_InputPt.x, 0), 0);
  float4 c2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv + float2(-u_InputPt.x, u_InputPt.y), 0);
  float4 d2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv + float2(0, -u_InputPt.y), 0);
  float4 e2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv, 0);
  float4 f2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv + float2(0, u_InputPt.y), 0);
  float4 g2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv + float2(u_InputPt.x, -u_InputPt.y), 0);
  float4 h2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv + float2(u_InputPt.x, 0), 0);
  float4 i2 = u_Texture1.SampleLevel(u_Texture1_sampler, uv + float2(u_InputPt.x, u_InputPt.y), 0);

  float4 na2 = max(-a2, 0);
  float4 nb2 = max(-b2, 0);
  float4 nc2 = max(-c2, 0);
  float4 nd2 = max(-d2, 0);
  float4 ne2 = max(-e2, 0);
  float4 nf2 = max(-f2, 0);
  float4 ng2 = max(-g2, 0);
  float4 nh2 = max(-h2, 0);
  float4 ni2 = max(-i2, 0);

  a2 = max(a2, 0);
  b2 = max(b2, 0);
  c2 = max(c2, 0);
  d2 = max(d2, 0);
  e2 = max(e2, 0);
  f2 = max(f2, 0);
  g2 = max(g2, 0);
  h2 = max(h2, 0);
  i2 = max(i2, 0);

  float4 target1 = { 0.0052180276, 0.022526434, 0.022657124, 0.016289035 };
  target1 += mul(a1, float4x4(-0.07314084, 0.08021976, -0.08299374, -0.21340942, -0.0088407695, 0.04742526, -0.038566757, -0.058931205, 0.0009213959, 0.19193986, -0.05906689, -0.0038934543, -0.12937409, 0.100754194, 0.1683601, 0.07552924));
  target1 += mul(b1, float4x4(-0.022257961, 0.08347593, -0.02279838, 0.10150892, -0.02083181, 0.07064587, 0.26308942, -0.13609628, 0.023648601, 0.1475858, 0.12856342, 0.2650287, -0.038316045, -0.35173503, 0.09157486, 0.16609442));
  target1 += mul(c1, float4x4(-0.13746555, 0.15315081, -0.032931942, 0.07487079, 0.09694968, 0.014459765, 0.06814075, -0.059461202, 0.25045857, -0.0071333316, 0.067206055, -0.21697883, 0.023228496, -0.13146883, 0.07486156, -0.030696157));
  target1 += mul(d1, float4x4(-0.0069204876, -0.18402638, 0.085326575, 0.18288516, 0.036785558, -0.019116882, 0.017438713, 0.029095992, 0.10944869, -0.09473364, 0.10444152, -0.028845368, 0.0909169, -0.10593229, 0.14518781, 0.05546837));
  target1 += mul(e1, float4x4(0.53389466, -0.018921841, -0.05050542, 0.21149407, 0.3041209, -0.2594824, -0.18464427, 0.20736529, 0.18971719, -0.05058395, -0.13514072, -0.009045264, 0.20910244, 0.29242986, 0.28958234, 0.2870443));
  target1 += mul(f1, float4x4(0.03259606, 0.2126493, 0.6004735, 0.14007168, -0.1424266, 0.04352873, 0.17071731, 0.10630275, -0.2755667, 0.27345222, -0.06420644, 0.032743722, 0.026045147, -0.23541754, 0.01393772, -0.1476582));
  target1 += mul(g1, float4x4(0.06258474, -0.040185593, -0.092409454, -0.095720276, 0.050550956, -0.026547447, 0.099580996, 0.04878719, 0.15659782, -0.007606541, -0.061156776, 0.11329769, -0.019249229, 0.028775204, -0.24508974, -0.052828208));
  target1 += mul(h1, float4x4(-0.16975857, -0.008542089, 0.30186546, 0.33199415, 0.03747256, 0.15057808, 0.017838268, -0.030345246, 0.019341556, 0.3217693, 0.24844399, 0.06951953, -0.10805396, -0.08874898, -0.068681985, -0.2677526));
  target1 += mul(i1, float4x4(-0.06813968, 0.087481484, -0.11338694, -0.08698839, -0.07585716, 0.079565816, -0.066336565, 0.050449606, 0.11338618, 0.38572344, 0.0024759274, 0.12706435, 0.16759671, 0.0254419, -0.06910047, -0.21917519));
  target1 += mul(a2, float4x4(0.0039553675, -0.17838223, 0.038052835, 0.027201787, 0.06518285, 0.08250212, -0.052679926, -0.021249574, -0.13604519, 0.12234797, -0.16008313, -0.07422232, -0.0930264, -0.07480355, -0.0067053377, 0.13964424));
  target1 += mul(b2, float4x4(-0.05491681, 0.16191071, -0.13063031, -0.2889149, -0.045188528, 0.29249623, -0.061093148, -0.083284624, -0.19250835, -0.103631295, -0.23577131, 0.108691126, 0.028907659, -0.2708106, 0.06986715, 0.22996326));
  target1 += mul(c2, float4x4(-0.07838976, -0.063634194, 0.06297176, -0.09969828, 0.10518915, 0.062185638, 0.033053298, 0.023406805, -0.2801067, -0.13414349, -0.02466297, -0.1110011, 0.040580552, 0.033576507, 0.07127022, -0.068416506));
  target1 += mul(d2, float4x4(-0.05786512, 0.17169164, -0.09276801, -0.1444394, 0.13971466, -0.168134, 0.012722911, 0.06788442, 0.02493809, 0.04105174, 0.09471395, 0.21363391, -0.12093948, 0.067423604, -0.054669242, 0.06764739));
  target1 += mul(e2, float4x4(0.2954526, 0.15885043, -0.05164922, 0.3646313, 0.013329013, 0.044056762, 0.01717495, -0.030439444, 0.32433322, -0.29044852, 0.32627285, 0.150364, 0.14502852, -0.22193567, -0.18879528, 0.018430077));
  target1 += mul(f2, float4x4(-0.2973998, -0.41863972, 0.0048396075, 0.06709588, -0.12029818, -0.05315725, -0.11457002, 0.0071458486, 0.26290894, 0.11030596, 0.082195595, -0.27480638, -0.011602335, 0.019122265, -0.18927693, -0.24246486));
  target1 += mul(g2, float4x4(0.09974451, 0.07223917, -0.09586719, -0.08288307, -0.06436462, -0.027324842, -0.0019976476, 0.19203754, 0.015929956, -0.12534836, -0.0038582094, 0.11275662, -0.031039666, 0.010430081, -0.023713758, -0.21801127));
  target1 += mul(h2, float4x4(0.054167796, 0.0634282, -0.047591783, -0.06402415, -0.0709014, 0.082054086, 0.28418478, 0.06584792, -0.18744822, -0.006312915, -0.0075474046, 0.0829434, -0.032414634, 0.19225785, -0.082302466, -0.3142319));
  target1 += mul(i2, float4x4(-0.0026932533, -0.110426664, 0.021643564, -0.14368293, -0.0048789545, 0.11043582, -0.040021945, 0.058764413, -0.009000321, 0.10833911, 0.05681704, -0.039960742, 0.0014395626, 0.022780152, -0.09172437, -0.085687816));
  target1 += mul(na1, float4x4(0.12509525, -0.18352552, -0.07638094, -0.00756009, 0.05407378, -0.14584734, -0.08163636, -0.13222884, 0.039648265, -0.15960212, 0.074228585, 0.009451507, 0.17933762, -0.17743796, 0.007834195, 0.0037116117));
  target1 += mul(nb1, float4x4(-0.10942205, 0.1585392, 0.040241007, 0.10526164, 0.16979292, 0.29029292, -0.009487742, 0.24926443, -0.1047842, 0.03604099, 0.19281772, 0.03798268, 0.17581491, 0.25031644, 0.055782937, -0.30455682));
  target1 += mul(nc1, float4x4(0.06714908, -0.09112766, -0.022286715, 0.09795178, -0.014092309, 0.26703134, 0.15334776, 0.33441234, 0.13753732, -0.13819148, 0.22796239, 0.16050872, 0.05523446, 0.082806356, -0.053028688, -0.0400533));
  target1 += mul(nd1, float4x4(-0.028462043, 0.18224953, 0.026658487, -0.15048791, 0.106156826, -0.07361365, 0.3529029, 0.06473894, -0.032005392, 0.037034214, 0.039220046, -0.012491292, -0.09503139, 0.0444902, -0.31978187, -0.2923563));
  target1 += mul(ne1, float4x4(-0.3674723, 0.22560489, 0.38837367, 0.17128418, -0.0948159, 0.6298207, 0.59135467, 0.3350841, -0.1859739, 0.31080073, 0.03317792, 0.20958795, -0.097624235, -0.07605166, 0.10135128, -0.08953993));
  target1 += mul(nf1, float4x4(0.320043, 0.002823138, -0.08849585, -0.06356955, 0.19898786, 0.272037, 0.1241285, 0.18131523, -0.05760319, -0.19315276, -0.033923294, 0.09981398, -0.07670874, -0.25949827, 0.062826484, 0.011877337));
  target1 += mul(ng1, float4x4(-0.019341033, -0.03938962, 0.10163529, 0.05033707, -0.03194324, -0.13427012, 0.16106506, -0.05596736, -0.04438277, 0.0045224032, 0.20575951, -0.10359912, 0.03423479, -0.17256664, 0.32534334, -0.09378658));
  target1 += mul(nh1, float4x4(0.19792143, 0.038506437, -0.21047395, -0.27926794, 0.23113485, -0.053830303, 0.4963027, 0.34639266, 0.108149074, -0.10592886, 0.09575202, 0.12385147, 0.08751849, -0.050622147, 0.033647005, 0.2588364));
  target1 += mul(ni1, float4x4(0.04931599, -0.14498134, 0.0073008477, -0.05298649, 0.29398152, 0.16829367, 0.089691155, -0.01749789, 0.20039341, -0.13137043, 0.1884996, -0.03018221, -0.06793498, -0.03220071, 0.06326444, 0.017898731));
  target1 += mul(na2, float4x4(0.011310341, 0.15556115, -0.08003895, -0.07396207, -0.06434896, -0.14684777, -0.019239893, 0.009520887, 0.013242985, -0.12733786, -0.040152796, 0.0064262203, 0.087119006, 0.08165867, 0.12353576, 0.002600503));
  target1 += mul(nb2, float4x4(0.14877501, -0.056240283, -0.11846124, 0.16736585, -0.0018247389, 0.0095979795, -0.07605829, 0.13583913, -0.008851887, 0.16578445, -0.04152669, -0.059164364, -0.021962654, 0.312347, 0.0129089225, -0.097307086));
  target1 += mul(nc2, float4x4(-0.122485265, 0.06891502, -0.1807204, 0.10579281, -0.0061903363, -0.025644284, 0.08879091, -0.09492319, -0.019361734, -0.10903786, -0.08949264, 0.055067465, -0.027095577, -0.06629012, -0.05580654, 0.045552503));
  target1 += mul(nd2, float4x4(-0.025895944, 0.18728323, 0.09764548, 0.49504116, -0.030123139, -0.012580951, 0.090377375, -0.18767111, -0.06874367, 0.11378584, 0.0127285635, -0.101479106, 0.07010412, -0.02272616, -0.03455195, 0.040611476));
  target1 += mul(ne2, float4x4(-0.58637494, -0.13186562, -0.26627728, -0.40135092, 0.19139144, 0.27310577, 0.07761293, 0.10058002, -0.3126869, -0.07982417, 0.04237517, 0.25126198, -0.17133251, 0.122523, -0.0053142905, -0.22283912));
  target1 += mul(nf2, float4x4(-0.0023953887, 0.30968156, -0.1303385, 0.046937056, 0.20530851, 0.07276076, -0.086923674, -0.17881633, 0.08715105, 0.25641996, -0.22557895, -0.0017721896, -0.2347971, -0.07164777, -0.103000194, 0.22468017));
  target1 += mul(ng2, float4x4(-0.12947787, -0.05199853, -0.0899567, 0.087013826, 0.018399805, 0.14997742, -0.20396905, -0.20554177, -0.014265392, 0.048660364, 0.07077151, -0.05911514, 0.003051989, 0.07242704, -0.16232954, 0.19634365));
  target1 += mul(nh2, float4x4(0.13121666, 0.03174777, 0.07853035, -0.04881682, 0.10043158, -0.036237933, -0.2178651, -0.06562213, 0.021113047, 0.0068006255, -0.16305129, -1.9600706e-05, -0.14886445, -0.17729987, -0.17907865, 0.21547341));
  target1 += mul(ni2, float4x4(-0.03263096, -0.064234234, 0.03990361, 0.09057224, -0.05704657, -0.107518636, 0.09328312, 0.014857798, -0.060736485, -0.033695858, -0.07943859, -0.0054049506, -0.072932534, -0.023306495, -0.06615389, 0.029145932));

  float4 target2 = { -0.034925383, -0.0010656221, -0.023427188, -0.021127155 };
  target2 += mul(a1, float4x4(0.012031344, 0.0075636036, -0.033211436, 0.018453801, -0.23412584, -0.113123864, 0.068607934, -0.018517016, -0.19748597, -0.2571716, -0.026148321, -0.00019766031, 0.012040108, 0.12122093, 0.0714374, -0.10087335));
  target2 += mul(b1, float4x4(-0.029292978, -0.025254043, -0.034099232, 0.085234866, 0.24252516, 0.076297395, -0.12717746, -0.03457669, 0.033755753, -0.0531509, -0.04005856, -0.20840853, -0.0078028045, 0.12575904, -0.010887013, -0.046326064));
  target2 += mul(c1, float4x4(-0.003266499, -0.017687857, -0.012221699, -0.2251586, 0.00208294, 0.007880196, 0.09037794, 0.08328994, -0.0428717, 0.027112724, 0.08032711, 0.1513152, -0.043068174, 0.07987632, -0.008801098, 0.08133886));
  target2 += mul(d1, float4x4(-0.1827595, 0.18459928, -0.1918044, -0.05324067, -0.1705114, -0.01887987, -0.14486305, -0.17456877, -0.18964832, -0.07162095, -0.13871318, -0.046433818, -0.018604748, -0.11131921, -0.08050445, -0.08619502));
  target2 += mul(e1, float4x4(-0.0717377, -0.12163745, 0.18497953, -0.08643892, 0.0007879318, -0.050351888, 0.17640385, 0.17240365, -0.14958718, -0.056793597, 0.03742872, -0.1015922, 0.3117469, -0.39953762, 0.0152286505, -0.13784732));
  target2 += mul(f1, float4x4(0.07879097, -0.39204946, -0.07003556, -0.24708183, -0.058046583, -0.09865189, -0.048411854, -0.05027539, -0.12736927, -0.23946127, -0.08323304, 0.028160958, -0.059784077, -0.0064917994, 0.038013496, 0.08928725));
  target2 += mul(g1, float4x4(0.07403741, -0.004601062, 0.13563065, 0.054981887, -0.08022936, 0.022921488, -0.053264186, -0.016605966, -0.20883927, -0.19978985, -0.058101434, 0.15126002, 0.020758694, 0.12837122, 0.13368484, 0.1443778));
  target2 += mul(h1, float4x4(-0.08701922, -0.041025855, -0.03362371, -0.19846733, -0.009003309, 0.06708822, 0.06784735, 0.049892817, 0.123487085, -0.008921262, -0.0883188, -0.09103165, 0.070733, 0.1474191, -0.08228257, 0.12713781));
  target2 += mul(i1, float4x4(0.16015989, 0.19007389, -0.12680867, 0.056614764, -0.008470681, 0.099433914, 0.008811413, -0.09471121, -0.09722353, 0.0649324, 0.021527816, -0.21614286, 0.07569941, -0.16433574, -0.0069269636, 0.16142729));
  target2 += mul(a2, float4x4(-0.08708631, -0.017263759, 0.034016605, -0.009168008, -0.16427393, -0.11225274, -0.005249783, 0.13672975, -0.0844234, -0.022700429, 0.109927036, -0.041033685, -0.064794436, 0.015655773, -0.03411672, -0.12218549));
  target2 += mul(b2, float4x4(-0.016761513, -0.027447775, -0.01290059, 0.0007822344, 0.07433617, -0.035145793, -0.03797909, -0.16871531, -0.029095095, -0.2073536, 0.12309633, -0.16626619, -0.04203133, -0.018517911, -0.06946039, -0.11132114));
  target2 += mul(c2, float4x4(0.11052091, -0.030863507, -0.03229482, 0.11673996, -0.0455341, -0.00649463, 0.020642368, 0.04092308, 0.20173405, -0.012926573, -0.0244531, 0.055338163, -0.01835753, 0.024072325, -0.06893433, 0.048774183));
  target2 += mul(d2, float4x4(0.3568486, -0.14506009, -0.13730963, -0.027905643, -0.37042627, -0.016187102, 0.12948507, 0.016912838, -0.089135066, -0.15287507, -0.092210636, 0.043153215, 0.2077129, 0.04429632, -0.107345045, -0.015176141));
  target2 += mul(e2, float4x4(-0.33605802, -0.22235338, 0.1270437, -0.23185425, 0.29133183, -0.005394921, -0.07139614, -0.049961478, 0.017125877, 0.499106, -0.0048643304, -0.14794266, -0.06752325, 0.29848218, 0.11979753, 0.033426132));
  target2 += mul(f2, float4x4(0.11241839, -0.09014392, -0.011629057, 0.17028853, -0.100855775, 0.100789815, -0.05269513, 0.06573697, 0.27869916, -0.057539526, -0.04528007, 0.30135208, -0.02261679, 0.0688468, 0.059139624, 0.13873443));
  target2 += mul(g2, float4x4(0.04780322, -0.008265764, -0.014270074, 0.0834061, 0.055182222, -0.059819162, 0.010733226, -0.040952608, -0.14509161, 0.17645077, 0.05801798, -0.042507146, 0.24863482, 0.1040497, -0.045867782, 0.120007925));
  target2 += mul(h2, float4x4(0.12579694, 0.09167574, 0.21078496, 0.052945495, -0.05036728, -0.11384816, -0.07594621, -0.09991826, 0.010668207, -0.05676672, -0.06273805, -0.06883917, -0.2184931, -0.1647689, -0.056467786, 0.109850615));
  target2 += mul(i2, float4x4(-0.11352159, 0.026516005, 0.042277884, 0.14155892, -0.017015357, -0.03407179, 0.014961351, -0.13766216, 0.20035928, -0.038310144, 0.002857473, -0.04447413, 0.011375937, -0.07345281, 0.01680756, 0.0089689195));
  target2 += mul(na1, float4x4(0.18048844, 0.025165293, -0.013590799, 0.21590467, 0.026852742, -0.06107904, -0.0012434963, 0.047840245, -0.07294931, -0.011157553, 0.11376999, -0.0086454, -0.028179385, -0.11118097, -0.15483098, 0.19983171));
  target2 += mul(nb1, float4x4(-0.15175144, 0.2142459, 0.1478812, -0.14039889, -0.19821295, -0.37290373, 0.19691283, 0.115997985, 0.1284214, 0.19273835, -0.096292645, -0.022643294, 0.15401742, -0.2267051, -0.15150996, 0.099672556));
  target2 += mul(nc1, float4x4(-0.068340585, -0.017279925, 0.04846922, -0.034003776, 0.055793036, -0.25135002, -0.03544407, -0.56164503, -0.19032021, -0.009258663, 0.070812754, -0.08191077, 0.047685042, -0.020684654, -0.07035788, 0.0132855335));
  target2 += mul(nd1, float4x4(0.19441503, -0.15030424, 0.12302495, 0.047762766, -0.095896654, -0.15033515, 0.007605368, 0.0570889, -0.038431447, -0.08560695, -0.0029293734, -0.01375586, 0.047505997, 0.014071177, 0.1479392, 0.25642776));
  target2 += mul(ne1, float4x4(-0.28587586, -0.39141047, -0.3444917, -0.2408476, -0.64026415, -0.35294148, -0.1317, -0.21601357, 0.12164572, -0.48452628, 0.16729403, -0.21575572, 0.41301385, 0.017696327, 0.057344552, -0.27020162));
  target2 += mul(nf1, float4x4(-0.033119988, 0.0012006643, 0.08465847, 0.015564506, -0.124659166, -0.09455984, 0.0035544615, -0.35156307, -0.15252608, 0.016244112, 0.0138391815, -0.04670501, 0.1383293, -0.037926193, 0.025957817, 0.1730784));
  target2 += mul(ng1, float4x4(-0.012701927, -0.025511298, -0.06721094, -0.07040279, 0.06377799, 0.13967788, -0.14234799, -0.058825023, 0.041205924, -0.00032473358, -0.055379577, -0.033738375, 0.13665317, -0.02562686, -0.18523781, -0.06958092));
  target2 += mul(nh1, float4x4(0.17461562, 0.07647785, -0.02202248, 0.21096313, -0.22494456, 0.10868611, -0.33091828, -0.27529812, -0.25206757, 0.1884099, -0.17850949, -0.1006927, 0.045536183, -0.100012675, 0.061030168, -0.025509179));
  target2 += mul(ni1, float4x4(0.0337314, -0.052486207, -0.05584458, 0.0969859, 0.18508333, -0.04521821, -0.08331424, 0.076726556, 0.118076116, 0.019730117, 0.022492286, 0.09869008, -0.115276754, 0.097966135, 0.023186501, -0.060849246));
  target2 += mul(na2, float4x4(-0.09427026, 0.14057149, -0.07478311, 0.029171692, 0.14987083, -0.08649628, -0.01750609, 0.06958318, 0.085471064, -0.058146793, -0.029388946, 0.10720532, -0.030614216, 0.17328379, -0.03433174, -0.022483094));
  target2 += mul(nb2, float4x4(-0.085193954, -0.1348099, 0.07675298, -0.25627816, -0.07467235, -0.18559869, 0.100543626, -0.2201029, -0.015106581, -0.013150452, 0.10482805, -0.04446529, -0.15954255, 0.13659625, -0.10310867, -0.010787774));
  target2 += mul(nc2, float4x4(-0.13365999, 0.02036792, -0.09569852, -0.088586286, 0.18445042, -0.14354594, -0.09319419, 0.084703825, -0.018052364, 0.04344066, -0.0589665, -0.0065992875, 0.030960705, 0.08472253, -0.022175593, -0.020301547));
  target2 += mul(nd2, float4x4(-0.12315616, 0.05191162, 0.3044562, -0.066225395, 0.13523789, 0.24786936, -0.2531183, 0.008910162, 0.3662465, 0.2633546, -0.11816884, -0.108501054, -0.30446148, 0.094746254, 0.22515038, -0.048324294));
  target2 += mul(ne2, float4x4(0.34875512, 0.22885701, -0.22425419, 0.30605644, 0.13452671, 0.16655035, -0.10293953, 0.23753232, -0.5908745, -0.15148452, -0.3885865, 0.14085245, -0.12627047, -0.09645269, 0.101941, -0.062304396));
  target2 += mul(nf2, float4x4(-0.18468879, 0.11713357, 0.04766135, -0.25752118, 0.076471716, 0.06850848, -0.06427401, 0.028061042, 0.017875634, 0.09589284, -0.020327348, -0.1585817, 0.19669123, 0.10955879, -0.18545902, -0.074755065));
  target2 += mul(ng2, float4x4(0.1056897, 0.08521911, -0.017700022, -0.004319419, 0.15351436, -0.11358399, 0.065656774, 0.101860404, 0.08894655, -0.060075074, 0.14363492, -0.10447328, -0.27426496, -0.19959188, 0.16687778, -0.09456175));
  target2 += mul(nh2, float4x4(-0.05424188, -0.16305181, 0.028440254, -0.013702167, -0.010122417, -0.13160124, 0.08733208, 0.111403994, -0.13586052, 0.016545279, 0.12953275, -0.01298413, 0.19755821, 0.029597677, 0.004327247, 0.093656704));
  target2 += mul(ni2, float4x4(-0.016224308, -0.020333769, 0.015944391, -0.044774864, 0.09308092, -0.06174809, 0.009493231, 0.00109714, 0.030341865, 0.0085925255, 0.023199126, 0.029012285, 0.050746094, 0.15161276, 0.053011492, -0.022610705));

  PSOut.Color0 = target1;
  PSOut.Color1 = target2;
}
)";

extern const std::string kHLSL_Anime4K_UDL_Pass3_Pixel = R"(
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

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  uint2 pixel = uint2(PSIn.UV * u_OutputSize);
  uint sub_idx = (pixel.x & 1) + ((pixel.y & 1) << 1);
  float2 feature_uv = (float2(pixel >> 1) + 0.5) * u_InputPt;

  float2 p = u_InputPt;

  float4 a1 = u_Texture1.SampleLevel(u_Texture1_sampler, feature_uv + float2(-p.x, -p.y), 0);
  float4 b1 = u_Texture1.SampleLevel(u_Texture1_sampler, feature_uv + float2(-p.x, 0), 0);
  float4 c1 = u_Texture1.SampleLevel(u_Texture1_sampler, feature_uv + float2(-p.x, p.y), 0);
  float4 d1 = u_Texture1.SampleLevel(u_Texture1_sampler, feature_uv + float2(0, -p.y), 0);
  float4 e1 = u_Texture1.SampleLevel(u_Texture1_sampler, feature_uv, 0);
  float4 f1 = u_Texture1.SampleLevel(u_Texture1_sampler, feature_uv + float2(0, p.y), 0);
  float4 g1 = u_Texture1.SampleLevel(u_Texture1_sampler, feature_uv + float2(p.x, -p.y), 0);
  float4 h1 = u_Texture1.SampleLevel(u_Texture1_sampler, feature_uv + float2(p.x, 0), 0);
  float4 i1 = u_Texture1.SampleLevel(u_Texture1_sampler, feature_uv + float2(p.x, p.y), 0);

  float4 na1 = max(-a1, 0); a1 = max(a1, 0);
  float4 nb1 = max(-b1, 0); b1 = max(b1, 0);
  float4 nc1 = max(-c1, 0); c1 = max(c1, 0);
  float4 nd1 = max(-d1, 0); d1 = max(d1, 0);
  float4 ne1 = max(-e1, 0); e1 = max(e1, 0);
  float4 nf1 = max(-f1, 0); f1 = max(f1, 0);
  float4 ng1 = max(-g1, 0); g1 = max(g1, 0);
  float4 nh1 = max(-h1, 0); h1 = max(h1, 0);
  float4 ni1 = max(-i1, 0); i1 = max(i1, 0);

  float4 a2 = u_Texture2.SampleLevel(u_Texture2_sampler, feature_uv + float2(-p.x, -p.y), 0);
  float4 b2 = u_Texture2.SampleLevel(u_Texture2_sampler, feature_uv + float2(-p.x, 0), 0);
  float4 c2 = u_Texture2.SampleLevel(u_Texture2_sampler, feature_uv + float2(-p.x, p.y), 0);
  float4 d2 = u_Texture2.SampleLevel(u_Texture2_sampler, feature_uv + float2(0, -p.y), 0);
  float4 e2 = u_Texture2.SampleLevel(u_Texture2_sampler, feature_uv, 0);
  float4 f2 = u_Texture2.SampleLevel(u_Texture2_sampler, feature_uv + float2(0, p.y), 0);
  float4 g2 = u_Texture2.SampleLevel(u_Texture2_sampler, feature_uv + float2(p.x, -p.y), 0);
  float4 h2 = u_Texture2.SampleLevel(u_Texture2_sampler, feature_uv + float2(p.x, 0), 0);
  float4 i2 = u_Texture2.SampleLevel(u_Texture2_sampler, feature_uv + float2(p.x, p.y), 0);

  float4 na2 = max(-a2, 0); a2 = max(a2, 0);
  float4 nb2 = max(-b2, 0); b2 = max(b2, 0);
  float4 nc2 = max(-c2, 0); c2 = max(c2, 0);
  float4 nd2 = max(-d2, 0); d2 = max(d2, 0);
  float4 ne2 = max(-e2, 0); e2 = max(e2, 0);
  float4 nf2 = max(-f2, 0); f2 = max(f2, 0);
  float4 ng2 = max(-g2, 0); g2 = max(g2, 0);
  float4 nh2 = max(-h2, 0); h2 = max(h2, 0);
  float4 ni2 = max(-i2, 0); i2 = max(i2, 0);

  float4 target1 = float4(0.0037637148, 0.003693704, 0.0034614028, 0.0033483643);
  target1 += mul(a1, float4x4(0.009722335, -5.8660436e-05, -0.0069387504, -0.0052446183, -0.040276118, 0.0041334885, -0.013106614, -0.0047898176, -0.008160448, 0.011272557, -0.008908942, -0.015969492, 0.036588583, -0.0069453213, 0.03697349, 0.024233166));
  target1 += mul(b1, float4x4(0.07749142, -0.0112727145, 0.064222045, -0.015094554, 0.0032031287, 0.03247034, -0.016756386, 0.023846423, -0.028618578, 0.02300731, -0.015894018, 0.037608027, 0.014199439, -0.043177396, -0.004832348, -0.05518754));
  target1 += mul(c1, float4x4(0.008171211, -0.016406616, 0.04668373, -0.0020393482, -0.008888379, 0.001380358, -0.008963435, 0.0012900458, -0.030172894, -0.0017824832, -0.037534058, 0.000615256, 0.030373376, 0.002216906, 0.04730168, -0.0028000386));
  target1 += mul(d1, float4x4(0.060749017, 0.006499037, -0.03925888, -0.043421242, 0.0014141012, -0.040274277, 0.020051334, 0.02141008, -0.0046555796, -0.032477897, 0.02811765, 0.014327698, 0.008681297, 0.044408746, -0.028984996, 0.00985357));
  target1 += mul(e1, float4x4(0.22245905, 0.2221309, 0.21369153, 0.17244695, -0.16802068, -0.09160697, -0.13712268, -0.104401335, -0.18699472, -0.117237985, -0.13240008, -0.121350996, 0.027870163, 0.09320937, 0.07950856, 0.08880132));
  target1 += mul(f1, float4x4(-0.002709059, -0.0070304363, 0.10570918, 0.08184527, -0.014383472, -0.020202143, -0.0810668, -0.054163996, -0.018711304, -0.035145987, -0.098869935, -0.06942387, -0.011235106, 0.008683168, -0.02585752, 0.024761796));
  target1 += mul(g1, float4x4(-0.017611317, 0.033189557, 0.0014886355, 0.0063918163, 0.0033280635, 0.00871624, 0.018652624, 0.0072240643, 0.028240945, 0.027274653, -0.0044101775, 0.012503479, -0.009022953, -0.0037992215, 0.007457012, -0.0075594983));
  target1 += mul(h1, float4x4(-0.042642962, 0.061122447, -0.0661494, 0.046923082, 0.014721836, -0.07878182, 0.013244828, -0.047850955, 0.016932828, -0.07947459, 0.05953852, -0.007192553, -0.022235982, -0.026965706, -0.034282424, -0.007242096));
  target1 += mul(i1, float4x4(-0.012262586, -0.014608243, -0.0039572082, 0.045586918, 0.011789637, 0.00811699, 0.004699602, -0.032348834, 0.017336411, 0.00069143757, 0.000303623, -0.061924953, -0.0064005707, -0.0043993946, -0.008697915, -0.012118654));
  target1 += mul(a2, float4x4(-0.0012260727, 0.006306051, -0.004919151, -0.014706935, 0.06893623, -0.03855539, 0.0025126948, -0.013461133, 0.051023327, -0.015535766, -0.0125827445, -0.059677888, -0.0021585734, -0.019920474, -0.025212945, 0.017173553));
  target1 += mul(b2, float4x4(-0.014818789, -0.004695369, 0.11874947, -0.025116654, -0.010446815, -0.015087738, 0.060040206, -0.053225394, -0.059700467, -0.0084348805, 0.11633143, 0.01912765, -0.046732634, 0.02437617, 0.014276953, -0.017528424));
  target1 += mul(c2, float4x4(0.03403683, 0.035661116, -0.05422196, 0.00086722866, 0.0069361166, 0.0030528181, 0.0011153776, 0.0040823813, -0.052100085, 0.016703505, -0.16275159, 0.019807467, -0.0046826405, -0.01290693, -0.00867241, -0.0074261916));
  target1 += mul(d2, float4x4(0.091117546, 0.050540023, -0.018510593, -0.007402161, -0.1193577, 0.018770888, -0.011340929, -0.02110343, -0.032088384, 0.010691935, 0.004420295, -0.025953075, 0.047472738, 0.108008265, 0.007997121, -0.03855365));
  target1 += mul(e2, float4x4(-0.21882823, -0.18101972, 0.13662423, 0.3109504, -0.101242945, 0.3064065, -0.22530204, 0.2612257, -0.07345098, 0.31937975, -0.15872811, 0.23400135, -0.04057178, -0.11676629, -0.34227282, -0.18310128));
  target1 += mul(f2, float4x4(-0.01088255, 0.026722692, -0.0071181543, -0.07676996, -0.054152276, -0.08521186, -0.029249348, 0.005593179, 0.012496848, -0.055432145, 0.06396825, 0.056608576, -0.006908986, 0.018192623, -0.027572934, 0.03749799));
  target1 += mul(g2, float4x4(-0.00788736, 0.032808263, -0.0034198891, -0.01124656, 0.014423269, 0.058434688, 0.0139339, 0.0024755867, 0.042650267, 0.01773591, 0.017099075, 0.00094137667, 0.033293027, 0.008411577, 0.018532667, 0.016402127));
  target1 += mul(h2, float4x4(0.0013495176, -0.05906597, -0.011892358, -0.04260839, 0.0040078545, -0.12263263, -0.005952629, -0.031151159, 0.009523005, -0.04784067, 0.07216081, 0.007988283, -0.010771301, -0.019751243, 0.017268918, -0.1053882));
  target1 += mul(i2, float4x4(0.021729292, -0.006699109, -0.017977247, -0.008347603, 0.030392287, -0.035512295, 0.047333952, -0.061986152, -0.00917743, -0.023669569, -0.051791556, -0.057909377, -0.008901611, -0.010565621, -0.022557132, -0.06957076));
  target1 += mul(na1, float4x4(-0.096115954, 0.013176027, -0.046984393, -0.0064583416, -0.13834997, -0.024369081, 0.049557988, -0.013092948, 0.10623086, -0.0071193436, 0.025198812, -0.00963305, -0.051104847, 0.009814798, 0.0050332784, 0.0058091953));
  target1 += mul(nb1, float4x4(0.03568169, 0.01623718, -0.0020163557, 0.043042913, 0.027783269, -0.06342661, 0.10441675, 0.031614527, -0.17076227, 0.07228563, 0.04167568, 0.022664918, 0.0002446228, 0.01977757, -0.14741875, 0.03596493));
  target1 += mul(nc1, float4x4(-0.028803155, 0.02343672, -0.037556753, 0.004386295, 0.023776755, -0.0024816473, 0.0017886858, -0.005105568, 0.008360341, -0.008270227, -0.12140172, 0.047693867, -0.03565588, -0.0082427105, 0.012581843, 0.0018308035));
  target1 += mul(nd1, float4x4(0.17737128, -0.23239174, 0.14191973, 0.0083567705, 0.022397157, -0.20152177, 0.076320365, 0.11157701, 0.005601583, -0.06157629, -0.060806494, 0.03030779, -0.17968388, -0.2081318, 0.051927045, 0.075377926));
  target1 += mul(ne1, float4x4(-0.28773892, -0.26089972, -0.13325682, -0.46006975, 0.35241324, 0.29463127, -0.16573308, 0.022810405, 0.388681, -0.036075145, 0.2998638, -0.15629162, 0.14321181, 0.10493886, -0.052218314, -0.27016288));
  target1 += mul(nf1, float4x4(0.03584634, 0.006315728, -0.08617273, -0.024391597, -0.016952977, 0.022077272, 0.12980743, 0.04512367, 0.003348057, 0.0946866, 0.16312122, 0.13436604, -0.011872978, -0.031965427, 0.0024880085, 0.033216927));
  target1 += mul(ng1, float4x4(0.016087456, 0.043138605, -0.028770814, 0.0061788377, 0.024897626, 0.10882443, -0.036830436, -0.009145524, -0.057872005, 0.08097352, -0.024710376, 0.0068731857, -0.018163942, -0.04771538, 0.027653048, 0.01914395));
  target1 += mul(nh1, float4x4(0.011542096, -0.073137596, 0.09102133, 0.049714323, -0.06767178, 0.070273116, -0.010473078, -0.120707616, -0.026583942, 0.0730171, -0.08226194, 0.105516605, 0.018596884, 0.05840729, 0.04693975, 0.0863541));
  target1 += mul(ni1, float4x4(0.0127724055, 0.02520005, -0.028792456, -0.06910211, -0.019357776, -0.026941938, 0.05015806, 0.12642363, -0.01354065, -0.015913904, 0.009398767, 0.034318734, -0.0034223567, -0.0146218045, -0.0067832484, -0.010091871));
  target1 += mul(na2, float4x4(-0.02916006, 0.014765165, 0.004575115, 0.0110705905, 0.024664888, 0.003658985, 0.0073659574, 0.0013673811, 0.02650946, 0.014014751, 0.026595473, 0.01877218, 0.016845545, -0.0031619575, -0.011036392, -0.014638798));
  target1 += mul(nb2, float4x4(0.012505482, 0.0023665216, -0.010882385, 0.009143886, -0.030671602, -0.004167823, 0.003649345, -0.00058618153, -0.038002256, -0.0061475867, -0.017000455, -0.015222981, 0.0066633034, 0.013324137, 0.022223728, 0.015254626));
  target1 += mul(nc2, float4x4(-0.019684946, -0.011194834, -0.011896193, -0.009636412, 0.0064974707, -0.018297167, -0.01162353, -0.00998448, 0.022304865, -0.0044090357, -0.0013151226, 0.009721475, -0.0029337434, 0.004208434, -0.008193774, 0.005379128));
  target1 += mul(nd2, float4x4(-0.012884837, -0.057319585, -0.002133779, -0.005586696, -0.03216661, 0.0015534499, -0.004120608, 0.0040779933, -0.044278033, 0.005608415, 0.009365155, 0.04694537, 0.024845028, 0.04563515, 0.018941263, 0.011450428));
  target1 += mul(ne2, float4x4(0.008597113, -0.010005085, -0.050961174, -0.07333081, 0.016683497, -0.056169543, -0.032008786, -0.037104256, -0.01117272, -0.011676191, -0.09071649, -0.049224474, 0.20027469, 0.06436799, 0.1351019, 0.069967836));
  target1 += mul(nf2, float4x4(0.022842692, 0.005048976, 0.05957191, 0.026581423, 0.03748738, 0.074060254, 0.053102568, 0.046449862, -0.013734466, -0.01722293, 0.030430514, -0.02180546, 0.007762467, -0.006432996, 0.08406507, 0.034061644));
  target1 += mul(ng2, float4x4(0.0048395037, 0.012762459, -0.0033284645, -0.0041399547, 0.01828778, 0.0043085683, 0.0019289661, -0.012415563, -0.023572162, -0.050695065, -0.013481175, -0.029202301, -0.03678883, -0.022862522, -0.025002036, -0.010764412));
  target1 += mul(nh2, float4x4(0.0075783907, 0.016249755, 0.0178703, 0.021285253, 0.013031193, 0.025416559, 0.043989707, 0.04750125, 0.0203218, 0.00335042, -0.024657877, -0.05417159, 0.0012374326, 0.115263805, -0.035001434, 0.049407292));
  target1 += mul(ni2, float4x4(0.0059729964, 0.017706383, 0.0004603757, 0.024557583, -0.014231813, 0.0022323965, -0.030447725, -0.005866556, 0.02305865, 0.02982909, 0.0549823, 0.06747715, -0.01014364, 0.0030060427, 0.01640388, 0.056874502));

  float4 target2 = float4(-0.0009249668, -0.0010178088, -0.00041991958, -0.0005421036);
  target2 += mul(a1, float4x4(-0.009785077, -0.007310227, 0.00081595866, -0.01268686, -0.014665477, -0.003956759, -0.0011089307, -0.011515727, 0.024502382, 0.025206817, 0.004246777, -0.0016346163, -0.016379429, -0.013535791, 0.01541915, 0.0095333215));
  target2 += mul(b1, float4x4(-0.017734146, 0.014389035, -0.0008451403, 0.013272096, 0.045607757, 0.01522117, 0.00904139, -0.001765619, 0.024920683, -0.012100507, 0.014870539, 0.0018603726, -0.030391455, 0.00632375, -0.055296343, -0.009885172));
  target2 += mul(c1, float4x4(0.0056769922, 0.0012991864, -0.014343983, 0.0073196087, 0.0061439234, -0.0009862045, 0.0323433, 0.0018582975, -0.00815158, -0.008821831, 0.016262496, -0.014280032, 0.024239268, 0.015745653, 0.016698766, 0.014503724));
  target2 += mul(d1, float4x4(0.039872967, -0.013257727, 0.055065673, 0.034231152, 0.086550154, 0.034081027, 0.045879394, 0.049891002, -0.011800151, -0.011743562, -0.015092318, -0.009334671, -0.017342495, -0.014658795, 0.014266523, 0.035314754));
  target2 += mul(e1, float4x4(-0.050990034, -0.06219798, -0.047669213, -0.07189862, -0.04856067, 0.031102043, 0.001354821, 0.01903025, 0.0037901315, 0.07694083, -0.016825065, 0.009997132, -0.18629807, -0.12768792, -0.104768254, -0.11861362));
  target2 += mul(f1, float4x4(0.017904822, 0.0042992756, 0.016748125, -0.025035992, -0.003724865, -0.0031921281, -0.019930473, 0.017328225, 0.024588963, 0.010205262, 0.04149686, 0.06978651, -0.022708472, -0.0057800277, -0.11644439, -0.06476094));
  target2 += mul(g1, float4x4(-0.02426752, -0.0034115477, -0.0015359819, 0.026405398, -0.013942422, 0.034148987, -0.009329464, -0.005556865, 0.010035298, 0.0042479886, -0.0045719417, -0.007970587, 0.0048700697, -0.0031006113, 0.005171075, 0.0020327016));
  target2 += mul(h1, float4x4(0.0015553721, -0.006999807, -0.027763836, -0.03493009, 0.0047000614, -0.034220867, 0.0021388065, 0.004188802, -0.007897541, -0.025793487, 0.017545879, 0.0013863312, 0.042826407, -0.050083816, 0.037378658, -0.011360738));
  target2 += mul(i1, float4x4(-0.007821516, -0.0034771548, 0.00051019643, 0.017586451, 0.01144453, 0.012032973, 0.0025295757, -0.011105711, 0.009102745, 0.015189803, -0.00083253905, -0.0025097867, -0.008002886, -0.020810502, -0.00023807488, -0.04825592));
  target2 += mul(a2, float4x4(0.005066405, 0.017425792, -0.0004840731, -0.0009944261, 0.07663847, -0.04755453, 0.004607992, -0.020050947, 0.021402068, -0.034427766, -0.0130948955, -0.042138048, 0.015383988, -0.0085578235, -0.036823586, 0.001125214));
  target2 += mul(b2, float4x4(-0.024459356, -0.019538784, 0.13201334, -0.025238393, -0.009611914, -0.017932015, 0.06330252, -0.05036921, -0.09405187, 0.0016108088, 0.07035366, -0.026231728, -0.036375783, 0.047566332, 0.033421457, 0.011572374));
  target2 += mul(c2, float4x4(0.03742729, 0.03181365, -0.05451164, -0.009032132, 0.017350135, -0.011311124, 0.0147211, -0.01298328, -0.011024085, 0.028534293, -0.12944345, 0.07152882, 0.005176979, -0.00048127733, -0.0063332263, -0.0034040876));
  target2 += mul(d2, float4x4(0.06455105, 0.033970848, -0.04488856, -0.027959615, -0.094514206, 0.033421617, 0.031325165, 0.0088970335, -0.031805996, 0.007078957, 0.008114225, -0.017701747, 0.048437405, 0.12445195, 0.02138049, -0.017392302));
  target2 += mul(e2, float4x4(-0.21116845, -0.17855385, 0.12160961, 0.32197994, -0.14490715, 0.2886178, -0.28124997, 0.21847156, -0.04988429, 0.32125694, -0.118747145, 0.26057142, -0.045630034, -0.1453716, -0.3682217, -0.22081932));
  target2 += mul(f2, float4x4(0.0057057277, 0.03872448, 0.020275556, -0.05959739, 0.0150841605, -0.02288727, 0.033048235, 0.08510421, 0.01309789, -0.050875448, 0.051518645, 0.041827686, -0.028529504, -0.0015568004, -0.023128182, 0.03178304));
  target2 += mul(g2, float4x4(0.0016438053, 0.028251547, 0.0003874817, -0.021485088, 0.008020942, 0.052520994, 0.009027988, 0.004729575, 0.026685065, 0.008003427, 0.013078419, -0.008256319, 0.022743277, -0.001293671, 0.018562315, 0.016649859));
  target2 += mul(h2, float4x4(0.013438089, -0.049052995, 0.0060880547, -0.044865325, 0.031890247, -0.102749884, 0.0047795745, -0.028551944, -0.018443404, -0.061510604, 0.031782348, -0.0005923042, 0.014257579, 0.010379952, 0.02929872, -0.090405114));
  target2 += mul(i2, float4x4(0.009318741, -0.0061841, -0.02420737, 0.0018885462, 0.022010826, -0.023001686, 0.035959963, -0.057635445, 0.012495818, -0.008206369, -0.026234211, -0.04719263, 0.0057711657, -0.003004966, 0.0046920753, -0.041684203));
  target2 += mul(na1, float4x4(-0.050602015, 0.021741746, -0.059019636, -0.008416951, -0.1789153, -0.01835426, 0.03100039, -0.017736796, 0.09091737, -0.026542341, 0.010933376, -0.031898204, -0.015792761, 0.013789206, 0.031699985, 0.018964434));
  target2 += mul(nb1, float4x4(0.099863164, -0.01637541, 0.083744444, 0.011983074, 0.013478042, -0.04780451, 0.08646149, 0.050255097, -0.22476238, 0.11746969, 0.038574144, 0.069615066, 0.047265753, -0.03212485, -0.12651724, -0.0065722666));
  target2 += mul(nc1, float4x4(-0.026888395, 0.0053314343, -0.0018114679, -0.007841625, 0.00037234774, -0.005450839, -0.03730409, -0.00441375, -0.014338566, 0.002887282, -0.19375902, 0.06374498, -0.033998128, -0.03480894, 0.061709825, -0.016935369));
  target2 += mul(nd1, float4x4(0.18882285, -0.19729713, 0.064650975, -0.07342598, -0.039107442, -0.28614163, 0.081506595, 0.111678764, -0.0019596675, -0.071805045, -0.019774346, 0.055490687, -0.1405711, -0.16753702, 0.031397972, 0.054546997));
  target2 += mul(ne1, float4x4(-0.007561914, 0.0010002917, 0.12623467, -0.17501056, 0.22664371, 0.2080332, -0.3194733, -0.1065412, 0.21299458, -0.23856679, 0.17237303, -0.2863369, 0.35997602, 0.354653, 0.15091361, -0.07142766));
  target2 += mul(nf1, float4x4(0.02403396, 0.0037063402, -0.004992154, 0.047530055, -0.03227084, -0.0055595553, 0.06554937, -0.025955811, -0.03792351, 0.041418597, 0.04285587, -0.0118592, 0.00012291886, -0.013734975, 0.07748641, 0.14016038));
  target2 += mul(ng1, float4x4(0.015037119, 0.058259863, -0.020877289, -0.0059153647, 0.04133679, 0.108832926, -0.026314106, -0.0010898053, -0.057873078, 0.07802038, -0.029681025, 0.020011986, -0.03940851, -0.038397703, 0.013701823, 0.01657068));
  target2 += mul(nh1, float4x4(-0.016823404, 0.007905321, 0.034658395, 0.09977579, -0.05916761, 0.004779212, 0.018820778, -0.15795219, -0.013125517, 0.021101758, -0.055992976, 0.08024182, -0.04333755, 0.070356764, -0.030624833, 0.09123745));
  target2 += mul(ni1, float4x4(-0.007931201, 0.0069976873, -0.016831044, -0.027368804, -0.03332386, -0.041667387, 0.04094055, 0.095304705, -0.006027611, -0.019209528, -0.0008929939, -0.017201519, 0.005464988, 0.0038448595, -0.01248845, 0.008877873));
  target2 += mul(na2, float4x4(-0.042160366, 0.0036025376, -0.008628986, -0.005607383, 0.028637825, 0.005296032, -0.0004143198, 0.008265197, 0.033176135, 0.014727739, 0.0145593295, 0.011159069, 0.00833305, -0.0025515268, -0.00015546188, 0.002805437));
  target2 += mul(nb2, float4x4(0.016752163, 0.013423374, -0.018342504, 0.013459657, -0.038428728, -0.005804395, 0.019692563, -0.005745392, -0.030070104, 0.01058409, 0.003989377, 0.0074200635, -0.01936366, -0.01608809, 0.0071134195, -0.0038598357));
  target2 += mul(nc2, float4x4(-0.018000437, -0.0121247275, -0.01288339, -0.0060898345, -0.006138964, -0.0035810755, -0.03902352, 0.002276941, 0.0032195079, -0.02730975, -0.011268412, -0.0036179612, -0.004836894, -0.0015986725, -0.019751905, -0.0071931942));
  target2 += mul(nd2, float4x4(0.014426659, -0.05161329, 0.019196855, 0.002317663, -0.055477437, -0.007086505, -0.04151144, -0.027518485, -0.027440753, 0.003857541, -0.002143262, 0.013090804, 0.015745236, 0.021075105, 7.93909e-06, -0.009694458));
  target2 += mul(ne2, float4x4(0.0025894733, -0.017304689, -0.03299281, -0.0754248, 0.03428733, -0.03397887, 0.0108591765, 0.021311574, -0.04203291, -0.019728655, -0.09826257, -0.046157785, 0.22522739, 0.086717755, 0.15654634, 0.08489247));
  target2 += mul(nf2, float4x4(0.008495083, 0.00074552774, 0.038054205, 0.013044046, -0.027891211, 0.003249458, -0.018353004, -0.035205863, -0.010195661, -0.008145831, 0.014239584, -0.019779535, 0.011452498, 0.004117014, 0.08403766, 0.04357078));
  target2 += mul(ng2, float4x4(0.00020427872, 0.026861027, -0.01047743, 0.0034385168, 0.015686916, 0.00038722693, 0.0017860534, -0.021630246, -0.0084784245, -0.022648407, -0.0050631054, -0.016437376, -0.026458954, -0.011239073, -0.01145464, -0.0058855377));
  target2 += mul(nh2, float4x4(-0.0012052609, 0.009248192, 0.008875674, 0.03043022, 0.012489936, 0.019402692, 0.0378006, 0.05519605, 0.029059285, -0.0072894073, 0.0014154738, -0.03802288, -0.02321437, 0.09558396, -0.0550932, 0.036936663));
  target2 += mul(ni2, float4x4(0.010010094, 0.012796987, 0.0025080708, 0.013876455, -0.00536739, -0.016932324, -0.012128944, -0.0241354, 0.0077782627, 0.01584833, 0.033727348, 0.039302748, -0.026609577, -0.0062910756, -0.011042692, 0.031207075));

  float4 target3 = float4(-0.0021447246, -0.0025527438, -0.0016466968, -0.0020245572);
  target3 += mul(a1, float4x4(-0.01766077, -0.017591428, -0.0038036762, -0.023304595, -0.012525157, -0.0058148014, -0.0030130956, -0.011804012, 0.030511979, 0.028687771, 0.007858589, 0.004475508, -0.02585795, -0.01785211, 0.0053741997, 0.00074623496));
  target3 += mul(b1, float4x4(-0.040601525, 0.016486213, -0.01966552, 0.014969501, 0.05400945, 0.019022502, 0.0149923405, -0.0017570893, 0.040684238, -0.009271634, 0.026908487, 0.002365157, -0.03371985, 0.00928091, -0.058665182, -0.0047038617));
  target3 += mul(c1, float4x4(0.0034900296, 0.0028777388, -0.02543823, 0.005724228, 0.012073974, 0.0043754885, 0.04109826, 0.008040286, -0.00049979525, -0.0063444753, 0.030565983, -0.009352674, 0.01949427, 0.014168137, 0.009640578, 0.011481213));
  target3 += mul(d1, float4x4(0.026645018, -0.02211462, 0.06119815, 0.039082125, 0.09945218, 0.042240527, 0.054267537, 0.04693634, -0.004510591, -0.0041247807, -0.012629442, -0.008053095, -0.025141539, -0.025081929, 0.011338651, 0.029372308));
  target3 += mul(e1, float4x4(-0.102688424, -0.11533188, -0.09621349, -0.116714895, -0.025504943, 0.05013811, 0.024331303, 0.03946124, 0.026381869, 0.1011479, -0.0017481856, 0.027152762, -0.18783632, -0.13439077, -0.112003446, -0.12810163));
  target3 += mul(f1, float4x4(0.010783576, -0.00025257064, -0.0075445045, -0.04681932, -0.0021722934, -0.005758047, -0.0110701695, 0.023468157, 0.036986902, 0.023351438, 0.063143626, 0.09269854, -0.025713218, -0.011750105, -0.11722637, -0.07038934));
  target3 += mul(g1, float4x4(-0.026961634, -0.015106367, -0.0034014166, 0.02482031, -0.013892242, 0.04203608, -0.008226002, 0.004619446, 0.012888606, 0.010721662, -1.3880494e-05, -0.0033224574, 0.006727405, -0.0035630877, 0.0021499102, -0.00091816986));
  target3 += mul(h1, float4x4(0.0016877668, -0.02695227, -0.023388471, -0.053411417, 0.006777518, -0.024251794, 0.0015210172, 0.010034961, -0.00795588, -0.01645489, 0.012691467, 0.0061330614, 0.054507505, -0.041002143, 0.048495438, -0.004843492));
  target3 += mul(i1, float4x4(-0.0159168, -0.013163069, -0.0091357315, 0.0011109188, 0.022993349, 0.025777856, 0.013487494, 0.00304372, 0.014121591, 0.02415322, 0.006453722, 0.010679647, -0.00626483, -0.017908117, 0.0063728937, -0.04091484));
  target3 += mul(a2, float4x4(-0.0026799496, 0.0154166315, -0.0037383793, -0.002577431, 0.073905826, -0.043148544, 0.011774636, -0.016023275, 0.0099145975, -0.04718069, -0.013578048, -0.04220935, 0.018033838, -0.0025958812, -0.029762078, 0.0034059538));
  target3 += mul(b2, float4x4(-0.03239311, -0.025743088, 0.1116615, -0.027325295, -0.014691433, -0.013614988, 0.05034416, -0.04294835, -0.11013415, -0.014086726, 0.048601545, -0.04762435, -0.01944709, 0.054276068, 0.04073586, 0.019288493));
  target3 += mul(c2, float4x4(0.027851144, 0.014083208, -0.06432852, -0.024642657, 0.021185134, -0.015441491, 0.018058551, -0.017353412, -0.018814132, 0.026259383, -0.14238997, 0.06301044, 0.007324441, 0.00494394, 0.00020533071, 0.0024405916));
  target3 += mul(d2, float4x4(0.06092095, 0.025730716, -0.042129956, -0.026382709, -0.08284398, 0.03344148, 0.038016047, 0.0137958275, -0.025555719, 0.008199355, 0.0070835026, -0.01420561, 0.0493976, 0.121205755, 0.026178997, -0.006300481));
  target3 += mul(e2, float4x4(-0.18660638, -0.1658202, 0.116562665, 0.29287666, -0.13814074, 0.2658047, -0.270531, 0.19597577, -0.04692207, 0.28904793, -0.09829146, 0.24158104, -0.03946344, -0.12598358, -0.3361825, -0.19800447));
  target3 += mul(f2, float4x4(0.020092675, 0.049266458, 0.03696139, -0.046251137, 0.029122403, -0.008378672, 0.044602558, 0.092563495, -0.0036082428, -0.072675824, 0.030523287, 0.006169521, -0.031133244, -0.011250458, -0.026590217, 0.023079094));
  target3 += mul(g2, float4x4(0.007384019, 0.031913586, 0.002072675, -0.019807052, 0.010384438, 0.050076224, 0.010438329, 0.009595051, 0.022497892, 0.012009176, 0.009222753, -0.008563874, 0.017106988, -0.003105622, 0.01070336, 0.011805944));
  target3 += mul(h2, float4x4(0.017091183, -0.035133313, 0.012425838, -0.03395959, 0.03418688, -0.10616231, 0.0101681305, -0.03682252, -0.016497994, -0.05231084, 0.025178006, 0.008926557, 0.025942912, 0.019970346, 0.03534238, -0.07596637));
  target3 += mul(i2, float4x4(0.007215777, -0.0006424821, -0.020822426, 0.011314772, 0.0183502, -0.015352454, 0.02972497, -0.053287935, 0.024020335, -0.006380922, -0.008620774, -0.041896872, 0.021631774, 0.013320375, 0.024711635, -0.020357909));
  target3 += mul(na1, float4x4(-0.033131246, 0.027936278, -0.047840517, 0.0019488486, -0.17501047, -0.0178374, 0.02549812, -0.019010937, 0.079489246, -0.027291514, 0.004313802, -0.03478066, -0.004887971, 0.019281879, 0.04073947, 0.022658588));
  target3 += mul(nb1, float4x4(0.110482916, -0.021340236, 0.09848104, 0.0034104201, 0.0032655075, -0.04557326, 0.07156056, 0.045965493, -0.22822224, 0.115162075, 0.027745042, 0.07251069, 0.05100454, -0.034554593, -0.11214564, -0.009064197));
  target3 += mul(nc1, float4x4(-0.017621655, 0.01024623, 0.009554872, -0.00078690174, -0.0069463328, -0.014670676, -0.041410644, -0.007414249, -0.031177497, -0.007517117, -0.20814678, 0.049873244, -0.02482445, -0.031338003, 0.06920326, -0.015171424));
  target3 += mul(nd1, float4x4(0.18918292, -0.15450309, 0.05504167, -0.061840136, -0.057958793, -0.28908864, 0.06820344, 0.09923399, -0.008387437, -0.075379215, -0.01747373, 0.048925415, -0.13222353, -0.15354146, 0.022480693, 0.04943612));
  target3 += mul(ne1, float4x4(0.0469381, 0.05393423, 0.1681062, -0.10543653, 0.17948511, 0.16570628, -0.33344334, -0.13197891, 0.16509773, -0.26174626, 0.13757275, -0.29244694, 0.35424834, 0.35368237, 0.156861, -0.04775442));
  target3 += mul(nf1, float4x4(0.026892537, 0.0075510717, 0.015918663, 0.06070227, -0.02288592, 0.0027507204, 0.05279965, -0.03042772, -0.044760384, 0.0234673, 0.01604264, -0.04277388, 0.0011313064, -0.0052253264, 0.08374709, 0.14929597));
  target3 += mul(ng1, float4x4(0.016119812, 0.061383534, -0.013537205, -0.0017921093, 0.043676157, 0.09811408, -0.015655283, 0.0007943268, -0.053843908, 0.069290705, -0.028319253, 0.020141726, -0.038996387, -0.03628716, 0.012679114, 0.015012319));
  target3 += mul(nh1, float4x4(-0.02019775, 0.022393003, 0.020688228, 0.10277296, -0.06365119, -0.015666502, 0.012721399, -0.16204305, -0.0037819904, 0.012113873, -0.040969223, 0.069086574, -0.052415807, 0.060331605, -0.04201384, 0.07953157));
  target3 += mul(ni1, float4x4(-0.0019123453, 0.012750492, -0.007235785, -0.01268919, -0.038674437, -0.043993857, 0.028753003, 0.07664717, -0.015077012, -0.027486047, -0.011141094, -0.030269727, 0.0016567699, -0.003331901, -0.021631587, -0.00040226072));
  target3 += mul(na2, float4x4(-0.03769701, 0.0045639244, -0.0069983527, -0.0064906892, 0.03318896, 0.011733902, 0.0023203227, 0.013374876, 0.037507236, 0.018019466, 0.013330661, 0.009231364, 0.00018865235, -0.005706915, -0.00011657552, 0.0038968239));
  target3 += mul(nb2, float4x4(0.022072105, 0.019486066, -0.013029048, 0.017470635, -0.03662149, -0.011397823, 0.02397534, -0.008561204, -0.026196644, 0.01626692, 0.011886567, 0.021061733, -0.03310679, -0.025446283, -0.006469576, -0.010118362));
  target3 += mul(nc2, float4x4(-0.014853227, -0.0062806485, -0.005624992, 0.0017175867, -0.007843849, 0.0008925535, -0.041000694, 0.0049381475, 0.0019743184, -0.035099152, -0.01074269, -0.0128827905, -0.010841019, -0.0093286475, -0.030476939, -0.018505717));
  target3 += mul(nd2, float4x4(0.016344415, -0.04647131, 0.021242643, 0.004836572, -0.061090752, -0.006488986, -0.050970413, -0.029668579, -0.015889898, 0.010811246, 0.0018357672, 0.012481409, 0.008317143, 0.009978102, -0.0015472731, -0.011174326));
  target3 += mul(ne2, float4x4(-0.004087798, -0.01634328, -0.031607483, -0.068488315, 0.038035624, -0.02797923, 0.017972443, 0.029961389, -0.029277585, -0.015558678, -0.08634699, -0.039436456, 0.19870138, 0.06507983, 0.130592, 0.059745777));
  target3 += mul(nf2, float4x4(-0.0028183246, -0.008089249, 0.02188247, 0.0049699014, -0.03830487, -0.0079993615, -0.028960107, -0.045729056, 0.0021651732, 0.010072074, 0.031335246, 0.0012719089, 0.015795005, 0.011290197, 0.08071912, 0.04273827));
  target3 += mul(ng2, float4x4(-0.0011167483, 0.024682038, -0.009224286, 0.005520499, 0.014198537, -0.0032909375, 0.0005767499, -0.02676088, -0.0019766665, -0.015222206, -0.00080782827, -0.011807755, -0.02560086, -0.015391911, -0.008948504, -0.0062184683));
  target3 += mul(nh2, float4x4(-0.009399661, -0.0019192873, 0.000261681, 0.020112153, 0.0077712294, 0.019477246, 0.030144244, 0.053777162, 0.030650103, 0.0021887033, 0.0092345085, -0.029658241, -0.03723785, 0.073152155, -0.058525253, 0.0230170564));
  target3 += mul(ni2, float4x4(0.012911211, 0.010375983, -0.00055489264, 0.005504194, -0.004187377, -0.02239082, -0.008734182, -0.027458502, -0.005602922, 0.009588401, 0.015889015, 0.036346428, -0.038325973, -0.018252429, -0.02944341, 0.01149068));

  float3 cnn_rgb;
  if (sub_idx == 0)      cnn_rgb = float3(target1.x, target2.x, target3.x);
  else if (sub_idx == 1) cnn_rgb = float3(target1.y, target2.y, target3.y);
  else if (sub_idx == 2) cnn_rgb = float3(target1.z, target2.z, target3.z);
  else                   cnn_rgb = float3(target1.w, target2.w, target3.w);

  float3 residual = u_Texture.SampleLevel(u_Texture_sampler, PSIn.UV, 0).rgb;
  PSOut.Color = float4(cnn_rgb + residual, 1.0);
}
)";

}  // namespace renderer
