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

void MakeTextureWorldInternal(renderer::RenderDevice* device,
                              const base::Vec2& bitmap_size,
                              Diligent::IBuffer** out) {
  renderer::WorldTransform world_matrix;
  renderer::MakeProjectionMatrix(world_matrix.projection, bitmap_size);
  renderer::MakeIdentityMatrix(world_matrix.transform);

  Diligent::CreateUniformBuffer(
      **device, sizeof(world_matrix), "bitmap.binding.world", out,
      Diligent::USAGE_IMMUTABLE, Diligent::BIND_UNIFORM_BUFFER,
      Diligent::CPU_ACCESS_WRITE, &world_matrix);
}

void GPUCreateTextureWithDataInternal(renderer::RenderDevice* device_base,
                                      SDL_Surface* initial_data,
                                      const std::string& name,
                                      TextureAgent* agent) {
  if (initial_data->format != SDL_PIXELFORMAT_ABGR8888) {
    SDL_Surface* conv =
        SDL_ConvertSurface(initial_data, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(initial_data);
    initial_data = conv;
  }

  agent->name = "bitmap.texture<\"" + name + "\">";

  renderer::CreateTexture2D(
      **device_base, &agent->data, agent->name, initial_data,
      Diligent::USAGE_DEFAULT,
      Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE);

  agent->size = base::Vec2i(initial_data->w, initial_data->h);
  agent->view =
      agent->data->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);

  MakeTextureWorldInternal(device_base, agent->size, &agent->world_buffer);

  Diligent::BufferViewDesc buffer_view_desc;
  buffer_view_desc.Name = "bitmap.world.buffer";
  buffer_view_desc.ViewType = Diligent::BUFFER_VIEW_SHADER_RESOURCE;
  agent->world_buffer->CreateView(buffer_view_desc, &agent->world_binding);

  SDL_DestroySurface(initial_data);
}

void GPUCreateTextureWithSizeInternal(renderer::RenderDevice* device_base,
                                      const base::Vec2i& initial_size,
                                      TextureAgent* agent) {
  agent->name = "bitmap.texture<\"" + std::to_string(initial_size.x) + "x" +
                std::to_string(initial_size.y) + "\">";

  renderer::CreateTexture2D(
      **device_base, &agent->data, agent->name, initial_size,
      Diligent::USAGE_DEFAULT,
      Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE);

  agent->size = initial_size;
  agent->view =
      agent->data->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);

  MakeTextureWorldInternal(device_base, agent->size, &agent->world_buffer);

  Diligent::BufferViewDesc buffer_view_desc;
  buffer_view_desc.Name = "bitmap.world.buffer";
  buffer_view_desc.ViewType = Diligent::BUFFER_VIEW_SHADER_RESOURCE;
  agent->world_buffer->CreateView(buffer_view_desc, &agent->world_binding);
}

void GPUBlendBlitTextureInternal(CanvasScheduler* scheduler,
                                 TextureAgent* dst_texture,
                                 const base::Rect& dst_region,
                                 TextureAgent* src_texture,
                                 const base::Rect& src_region,
                                 float blit_alpha) {
  auto* context = scheduler->GetDevice()->GetContext();
  auto& pipeline_set = scheduler->GetDevice()->GetPipelines()->base;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NORMAL);

  base::Vec4 blend_alpha;
  blend_alpha.w = blit_alpha / 255.0f;

  renderer::Quad transient_quad;
  renderer::Quad::SetTexCoordRect(&transient_quad, src_region,
                                  src_texture->size);
  renderer::Quad::SetPositionRect(&transient_quad, dst_region);
  renderer::Quad::SetColor(&transient_quad, blend_alpha);
  scheduler->quad_batch()->QueueWrite(context, &transient_quad);

  // Setup render target
  Diligent::ITextureView* render_target_view = dst_texture->view;
  context->SetRenderTargets(
      1, &render_target_view, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Set scissor region
  Diligent::Rect render_scissor;
  render_scissor.right = dst_texture->size.x;
  render_scissor.bottom = dst_texture->size.y;
  context->SetScissorRects(1, &render_scissor, 1,
                           render_scissor.bottom + render_scissor.top);

  // Setup uniform params
  scheduler->base_binding()->u_transform->Set(dst_texture->world_binding);
  scheduler->base_binding()->u_texture->Set(src_texture->view);

  // Apply pipeline state
  context->SetPipelineState(pipeline);
  context->CommitShaderResources(
      **scheduler->base_binding(),
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Apply vertex index
  Diligent::IBuffer* const vertex_buffer = **scheduler->quad_batch();
  context->SetVertexBuffers(
      0, 1, &vertex_buffer, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->SetIndexBuffer(**scheduler->GetDevice()->GetQuadIndex(), 0,
                          Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Execute render command
  Diligent::DrawIndexedAttribs draw_indexed_attribs;
  draw_indexed_attribs.NumIndices = 6;
  draw_indexed_attribs.IndexType =
      scheduler->GetDevice()->GetQuadIndex()->format();
  context->DrawIndexed(draw_indexed_attribs);
}

void GPUDestroyTextureInternal(TextureAgent* agent) {
  delete agent;
}

void GPUFetchTexturePixelsDataInternal(CanvasScheduler* scheduler,
                                       TextureAgent* agent,
                                       SDL_Surface* surface_cache) {
  auto* context = scheduler->GetDevice()->GetContext();
  auto& device = *scheduler->GetDevice();

  // Create temp surface buffer
  Diligent::TextureDesc stage_buffer_desc = agent->data->GetDesc();
  stage_buffer_desc.Name = "bitmap.read.stagebuffer";
  stage_buffer_desc.Usage = Diligent::USAGE_STAGING;
  stage_buffer_desc.BindFlags = Diligent::BIND_NONE;
  stage_buffer_desc.CPUAccessFlags = Diligent::CPU_ACCESS_READ;

  RRefPtr<Diligent::ITexture> stage_read_buffer;
  device->CreateTexture(stage_buffer_desc, nullptr, &stage_read_buffer);

  // Reset render target
  context->SetRenderTargets(
      0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Copy textrue to temp buffer
  Diligent::CopyTextureAttribs copy_texture_attribs(
      agent->data.RawPtr(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
      stage_read_buffer.RawPtr(),
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->CopyTexture(copy_texture_attribs);
  context->WaitForIdle();

  // Read buffer data from mapping
  Diligent::MappedTextureSubresource MappedData;
  context->MapTextureSubresource(stage_read_buffer, 0, 0, Diligent::MAP_READ,
                                 Diligent::MAP_FLAG_DO_NOT_WAIT, nullptr,
                                 MappedData);

  uint8_t* dst_data = static_cast<uint8_t*>(surface_cache->pixels);
  uint8_t* src_data = static_cast<uint8_t*>(MappedData.pData);
  for (size_t i = 0; i < surface_cache->h; ++i) {
    memcpy(dst_data, src_data, surface_cache->pitch);
    dst_data += surface_cache->pitch;
    src_data += MappedData.Stride;
  }

  context->UnmapTextureSubresource(stage_read_buffer, 0, 0);
}

void GPUUpdateTexturePixelsDataInternal(CanvasScheduler* scheduler,
                                        TextureAgent* agent,
                                        std::vector<uint8_t> pixels,
                                        const base::Vec2i& surface_size) {
  auto* context = scheduler->GetDevice()->GetContext();

  Diligent::TextureSubResData texture_sub_res_data(pixels.data(),
                                                   surface_size.x * 4);
  Diligent::Box box(0, surface_size.x, 0, surface_size.y);

  context->UpdateTexture(agent->data, 0, 0, box, texture_sub_res_data,
                         Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                         Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void GPUCanvasFillRectInternal(CanvasScheduler* scheduler,
                               TextureAgent* agent,
                               const base::Rect& region,
                               const base::Vec4& color) {
  auto* context = scheduler->GetDevice()->GetContext();
  auto& pipeline_set = scheduler->GetDevice()->GetPipelines()->color;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NO_BLEND);

  renderer::Quad transient_quad;
  renderer::Quad::SetPositionRect(&transient_quad, region);
  renderer::Quad::SetColor(&transient_quad, color);
  scheduler->quad_batch()->QueueWrite(context, &transient_quad);

  Diligent::ITextureView* render_target_view = agent->view;
  context->SetRenderTargets(
      1, &render_target_view, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Set scissor region
  Diligent::Rect render_scissor;
  render_scissor.right = agent->size.x;
  render_scissor.bottom = agent->size.y;
  context->SetScissorRects(1, &render_scissor, 1,
                           render_scissor.bottom + render_scissor.top);

  // Setup uniform params
  scheduler->color_binding()->u_transform->Set(agent->world_binding);

  // Apply pipeline state
  context->SetPipelineState(pipeline);
  context->CommitShaderResources(
      **scheduler->color_binding(),
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Apply vertex index
  Diligent::IBuffer* const vertex_buffer = **scheduler->quad_batch();
  context->SetVertexBuffers(
      0, 1, &vertex_buffer, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->SetIndexBuffer(**scheduler->GetDevice()->GetQuadIndex(), 0,
                          Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Execute render command
  Diligent::DrawIndexedAttribs draw_indexed_attribs;
  draw_indexed_attribs.NumIndices = 6;
  draw_indexed_attribs.IndexType =
      scheduler->GetDevice()->GetQuadIndex()->format();
  context->DrawIndexed(draw_indexed_attribs);
}

void GPUCanvasGradientFillRectInternal(CanvasScheduler* scheduler,
                                       TextureAgent* agent,
                                       const base::Rect& region,
                                       const base::Vec4& color1,
                                       const base::Vec4& color2,
                                       bool vertical) {
  auto* context = scheduler->GetDevice()->GetContext();
  auto& pipeline_set = scheduler->GetDevice()->GetPipelines()->color;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NO_BLEND);

  renderer::Quad transient_quad;
  renderer::Quad::SetPositionRect(&transient_quad, region);
  if (vertical) {
    transient_quad.vertices[0].color = color1;
    transient_quad.vertices[1].color = color1;
    transient_quad.vertices[2].color = color2;
    transient_quad.vertices[3].color = color2;
  } else {
    transient_quad.vertices[0].color = color1;
    transient_quad.vertices[1].color = color2;
    transient_quad.vertices[2].color = color2;
    transient_quad.vertices[3].color = color1;
  }
  scheduler->quad_batch()->QueueWrite(context, &transient_quad);

  Diligent::ITextureView* render_target_view = agent->view;
  context->SetRenderTargets(
      1, &render_target_view, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Set scissor region
  Diligent::Rect render_scissor;
  render_scissor.right = agent->size.x;
  render_scissor.bottom = agent->size.y;
  context->SetScissorRects(1, &render_scissor, 1,
                           render_scissor.bottom + render_scissor.top);

  // Setup uniform params
  scheduler->color_binding()->u_transform->Set(agent->world_binding);

  // Apply pipeline state
  context->SetPipelineState(pipeline);
  context->CommitShaderResources(
      **scheduler->color_binding(),
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Apply vertex index
  Diligent::IBuffer* const vertex_buffer = **scheduler->quad_batch();
  context->SetVertexBuffers(
      0, 1, &vertex_buffer, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->SetIndexBuffer(**scheduler->GetDevice()->GetQuadIndex(), 0,
                          Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Execute render command
  Diligent::DrawIndexedAttribs draw_indexed_attribs;
  draw_indexed_attribs.NumIndices = 6;
  draw_indexed_attribs.IndexType =
      scheduler->GetDevice()->GetQuadIndex()->format();
  context->DrawIndexed(draw_indexed_attribs);
}

void GPUCanvasDrawTextSurfaceInternal(CanvasScheduler* scheduler,
                                      TextureAgent* agent,
                                      const base::Rect& region,
                                      SDL_Surface* text,
                                      float opacity,
                                      int align) {
  auto* context = scheduler->GetDevice()->GetContext();
  auto& pipeline_set = scheduler->GetDevice()->GetPipelines()->base;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NORMAL);

  if (!agent->text_cache_texture || agent->text_cache_size.x < text->w ||
      agent->text_cache_size.y < text->h) {
    agent->text_cache_size.x =
        std::max<int32_t>(agent->text_cache_size.x, text->w);
    agent->text_cache_size.y =
        std::max<int32_t>(agent->text_cache_size.y, text->h);

    renderer::CreateTexture2D(**scheduler->GetDevice(),
                              &agent->text_cache_texture, "textdraw.cache",
                              agent->text_cache_size);
  }

  // Update text texture cache
  Diligent::TextureSubResData texture_sub_res_data(text->pixels, text->pitch);
  Diligent::Box box(0, text->w, 0, text->h);

  context->UpdateTexture(agent->text_cache_texture, 0, 0, box,
                         texture_sub_res_data,
                         Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                         Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Render to self texture
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

  // Assemble attributes
  float zoom_x = static_cast<float>(region.width) / text->w;
  zoom_x = std::min(zoom_x, 1.0f);
  base::Rect compose_position(align_x, align_y, text->w * zoom_x, text->h);

  renderer::Quad transient_quad;
  renderer::Quad::SetTexCoordRect(&transient_quad, base::Vec2(text->w, text->h),
                                  agent->text_cache_size);
  renderer::Quad::SetPositionRect(&transient_quad, compose_position);
  renderer::Quad::SetColor(&transient_quad, blend_alpha);
  scheduler->quad_batch()->QueueWrite(context, &transient_quad);

  // Setup render target
  Diligent::ITextureView* render_target_view = agent->view;
  context->SetRenderTargets(
      1, &render_target_view, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Set scissor region
  Diligent::Rect render_scissor;
  render_scissor.right = agent->size.x;
  render_scissor.bottom = agent->size.y;
  context->SetScissorRects(1, &render_scissor, 1,
                           render_scissor.bottom + render_scissor.top);

  // Setup uniform params
  scheduler->base_binding()->u_transform->Set(agent->world_binding);
  scheduler->base_binding()->u_texture->Set(
      agent->text_cache_texture->GetDefaultView(
          Diligent::TEXTURE_VIEW_SHADER_RESOURCE));

  // Apply pipeline state
  context->SetPipelineState(pipeline);
  context->CommitShaderResources(
      **scheduler->base_binding(),
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Apply vertex index
  Diligent::IBuffer* const vertex_buffer = **scheduler->quad_batch();
  context->SetVertexBuffers(
      0, 1, &vertex_buffer, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->SetIndexBuffer(**scheduler->GetDevice()->GetQuadIndex(), 0,
                          Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Execute render command
  Diligent::DrawIndexedAttribs draw_indexed_attribs;
  draw_indexed_attribs.NumIndices = 6;
  draw_indexed_attribs.IndexType =
      scheduler->GetDevice()->GetQuadIndex()->format();
  context->DrawIndexed(draw_indexed_attribs);
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

scoped_refptr<Bitmap> Bitmap::Deserialize(ExecutionContext* execution_context,
                                          const std::string&,
                                          ExceptionState& exception_state) {
  return nullptr;
}

std::string Bitmap::Serialize(ExecutionContext* execution_context,
                              scoped_refptr<Bitmap>,
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
  canvas_texture_agent->name =
      std::to_string(size.x) + "x" + std::to_string(size.y);
  canvas_texture_agent->size = size;

  base::ThreadWorker::PostTask(
      scheduler->render_worker(),
      base::BindOnce(&GPUCreateTextureWithSizeInternal, scheduler->GetDevice(),
                     size, canvas_texture_agent));

  return new CanvasImpl(screen, scheduler, canvas_texture_agent,
                        new FontImpl(font_data), canvas_texture_agent->name);
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
  canvas_texture_agent->name = filename;
  canvas_texture_agent->size =
      base::Vec2i(memory_texture->w, memory_texture->h);

  base::ThreadWorker::PostTask(
      scheduler->render_worker(),
      base::BindOnce(&GPUCreateTextureWithDataInternal, scheduler->GetDevice(),
                     memory_texture, filename, canvas_texture_agent));

  return new CanvasImpl(screen, scheduler, canvas_texture_agent,
                        new FontImpl(font_data), canvas_texture_agent->name);
}

CanvasImpl::CanvasImpl(RenderScreenImpl* screen,
                       CanvasScheduler* scheduler,
                       TextureAgent* texture,
                       scoped_refptr<Font> font,
                       const std::string& name)
    : Disposable(screen),
      scheduler_(scheduler),
      texture_(texture),
      canvas_cache_(nullptr),
      name_(name),
      font_(FontImpl::From(font)) {
  scheduler->children_.Append(this);
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
  // Notify children observers
  observers_.Notify();

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
  command->region = base::Rect(x, y, width, height);
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
