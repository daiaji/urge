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

  const float3 kOnes = float3(1.0, 1.0, 1.0);
  float suml = dot(linetaps1, kOnes) + dot(linetaps2, kOnes);
  float sumc = dot(columntaps1, kOnes) + dot(columntaps2, kOnes);
  linetaps1 /= suml;
  linetaps2 /= suml;
  columntaps1 /= sumc;
  columntaps2 /= sumc;

  pos -= f + 1.5;

  float3 src[6][6];

  [unroll] for (uint i = 0; i <= 4; i += 2) {
    [unroll] for (uint j = 0; j <= 4; j += 2) {
      float2 tpos = (pos + uint2(i, j)) * input_pt;
      const float4 sr = u_Texture.GatherRed(u_Texture_sampler, tpos);
      const float4 sg = u_Texture.GatherGreen(u_Texture_sampler, tpos);
      const float4 sb = u_Texture.GatherBlue(u_Texture_sampler, tpos);

      src[i][j].x = sr.w;
      src[i][j].y = sg.w;
      src[i][j].z = sb.w;
      // GLSL Gather returns wzyx instead of HLSL's wxyz.
      // When URGE_GLSL_GATHER is defined (OpenGL), swap .x and .z.
#ifdef URGE_GLSL_GATHER
      src[i][j + 1].x = sr.z;
      src[i][j + 1].y = sg.z;
      src[i][j + 1].z = sb.z;
      src[i + 1][j].x = sr.x;
      src[i + 1][j].y = sg.x;
      src[i + 1][j].z = sb.x;
#else
      src[i][j + 1].x = sr.x;
      src[i][j + 1].y = sg.x;
      src[i][j + 1].z = sb.x;
      src[i + 1][j].x = sr.z;
      src[i + 1][j].y = sg.z;
      src[i + 1][j].z = sb.z;
#endif
      src[i + 1][j + 1].x = sr.y;
      src[i + 1][j + 1].y = sg.y;
      src[i + 1][j + 1].z = sb.y;
    }
  }

  float3 color = float3(0, 0, 0);
  [unroll] for (uint i = 0; i <= 4; i += 2) {
    color += (mul(linetaps1, float3x3(src[0][i], src[2][i], src[4][i])) +
              mul(linetaps2, float3x3(src[1][i], src[3][i], src[5][i]))) *
             columntaps1[i / 2] +
             (mul(linetaps1, float3x3(src[0][i + 1], src[2][i + 1], src[4][i + 1])) +
              mul(linetaps2, float3x3(src[1][i + 1], src[3][i + 1], src[5][i + 1]))) *
             columntaps2[i / 2];
  }

  float3 min_sample = min(min(src[2][2], src[3][2]), min(src[2][3], src[3][3]));
  float3 max_sample = max(max(src[2][2], src[3][2]), max(src[2][3], src[3][3]));
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

// ---- Main ----
void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  float2 uv = PSIn.UV;

  if (u_Mode == MODE_NEAREST) {
    float2 texel = floor(uv * u_InputSize) + 0.5;
    uv = texel * u_InputPt;
    PSOut.Color = u_Texture.Sample(u_Texture_sampler, uv);
  } else if (u_Mode == MODE_LANCZOS3) {
    PSOut.Color = float4(SampleLanczos3(uv), 1.0);
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

}  // namespace renderer
