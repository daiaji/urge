// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/pipeline/render_pipeline.h"

#include <chrono>
#include <cstring>
#include <string>
#include <utility>

#include "base/debug/logging.h"
#include "DataBlobImpl.hpp"
#include "renderer/pipeline/builtin_hlsl.h"

namespace renderer {

namespace {

class ScopedPipelineTimer {
 public:
  explicit ScopedPipelineTimer(std::string label)
      : label_(std::move(label)), start_(std::chrono::steady_clock::now()) {}

  ~ScopedPipelineTimer() {
    const auto elapsed = std::chrono::steady_clock::now() - start_;
    LOG(INFO) << label_ << " finished in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed)
                     .count()
              << "ms";
  }

 private:
  std::string label_;
  std::chrono::steady_clock::time_point start_;
};

void LogShaderCreateInfo(const char* stage,
                         const std::string& name,
                         size_t source_length,
                         size_t macro_count) {
  LOG(INFO) << "[Graphics] Creating " << stage << " shader " << name
            << " source_bytes=" << source_length
            << " macros=" << macro_count;
}

void LogShaderCreateFailure(const char* stage, const std::string& name) {
  LOG(ERROR) << "[Graphics] Failed to create " << stage << " shader " << name;
}

}  // namespace

bool PipelineSet::EnsureAnime4KUDLLoaders() {
  if (anime4k_udl_pass0 && anime4k_udl_pass1 && anime4k_udl_pass2 &&
      anime4k_udl_pass3) {
    return true;
  }

  anime4k_udl_pass0 = std::make_unique<Pipeline_Anime4K_UDL_Pass0>(init_params);
  anime4k_udl_pass1 = std::make_unique<Pipeline_Anime4K_UDL_Pass1>(init_params);
  anime4k_udl_pass2 = std::make_unique<Pipeline_Anime4K_UDL_Pass2>(init_params);
  anime4k_udl_pass3 = std::make_unique<Pipeline_Anime4K_UDL_Pass3>(init_params);
  if (!anime4k_udl_pass0 || !anime4k_udl_pass1 || !anime4k_udl_pass2 ||
      !anime4k_udl_pass3) {
    LOG(ERROR) << "[Graphics] Failed to create Anime4K UDL pipeline loaders";
    anime4k_udl_pass0.reset();
    anime4k_udl_pass1.reset();
    anime4k_udl_pass2.reset();
    anime4k_udl_pass3.reset();
    return false;
  }

  return true;
}

bool PipelineSet::EnsureCuNNy4x16Loaders() {
  if (cunny_4x16_p1 && cunny_4x16_p2 && cunny_4x16_p3 && cunny_4x16_p4 &&
      cunny_4x16_p5 && cunny_4x16_p6) {
    return true;
  }

  cunny_4x16_p1 = std::make_unique<Pipeline_CuNNy_4x16_Pass1>(init_params);
  cunny_4x16_p2 = std::make_unique<Pipeline_CuNNy_4x16_Pass2>(init_params);
  cunny_4x16_p3 = std::make_unique<Pipeline_CuNNy_4x16_Pass3>(init_params);
  cunny_4x16_p4 = std::make_unique<Pipeline_CuNNy_4x16_Pass4>(init_params);
  cunny_4x16_p5 = std::make_unique<Pipeline_CuNNy_4x16_Pass5>(init_params);
  cunny_4x16_p6 = std::make_unique<Pipeline_CuNNy_4x16_Pass6>(init_params);
  if (!cunny_4x16_p1 || !cunny_4x16_p2 || !cunny_4x16_p3 ||
      !cunny_4x16_p4 || !cunny_4x16_p5 || !cunny_4x16_p6) {
    LOG(ERROR) << "[Graphics] Failed to create CuNNy-4x16 pipeline loaders";
    cunny_4x16_p1.reset();
    cunny_4x16_p2.reset();
    cunny_4x16_p3.reset();
    cunny_4x16_p4.reset();
    cunny_4x16_p5.reset();
    cunny_4x16_p6.reset();
    return false;
  }

  return true;
}

bool PipelineSet::EnsureCuNNy4x24Loaders() {
  if (cunny_4x24_p1 && cunny_4x24_p2 && cunny_4x24_p3 && cunny_4x24_p4 &&
      cunny_4x24_p5 && cunny_4x24_p6) {
    return true;
  }

  cunny_4x24_p1 = std::make_unique<Pipeline_CuNNy_4x24_Pass1>(init_params);
  cunny_4x24_p2 = std::make_unique<Pipeline_CuNNy_4x24_Pass2>(init_params);
  cunny_4x24_p3 = std::make_unique<Pipeline_CuNNy_4x24_Pass3>(init_params);
  cunny_4x24_p4 = std::make_unique<Pipeline_CuNNy_4x24_Pass4>(init_params);
  cunny_4x24_p5 = std::make_unique<Pipeline_CuNNy_4x24_Pass5>(init_params);
  cunny_4x24_p6 = std::make_unique<Pipeline_CuNNy_4x24_Pass6>(init_params);
  if (!cunny_4x24_p1 || !cunny_4x24_p2 || !cunny_4x24_p3 ||
      !cunny_4x24_p4 || !cunny_4x24_p5 || !cunny_4x24_p6) {
    LOG(ERROR) << "[Graphics] Failed to create CuNNy-4x24 pipeline loaders";
    cunny_4x24_p1.reset();
    cunny_4x24_p2.reset();
    cunny_4x24_p3.reset();
    cunny_4x24_p4.reset();
    cunny_4x24_p5.reset();
    cunny_4x24_p6.reset();
    return false;
  }

  return true;
}

RenderPipelineBase::RenderPipelineBase(
    Diligent::IRenderDevice* device,
    Diligent::IBytecodeCache* bytecode_cache)
    : device_(device), bytecode_cache_(bytecode_cache) {}

bool RenderPipelineBase::CreateShaderWithCache(
    const Diligent::ShaderCreateInfo& shader_desc,
    Diligent::IShader** output) {
  if (!bytecode_cache_) {
    device_->CreateShader(shader_desc, output);
    return false;
  }

  RRefPtr<Diligent::IDataBlob> bytecode;
  bytecode_cache_->GetBytecode(shader_desc, &bytecode);
  const bool cache_hit = bytecode != nullptr;
  LOG(INFO) << "[Graphics] Shader bytecode cache "
            << (cache_hit ? "hit" : "miss") << ": " << shader_desc.Desc.Name;

  if (cache_hit) {
    Diligent::ShaderCreateInfo cached_shader_desc = shader_desc;
    cached_shader_desc.Source = nullptr;
    cached_shader_desc.ByteCode = bytecode->GetConstDataPtr();
    cached_shader_desc.ByteCodeSize = bytecode->GetSize();
    device_->CreateShader(cached_shader_desc, output);
  } else {
    device_->CreateShader(shader_desc, output);
    if (*output) {
      const void* compiled_bytecode = nullptr;
      Diligent::Uint64 compiled_bytecode_size = 0;
      (*output)->GetBytecode(&compiled_bytecode, compiled_bytecode_size);
      if (compiled_bytecode && compiled_bytecode_size > 0) {
        RRefPtr<Diligent::IDataBlob> compiled_blob;
        compiled_blob = Diligent::DataBlobImpl::Create(
            static_cast<size_t>(compiled_bytecode_size));
        if (compiled_blob) {
          std::memcpy(compiled_blob->GetDataPtr(), compiled_bytecode,
                      compiled_bytecode_size);
          bytecode_cache_->AddBytecode(shader_desc, compiled_blob);
        }
      }
    }
  }

  if (cache_hit && !*output) {
    LOG(WARNING) << "[Graphics] Shader bytecode cache path failed, fallback "
                    "compiling: "
                 << shader_desc.Desc.Name;
    bytecode_cache_->RemoveBytecode(shader_desc);
    device_->CreateShader(shader_desc, output);
  }
  return cache_hit;
}

void RenderPipelineBase::BuildPipeline(
    Diligent::IPipelineState** output,
    const Diligent::BlendStateDesc& blend_state,
    const Diligent::RasterizerStateDesc& rasterizer_state,
    const Diligent::DepthStencilStateDesc& depth_stencil_state,
    Diligent::PRIMITIVE_TOPOLOGY topology,
    const std::vector<Diligent::TEXTURE_FORMAT>& target_formats,
    Diligent::TEXTURE_FORMAT depth_stencil_format,
    const Diligent::SampleDesc& sample_state) {
  Diligent::GraphicsPipelineStateCreateInfo pipeline_state_desc;

  // Pipeline name
  pipeline_state_desc.PSODesc.Name = pipeline_name_.c_str();
  pipeline_state_desc.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;

  // Shaders
  pipeline_state_desc.pVS = vertex_shader_object_;
  pipeline_state_desc.pPS = pixel_shader_object_;

  // Resource signature
  std::vector<Diligent::IPipelineResourceSignature*> signatures;
  for (auto& it : resource_signatures_)
    signatures.push_back(it.RawPtr());
  pipeline_state_desc.ResourceSignaturesCount = signatures.size();
  pipeline_state_desc.ppResourceSignatures = signatures.data();

  // Input assembly
  pipeline_state_desc.GraphicsPipeline.InputLayout.NumElements =
      input_layout_.size();
  pipeline_state_desc.GraphicsPipeline.InputLayout.LayoutElements =
      input_layout_.data();

  // User specific states
  pipeline_state_desc.GraphicsPipeline.BlendDesc = blend_state;
  pipeline_state_desc.GraphicsPipeline.RasterizerDesc = rasterizer_state;
  pipeline_state_desc.GraphicsPipeline.DepthStencilDesc = depth_stencil_state;

  // Primitive topology
  pipeline_state_desc.GraphicsPipeline.PrimitiveTopology = topology;

  // Render targets
  const size_t num_render_targets =
      std::min<size_t>(target_formats.size(), DILIGENT_MAX_RENDER_TARGETS);
  pipeline_state_desc.GraphicsPipeline.NumRenderTargets = target_formats.size();
  for (size_t i = 0; i < num_render_targets; ++i)
    pipeline_state_desc.GraphicsPipeline.RTVFormats[i] = target_formats[i];

  // DSV format
  pipeline_state_desc.GraphicsPipeline.DSVFormat = depth_stencil_format;

  // Multisample desc
  pipeline_state_desc.GraphicsPipeline.SmplDesc = sample_state;

  // Make pipeline
  device_->CreateGraphicsPipelineState(pipeline_state_desc, output);
}

void RenderPipelineBase::BuildComputePipeline(
    Diligent::IPipelineState** output) {
  Diligent::ComputePipelineStateCreateInfo pipeline_state_desc;

  pipeline_state_desc.PSODesc.Name = pipeline_name_.c_str();
  pipeline_state_desc.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_COMPUTE;
  pipeline_state_desc.pCS = compute_shader_object_;

  std::vector<Diligent::IPipelineResourceSignature*> signatures;
  for (auto& it : resource_signatures_)
    signatures.push_back(it.RawPtr());
  pipeline_state_desc.ResourceSignaturesCount = signatures.size();
  pipeline_state_desc.ppResourceSignatures = signatures.data();

  device_->CreateComputePipelineState(pipeline_state_desc, output);
}

void RenderPipelineBase::SetupPipelineBasis(
    const ShaderSource& shader_source,
    const std::vector<Diligent::LayoutElement>& input_layout,
    const std::vector<RRefPtr<Diligent::IPipelineResourceSignature>>&
        signatures) {
  // Make pipeline debug name
  pipeline_name_ = "internal.pipeline<" + shader_source.name + ">";
  const std::string vertex_shader_name = shader_source.name + ".vertex";
  const std::string pixel_shader_name = shader_source.name + ".pixel";

  // Make vertex shader and pixel shader
  Diligent::ShaderCreateInfo shader_desc;
  shader_desc.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
  shader_desc.Desc.UseCombinedTextureSamplers = Diligent::True;
  shader_desc.CompileFlags =
      Diligent::SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;

  {  // Vertex shader
    shader_desc.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
    shader_desc.EntryPoint = shader_source.vertex_entry.c_str();
    shader_desc.Desc.Name = vertex_shader_name.c_str();
    shader_desc.Source = shader_source.vertex_shader.c_str();
    shader_desc.SourceLength = shader_source.vertex_shader.size();
    shader_desc.Macros.Count = shader_source.macros.size();
    shader_desc.Macros.Elements = shader_source.macros.data();
    LogShaderCreateInfo("vertex", shader_source.name,
                        shader_source.vertex_shader.size(),
                        shader_source.macros.size());
    {
      ScopedPipelineTimer timer("[Graphics] Create vertex shader " +
                                shader_source.name);
      CreateShaderWithCache(shader_desc, &vertex_shader_object_);
    }
    if (!vertex_shader_object_)
      LogShaderCreateFailure("vertex", shader_source.name);
  }

  {  // Pixel shader
    shader_desc.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
    shader_desc.EntryPoint = shader_source.pixel_entry.c_str();
    shader_desc.Desc.Name = pixel_shader_name.c_str();
    shader_desc.Source = shader_source.pixel_shader.c_str();
    shader_desc.SourceLength = shader_source.pixel_shader.size();
    shader_desc.Macros.Count = shader_source.macros.size();
    shader_desc.Macros.Elements = shader_source.macros.data();
    LogShaderCreateInfo("pixel", shader_source.name,
                        shader_source.pixel_shader.size(),
                        shader_source.macros.size());
    {
      ScopedPipelineTimer timer("[Graphics] Create pixel shader " +
                                shader_source.name);
      CreateShaderWithCache(shader_desc, &pixel_shader_object_);
    }
    if (!pixel_shader_object_)
      LogShaderCreateFailure("pixel", shader_source.name);
  }

  // Setup input attribute elements
  input_layout_ = input_layout;

  // Setup resource signature
  resource_signatures_ = signatures;
}

void RenderPipelineBase::SetupComputePipelineBasis(
    const ComputeShaderSource& shader_source,
    const std::vector<RRefPtr<Diligent::IPipelineResourceSignature>>&
        signatures) {
  pipeline_name_ = "internal.pipeline<" + shader_source.name + ">";

  Diligent::ShaderCreateInfo shader_desc;
  shader_desc.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
  shader_desc.Desc.UseCombinedTextureSamplers = Diligent::True;
  shader_desc.CompileFlags =
      Diligent::SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;
  shader_desc.Desc.ShaderType = Diligent::SHADER_TYPE_COMPUTE;
  shader_desc.EntryPoint = shader_source.compute_entry.c_str();
  shader_desc.Desc.Name = shader_source.name.c_str();
  shader_desc.Source = shader_source.compute_shader.c_str();
  shader_desc.SourceLength = shader_source.compute_shader.size();
  shader_desc.Macros.Count = shader_source.macros.size();
  shader_desc.Macros.Elements = shader_source.macros.data();
  LogShaderCreateInfo("compute", shader_source.name,
                      shader_source.compute_shader.size(),
                      shader_source.macros.size());
  {
    ScopedPipelineTimer timer("[Graphics] Create compute shader " +
                              shader_source.name);
    CreateShaderWithCache(shader_desc, &compute_shader_object_);
  }
  if (!compute_shader_object_)
    LogShaderCreateFailure("compute", shader_source.name);

  resource_signatures_ = signatures;
}

RRefPtr<Diligent::IPipelineResourceSignature>
RenderPipelineBase::MakeResourceSignature(
    const std::vector<Diligent::PipelineResourceDesc>& variables,
    const std::vector<Diligent::ImmutableSamplerDesc>& samplers,
    uint8_t binding_index) {
  Diligent::PipelineResourceSignatureDesc resource_signature_desc;
  resource_signature_desc.Resources = variables.data();
  resource_signature_desc.NumResources = variables.size();
  resource_signature_desc.ImmutableSamplers = samplers.data();
  resource_signature_desc.NumImmutableSamplers = samplers.size();
  resource_signature_desc.BindingIndex = binding_index;
  resource_signature_desc.UseCombinedTextureSamplers = Diligent::True;

  RRefPtr<Diligent::IPipelineResourceSignature> signature;
  device_->CreatePipelineResourceSignature(resource_signature_desc, &signature);

  return signature;
}

namespace {

Diligent::SamplerDesc MakeClampSampler(Diligent::FILTER_TYPE filter) {
  return Diligent::SamplerDesc{filter,
                               filter,
                               filter,
                               Diligent::TEXTURE_ADDRESS_CLAMP,
                               Diligent::TEXTURE_ADDRESS_CLAMP,
                               Diligent::TEXTURE_ADDRESS_CLAMP};
}

const char* GetCuNNyTextureName(int index) {
  static constexpr const char* kNames[] = {
      "u_Texture0", "u_Texture1", "u_Texture2",  "u_Texture3",
      "u_Texture4", "u_Texture5", "u_Texture6",  "u_Texture7",
      "u_Texture8", "u_Texture9", "u_Texture10", "u_Texture11",
  };
  return kNames[index];
}

const char* GetCuNNyOutputName(int index) {
  static constexpr const char* kNames[] = {
      "u_Output0", "u_Output1", "u_Output2",
      "u_Output3", "u_Output4", "u_Output5",
  };
  return kNames[index];
}

std::string ReplaceAll(std::string source,
                       const std::string& from,
                       const std::string& to) {
  size_t pos = 0;
  while ((pos = source.find(from, pos)) != std::string::npos) {
    source.replace(pos, from.size(), to);
    pos += to.size();
  }
  return source;
}

std::string ApplyCuNNyFP16Macros(std::string source) {
  source = ReplaceAll(source, "#define MF float\n", "#define MF min16float\n");

  for (int i = 1; i <= 4; ++i) {
    const std::string n = std::to_string(i);
    source = ReplaceAll(source, "#define MF" + n + " float" + n + "\n",
                        "#define MF" + n + " min16float" + n + "\n");

    for (int j = 1; j <= 4; ++j) {
      const std::string m = std::to_string(j);
      source = ReplaceAll(source,
                          "#define MF" + n + "x" + m + " float" + n +
                              "x" + m + "\n",
                          "#define MF" + n + "x" + m + " min16float" +
                              n + "x" + m + "\n");
    }
  }

  return source;
}

std::string ApplyCuNNyMadMulAdd(std::string source) {
  static constexpr const char kMulAddUsingMul[] = R"(MF2 MulAdd(MF2 x, MF2x2 y, MF2 a) { return mul(x, y) + a; }
MF3 MulAdd(MF2 x, MF2x3 y, MF3 a) { return mul(x, y) + a; }
MF4 MulAdd(MF2 x, MF2x4 y, MF4 a) { return mul(x, y) + a; }
MF2 MulAdd(MF3 x, MF3x2 y, MF2 a) { return mul(x, y) + a; }
MF3 MulAdd(MF3 x, MF3x3 y, MF3 a) { return mul(x, y) + a; }
MF4 MulAdd(MF3 x, MF3x4 y, MF4 a) { return mul(x, y) + a; }
MF2 MulAdd(MF4 x, MF4x2 y, MF2 a) { return mul(x, y) + a; }
MF3 MulAdd(MF4 x, MF4x3 y, MF3 a) { return mul(x, y) + a; }
MF4 MulAdd(MF4 x, MF4x4 y, MF4 a) { return mul(x, y) + a; }
)";

  static constexpr const char kMulAddUsingMad[] = R"(MF2 MulAdd(MF2 x, MF2x2 y, MF2 a) {
  MF2 result = a;
  result = mad(x.x, y._m00_m01, result);
  result = mad(x.y, y._m10_m11, result);
  return result;
}
MF3 MulAdd(MF2 x, MF2x3 y, MF3 a) {
  MF3 result = a;
  result = mad(x.x, y._m00_m01_m02, result);
  result = mad(x.y, y._m10_m11_m12, result);
  return result;
}
MF4 MulAdd(MF2 x, MF2x4 y, MF4 a) {
  MF4 result = a;
  result = mad(x.x, y._m00_m01_m02_m03, result);
  result = mad(x.y, y._m10_m11_m12_m13, result);
  return result;
}
MF2 MulAdd(MF3 x, MF3x2 y, MF2 a) {
  MF2 result = a;
  result = mad(x.x, y._m00_m01, result);
  result = mad(x.y, y._m10_m11, result);
  result = mad(x.z, y._m20_m21, result);
  return result;
}
MF3 MulAdd(MF3 x, MF3x3 y, MF3 a) {
  MF3 result = a;
  result = mad(x.x, y._m00_m01_m02, result);
  result = mad(x.y, y._m10_m11_m12, result);
  result = mad(x.z, y._m20_m21_m22, result);
  return result;
}
MF4 MulAdd(MF3 x, MF3x4 y, MF4 a) {
  MF4 result = a;
  result = mad(x.x, y._m00_m01_m02_m03, result);
  result = mad(x.y, y._m10_m11_m12_m13, result);
  result = mad(x.z, y._m20_m21_m22_m23, result);
  return result;
}
MF2 MulAdd(MF4 x, MF4x2 y, MF2 a) {
  MF2 result = a;
  result = mad(x.x, y._m00_m01, result);
  result = mad(x.y, y._m10_m11, result);
  result = mad(x.z, y._m20_m21, result);
  result = mad(x.w, y._m30_m31, result);
  return result;
}
MF3 MulAdd(MF4 x, MF4x3 y, MF3 a) {
  MF3 result = a;
  result = mad(x.x, y._m00_m01_m02, result);
  result = mad(x.y, y._m10_m11_m12, result);
  result = mad(x.z, y._m20_m21_m22, result);
  result = mad(x.w, y._m30_m31_m32, result);
  return result;
}
MF4 MulAdd(MF4 x, MF4x4 y, MF4 a) {
  MF4 result = a;
  result = mad(x.x, y._m00_m01_m02_m03, result);
  result = mad(x.y, y._m10_m11_m12_m13, result);
  result = mad(x.z, y._m20_m21_m22_m23, result);
  result = mad(x.w, y._m30_m31_m32_m33, result);
  return result;
}
)";

  return ReplaceAll(source, kMulAddUsingMul, kMulAddUsingMad);
}

bool SupportsCuNNyFP16(Diligent::IRenderDevice* device) {
  if (!device)
    return false;

  const auto& info = device->GetDeviceInfo();
  if (info.Type == Diligent::RENDER_DEVICE_TYPE_GL ||
      info.Type == Diligent::RENDER_DEVICE_TYPE_GLES) {
    return false;
  }

  return info.Features.ShaderFloat16 == Diligent::DEVICE_FEATURE_STATE_ENABLED;
}

std::string MakeCuNNyComputeShader(const std::string& hlsl, bool use_fp16) {
  std::string source = hlsl;
  if (use_fp16)
    source = ApplyCuNNyFP16Macros(source);
  source = ApplyCuNNyMadMulAdd(source);
  return source;
}

void AddCuNNyComputeInputs(
    std::vector<Diligent::PipelineResourceDesc>& variables,
    std::vector<Diligent::ImmutableSamplerDesc>& samplers,
    const Diligent::SamplerDesc& point_sampler,
    int input_start,
    int input_count) {
  for (int i = 0; i < input_count; ++i) {
    const char* name = GetCuNNyTextureName(input_start + i);
    variables.push_back({Diligent::SHADER_TYPE_COMPUTE, name,
        Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
        Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
    samplers.push_back({Diligent::SHADER_TYPE_COMPUTE, name, point_sampler});
  }
}

void AddCuNNyComputeOutputs(
    std::vector<Diligent::PipelineResourceDesc>& variables,
    int output_count,
    bool final_pass) {
  if (final_pass) {
    variables.push_back({Diligent::SHADER_TYPE_COMPUTE, "u_Output",
        Diligent::SHADER_RESOURCE_TYPE_TEXTURE_UAV,
        Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
    return;
  }

  for (int i = 0; i < output_count; ++i) {
    variables.push_back({Diligent::SHADER_TYPE_COMPUTE, GetCuNNyOutputName(i),
        Diligent::SHADER_RESOURCE_TYPE_TEXTURE_UAV,
        Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
  }
}

}  // namespace

/// Pipeline Bake
///

#define PIPELINE_HEADER(name)                                             \
  Pipeline_##name::Pipeline_##name(const PipelineInitParams& init_params) \
      : RenderPipelineBase(init_params.render_device, init_params.bytecode_cache)

PIPELINE_HEADER(Base) {
  const ShaderSource shader_source{kHLSL_BaseRender_Vertex,
                                   kHLSL_BaseRender_Pixel, "base.render"};

  const std::vector<Diligent::PipelineResourceDesc> variables = {
      {Diligent::SHADER_TYPE_VERTEX, "WorldMatrixBuffer",
       Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_Texture",
          init_params.immutable_sampler,
      },
  };

  auto binding0 = MakeResourceSignature(variables, samplers, 0);
  SetupPipelineBasis(shader_source, Vertex::GetLayout(), {binding0});
}

PIPELINE_HEADER(BitmapBlt) {
  const ShaderSource shader_source{kHLSL_BitmapBltRender_Vertex,
                                   kHLSL_BitmapBltRender_Pixel,
                                   "bitmapblt.render"};

  const std::vector<Diligent::PipelineResourceDesc> variables = {
      {Diligent::SHADER_TYPE_VERTEX, "WorldMatrixBuffer",
       Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_DstTexture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_Texture",
          init_params.immutable_sampler,
      },
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_DstTexture",
          init_params.immutable_sampler,
      },
  };

  auto binding0 = MakeResourceSignature(variables, samplers, 0);
  SetupPipelineBasis(shader_source, Vertex::GetLayout(), {binding0});
}

PIPELINE_HEADER(BitmapClipBlt) {
  const ShaderSource shader_source{kHLSL_BitmapClipBltRender_Vertex,
                                   kHLSL_BitmapClipBltRender_Pixel,
                                   "bitmapclipblt.render"};

  const std::vector<Diligent::PipelineResourceDesc> variables = {
      {Diligent::SHADER_TYPE_VERTEX, "WorldMatrixBuffer",
       Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_ClipTexture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_DstTexture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_Texture",
          init_params.immutable_sampler,
      },
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_ClipTexture",
          init_params.immutable_sampler,
      },
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_DstTexture",
          init_params.immutable_sampler,
      },
  };

  auto binding0 = MakeResourceSignature(variables, samplers, 0);
  SetupPipelineBasis(shader_source, Vertex::GetLayout(), {binding0});
}

PIPELINE_HEADER(Color) {
  const ShaderSource shader_source{kHLSL_ColorRender_Vertex,
                                   kHLSL_ColorRender_Pixel, "color.render"};

  const std::vector<Diligent::PipelineResourceDesc> variables = {
      {Diligent::SHADER_TYPE_VERTEX, "WorldMatrixBuffer",
       Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  auto binding0 = MakeResourceSignature(variables, {}, 0);
  SetupPipelineBasis(shader_source, Vertex::GetLayout(), {binding0});
}

PIPELINE_HEADER(Flat) {
  const ShaderSource shader_source{kHLSL_FlatRender_Vertex,
                                   kHLSL_FlatRender_Pixel, "flat.render"};

  const std::vector<Diligent::PipelineResourceDesc> variables = {
      {Diligent::SHADER_TYPE_VERTEX, "WorldMatrixBuffer",
       Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "FlatUniformConstants",
       Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_Texture",
          init_params.immutable_sampler,
      },
  };

  auto binding0 = MakeResourceSignature(variables, samplers, 0);
  SetupPipelineBasis(shader_source, Vertex::GetLayout(), {binding0});
}

PIPELINE_HEADER(Sprite) {
  const auto& device_info = init_params.render_device->GetDeviceInfo();

  if (device_info.Type == Diligent::RENDER_DEVICE_TYPE_GLES) {
    // Disable batch on any OGLES platform
    storage_buffer_support = false;
  } else if (device_info.Type == Diligent::RENDER_DEVICE_TYPE_GL) {
    // Enable batch on available OGL desktop platform
    storage_buffer_support =
        device_info.Features.VertexPipelineUAVWritesAndAtomics > 0;
  } else {
    // Enable batch on D3D11, D3D12 and Vulkan backend
    storage_buffer_support = true;
  }

  if (!storage_buffer_support)
    LOG(INFO) << "[Pipeline] Disable sprite batch process.";

  Diligent::ShaderMacro vertex_macro = {"STORAGE_BUFFER_SUPPORT",
                                        storage_buffer_support ? "1" : "0"};

  const ShaderSource shader_source{kHLSL_SpriteRender_Vertex,
                                   kHLSL_SpriteRender_Pixel,
                                   "sprite.render",
                                   {vertex_macro}};

  const std::vector<Diligent::PipelineResourceDesc> variables = {
      {Diligent::SHADER_TYPE_VERTEX, "WorldMatrixBuffer",
       Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_VERTEX,
       storage_buffer_support ? "u_Params" : "SpriteUniformConstants",
       storage_buffer_support ? Diligent::SHADER_RESOURCE_TYPE_BUFFER_SRV
                              : Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_Texture",
          init_params.immutable_sampler,
      },
  };

  auto binding0 = MakeResourceSignature(variables, samplers, 0);
  SetupPipelineBasis(shader_source, Vertex::GetLayout(), {binding0});
}

PIPELINE_HEADER(AlphaTransition) {
  const ShaderSource shader_source{kHLSL_AlphaTransitionRender_Vertex,
                                   kHLSL_AlphaTransitionRender_Pixel,
                                   "alpha.trans.render"};

  const std::vector<Diligent::PipelineResourceDesc> variables = {
      {Diligent::SHADER_TYPE_PIXEL, "u_FrozenTexture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_CurrentTexture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_FrozenTexture",
          init_params.immutable_sampler,
      },
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_CurrentTexture",
          init_params.immutable_sampler,
      },
  };

  auto binding0 = MakeResourceSignature(variables, samplers, 0);
  SetupPipelineBasis(shader_source, Vertex::GetLayout(), {binding0});
}

PIPELINE_HEADER(VagueTransition) {
  const ShaderSource shader_source{kHLSL_MappingTransitionRender_Vertex,
                                   kHLSL_MappingTransitionRender_Pixel,
                                   "vague.trans.render"};

  const std::vector<Diligent::PipelineResourceDesc> variables = {
      {Diligent::SHADER_TYPE_PIXEL, "u_FrozenTexture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_CurrentTexture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_TransTexture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_FrozenTexture",
          init_params.immutable_sampler,
      },
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_CurrentTexture",
          init_params.immutable_sampler,
      },
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_TransTexture",
          init_params.immutable_sampler,
      },
  };

  auto binding0 = MakeResourceSignature(variables, samplers, 0);
  SetupPipelineBasis(shader_source, Vertex::GetLayout(), {binding0});
}

PIPELINE_HEADER(Tilemap) {
  const ShaderSource shader_source{kHLSL_TilemapRender_Vertex,
                                   kHLSL_TilemapRender_Pixel, "tilemap.render"};

  const std::vector<Diligent::PipelineResourceDesc> variables = {
      {Diligent::SHADER_TYPE_VERTEX, "WorldMatrixBuffer",
       Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_VERTEX, "TilemapUniformBuffer",
       Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_Texture",
          init_params.immutable_sampler,
      },
  };

  auto binding0 = MakeResourceSignature(variables, samplers, 0);
  SetupPipelineBasis(shader_source, Vertex::GetLayout(), {binding0});
}

Pipeline_Tilemap2::Pipeline_Tilemap2(const PipelineInitParams& init_params)
    : RenderPipelineBase(init_params.render_device, init_params.bytecode_cache) {
  const ShaderSource shader_source{kHLSL_Tilemap2Render_Vertex,
                                   kHLSL_Tilemap2Render_Pixel,
                                   "tilemap2.render"};

  const std::vector<Diligent::PipelineResourceDesc> variables = {
      {Diligent::SHADER_TYPE_VERTEX, "WorldMatrixBuffer",
       Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_VERTEX, "Tilemap2UniformBuffer",
       Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_Texture",
          init_params.immutable_sampler,
      },
  };

  auto binding0 = MakeResourceSignature(variables, samplers, 0);
  SetupPipelineBasis(shader_source, Vertex::GetLayout(), {binding0});
}

PIPELINE_HEADER(BitmapHue) {
  const ShaderSource shader_source{kHLSL_BitmapHueRender_Vertex,
                                   kHLSL_BitmapHueRender_Pixel, "hue.render"};

  const std::vector<Diligent::PipelineResourceDesc> variables = {
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_Texture",
          init_params.immutable_sampler,
      },
  };

  auto binding0 = MakeResourceSignature(variables, samplers, 0);
  SetupPipelineBasis(shader_source, Vertex::GetLayout(), {binding0});
}

PIPELINE_HEADER(Upscale) {
  Diligent::ShaderMacro glsl_macro = {};
  bool is_gl = init_params.render_device->GetDeviceInfo().Type ==
               Diligent::RENDER_DEVICE_TYPE_GL;
  if (is_gl)
    glsl_macro = {"URGE_GLSL_GATHER", "1"};
  const ShaderSource shader_source{kHLSL_UpscalePass_Vertex,
                                   kHLSL_UpscalePass_Pixel, "upscale.pass",
                                   is_gl ? std::vector{glsl_macro}
                                         : std::vector<Diligent::ShaderMacro>{}};

  const std::vector<Diligent::PipelineResourceDesc> variables = {
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "ScalingParamsBuffer",
       Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       init_params.immutable_sampler},
  };

  auto binding0 = MakeResourceSignature(variables, samplers, 0);
  SetupPipelineBasis(shader_source, Vertex::GetLayout(), {binding0});
}

PIPELINE_HEADER(Anime4K_Enhance) {
  const ShaderSource shader_source{kHLSL_UpscalePass_Vertex,
                                   kHLSL_Anime4K_Enhance_Pixel,
                                   "anime4k.enhance"};

  const std::vector<Diligent::PipelineResourceDesc> variables = {
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "ScalingParamsBuffer",
       Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       init_params.immutable_sampler},
  };

  auto binding0 = MakeResourceSignature(variables, samplers, 0);
  SetupPipelineBasis(shader_source, Vertex::GetLayout(), {binding0});
}

PIPELINE_HEADER(CAS) {
  const ShaderSource shader_source{kHLSL_UpscalePass_Vertex,
                                   kHLSL_CAS_Pixel, "cas.pass"};

  const std::vector<Diligent::PipelineResourceDesc> variables = {
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "ScalingParamsBuffer",
       Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {Diligent::SHADER_TYPE_PIXEL, "u_Texture",
       init_params.immutable_sampler},
  };

  auto binding0 = MakeResourceSignature(variables, samplers, 0);
  SetupPipelineBasis(shader_source, Vertex::GetLayout(), {binding0});
}

PIPELINE_HEADER(YUV) {
  const ShaderSource shader_source{kHLSL_YUVRender_Vertex,
                                   kHLSL_YUVRender_Pixel, "yuv.render"};

  const std::vector<Diligent::PipelineResourceDesc> variables = {
      {Diligent::SHADER_TYPE_PIXEL, "u_TextureY",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_TextureU",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
      {Diligent::SHADER_TYPE_PIXEL, "u_TextureV",
       Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV,
       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
  };

  const std::vector<Diligent::ImmutableSamplerDesc> samplers = {
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_TextureY",
          init_params.immutable_sampler,
      },
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_TextureU",
          init_params.immutable_sampler,
      },
      {
          Diligent::SHADER_TYPE_PIXEL,
          "u_TextureV",
          init_params.immutable_sampler,
      },
  };

  auto binding0 = MakeResourceSignature(variables, samplers, 0);
  SetupPipelineBasis(shader_source, Vertex::GetLayout(), {binding0});
}

// Anime4K Upscale_Denoise_L compute pipeline definitions.
#define MAKE_ANIME4K_UDL_COMPUTE(hlsl, cls, shader_name, in_start, in_count, \
                                 out_count, screen_input, final_pass) \
  PIPELINE_HEADER(cls) { \
    const auto point_sampler = MakeClampSampler(Diligent::FILTER_TYPE_POINT); \
    const auto linear_sampler = MakeClampSampler(Diligent::FILTER_TYPE_LINEAR); \
    std::vector<Diligent::PipelineResourceDesc> vars; \
    std::vector<Diligent::ImmutableSamplerDesc> samplers; \
    if (screen_input) { \
      vars.push_back({Diligent::SHADER_TYPE_COMPUTE, "u_Texture", \
          Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV, \
          Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}); \
      samplers.push_back({Diligent::SHADER_TYPE_COMPUTE, "u_Texture",\
          final_pass ? linear_sampler : point_sampler}); \
    } \
    AddCuNNyComputeInputs(vars, samplers, point_sampler, in_start, in_count); \
    AddCuNNyComputeOutputs(vars, out_count, final_pass); \
    vars.push_back({Diligent::SHADER_TYPE_COMPUTE, "ScalingParamsBuffer", \
        Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, \
        Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}); \
    auto b0 = MakeResourceSignature(vars, samplers, 0); \
    const ComputeShaderSource src{ \
        MakeCuNNyComputeShader(hlsl, \
            SupportsCuNNyFP16(init_params.render_device)), \
        shader_name}; \
    SetupComputePipelineBasis(src, {b0}); \
  }

MAKE_ANIME4K_UDL_COMPUTE(kHLSL_Anime4K_UDL_Pass0_Compute,
                         Anime4K_UDL_Pass0,
                         "anime4k_udl.Pass0.compute", 0, 0, 2, true,
                         false);
MAKE_ANIME4K_UDL_COMPUTE(kHLSL_Anime4K_UDL_Pass1_Compute,
                         Anime4K_UDL_Pass1,
                         "anime4k_udl.Pass1.compute", 1, 2, 2, false,
                         false);
MAKE_ANIME4K_UDL_COMPUTE(kHLSL_Anime4K_UDL_Pass2_Compute,
                         Anime4K_UDL_Pass2,
                         "anime4k_udl.Pass2.compute", 3, 2, 2, false,
                         false);
MAKE_ANIME4K_UDL_COMPUTE(kHLSL_Anime4K_UDL_Pass3_Compute,
                         Anime4K_UDL_Pass3,
                         "anime4k_udl.Pass3.compute", 1, 2, 1, true,
                         true);

#undef MAKE_ANIME4K_UDL_COMPUTE



// CuNNy pipeline implementations

#define MAKE_CUNNY_COMPUTE(hlsl, cls, shader_name, in_start, in_count, \
                           out_count, screen_input, screen_filter, final_pass) \
  PIPELINE_HEADER(cls) { \
    const auto point_sampler = MakeClampSampler(Diligent::FILTER_TYPE_POINT); \
    const auto screen_sampler = MakeClampSampler(screen_filter); \
    std::vector<Diligent::PipelineResourceDesc> vars; \
    std::vector<Diligent::ImmutableSamplerDesc> samplers; \
    if (screen_input) { \
      vars.push_back({Diligent::SHADER_TYPE_COMPUTE, "u_Texture", \
          Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV, \
          Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}); \
      samplers.push_back({Diligent::SHADER_TYPE_COMPUTE, "u_Texture", screen_sampler}); \
    } \
    AddCuNNyComputeInputs(vars, samplers, point_sampler, in_start, in_count); \
    AddCuNNyComputeOutputs(vars, out_count, final_pass); \
    vars.push_back({Diligent::SHADER_TYPE_COMPUTE, "ScalingParamsBuffer", \
        Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, \
        Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}); \
    auto b0 = MakeResourceSignature(vars, samplers, 0); \
    const ComputeShaderSource src{ \
        MakeCuNNyComputeShader(hlsl, \
                                SupportsCuNNyFP16(init_params.render_device)), \
        shader_name}; \
    SetupComputePipelineBasis(src, {b0}); \
  }

MAKE_CUNNY_COMPUTE(kHLSL_CuNNy_4x16_Pass1_Pixel, CuNNy_4x16_Pass1,
                   "CuNNy_4x16.Pass1.compute", 0, 0, 4, true,
                   Diligent::FILTER_TYPE_POINT, false);
MAKE_CUNNY_COMPUTE(kHLSL_CuNNy_4x16_Pass2_Pixel, CuNNy_4x16_Pass2,
                   "CuNNy_4x16.Pass2.compute", 0, 4, 4, false,
                   Diligent::FILTER_TYPE_POINT, false);
MAKE_CUNNY_COMPUTE(kHLSL_CuNNy_4x16_Pass3_Pixel, CuNNy_4x16_Pass3,
                   "CuNNy_4x16.Pass3.compute", 4, 4, 4, false,
                   Diligent::FILTER_TYPE_POINT, false);
MAKE_CUNNY_COMPUTE(kHLSL_CuNNy_4x16_Pass4_Pixel, CuNNy_4x16_Pass4,
                   "CuNNy_4x16.Pass4.compute", 0, 4, 4, false,
                   Diligent::FILTER_TYPE_POINT, false);
MAKE_CUNNY_COMPUTE(kHLSL_CuNNy_4x16_Pass5_Pixel, CuNNy_4x16_Pass5,
                   "CuNNy_4x16.Pass5.compute", 4, 4, 4, false,
                   Diligent::FILTER_TYPE_POINT, false);
MAKE_CUNNY_COMPUTE(kHLSL_CuNNy_4x16_Pass6_Pixel, CuNNy_4x16_Pass6,
                   "CuNNy_4x16.Pass6.compute", 0, 4, 1, true,
                   Diligent::FILTER_TYPE_LINEAR, true);

MAKE_CUNNY_COMPUTE(kHLSL_CuNNy_4x24_Pass1_Pixel, CuNNy_4x24_Pass1,
                   "CuNNy_4x24.Pass1.compute", 0, 0, 6, true,
                   Diligent::FILTER_TYPE_POINT, false);
MAKE_CUNNY_COMPUTE(kHLSL_CuNNy_4x24_Pass2_Pixel, CuNNy_4x24_Pass2,
                   "CuNNy_4x24.Pass2.compute", 0, 6, 6, false,
                   Diligent::FILTER_TYPE_POINT, false);
MAKE_CUNNY_COMPUTE(kHLSL_CuNNy_4x24_Pass3_Pixel, CuNNy_4x24_Pass3,
                   "CuNNy_4x24.Pass3.compute", 6, 6, 6, false,
                   Diligent::FILTER_TYPE_POINT, false);
MAKE_CUNNY_COMPUTE(kHLSL_CuNNy_4x24_Pass4_Pixel, CuNNy_4x24_Pass4,
                   "CuNNy_4x24.Pass4.compute", 0, 6, 6, false,
                   Diligent::FILTER_TYPE_POINT, false);
MAKE_CUNNY_COMPUTE(kHLSL_CuNNy_4x24_Pass5_Pixel, CuNNy_4x24_Pass5,
                   "CuNNy_4x24.Pass5.compute", 6, 6, 6, false,
                   Diligent::FILTER_TYPE_POINT, false);
MAKE_CUNNY_COMPUTE(kHLSL_CuNNy_4x24_Pass6_Pixel, CuNNy_4x24_Pass6,
                   "CuNNy_4x24.Pass6.compute", 0, 6, 1, true,
                   Diligent::FILTER_TYPE_LINEAR, true);

#undef MAKE_CUNNY_COMPUTE

}  // namespace renderer
