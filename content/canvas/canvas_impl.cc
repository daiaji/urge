// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/canvas/canvas_impl.h"

#include "SDL3_image/SDL_image.h"
#include "SDL3_ttf/SDL_ttf.h"

#include "content/canvas/canvas_scheduler.h"
#include "content/common/color_impl.h"
#include "content/common/rect_impl.h"
#include "content/context/execution_context.h"
#include "content/screen/renderscreen_impl.h"
#include "renderer/utils/texture_utils.h"

namespace content {

namespace {

wgpu::BindGroup MakeTextureWorldInternal(renderer::RenderDevice* device_base,
                                         const base::Vec2& bitmap_size) {
  renderer::WorldMatrixUniform world_matrix;
  renderer::MakeProjectionMatrix(world_matrix.projection, bitmap_size);
  renderer::MakeIdentityMatrix(world_matrix.transform);

  auto uniform_buffer =
      renderer::CreateUniformBuffer<renderer::WorldMatrixUniform>(
          **device_base, "bitmap.world", wgpu::BufferUsage::None,
          &world_matrix);

  return renderer::WorldMatrixUniform::CreateGroup(**device_base,
                                                   uniform_buffer);
}

wgpu::BindGroup MakeTextureBindingGroupInternal(
    renderer::RenderDevice* device_base,
    const base::Vec2& bitmap_size,
    const wgpu::TextureView& view,
    const wgpu::Sampler& sampler) {
  renderer::TextureBindingUniform texture_size;
  texture_size.texture_size = base::MakeInvert(bitmap_size);

  auto uniform_buffer =
      renderer::CreateUniformBuffer<renderer::TextureBindingUniform>(
          **device_base, "bitmap.texture", wgpu::BufferUsage::None,
          &texture_size);

  return renderer::TextureBindingUniform::CreateGroup(**device_base, view,
                                                      sampler, uniform_buffer);
}

wgpu::BindGroup MakeTextCacheInternal(renderer::RenderDevice* device_base,
                                      const base::Vec2i& cache_size,
                                      const wgpu::Sampler& sampler,
                                      wgpu::Texture* cache) {
  renderer::TextureBindingUniform texture_size;
  texture_size.texture_size = base::MakeInvert(cache_size);

  auto uniform_buffer =
      renderer::CreateUniformBuffer<renderer::TextureBindingUniform>(
          **device_base, "text.cache.texture", wgpu::BufferUsage::None,
          &texture_size);

  // Create video memory texture
  *cache = renderer::CreateTexture2D(
      **device_base, "textdraw.cache",
      wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding,
      cache_size);

  return renderer::TextureBindingUniform::CreateGroup(
      **device_base, cache->CreateView(), sampler, uniform_buffer);
}

void GPUCreateTextureWithDataInternal(renderer::RenderDevice* device_base,
                                      SDL_Surface* initial_data,
                                      TextureAgent* agent) {
  wgpu::TextureUsage texture_usage =
      wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc |
      wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;

  agent->size = base::Vec2i(initial_data->w, initial_data->h);
  agent->data = renderer::CreateTexture2D(**device_base, "bitmap.texture",
                                          texture_usage, agent->size);
  agent->view = agent->data.CreateView();
  agent->sampler = (*device_base)->CreateSampler();
  agent->world = MakeTextureWorldInternal(device_base, agent->size);
  agent->binding = MakeTextureBindingGroupInternal(device_base, agent->size,
                                                   agent->view, agent->sampler);

  // Write data in video memory
  wgpu::TexelCopyTextureInfo copy_texture;
  copy_texture.texture = agent->data;

  wgpu::TexelCopyBufferLayout texture_data_layout;
  texture_data_layout.bytesPerRow = initial_data->pitch;

  wgpu::Extent3D write_size;
  write_size.width = initial_data->w;
  write_size.height = initial_data->h;

  device_base->GetQueue()->WriteTexture(&copy_texture, initial_data->pixels,
                                        initial_data->pitch * initial_data->h,
                                        &texture_data_layout, &write_size);
}

void GPUCreateTextureWithSizeInternal(renderer::RenderDevice* device_base,
                                      const base::Vec2i& initial_size,
                                      TextureAgent* agent) {
  wgpu::TextureUsage texture_usage =
      wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc |
      wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;

  agent->size = initial_size;
  agent->data = renderer::CreateTexture2D(**device_base, "bitmap.texture",
                                          texture_usage, agent->size);
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
                                 float blit_alpha) {
  auto& pipeline_set = scheduler->GetDevice()->GetPipelines()->base;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NORMAL);

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
                                    scheduler->index_cache()->format());
  renderpass_encoder.SetBindGroup(0, dst_texture->world);
  renderpass_encoder.SetBindGroup(1, src_texture->binding);
  renderpass_encoder.DrawIndexed(6);
  renderpass_encoder.End();
}

void GPUDestroyTextureInternal(TextureAgent* agent) {
  agent->data = nullptr;
  agent->view = nullptr;

  agent->sampler = nullptr;
  agent->world = nullptr;
  agent->binding = nullptr;

  agent->text_surface_cache = nullptr;
  agent->text_surface_cache = nullptr;
  agent->text_write_cache = nullptr;

  delete agent;
}

void GPUFetchTexturePixelsDataInternal(CanvasScheduler* scheduler,
                                       TextureAgent* agent,
                                       SDL_Surface* surface_cache) {
  auto& device = *scheduler->GetDevice();
  auto* encoder = scheduler->GetContext()->GetImmediateEncoder();

  // Buffer align
  uint32_t aligned_bytes_per_row = (surface_cache->pitch + 255) & ~255;

  // Temp pixels read buffer
  wgpu::BufferDescriptor buffer_desc;
  buffer_desc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
  buffer_desc.size = aligned_bytes_per_row * surface_cache->h;
  buffer_desc.mappedAtCreation = false;
  wgpu::Buffer read_buffer = device->CreateBuffer(&buffer_desc);

  // Copy pixels data to temp buffer
  wgpu::TexelCopyTextureInfo copy_texture;
  wgpu::TexelCopyBufferInfo copy_buffer;
  wgpu::Extent3D copy_extent;
  copy_texture.texture = agent->data;
  copy_buffer.buffer = read_buffer;
  copy_buffer.layout.bytesPerRow = aligned_bytes_per_row;
  copy_buffer.layout.rowsPerImage = surface_cache->h;
  copy_extent.width = surface_cache->w;
  copy_extent.height = surface_cache->h;
  encoder->CopyTextureToBuffer(&copy_texture, &copy_buffer, &copy_extent);

  // Synchronize immediate context and flush
  scheduler->GetContext()->Flush();

  // Fetch buffer to texture
  auto future = read_buffer.MapAsync(
      wgpu::MapMode::Read, 0, read_buffer.GetSize(),
      wgpu::CallbackMode::WaitAnyOnly,
      [&](wgpu::MapAsyncStatus status, wgpu::StringView message) {
        const auto* src_pixels =
            static_cast<const uint8_t*>(read_buffer.GetConstMappedRange());

        for (uint32_t y = 0; y < surface_cache->h; ++y)
          std::memcpy(static_cast<uint8_t*>(surface_cache->pixels) +
                          y * surface_cache->pitch,
                      src_pixels + y * aligned_bytes_per_row,
                      surface_cache->pitch);

        read_buffer.Unmap();
      });

  // Wait async mapping
  renderer::RenderDevice::GetGPUInstance()->WaitAny(future, UINT64_MAX);
}

void GPUUpdateTexturePixelsDataInternal(CanvasScheduler* scheduler,
                                        TextureAgent* agent,
                                        std::vector<uint8_t> pixels,
                                        const base::Vec2i& surface_size) {
  auto& device = *scheduler->GetDevice();
  auto* encoder = scheduler->GetContext()->GetImmediateEncoder();

  // Temp pixels read buffer
  wgpu::BufferDescriptor buffer_desc;
  buffer_desc.label = "updatetexture.temp.buffer";
  buffer_desc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;
  buffer_desc.size = pixels.size();
  buffer_desc.mappedAtCreation = true;
  wgpu::Buffer write_buffer = device->CreateBuffer(&buffer_desc);

  std::memcpy(write_buffer.GetMappedRange(), pixels.data(), pixels.size());
  write_buffer.Unmap();

  // Align size
  uint32_t aligned_bytes_per_row = (surface_size.x * 4 + 255) & ~255;

  // Copy pixels data to texture
  wgpu::TexelCopyTextureInfo copy_texture;
  wgpu::TexelCopyBufferInfo copy_buffer;
  wgpu::Extent3D copy_extent;
  copy_texture.texture = agent->data;
  copy_buffer.buffer = write_buffer;
  copy_buffer.layout.bytesPerRow = aligned_bytes_per_row;
  copy_buffer.layout.rowsPerImage = surface_size.y;
  copy_extent.width = surface_size.x;
  copy_extent.height = surface_size.y;
  encoder->CopyBufferToTexture(&copy_buffer, &copy_texture, &copy_extent);
}

void GPUCanvasFillRectInternal(CanvasScheduler* scheduler,
                               TextureAgent* agent,
                               const base::Rect& region,
                               const base::Vec4& color) {
  auto& pipeline_set = scheduler->GetDevice()->GetPipelines()->color;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NO_BLEND);

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
                                    scheduler->index_cache()->format());
  renderpass_encoder.SetBindGroup(0, agent->world);
  renderpass_encoder.DrawIndexed(6);
  renderpass_encoder.End();
}

void GPUCanvasGradientFillRectInternal(CanvasScheduler* scheduler,
                                       TextureAgent* agent,
                                       const base::Rect& region,
                                       const base::Vec4& color1,
                                       const base::Vec4& color2,
                                       bool vertical) {
  auto& pipeline_set = scheduler->GetDevice()->GetPipelines()->color;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NO_BLEND);

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
                                    scheduler->index_cache()->format());
  renderpass_encoder.SetBindGroup(0, agent->world);
  renderpass_encoder.DrawIndexed(6);
  renderpass_encoder.End();
}

void GPUCanvasDrawTextSurfaceInternal(CanvasScheduler* scheduler,
                                      TextureAgent* agent,
                                      const base::Rect& region,
                                      SDL_Surface* text,
                                      float opacity,
                                      int align) {
  auto& pipeline_set = scheduler->GetDevice()->GetPipelines()->base;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NORMAL);

  if (!agent->text_surface_cache ||
      agent->text_surface_cache.GetWidth() < text->w ||
      agent->text_surface_cache.GetHeight() < text->h) {
    base::Vec2i text_size;
    text_size.x = std::max<int32_t>(
        agent->text_surface_cache ? agent->text_surface_cache.GetWidth() : 0,
        text->w);
    text_size.y = std::max<int32_t>(
        agent->text_surface_cache ? agent->text_surface_cache.GetHeight() : 0,
        text->h);

    agent->text_cache_binding =
        MakeTextCacheInternal(scheduler->GetDevice(), text_size, agent->sampler,
                              &agent->text_surface_cache);
  }

  wgpu::RenderPassColorAttachment attachment;
  attachment.view = agent->view;
  attachment.loadOp = wgpu::LoadOp::Load;
  attachment.storeOp = wgpu::StoreOp::Store;

  wgpu::RenderPassDescriptor renderpass;
  renderpass.colorAttachmentCount = 1;
  renderpass.colorAttachments = &attachment;

  auto* command_encoder = scheduler->GetContext()->GetImmediateEncoder();

  // Align size
  uint32_t aligned_bytes_per_row =
      (agent->text_surface_cache.GetWidth() * 4 + 255) & ~255;
  uint32_t buffer_size =
      aligned_bytes_per_row * agent->text_surface_cache.GetHeight();

  if (!agent->text_write_cache ||
      agent->text_write_cache.GetSize() < buffer_size) {
    // Temp pixels read buffer
    wgpu::BufferDescriptor buffer_desc;
    buffer_desc.label = "drawtext.temp.buffer";
    buffer_desc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    buffer_desc.size = buffer_size;
    buffer_desc.mappedAtCreation = false;
    agent->text_write_cache =
        (*scheduler->GetDevice())->CreateBuffer(&buffer_desc);
  }

  for (size_t i = 0; i < text->h; ++i)
    command_encoder->WriteBuffer(
        agent->text_write_cache, aligned_bytes_per_row * i,
        static_cast<uint8_t*>(text->pixels) + text->pitch * i, text->pitch);

  // Copy pixels data to texture
  wgpu::TexelCopyTextureInfo copy_texture;
  wgpu::TexelCopyBufferInfo copy_buffer;
  wgpu::Extent3D copy_extent;
  copy_texture.texture = agent->text_surface_cache;
  copy_buffer.buffer = agent->text_write_cache;
  copy_buffer.layout.bytesPerRow = aligned_bytes_per_row;
  copy_buffer.layout.rowsPerImage = agent->text_surface_cache.GetHeight();
  copy_extent.width = text->w;
  copy_extent.height = text->h;
  command_encoder->CopyBufferToTexture(&copy_buffer, &copy_texture,
                                       &copy_extent);

  base::Vec4 blend_alpha;
  blend_alpha.w = opacity / 255.0f;

  int align_x = region.x, align_y = region.y + (region.height - text->h) / 2;
  switch (align) {
    default:
    case 0:  // Left
      break;
    case 1:  // Center
      align_x += (region.width - text->w) / 2;
      break;
    case 2:  // Right
      align_x += region.width - text->w;
      break;
  }

  float zoom_x = static_cast<float>(region.width) / text->w;
  zoom_x = std::min(zoom_x, 1.0f);
  base::Rect compose_position(align_x, align_y, text->w * zoom_x, text->h);

  renderer::FullVertexLayout transient_vertices[4];
  renderer::FullVertexLayout::SetTexCoordRect(transient_vertices,
                                              base::Vec2(text->w, text->h));
  renderer::FullVertexLayout::SetPositionRect(transient_vertices,
                                              compose_position);
  renderer::FullVertexLayout::SetColor(transient_vertices, blend_alpha);
  scheduler->vertex_buffer()->QueueWrite(*command_encoder, transient_vertices,
                                         _countof(transient_vertices));

  auto renderpass_encoder = command_encoder->BeginRenderPass(&renderpass);
  renderpass_encoder.SetViewport(0, 0, agent->size.x, agent->size.y, 0, 0);
  renderpass_encoder.SetPipeline(*pipeline);
  renderpass_encoder.SetVertexBuffer(0, **scheduler->vertex_buffer());
  renderpass_encoder.SetIndexBuffer(**scheduler->index_cache(),
                                    scheduler->index_cache()->format());
  renderpass_encoder.SetBindGroup(0, agent->world);
  renderpass_encoder.SetBindGroup(1, agent->text_cache_binding);
  renderpass_encoder.DrawIndexed(6);
  renderpass_encoder.End();
}

}  // namespace

scoped_refptr<Bitmap> Bitmap::New(ExecutionContext* execution_context,
                                  const std::string& filename,
                                  ExceptionState& exception_state) {
  return CanvasImpl::Create(
      execution_context->canvas_scheduler, execution_context->graphics,
      execution_context->font_context, filename, exception_state);
}

scoped_refptr<Bitmap> Bitmap::New(ExecutionContext* execution_context,
                                  uint32_t width,
                                  uint32_t height,
                                  ExceptionState& exception_state) {
  return CanvasImpl::Create(execution_context->canvas_scheduler,
                            execution_context->graphics,
                            execution_context->font_context,
                            base::Vec2i(width, height), exception_state);
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
                                          ExceptionState& exception_state) {
  return nullptr;
}

std::string Bitmap::Serialize(scoped_refptr<Bitmap>,
                              ExceptionState& exception_state) {
  return std::string();
}

scoped_refptr<CanvasImpl> CanvasImpl::Create(CanvasScheduler* scheduler,
                                             RenderScreenImpl* screen,
                                             ScopedFontData* font_data,
                                             const base::Vec2i& size,
                                             ExceptionState& exception_state) {
  if (size.x <= 0 || size.y <= 0) {
    exception_state.ThrowContentError(
        ExceptionCode::CONTENT_ERROR,
        "Invalid bitmap size: " + std::to_string(size.x) + "x" +
            std::to_string(size.y));
    return nullptr;
  }

  auto* canvas_texture_agent = new TextureAgent;
  canvas_texture_agent->size = size;

  base::ThreadWorker::PostTask(
      scheduler->render_worker(),
      base::BindOnce(&GPUCreateTextureWithSizeInternal, scheduler->GetDevice(),
                     size, canvas_texture_agent));

  return new CanvasImpl(screen, scheduler, canvas_texture_agent,
                        new FontImpl(font_data));
}

scoped_refptr<CanvasImpl> CanvasImpl::Create(CanvasScheduler* scheduler,
                                             RenderScreenImpl* screen,
                                             ScopedFontData* font_data,
                                             const std::string& filename,
                                             ExceptionState& exception_state) {
  SDL_Surface* memory_texture = nullptr;
  auto file_handler = base::BindRepeating(
      [](SDL_Surface** surf, SDL_IOStream* ops, const std::string& ext) {
        *surf = IMG_LoadTyped_IO(ops, true, ext.c_str());
        return !!*surf;
      },
      &memory_texture);

  filesystem::IOState io_state;
  scheduler->GetIO()->OpenRead(filename, file_handler, &io_state);

  if (io_state.error_count) {
    exception_state.ThrowContentError(
        ExceptionCode::IO_ERROR,
        "Failed to read file: " + filename + " - " + io_state.error_message);
    return nullptr;
  }

  if (!memory_texture) {
    exception_state.ThrowContentError(
        ExceptionCode::CONTENT_ERROR,
        "Failed to load image: " + filename + " - " + SDL_GetError());
    return nullptr;
  }

  auto* canvas_texture_agent = new TextureAgent;
  canvas_texture_agent->size =
      base::Vec2i(memory_texture->w, memory_texture->h);

  base::ThreadWorker::PostTask(
      scheduler->render_worker(),
      base::BindOnce(&GPUCreateTextureWithDataInternal, scheduler->GetDevice(),
                     memory_texture, canvas_texture_agent));

  return new CanvasImpl(screen, scheduler, canvas_texture_agent,
                        new FontImpl(font_data));
}

CanvasImpl::CanvasImpl(RenderScreenImpl* screen,
                       CanvasScheduler* scheduler,
                       TextureAgent* texture,
                       scoped_refptr<Font> font)
    : Disposable(screen),
      scheduler_(scheduler),
      texture_(texture),
      canvas_cache_(nullptr),
      font_(FontImpl::From(font)) {
  scheduler->AttachChildCanvas(this);
}

CanvasImpl::~CanvasImpl() {
  ExceptionState exception_state;
  Dispose(exception_state);
}

scoped_refptr<CanvasImpl> CanvasImpl::FromBitmap(scoped_refptr<Bitmap> host) {
  return static_cast<CanvasImpl*>(host.get());
}

SDL_Surface* CanvasImpl::RequireMemorySurface() {
  if (!canvas_cache_) {
    // Create empty cache
    canvas_cache_ = SDL_CreateSurface(texture_->size.x, texture_->size.y,
                                      SDL_PIXELFORMAT_ABGR8888);

    // Submit pending commands
    SubmitQueuedCommands();

    // Sync fetch pixels to memory
    base::ThreadWorker::PostTask(
        scheduler_->render_worker(),
        base::BindOnce(&GPUFetchTexturePixelsDataInternal, scheduler_, texture_,
                       canvas_cache_));
    base::ThreadWorker::WaitWorkerSynchronize(scheduler_->render_worker());
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
    // Aligned bytes
    uint32_t aligned_bytes_per_row = (canvas_cache_->pitch + 255) & ~255;

    // Copy pixels to temp closure parameter
    std::vector<uint8_t> pixels;
    pixels.assign(aligned_bytes_per_row * canvas_cache_->h, 0);

    for (size_t y = 0; y < canvas_cache_->h; ++y)
      std::memcpy(pixels.data() + y * aligned_bytes_per_row,
                  static_cast<uint8_t*>(canvas_cache_->pixels) +
                      y * canvas_cache_->pitch,
                  canvas_cache_->pitch);

    // Post async upload request
    base::ThreadWorker::PostTask(
        scheduler_->render_worker(),
        base::BindOnce(&GPUUpdateTexturePixelsDataInternal, scheduler_,
                       texture_, std::move(pixels),
                       base::Vec2i(canvas_cache_->pitch, canvas_cache_->h)));
  }
}

void CanvasImpl::SubmitQueuedCommands() {
  Command* command_sequence = commands_;
  // Execute pending commands,
  // encode draw command in wgpu encoder.
  while (command_sequence) {
    switch (command_sequence->id) {
      case CommandID::GRADIENT_FILL_RECT: {
        const auto* c =
            static_cast<Command_GradientFillRect*>(command_sequence);
        base::ThreadWorker::PostTask(
            scheduler_->render_worker(),
            base::BindOnce(&GPUCanvasGradientFillRectInternal, scheduler_,
                           texture_, c->region, c->color1, c->color2,
                           c->vertical));
      } break;
      case CommandID::HUE_CHANGE: {
        const auto* c = static_cast<Command_HueChange*>(command_sequence);

      } break;
      case CommandID::RADIAL_BLUR: {
        const auto* c = static_cast<Command_RadialBlur*>(command_sequence);

      } break;
      case CommandID::DRAW_TEXT: {
        const auto* c = static_cast<Command_DrawText*>(command_sequence);
        base::ThreadWorker::PostTask(
            scheduler_->render_worker(),
            base::BindOnce(&GPUCanvasDrawTextSurfaceInternal, scheduler_,
                           texture_, c->region, c->text, c->opacity, c->align));
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

base::Vec2i CanvasImpl::AsBaseSize() const {
  if (texture_)
    return texture_->size;
  return base::Vec2i(0);
}

void CanvasImpl::Dispose(ExceptionState& exception_state) {
  Disposable::Dispose(exception_state);
}

bool CanvasImpl::IsDisposed(ExceptionState& exception_state) {
  return Disposable::IsDisposed(exception_state);
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

  return new RectImpl(texture_->size);
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

  InvalidateSurfaceCache();
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

  InvalidateSurfaceCache();
}

void CanvasImpl::GradientFillRect(int32_t x,
                                  int32_t y,
                                  uint32_t width,
                                  uint32_t height,
                                  scoped_refptr<Color> color1,
                                  scoped_refptr<Color> color2,
                                  ExceptionState& exception_state) {
  GradientFillRect(x, y, width, height, color1, color2, false, exception_state);
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

void CanvasImpl::GradientFillRect(scoped_refptr<Rect> rect,
                                  scoped_refptr<Color> color1,
                                  scoped_refptr<Color> color2,
                                  ExceptionState& exception_state) {
  GradientFillRect(rect->Get_X(exception_state), rect->Get_Y(exception_state),
                   rect->Get_Width(exception_state),
                   rect->Get_Height(exception_state), color1, color2, false,
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

  InvalidateSurfaceCache();
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

  InvalidateSurfaceCache();
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

  if (x < 0 || x >= texture_->size.x || y < 0 || y >= texture_->size.y)
    return nullptr;

  SDL_Surface* surface = RequireMemorySurface();
  auto* pixel_detail = SDL_GetPixelFormatDetails(surface->format);
  int bpp = pixel_detail->bytes_per_pixel;
  uint8_t* pixel = static_cast<uint8_t*>(surface->pixels) +
                   static_cast<size_t>(y) * surface->pitch +
                   static_cast<size_t>(x) * bpp;

  uint8_t color[4];
  SDL_GetRGBA(*reinterpret_cast<uint32_t*>(pixel), pixel_detail, nullptr,
              &color[0], &color[1], &color[2], &color[3]);

  return new ColorImpl(base::Vec4(color[0], color[1], color[2], color[3]));
}

void CanvasImpl::SetPixel(int32_t x,
                          int32_t y,
                          scoped_refptr<Color> color,
                          ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  if (x < 0 || x >= texture_->size.x || y < 0 || y >= texture_->size.y)
    return;

  auto* command = AllocateCommand<Command_GradientFillRect>();
  command->region = base::Rect(x, y, 1, 1);
  command->color1 = ColorImpl::From(color.get())->AsNormColor();
  command->color2 = command->color1;
  command->vertical = false;

  InvalidateSurfaceCache();
}

void CanvasImpl::HueChange(int32_t hue, ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  if (hue % 360 == 0)
    return;

  while (hue < 0)
    hue += 359;
  hue %= 359;

  auto* command = AllocateCommand<Command_HueChange>();
  command->hue = hue;

  InvalidateSurfaceCache();
}

void CanvasImpl::Blur(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto* command = AllocateCommand<Command_RadialBlur>();
  command->blur = true;

  InvalidateSurfaceCache();
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

  InvalidateSurfaceCache();
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

  uint8_t font_opacity = 0;
  auto* text_surface =
      font_object->RenderText(str, &font_opacity, exception_state);
  if (!text_surface)
    return;

  // Destroy memory surface in queue deferrer.
  auto* command = AllocateCommand<Command_DrawText>();
  command->region = base::Rect(x, y, width, height);
  command->text = text_surface;  // Transient surface object
  command->opacity = static_cast<float>(font_opacity);
  command->align = align;

  InvalidateSurfaceCache();
}

void CanvasImpl::DrawText(int32_t x,
                          int32_t y,
                          uint32_t width,
                          uint32_t height,
                          const std::string& str,
                          ExceptionState& exception_state) {
  DrawText(x, y, width, height, str, 0, exception_state);
}

void CanvasImpl::DrawText(scoped_refptr<Rect> rect,
                          const std::string& str,
                          int32_t align,
                          ExceptionState& exception_state) {
  DrawText(rect->Get_X(exception_state), rect->Get_Y(exception_state),
           rect->Get_Width(exception_state), rect->Get_Height(exception_state),
           str, align, exception_state);
}

void CanvasImpl::DrawText(scoped_refptr<Rect> rect,
                          const std::string& str,
                          ExceptionState& exception_state) {
  DrawText(rect->Get_X(exception_state), rect->Get_Y(exception_state),
           rect->Get_Width(exception_state), rect->Get_Height(exception_state),
           str, 0, exception_state);
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
  return new RectImpl(base::Rect(0, 0, w, h));
}

void CanvasImpl::OnObjectDisposed() {
  // Unlink from canvas scheduler
  base::LinkNode<CanvasImpl>::RemoveFromList();

  // Destroy GPU texture
  base::ThreadWorker::PostTask(
      scheduler_->render_worker(),
      base::BindOnce(&GPUDestroyTextureInternal, texture_));
  texture_ = nullptr;

  // Free memory surface cache
  if (canvas_cache_) {
    SDL_DestroySurface(canvas_cache_);
    canvas_cache_ = nullptr;
  }
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
  base::ThreadWorker::PostTask(
      scheduler_->render_worker(),
      base::BindOnce(&GPUBlendBlitTextureInternal, scheduler_, texture_,
                     dst_rect, src_texture->texture_, src_rect, alpha));

  // Invalidate memory cache
  InvalidateSurfaceCache();
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
  *font_ = *FontImpl::From(value);
}

}  // namespace content
