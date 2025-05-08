// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/spine/spine_renderer.h"

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
      skeleton_(std::make_unique<Skeleton>(skeleton_data)),
      animation_state_(std::make_unique<AnimationState>(
          animation_state_data ? animation_state_data
                               : new AnimationStateData(skeleton_data))),
      skeleton_renderer_(std::unique_ptr<SkeletonRenderer>()),
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

void DiligentRenderer::Update(float delta, Physics physics) {
  animation_state_->update(delta);
  animation_state_->apply(*skeleton_);
  skeleton_->update(delta);
  skeleton_->updateWorldTransform(physics);
}

void DiligentRenderer::Render(Diligent::IDeviceContext* context,
                              Diligent::IBuffer* world_buffer) {
  RenderCommand* command = skeleton_renderer_->render(*skeleton_);
  while (command) {
    auto* texture = static_cast<Diligent::ITexture*>(command->texture);
    auto* texture_view =
        texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);

    command = command->next;
  }
}

}  // namespace spine
