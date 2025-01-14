// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/canvas/canvas_impl.h"

#include "MemoryPool/MemoryPool.h"
#include "SDL3_image/SDL_image.h"
#include "SDL3_ttf/SDL_ttf.h"

#include "content/canvas/canvas_scheduler.h"
#include "content/canvas/font_impl.h"
#include "content/common/color_impl.h"
#include "content/common/rect_impl.h"

namespace content {

namespace {

MemoryPool<TextureAgent> g_textures_pool;

wgpu::BindGroup MakeTextureWorldInternal(renderer::RenderDevice* device_base,
                                         const base::Vec2& bitmap_size) {
  float world_matrix[32];
  renderer::MakeProjectionMatrix(world_matrix, bitmap_size);
  renderer::MakeIdentityMatrix(world_matrix + 16);

  wgpu::BufferDescriptor uniform_desc;
  uniform_desc.label = "bitmap.world.uniform";
  uniform_desc.size = sizeof(world_matrix);
  uniform_desc.mappedAtCreation = true;
  uniform_desc.usage = wgpu::BufferUsage::Uniform;
  wgpu::Buffer world_matrix_uniform =
      (*device_base)->CreateBuffer(&uniform_desc);

  if (world_matrix_uniform.GetMapState() == wgpu::BufferMapState::Mapped) {
    memcpy(world_matrix_uniform.GetMappedRange(), world_matrix,
           sizeof(world_matrix));
    world_matrix_uniform.Unmap();
  }

  wgpu::BindGroupEntry entries;
  entries.binding = 0;
  entries.buffer = world_matrix_uniform;

  wgpu::BindGroupDescriptor binding_desc;
  binding_desc.entryCount = 1;
  binding_desc.entries = &entries;
  binding_desc.layout = *device_base->GetPipelines()->base.GetLayout(0);

  return (*device_base)->CreateBindGroup(&binding_desc);
}

wgpu::BindGroup MakeTextureBindingGroupInternal(
    renderer::RenderDevice* device_base,
    const base::Vec2& bitmap_size,
    const wgpu::TextureView& view,
    const wgpu::Sampler& sampler) {
  wgpu::BufferDescriptor uniform_desc;
  uniform_desc.label = "bitmap.binding.uniform";
  uniform_desc.size = sizeof(base::Vec2);
  uniform_desc.mappedAtCreation = true;
  uniform_desc.usage = wgpu::BufferUsage::Uniform;
  wgpu::Buffer texture_size_uniform =
      (*device_base)->CreateBuffer(&uniform_desc);

  if (texture_size_uniform.GetMapState() == wgpu::BufferMapState::Mapped) {
    memcpy(texture_size_uniform.GetMappedRange(), &bitmap_size,
           sizeof(bitmap_size));
    texture_size_uniform.Unmap();
  }

  wgpu::BindGroupEntry entries[3];
  entries[0].binding = 0;
  entries[0].textureView = view;
  entries[1].binding = 1;
  entries[1].sampler = sampler;
  entries[2].binding = 2;
  entries[2].buffer = texture_size_uniform;

  wgpu::BindGroupDescriptor binding_desc;
  binding_desc.entryCount = _countof(entries);
  binding_desc.entries = entries;
  binding_desc.layout = *device_base->GetPipelines()->base.GetLayout(1);

  return (*device_base)->CreateBindGroup(&binding_desc);
}

void GPUCreateTextureWithDataInternal(renderer::RenderDevice* device_base,
                                      SDL_Surface* initial_data,
                                      TextureAgent* agent,
                                      base::SingleWorker* worker) {
  // Create video memory texture
  wgpu::TextureDescriptor texture_desc;
  texture_desc.label = "bitmap.texture";
  texture_desc.usage =
      wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc |
      wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
  texture_desc.dimension = wgpu::TextureDimension::e2D;
  texture_desc.size.width = initial_data->w;
  texture_desc.size.height = initial_data->h;
  texture_desc.format = wgpu::TextureFormat::RGBA8Unorm;

  agent->size = base::Vec2i(initial_data->w, initial_data->h);
  agent->data = (*device_base)->CreateTexture(&texture_desc);
  agent->view = agent->data.CreateView();
  agent->sampler = (*device_base)->CreateSampler();
  agent->world = MakeTextureWorldInternal(device_base, agent->size);
  agent->binding = MakeTextureBindingGroupInternal(device_base, agent->size,
                                                   agent->view, agent->sampler);

  // Write data in video memory
  wgpu::ImageCopyTexture copy_texture;
  copy_texture.texture = agent->data;

  wgpu::TextureDataLayout texture_data_layout;
  texture_data_layout.bytesPerRow = initial_data->pitch;

  wgpu::Extent3D write_size;
  write_size.width = initial_data->w;
  write_size.height = initial_data->h;
  write_size.depthOrArrayLayers = 1;

  device_base->GetQueue()->WriteTexture(&copy_texture, initial_data->pixels,
                                        initial_data->pitch * initial_data->h,
                                        &texture_data_layout, &write_size);
}

void GPUCreateTextureWithSizeInternal(renderer::RenderDevice* device_base,
                                      const base::Vec2i& initial_size,
                                      TextureAgent* agent,
                                      base::SingleWorker* worker) {
  // Create video memory texture
  wgpu::TextureDescriptor texture_desc;
  texture_desc.label = "bitmap.texture";
  texture_desc.usage =
      wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc |
      wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
  texture_desc.dimension = wgpu::TextureDimension::e2D;
  texture_desc.size.width = initial_size.x;
  texture_desc.size.height = initial_size.y;
  texture_desc.format = wgpu::TextureFormat::RGBA8Unorm;

  agent->size = initial_size;
  agent->data = (*device_base)->CreateTexture(&texture_desc);
  agent->view = agent->data.CreateView();
  agent->sampler = (*device_base)->CreateSampler();
  agent->world = MakeTextureWorldInternal(device_base, agent->size);
  agent->binding = MakeTextureBindingGroupInternal(device_base, agent->size,
                                                   agent->view, agent->sampler);
}

void GPUBlendBlitTextureInternal(CanvasScheduler* scheduler,
                                 TextureAgent* dst_texture,
                                 const base::Rect& dst_region,
                                 TextureAgent* src_texture,
                                 const base::Rect& src_region,
                                 float blit_alpha,
                                 base::SingleWorker* worker) {
  auto& pipeline_set = scheduler->GetDevice()->GetPipelines()->base;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::kNormal);

  wgpu::RenderPassColorAttachment attachment;
  attachment.view = dst_texture->view;
  attachment.loadOp = wgpu::LoadOp::Load;
  attachment.storeOp = wgpu::StoreOp::Store;

  wgpu::RenderPassDescriptor renderpass;
  renderpass.colorAttachmentCount = 1;
  renderpass.colorAttachments = &attachment;

  auto* command_encoder = scheduler->GetContext()->GetImmediateEncoder();

  base::Vec4 blend_alpha;
  blend_alpha.w = blit_alpha / 255.0f;

  renderer::FullVertexLayout transient_vertices[4];
  renderer::FullVertexLayout::SetTexCoordRect(transient_vertices, src_region);
  renderer::FullVertexLayout::SetPositionRect(transient_vertices, dst_region);
  renderer::FullVertexLayout::SetColor(transient_vertices, blend_alpha);
  scheduler->vertex_buffer()->QueueWrite(*command_encoder, transient_vertices,
                                         _countof(transient_vertices));

  auto renderpass_encoder = command_encoder->BeginRenderPass(&renderpass);
  renderpass_encoder.SetViewport(0, 0, dst_texture->size.x, dst_texture->size.y,
                                 0, 0);
  renderpass_encoder.SetPipeline(*pipeline);
  renderpass_encoder.SetVertexBuffer(0, **scheduler->vertex_buffer());
  renderpass_encoder.SetIndexBuffer(**scheduler->index_cache(),
                                    wgpu::IndexFormat::Uint16);
  renderpass_encoder.SetBindGroup(0, dst_texture->world);
  renderpass_encoder.SetBindGroup(1, src_texture->binding);
  renderpass_encoder.Draw(6);
  renderpass_encoder.End();
}

void GPUDestroyTextureInternal(TextureAgent* agent,
                               base::SingleWorker* worker) {
  agent->data = nullptr;
  TextureAgent::Free(agent);
}

void GPUFetchTexturePixelsDataInternal(CanvasScheduler* scheduler,
                                       TextureAgent* agent,
                                       SDL_Surface* surface_cache,
                                       base::SingleWorker* worker) {
  auto& device = *scheduler->GetDevice();
  auto* encoder = scheduler->GetContext()->GetImmediateEncoder();

  // Temp pixels read buffer
  wgpu::BufferDescriptor buffer_desc;
  buffer_desc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
  buffer_desc.size = surface_cache->pitch * surface_cache->h;
  buffer_desc.mappedAtCreation = false;
  wgpu::Buffer read_buffer = device->CreateBuffer(&buffer_desc);

  // Copy pixels data to temp buffer
  wgpu::ImageCopyTexture copy_texture;
  wgpu::ImageCopyBuffer copy_buffer;
  wgpu::Extent3D copy_extent;
  copy_texture.texture = agent->data;
  copy_buffer.buffer = read_buffer;
  encoder->CopyTextureToBuffer(&copy_texture, &copy_buffer, &copy_extent);

  // Synchronize immediate context and flush
  scheduler->GetContext()->Flush();

  // Fetch buffer to texture
  auto future = read_buffer.MapAsync(
      wgpu::MapMode::Read, 0, read_buffer.GetSize(),
      wgpu::CallbackMode::WaitAnyOnly,
      [&](wgpu::MapAsyncStatus status, const char* message) {
        memcpy(surface_cache->pixels, read_buffer.GetMappedRange(),
               read_buffer.GetSize());
        read_buffer.Unmap();
      });

  // Wait async mapping
  renderer::RenderDevice::GetGPUInstance()->WaitAny(future, UINT64_MAX);
}

void GPUUpdateTexturePixelsDataInternal(CanvasScheduler* scheduler,
                                        TextureAgent* agent,
                                        std::vector<uint8_t> pixels,
                                        base::SingleWorker* worker) {
  auto& device = *scheduler->GetDevice();
  auto* encoder = scheduler->GetContext()->GetImmediateEncoder();

  // Temp pixels read buffer
  wgpu::BufferDescriptor buffer_desc;
  buffer_desc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;
  buffer_desc.size = pixels.size();
  buffer_desc.mappedAtCreation = true;
  wgpu::Buffer write_buffer = device->CreateBuffer(&buffer_desc);

  if (write_buffer.GetMapState() == wgpu::BufferMapState::Mapped) {
    memcpy(write_buffer.GetMappedRange(), pixels.data(), pixels.size());
    write_buffer.Unmap();
  }

  // Copy pixels data to texture
  wgpu::ImageCopyTexture copy_texture;
  wgpu::ImageCopyBuffer copy_buffer;
  wgpu::Extent3D copy_extent;
  copy_texture.texture = agent->data;
  copy_buffer.buffer = write_buffer;
  encoder->CopyBufferToTexture(&copy_buffer, &copy_texture, &copy_extent);
}

void GPUCanvasFillRectInternal(CanvasScheduler* scheduler,
                               TextureAgent* agent,
                               const base::Rect& region,
                               const base::Vec4& color,
                               base::SingleWorker* worker) {
  auto& pipeline_set = scheduler->GetDevice()->GetPipelines()->color;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::kNoBlend);

  wgpu::RenderPassColorAttachment attachment;
  attachment.view = agent->view;
  attachment.loadOp = wgpu::LoadOp::Load;
  attachment.storeOp = wgpu::StoreOp::Store;

  wgpu::RenderPassDescriptor renderpass;
  renderpass.colorAttachmentCount = 1;
  renderpass.colorAttachments = &attachment;

  auto* command_encoder = scheduler->GetContext()->GetImmediateEncoder();

  renderer::FullVertexLayout transient_vertices[4];
  renderer::FullVertexLayout::SetPositionRect(transient_vertices, region);
  renderer::FullVertexLayout::SetColor(transient_vertices, color);
  scheduler->vertex_buffer()->QueueWrite(*command_encoder, transient_vertices,
                                         _countof(transient_vertices));

  auto renderpass_encoder = command_encoder->BeginRenderPass(&renderpass);
  renderpass_encoder.SetViewport(0, 0, agent->size.x, agent->size.y, 0, 0);
  renderpass_encoder.SetPipeline(*pipeline);
  renderpass_encoder.SetVertexBuffer(0, **scheduler->vertex_buffer());
  renderpass_encoder.SetIndexBuffer(**scheduler->index_cache(),
                                    wgpu::IndexFormat::Uint16);
  renderpass_encoder.SetBindGroup(0, agent->world);
  renderpass_encoder.Draw(6);
  renderpass_encoder.End();
}

void GPUCanvasGradientFillRectInternal(CanvasScheduler* scheduler,
                                       TextureAgent* agent,
                                       const base::Rect& region,
                                       const base::Vec4& color1,
                                       const base::Vec4& color2,
                                       bool vertical,
                                       base::SingleWorker* worker) {
  auto& pipeline_set = scheduler->GetDevice()->GetPipelines()->color;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::kNoBlend);

  wgpu::RenderPassColorAttachment attachment;
  attachment.view = agent->view;
  attachment.loadOp = wgpu::LoadOp::Load;
  attachment.storeOp = wgpu::StoreOp::Store;

  wgpu::RenderPassDescriptor renderpass;
  renderpass.colorAttachmentCount = 1;
  renderpass.colorAttachments = &attachment;

  auto* command_encoder = scheduler->GetContext()->GetImmediateEncoder();

  renderer::FullVertexLayout transient_vertices[4];
  renderer::FullVertexLayout::SetPositionRect(transient_vertices, region);
  if (vertical) {
    renderer::FullVertexLayout::SetColor(transient_vertices, color1, 0);
    renderer::FullVertexLayout::SetColor(transient_vertices, color1, 1);
    renderer::FullVertexLayout::SetColor(transient_vertices, color2, 2);
    renderer::FullVertexLayout::SetColor(transient_vertices, color2, 3);
  } else {
    renderer::FullVertexLayout::SetColor(transient_vertices, color1, 0);
    renderer::FullVertexLayout::SetColor(transient_vertices, color2, 1);
    renderer::FullVertexLayout::SetColor(transient_vertices, color2, 2);
    renderer::FullVertexLayout::SetColor(transient_vertices, color1, 3);
  }
  scheduler->vertex_buffer()->QueueWrite(*command_encoder, transient_vertices,
                                         _countof(transient_vertices));

  auto renderpass_encoder = command_encoder->BeginRenderPass(&renderpass);
  renderpass_encoder.SetViewport(0, 0, agent->size.x, agent->size.y, 0, 0);
  renderpass_encoder.SetPipeline(*pipeline);
  renderpass_encoder.SetVertexBuffer(0, **scheduler->vertex_buffer());
  renderpass_encoder.SetIndexBuffer(**scheduler->index_cache(),
                                    wgpu::IndexFormat::Uint16);
  renderpass_encoder.SetBindGroup(0, agent->world);
  renderpass_encoder.Draw(6);
  renderpass_encoder.End();
}

void PostCanvasTask(CanvasScheduler* scheduler, base::WorkerTaskTraits&& task) {
  if (scheduler->render_worker()) {
    // Async post task on render thread.
    scheduler->render_worker()->SendTask(std::move(task));
    return;
  }

  // Execute task immediately
  std::move(task).Run(nullptr);
}

void RequireSynchronizeTask(CanvasScheduler* scheduler) {
  if (scheduler->render_worker()) {
    // Async post sync task on render thread.
    scheduler->render_worker()->WaitWorkerSynchronize();
  }
}

}  // namespace

scoped_refptr<Bitmap> Bitmap::New(ExecutionContext* execution_context,
                                  const std::string& filename,
                                  ExceptionState& exception_state) {
  SDL_Surface* memory_texture = IMG_Load(filename.c_str());
  if (!memory_texture) {
    exception_state.ThrowContentError(ExceptionCode::kContentError,
                                      "Failed to load image: " + filename);
    return nullptr;
  }

  auto* canvas_texture_agent = TextureAgent::Allocate();
  auto* scheduler = execution_context->GetCanvasScheduler();
  PostCanvasTask(
      scheduler,
      base::BindOnce(&GPUCreateTextureWithDataInternal, scheduler->GetDevice(),
                     memory_texture, canvas_texture_agent));

  return new CanvasImpl(scheduler, canvas_texture_agent);
}

scoped_refptr<Bitmap> Bitmap::New(ExecutionContext* execution_context,
                                  uint32_t width,
                                  uint32_t height,
                                  ExceptionState& exception_state) {
  if (width <= 0 || height <= 0) {
    exception_state.ThrowContentError(
        ExceptionCode::kContentError,
        "Invalid bitmap size: " + std::to_string(width) + "x" +
            std::to_string(height));
    return nullptr;
  }

  auto* canvas_texture_agent = TextureAgent::Allocate();
  auto* scheduler = execution_context->GetCanvasScheduler();
  PostCanvasTask(
      scheduler,
      base::BindOnce(&GPUCreateTextureWithSizeInternal, scheduler->GetDevice(),
                     base::Vec2i(width, height), canvas_texture_agent));

  return new CanvasImpl(scheduler, canvas_texture_agent);
}

scoped_refptr<Bitmap> Bitmap::Copy(ExecutionContext* execution_context,
                                   scoped_refptr<Bitmap> other,
                                   ExceptionState& exception_state) {
  scoped_refptr<Bitmap> duplicate_bitmap =
      Bitmap::New(execution_context, other->Width(exception_state),
                  other->Height(exception_state), exception_state);
  duplicate_bitmap->Blt(0, 0, other, other->GetRect(exception_state), 255,
                        exception_state);

  return duplicate_bitmap;
}

scoped_refptr<Bitmap> Bitmap::Deserialize(const std::string&,
                                          ExceptionState& exception_state) {}

std::string Bitmap::Serialize(scoped_refptr<Bitmap>,
                              ExceptionState& exception_state) {}

CanvasImpl::CanvasImpl(CanvasScheduler* scheduler, TextureAgent* texture)
    : scheduler_(scheduler), texture_(texture), canvas_cache_(nullptr) {}

CanvasImpl::~CanvasImpl() {
  ExceptionState exception_state;
  Dispose(exception_state);
}

SDL_Surface* CanvasImpl::RequireMemorySurface() {
  if (!canvas_cache_) {
    // Create empty cache
    canvas_cache_ = SDL_CreateSurface(texture_->size.x, texture_->size.y,
                                      SDL_PIXELFORMAT_ABGR8888);

    // Submit pending commands
    SubmitQueuedCommands();

    // Sync fetch pixels to memory
    PostCanvasTask(scheduler_,
                   base::BindOnce(&GPUFetchTexturePixelsDataInternal,
                                  scheduler_, texture_, canvas_cache_));
    RequireSynchronizeTask(scheduler_);
  }

  return canvas_cache_;
}

void CanvasImpl::InvalidateSurfaceCache() {
  if (canvas_cache_) {
    // Set cache ptr to null
    SDL_DestroySurface(canvas_cache_);
    canvas_cache_ = nullptr;
  }
}

void CanvasImpl::UpdateVideoMemory() {
  if (canvas_cache_) {
    // Copy pixels to temp closure parameter
    std::vector<uint8_t> pixels;
    pixels.assign(canvas_cache_->pitch * canvas_cache_->h, 0);
    memcpy(pixels.data(), canvas_cache_->pixels, pixels.size());

    // Post async upload request
    PostCanvasTask(scheduler_,
                   base::BindOnce(&GPUUpdateTexturePixelsDataInternal,
                                  scheduler_, texture_, std::move(pixels)));
  }
}

void CanvasImpl::SubmitQueuedCommands() {
  Command* command_sequence = nullptr;
  // Execute pending commands,
  // encode draw command in wgpu encoder.
  while (command_sequence) {
    switch (command_sequence->id) {
      case CommandID::kGradientFillRect: {
        auto* c = static_cast<Command_GradientFillRect*>(command_sequence);
        PostCanvasTask(scheduler_,
                       base::BindOnce(&GPUCanvasGradientFillRectInternal,
                                      scheduler_, texture_, c->region,
                                      c->color1, c->color2, c->vertical));
      } break;
      case CommandID::kHueChange: {
        auto* c = static_cast<Command_HueChange*>(command_sequence);

      } break;
      case CommandID::kRadialBlur: {
        auto* c = static_cast<Command_RadialBlur*>(command_sequence);

      } break;
      case CommandID::kDrawText: {
        auto* c = static_cast<Command_DrawText*>(command_sequence);

      } break;
      default:
        break;
    }

    command_sequence = command_sequence->next;
  }

  // Clear command pool,
  // no memory release.
  ClearPendingCommands();
}

void CanvasImpl::Dispose(ExceptionState& exception_state) {
  base::LinkNode<CanvasImpl>::RemoveFromList();

  if (!IsDisposed(exception_state)) {
    // Destroy GPU texture
    PostCanvasTask(scheduler_,
                   base::BindOnce(&GPUDestroyTextureInternal, texture_));
    texture_ = nullptr;

    // Free memory surface cache
    if (canvas_cache_) {
      SDL_DestroySurface(canvas_cache_);
      canvas_cache_ = nullptr;
    }
  }
}

bool CanvasImpl::IsDisposed(ExceptionState& exception_state) {
  return !texture_;
}

uint32_t CanvasImpl::Width(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;
  return texture_->size.x;
}

uint32_t CanvasImpl::Height(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;
  return texture_->size.y;
}

scoped_refptr<Rect> CanvasImpl::GetRect(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;
  return Rect::New(0, 0, texture_->size.x, texture_->size.y, exception_state);
}

void CanvasImpl::Blt(int32_t x,
                     int32_t y,
                     scoped_refptr<Bitmap> src_bitmap,
                     scoped_refptr<Rect> src_rect,
                     uint32_t opacity,
                     ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  CanvasImpl* src_canvas = static_cast<CanvasImpl*>(src_bitmap.get());
  BlitTextureInternal(base::Rect(x, y, src_rect->Get_Width(exception_state),
                                 src_rect->Get_Height(exception_state)),
                      src_canvas, RectImpl::From(src_rect)->AsBaseRect(),
                      opacity);
}

void CanvasImpl::StretchBlt(scoped_refptr<Rect> dest_rect,
                            scoped_refptr<Bitmap> src_bitmap,
                            scoped_refptr<Rect> src_rect,
                            uint32_t opacity,
                            ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  CanvasImpl* src_canvas = static_cast<CanvasImpl*>(src_bitmap.get());
  BlitTextureInternal(RectImpl::From(dest_rect)->AsBaseRect(), src_canvas,
                      RectImpl::From(src_rect)->AsBaseRect(), opacity);
}

void CanvasImpl::FillRect(int32_t x,
                          int32_t y,
                          uint32_t width,
                          uint32_t height,
                          scoped_refptr<Color> color,
                          ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto* command = AllocateCommand<Command_GradientFillRect>();
  command->region = base::Rect(x, y, width, height);
  command->color1 = ColorImpl::From(color.get())->AsNormColor();
  command->color2 = command->color1;
  command->vertical = false;
}

void CanvasImpl::FillRect(scoped_refptr<Rect> rect,
                          scoped_refptr<Color> color,
                          ExceptionState& exception_state) {
  FillRect(rect->Get_X(exception_state), rect->Get_Y(exception_state),
           rect->Get_Width(exception_state), rect->Get_Height(exception_state),
           color, exception_state);
}

void CanvasImpl::GradientFillRect(int32_t x,
                                  int32_t y,
                                  uint32_t width,
                                  uint32_t height,
                                  scoped_refptr<Color> color1,
                                  scoped_refptr<Color> color2,
                                  bool vertical,
                                  ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto* command = AllocateCommand<Command_GradientFillRect>();
  command->region = base::Rect(x, y, width, height);
  command->color1 = ColorImpl::From(color1.get())->AsNormColor();
  command->color2 = ColorImpl::From(color2.get())->AsNormColor();
  command->vertical = vertical;
}

void CanvasImpl::GradientFillRect(scoped_refptr<Rect> rect,
                                  scoped_refptr<Color> color1,
                                  scoped_refptr<Color> color2,
                                  bool vertical,
                                  ExceptionState& exception_state) {
  GradientFillRect(rect->Get_X(exception_state), rect->Get_Y(exception_state),
                   rect->Get_Width(exception_state),
                   rect->Get_Height(exception_state), color1, color2, vertical,
                   exception_state);
}

void CanvasImpl::Clear(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto* command = AllocateCommand<Command_GradientFillRect>();
  command->region =
      base::Rect(0, 0, Width(exception_state), Height(exception_state));
  command->color1 = base::Vec4(0);
  command->color2 = command->color1;
  command->vertical = false;
}

void CanvasImpl::ClearRect(int32_t x,
                           int32_t y,
                           uint32_t width,
                           uint32_t height,
                           ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto* command = AllocateCommand<Command_GradientFillRect>();
  command->region =
      base::Rect(0, 0, Width(exception_state), Height(exception_state));
  command->color1 = base::Vec4(0);
  command->color2 = command->color1;
  command->vertical = false;
}

void CanvasImpl::ClearRect(scoped_refptr<Rect> rect,
                           ExceptionState& exception_state) {
  ClearRect(rect->Get_X(exception_state), rect->Get_Y(exception_state),
            rect->Get_Width(exception_state), rect->Get_Height(exception_state),
            exception_state);
}

scoped_refptr<Color> CanvasImpl::GetPixel(int32_t x,
                                          int32_t y,
                                          ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  SDL_Surface* pixels = RequireMemorySurface();
}

void CanvasImpl::SetPixel(int32_t x,
                          int32_t y,
                          scoped_refptr<Color> color,
                          ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto* command = AllocateCommand<Command_GradientFillRect>();
  command->region = base::Rect(x, y, 1, 1);
  command->color1 = ColorImpl::From(color.get())->AsNormColor();
  command->color2 = command->color1;
  command->vertical = false;
}

void CanvasImpl::HueChange(int32_t hue, ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto* command = AllocateCommand<Command_HueChange>();
  command->hue = hue;
}

void CanvasImpl::Blur(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto* command = AllocateCommand<Command_RadialBlur>();
  command->blur = true;
}

void CanvasImpl::RadialBlur(int32_t angle,
                            int32_t division,
                            ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto* command = AllocateCommand<Command_RadialBlur>();
  command->blur = false;
  command->angle = angle;
  command->division = division;
}

void CanvasImpl::DrawText(int32_t x,
                          int32_t y,
                          uint32_t width,
                          uint32_t height,
                          const std::string& str,
                          int32_t align,
                          ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto* font_object = static_cast<FontImpl*>(font_.get());
  if (!font_object->GetCanonicalFont(exception_state))
    return;

  auto* text_surface = font_object->RenderText(str, 0, exception_state);
  if (!text_surface)
    return;

  // Destroy memory surface in queue deferrer.
  auto* command = AllocateCommand<Command_DrawText>();
  command->region = base::Rect(x, y, width, height);
  command->text = text_surface;  // Transient surface object
  command->align = align;
}

void CanvasImpl::DrawText(scoped_refptr<Rect> rect,
                          const std::string& str,
                          int32_t align,
                          ExceptionState& exception_state) {
  DrawText(rect->Get_X(exception_state), rect->Get_Y(exception_state),
           rect->Get_Width(exception_state), rect->Get_Height(exception_state),
           str, align, exception_state);
}

scoped_refptr<Rect> CanvasImpl::TextSize(const std::string& str,
                                         ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  TTF_Font* font =
      static_cast<FontImpl*>(font_.get())->GetCanonicalFont(exception_state);
  if (!font)
    return nullptr;

  int w, h;
  TTF_GetStringSize(font, str.c_str(), str.size(), &w, &h);
  return Rect::New(0, 0, w, h, exception_state);
}

bool CanvasImpl::CheckDisposed(ExceptionState& exception_state) {
  if (IsDisposed(exception_state)) {
    exception_state.ThrowContentError(ExceptionCode::kDisposedObject,
                                      "disposed object: bitmap");
    return true;
  }

  return false;
}

void CanvasImpl::BlitTextureInternal(const base::Rect& dst_rect,
                                     CanvasImpl* src_texture,
                                     const base::Rect& src_rect,
                                     float alpha) {
  // Synchronize pending queue immediately,
  // blit the sourcetexture to destination texture immediately.
  src_texture->SubmitQueuedCommands();
  SubmitQueuedCommands();

  // Execute blit immediately.
  PostCanvasTask(
      scheduler_,
      base::BindOnce(&GPUBlendBlitTextureInternal, scheduler_, texture_,
                     dst_rect, src_texture->texture_, src_rect, alpha));
}

scoped_refptr<Font> CanvasImpl::Get_Font(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;
  return font_;
}

void CanvasImpl::Put_Font(const scoped_refptr<Font>& value,
                          ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;
  font_ = value;
}

TextureAgent* TextureAgent::Allocate(size_t n) {
  TextureAgent* agents = g_textures_pool.allocate(n);
  for (int i = 0; i < n; ++i)
    g_textures_pool.construct(agents + i);
  return agents;
}

void TextureAgent::Free(TextureAgent* ptr, size_t n) {
  for (int i = 0; i < n; ++i)
    g_textures_pool.destroy(ptr + i);
  g_textures_pool.deallocate(ptr, n);
}

}  // namespace content
