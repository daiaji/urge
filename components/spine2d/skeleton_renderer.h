// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SPINE_SKELETON_RENDERER_H_
#define COMPONENTS_SPINE_SKELETON_RENDERER_H_

#include <memory>

#include "spine/spine.h"

#include "base/worker/thread_worker.h"
#include "components/filesystem/io_service.h"
#include "renderer/device/render_device.h"
#include "renderer/pipeline/render_pipeline.h"

namespace spine {

using SpineVertexBatch = renderer::BatchBuffer<renderer::SpineVertex,
                                               Diligent::BIND_VERTEX_BUFFER,
                                               Diligent::BUFFER_MODE_UNDEFINED,
                                               Diligent::CPU_ACCESS_NONE,
                                               Diligent::USAGE_DEFAULT>;

struct SpineRendererAgent {
  std::unique_ptr<SpineVertexBatch> vertex_batch;
  std::unique_ptr<renderer::Binding_Base> shader_binding;
};

class DiligentTextureLoader : public TextureLoader {
 public:
  DiligentTextureLoader(renderer::RenderDevice* device,
                        base::ThreadWorker* worker,
                        filesystem::IOService* io_service);
  ~DiligentTextureLoader() override;

  DiligentTextureLoader(const DiligentTextureLoader&) = delete;
  DiligentTextureLoader& operator=(const DiligentTextureLoader&) = delete;

  void load(AtlasPage& page, const String& path) override;
  void unload(void* texture) override;

 private:
  renderer::RenderDevice* device_;
  base::ThreadWorker* worker_;
  filesystem::IOService* io_service_;
};

class DiligentRenderer {
 public:
  ~DiligentRenderer();

  DiligentRenderer(const DiligentRenderer&) = delete;
  DiligentRenderer& operator=(const DiligentRenderer&) = delete;

  static std::unique_ptr<DiligentRenderer> Create(
      renderer::RenderDevice* device,
      base::ThreadWorker* worker,
      SkeletonData* skeleton_data,
      AnimationStateData* animation_state_data);

  void Update(Diligent::IDeviceContext* context, float delta, Physics physics);
  void Render(Diligent::IDeviceContext* context,
              Diligent::IBuffer* world_buffer,
              bool premultiplied_alpha);

 private:
  DiligentRenderer(renderer::RenderDevice* device,
                   base::ThreadWorker* worker,
                   SkeletonData* skeleton_data,
                   AnimationStateData* animation_state_data);

  renderer::RenderDevice* device_;
  base::ThreadWorker* worker_;
  SpineRendererAgent* agent_;
  std::vector<renderer::SpineVertex> vertex_cache_;

  std::unique_ptr<Skeleton> skeleton_;
  std::unique_ptr<AnimationState> animation_state_;

  std::unique_ptr<SkeletonRenderer> skeleton_renderer_;
  RenderCommand* pending_commands_;
  bool owns_animation_state_data_;
};

}  // namespace spine

#endif  //! COMPONENTS_SPINE_SKELETON_RENDERER_H_
