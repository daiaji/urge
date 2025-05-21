// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/spine2d/skeleton_renderer.h"

#include "SDL3_image/SDL_image.h"
#include "spine/Debug.h"

#include "base/debug/logging.h"
#include "renderer/utils/texture_utils.h"

namespace spine {

namespace {

Diligent::TEXTURE_ADDRESS_MODE TexAddressWrap(TextureWrap wrap) {
  switch (wrap) {
    default:
    case TextureWrap_MirroredRepeat:
      return Diligent::TEXTURE_ADDRESS_MIRROR;
    case TextureWrap_ClampToEdge:
      return Diligent::TEXTURE_ADDRESS_CLAMP;
    case TextureWrap_Repeat:
      return Diligent::TEXTURE_ADDRESS_WRAP;
  }
}

Diligent::FILTER_TYPE TexFilterWrap(TEXTURE_FILTER_ENUM wrap) {
  switch (wrap) {
    default:
    case TextureFilter_Unknown:
      return Diligent::FILTER_TYPE_UNKNOWN;
    case TextureFilter_Nearest:
      return Diligent::FILTER_TYPE_POINT;
    case TextureFilter_Linear:
      return Diligent::FILTER_TYPE_LINEAR;
  }
}

renderer::BlendType RenderBlendTypeWrap(BlendMode mode,
                                        bool premultiplied_alpha) {
  switch (mode) {
    default:
    case spine::BlendMode_Normal:
      return premultiplied_alpha ? renderer::NORMAL_PMA : renderer::NORMAL;
    case spine::BlendMode_Additive:
      return premultiplied_alpha ? renderer::ADDITION_PMA : renderer::ADDITION;
    case spine::BlendMode_Multiply:
      return renderer::MULTIPLY;
    case spine::BlendMode_Screen:
      return renderer::SCREEN;
  }
}

}  // namespace

DiligentTextureLoader::DiligentTextureLoader(renderer::RenderDevice* device,
                                             filesystem::IOService* io_service)
    : device_(device), io_service_(io_service) {}

DiligentTextureLoader::~DiligentTextureLoader() = default;

void DiligentTextureLoader::load(AtlasPage& page, const String& path) {
  filesystem::IOState io_state;
  SDL_Surface* atlas_surface = nullptr;

  io_service_->OpenRead(
      path.buffer(),
      base::BindRepeating(
          [](SDL_Surface** surf, SDL_IOStream* io, const std::string& ext) {
            *surf = IMG_LoadTyped_IO(io, true, ext.c_str());
            return !!*surf;
          },
          &atlas_surface),
      &io_state);

  if (io_state.error_count) {
    LOG(INFO) << "[Spine2D] " << io_state.error_message;
    return;
  }

  if (!atlas_surface) {
    LOG(INFO) << "[Spine2D] " << SDL_GetError();
    return;
  }

  if (atlas_surface->format != SDL_PIXELFORMAT_ABGR8888) {
    SDL_Surface* conv =
        SDL_ConvertSurface(atlas_surface, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(atlas_surface);
    atlas_surface = conv;
  }

  RRefPtr<Diligent::ITexture> texture;
  renderer::CreateTexture2D(
      **device_, &texture, "SpineAtlas:" + std::string(path.buffer()),
      atlas_surface, Diligent::USAGE_DEFAULT, Diligent::BIND_SHADER_RESOURCE);

  Diligent::SamplerDesc sampler_desc;
  sampler_desc.Name = "spine2d.atlas.sampler";
  sampler_desc.MinFilter = TexFilterWrap(page.minFilter);
  sampler_desc.MagFilter = TexFilterWrap(page.magFilter);
  sampler_desc.AddressU = TexAddressWrap(page.uWrap);
  sampler_desc.AddressV = TexAddressWrap(page.vWrap);

  RRefPtr<Diligent::ISampler> sampler;
  (*device_)->CreateSampler(sampler_desc, &sampler);

  auto* atlas_view =
      texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
  atlas_view->SetSampler(sampler);

  page.width = atlas_surface->w;
  page.height = atlas_surface->h;
  page.texture = texture.Detach();
}

void DiligentTextureLoader::unload(void* texture) {
  RRefPtr<Diligent::ITexture> render_texture;
  render_texture.Attach(static_cast<Diligent::ITexture*>(texture));
  render_texture.Release();
}

DiligentRenderer::DiligentRenderer(renderer::RenderDevice* device,
                                   SkeletonData* skeleton_data,
                                   AnimationStateData* animation_state_data)
    : device_(device),
      vertex_batch_(SpineVertexBatch::Make(**device)),
      shader_binding_(device->GetPipelines()->spine2d.CreateBinding()),
      skeleton_(std::make_unique<Skeleton>(skeleton_data)),
      animation_state_(std::make_unique<AnimationState>(
          animation_state_data ? animation_state_data
                               : new AnimationStateData(skeleton_data))),
      skeleton_renderer_(std::unique_ptr<SkeletonRenderer>()),
      pending_commands_(nullptr),
      owns_animation_state_data_(!animation_state_data) {}

DiligentRenderer::~DiligentRenderer() {
  if (owns_animation_state_data_)
    delete animation_state_->getData();
}

std::unique_ptr<DiligentRenderer> DiligentRenderer::Create(
    renderer::RenderDevice* device,
    SkeletonData* skeleton_data,
    AnimationStateData* animation_state_data) {
  return std::unique_ptr<DiligentRenderer>(
      new DiligentRenderer(device, skeleton_data, animation_state_data));
}

void DiligentRenderer::Update(Diligent::IDeviceContext* context,
                              float delta,
                              Physics physics) {
  animation_state_->update(delta);
  animation_state_->apply(*skeleton_);
  skeleton_->update(delta);
  skeleton_->updateWorldTransform(physics);

  vertex_cache_.clear();
  index_cache_.clear();
  pending_commands_ = skeleton_renderer_->render(*skeleton_);
  RenderCommand* command = pending_commands_;
  while (command) {
    float* positions = command->positions;
    float* uvs = command->uvs;
    uint32_t* colors = command->colors;
    uint32_t* darkColors = command->darkColors;
    uint16_t* indices = command->indices;

    // Vertex data
    int32_t num_command_vertices = command->numVertices;
    for (int32_t i = 0, j = 0; i < num_command_vertices; i++, j += 2) {
      renderer::SpineVertex vertex;
      vertex.position.x = positions[j];
      vertex.position.y = positions[j + 1];
      vertex.texcoord.x = uvs[j];
      vertex.texcoord.y = uvs[j + 1];
      uint32_t color = colors[i];
      vertex.light_color = (color & 0xFF00FF00) | ((color & 0x00FF0000) >> 16) |
                           ((color & 0x000000FF) << 16);
      uint32_t darkColor = darkColors[i];
      vertex.dark_color = (darkColor & 0xFF00FF00) |
                          ((darkColor & 0x00FF0000) >> 16) |
                          ((darkColor & 0x000000FF) << 16);
      vertex_cache_.push_back(vertex);
    }

    // Index data
    index_cache_.insert(index_cache_.end(), indices,
                        indices + command->numIndices);

    command = command->next;
  }

  // Reset index buffer if need
  int32_t num_command_indices = index_cache_.size();
  uint32_t index_buffer_size = num_command_indices * sizeof(uint16_t);
  if (!index_buffer_ || index_buffer_->GetDesc().Size < index_buffer_size) {
    Diligent::BufferDesc buffer_desc;
    buffer_desc.Name = "spine2d.index.buffer";
    buffer_desc.Usage = Diligent::USAGE_DEFAULT;
    buffer_desc.BindFlags = Diligent::BIND_INDEX_BUFFER;
    buffer_desc.Size = index_buffer_size;
    index_buffer_.Release();
    (*device_)->CreateBuffer(buffer_desc, nullptr, &index_buffer_);
  }

  // Upload data
  vertex_batch_->QueueWrite(context, vertex_cache_.data(),
                            vertex_cache_.size());
  context->UpdateBuffer(index_buffer_, 0, index_buffer_size,
                        index_cache_.data(),
                        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void DiligentRenderer::Render(Diligent::IDeviceContext* context,
                              Diligent::IBuffer* world_buffer,
                              bool premultiplied_alpha) {
  uint32_t vertices_offset = 0;
  RenderCommand* command = pending_commands_;
  while (command) {
    auto* texture = static_cast<Diligent::ITexture*>(command->texture);
    auto* texture_view =
        texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);

    auto& pipeline_set = device_->GetPipelines()->spine2d;
    auto* pipeline = pipeline_set.GetPipeline(
        RenderBlendTypeWrap(command->blendMode, premultiplied_alpha));

    // Setup uniform params
    shader_binding_->u_transform->Set(world_buffer);
    shader_binding_->u_texture->Set(texture_view);

    // Apply pipeline state
    context->SetPipelineState(pipeline);
    context->CommitShaderResources(
        **shader_binding_, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Apply vertex index
    Diligent::IBuffer* const vertex_buffer = **vertex_batch_;
    context->SetVertexBuffers(
        0, 1, &vertex_buffer, nullptr,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    context->SetIndexBuffer(
        index_buffer_, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Execute render command
    Diligent::DrawIndexedAttribs draw_indexed_attribs;
    draw_indexed_attribs.NumIndices = command->numIndices;
    draw_indexed_attribs.IndexType = Diligent::VT_UINT16;
    draw_indexed_attribs.FirstIndexLocation = vertices_offset;
    context->DrawIndexed(draw_indexed_attribs);

    // Apply offset
    vertices_offset += command->numIndices;

    // Step into next command
    command = command->next;
  }
}

}  // namespace spine
