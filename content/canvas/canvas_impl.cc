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
  binding_desc.layout = device_base->GetPipelines()
                            ->base.GetPipeline(renderer::BlendType())
                            ->GetBindGroupLayout(1);

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
  agent->binding = MakeTextureBindingGroupInternal(device_base, agent->size,
                                                   agent->view, agent->sampler);
}

void GPUBlendBlitTextureInternal(CanvasScheduler* scheduler,
                                 TextureAgent* dst_texture,
                                 const base::Rect& dst_region,
                                 TextureAgent* src_texture,
                                 const base::Rect& src_region,
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

  auto* command_encoder = scheduler->GetDrawContext()->GetImmediateEncoder();
  auto renderpass_encoder = command_encoder->BeginRenderPass(&renderpass);
  renderpass_encoder.SetViewport(0, 0, dst_texture->size.x, dst_texture->size.y,
                                 0, 0);
  renderpass_encoder.SetPipeline(*pipeline);
  renderpass_encoder.SetVertexBuffer(0, **scheduler->vertex_buffer());
  renderpass_encoder.SetIndexBuffer(**scheduler->index_cache(),
                                    wgpu::IndexFormat::Uint16);
  renderpass_encoder.SetBindGroup(0, dst_texture->binding);
  renderpass_encoder.Draw(6);
  renderpass_encoder.End();
}

void GPUDestroyTextureInternal(TextureAgent* agent,
                               base::SingleWorker* worker) {
  agent->data = nullptr;
  TextureAgent::Free(agent);
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
  // Submit pending commands
  auto* encoder = scheduler_->GetDrawContext()->GetImmediateEncoder();
  SubmitQueuedCommands(*encoder);

  // Synchronize immediate context and flush
  scheduler_->GetDrawContext()->Flush();

  // Fetch buffer to texture

  return nullptr;
}

void CanvasImpl::UpdateVideoMemory() {}

void CanvasImpl::SubmitQueuedCommands(const wgpu::CommandEncoder& encoder) {
  Command* command_sequence = nullptr;
  // Execute pending commands,
  // encode draw command in wgpu encoder.
  while (command_sequence) {
    switch (command_sequence->id) {
      case CommandID::kFillRect:
        break;
      case CommandID::kGradientFillRect:
        break;
      case CommandID::kHueChange:
        break;
      case CommandID::kBlur:
        break;
      case CommandID::kRadialBlur:
        break;
      case CommandID::kDrawText:
        break;
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
    PostCanvasTask(scheduler_,
                   base::BindOnce(&GPUDestroyTextureInternal, texture_));
    texture_ = nullptr;
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
                      src_canvas, RectImpl::From(src_rect)->AsBaseRect());
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
                      RectImpl::From(src_rect)->AsBaseRect());
}

void CanvasImpl::FillRect(int32_t x,
                          int32_t y,
                          uint32_t width,
                          uint32_t height,
                          scoped_refptr<Color> color,
                          ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto* command = AllocateCommand<Command_FillRect>();
  command->region = base::Rect(x, y, width, height);
  command->color = ColorImpl::From(color.get())->AsNormColor();
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

  auto* command = AllocateCommand<Command_FillRect>();
  command->region =
      base::Rect(0, 0, Width(exception_state), Height(exception_state));
  command->color = base::Vec4(0);
}

void CanvasImpl::ClearRect(int32_t x,
                           int32_t y,
                           uint32_t width,
                           uint32_t height,
                           ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto* command = AllocateCommand<Command_FillRect>();
  command->region =
      base::Rect(0, 0, Width(exception_state), Height(exception_state));
  command->color = base::Vec4(0);
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

  auto* command = AllocateCommand<Command_FillRect>();
  command->region = base::Rect(x, y, 1, 1);
  command->color = ColorImpl::From(color.get())->AsNormColor();
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
  command->text = text_surface;
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
                                     const base::Rect& src_rect) {
  auto* device_context = scheduler_->GetDrawContext();

  // Synchronize pending queue immediately,
  // blit the sourcetexture to destination texture immediately.
  src_texture->SubmitQueuedCommands(*device_context->GetImmediateEncoder());
  SubmitQueuedCommands(*device_context->GetImmediateEncoder());

  // Execute blit immediately.
  PostCanvasTask(scheduler_, base::BindOnce(&GPUBlendBlitTextureInternal,
                                            scheduler_, texture_, dst_rect,
                                            src_texture->texture_, src_rect));
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
