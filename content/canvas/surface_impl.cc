// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/canvas/surface_impl.h"

#include "SDL3_image/SDL_image.h"

#include "components/filesystem/io_service.h"
#include "content/common/color_impl.h"
#include "content/common/rect_impl.h"
#include "content/screen/renderscreen_impl.h"

namespace content {

scoped_refptr<Surface> Surface::New(ExecutionContext* execution_context,
                                    uint32_t width,
                                    uint32_t height,
                                    ExceptionState& exception_state) {
  auto* surface_data =
      SDL_CreateSurface(width, height, SDL_PIXELFORMAT_ABGR8888);
  if (!surface_data) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      SDL_GetError());
    return nullptr;
  }

  return new SurfaceImpl(surface_data);
}

scoped_refptr<Surface> Surface::New(ExecutionContext* execution_context,
                                    const std::string& filename,
                                    ExceptionState& exception_state) {
  SDL_Surface* memory_surface = nullptr;
  auto file_handler = base::BindRepeating(
      [](SDL_Surface** surf, SDL_IOStream* ops, const std::string& ext) {
        *surf = IMG_LoadTyped_IO(ops, true, ext.c_str());
        return !!*surf;
      },
      &memory_surface);

  filesystem::IOState io_state;
  execution_context->io_service->OpenRead(filename, file_handler, &io_state);

  if (io_state.error_count) {
    exception_state.ThrowContentError(
        ExceptionCode::IO_ERROR,
        "Failed to read file: " + filename + " - " + io_state.error_message);
    return nullptr;
  }

  if (!memory_surface) {
    exception_state.ThrowContentError(
        ExceptionCode::CONTENT_ERROR,
        "Failed to load image: " + filename + " - " + SDL_GetError());
    return nullptr;
  }

  return new SurfaceImpl(memory_surface);
}

scoped_refptr<Surface> Surface::FromDump(ExecutionContext* execution_context,
                                         const std::string& dump_data,
                                         ExceptionState& exception_state) {
  if (dump_data.size() < sizeof(uint32_t) * 2) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      "Invalid surface header dump data.");
    return nullptr;
  }

  auto* raw_data = reinterpret_cast<const uint32_t*>(dump_data.data());
  uint32_t width = *(raw_data + 0), height = *(raw_data + 1);

  if (dump_data.size() < sizeof(uint32_t) * 2 + width * height * 4) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      "Invalid surface body dump data.");
    return nullptr;
  }

  auto* surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_ABGR8888);
  if (!surface) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      SDL_GetError());
    return nullptr;
  }

  std::memcpy(surface->pixels, raw_data + 2, surface->pitch * surface->h);

  return new SurfaceImpl(surface);
}

scoped_refptr<Surface> Surface::Copy(ExecutionContext* execution_context,
                                     scoped_refptr<Surface> other,
                                     ExceptionState& exception_state) {
  auto* src_surf = SurfaceImpl::From(other)->GetRawSurface();
  if (!src_surf) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      "Invalid copy surface target.");
    return nullptr;
  }

  auto* dst_surf =
      SDL_CreateSurfaceFrom(src_surf->w, src_surf->h, src_surf->format,
                            src_surf->pixels, src_surf->pitch);
  if (!dst_surf) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      SDL_GetError());
    return nullptr;
  }

  return new SurfaceImpl(dst_surf);
}

scoped_refptr<Surface> Surface::Deserialize(ExecutionContext* execution_context,
                                            const std::string& data,
                                            ExceptionState& exception_state) {
  const uint8_t* raw_data = reinterpret_cast<const uint8_t*>(data.data());

  if (data.size() < sizeof(uint32_t) * 2) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      "Invalid bitmap header data.");
    return nullptr;
  }

  uint32_t surface_width = 0, surface_height = 0;
  std::memcpy(&surface_width, raw_data + sizeof(uint32_t) * 0,
              sizeof(uint32_t));
  std::memcpy(&surface_height, raw_data + sizeof(uint32_t) * 1,
              sizeof(uint32_t));

  if (data.size() < sizeof(uint32_t) * 2 + surface_width * surface_height * 4) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      "Invalid bitmap dump data.");
    return nullptr;
  }

  SDL_Surface* surface = SDL_CreateSurface(surface_width, surface_height,
                                           SDL_PIXELFORMAT_ABGR8888);
  std::memcpy(surface->pixels, raw_data + sizeof(uint32_t) * 2,
              surface->pitch * surface->h);

  return new SurfaceImpl(surface);
}

std::string Surface::Serialize(ExecutionContext* execution_context,
                               scoped_refptr<Surface> value,
                               ExceptionState& exception_state) {
  scoped_refptr<SurfaceImpl> surface = SurfaceImpl::From(value);
  SDL_Surface* raw_surface = surface->GetRawSurface();

  std::string serialized_data(
      sizeof(uint32_t) * 2 + raw_surface->pitch * raw_surface->h, 0);

  uint32_t surface_width = raw_surface->w, surface_height = raw_surface->h;
  std::memcpy(serialized_data.data() + sizeof(uint32_t) * 0, &surface_width,
              sizeof(uint32_t));
  std::memcpy(serialized_data.data() + sizeof(uint32_t) * 1, &surface_height,
              sizeof(uint32_t));
  std::memcpy(serialized_data.data() + sizeof(uint32_t) * 2,
              raw_surface->pixels, raw_surface->pitch * raw_surface->h);

  return serialized_data;
}

SurfaceImpl::SurfaceImpl(SDL_Surface* surface)
    : Disposable(nullptr), surface_(surface) {}

SurfaceImpl::~SurfaceImpl() {
  ExceptionState exception_state;
  Dispose(exception_state);
}

scoped_refptr<SurfaceImpl> SurfaceImpl::From(scoped_refptr<Surface> host) {
  return static_cast<SurfaceImpl*>(host.get());
}

void SurfaceImpl::Dispose(ExceptionState& exception_state) {
  Disposable::Dispose(exception_state);
}

bool SurfaceImpl::IsDisposed(ExceptionState& exception_state) {
  return Disposable::IsDisposed(exception_state);
}

uint32_t SurfaceImpl::Width(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return surface_->w;
}

uint32_t SurfaceImpl::Height(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return surface_->h;
}

scoped_refptr<Rect> SurfaceImpl::GetRect(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  return new RectImpl(base::Rect(0, 0, surface_->w, surface_->h));
}

void SurfaceImpl::Blt(int32_t x,
                      int32_t y,
                      scoped_refptr<Surface> src_surface,
                      scoped_refptr<Rect> src_rect,
                      uint32_t opacity,
                      ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto* src_raw_surface = SurfaceImpl::From(src_surface)->GetRawSurface();
  if (!src_raw_surface) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      "Invalid blt target.");
    return;
  }

  auto src_region = RectImpl::From(src_rect)->AsSDLRect();
  SDL_Rect dest_pos = {x, y, 0, 0};

  SDL_BlitSurface(src_raw_surface, &src_region, surface_, &dest_pos);
}

void SurfaceImpl::StretchBlt(scoped_refptr<Rect> dest_rect,
                             scoped_refptr<Surface> src_surface,
                             scoped_refptr<Rect> src_rect,
                             uint32_t opacity,
                             ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto* src_raw_surface = SurfaceImpl::From(src_surface)->GetRawSurface();
  if (!src_raw_surface) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      "Invalid blt target.");
    return;
  }

  auto src_region = RectImpl::From(src_rect)->AsSDLRect();
  auto dest_region = RectImpl::From(dest_rect)->AsSDLRect();

  SDL_BlitSurfaceScaled(src_raw_surface, &src_region, surface_, &dest_region,
                        SDL_SCALEMODE_LINEAR);
}

void SurfaceImpl::FillRect(int32_t x,
                           int32_t y,
                           uint32_t width,
                           uint32_t height,
                           scoped_refptr<Color> color,
                           ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto sdl_color = ColorImpl::From(color)->AsSDLColor();
  auto color32 =
      SDL_MapRGBA(SDL_GetPixelFormatDetails(surface_->format), nullptr,
                  sdl_color.r, sdl_color.g, sdl_color.b, sdl_color.a);

  SDL_Rect rect{x, y, static_cast<int32_t>(width),
                static_cast<int32_t>(height)};
  SDL_FillSurfaceRect(surface_, &rect, color32);
}

void SurfaceImpl::FillRect(scoped_refptr<Rect> rect,
                           scoped_refptr<Color> color,
                           ExceptionState& exception_state) {
  FillRect(rect->Get_X(exception_state), rect->Get_Y(exception_state),
           rect->Get_Width(exception_state), rect->Get_Height(exception_state),
           color, exception_state);
}

void SurfaceImpl::Clear(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  SDL_ClearSurface(surface_, 0, 0, 0, 0);
}

void SurfaceImpl::ClearRect(int32_t x,
                            int32_t y,
                            uint32_t width,
                            uint32_t height,
                            ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  SDL_Rect rect{x, y, static_cast<int32_t>(width),
                static_cast<int32_t>(height)};
  SDL_FillSurfaceRect(surface_, &rect, 0);
}

void SurfaceImpl::ClearRect(scoped_refptr<Rect> rect,
                            ExceptionState& exception_state) {
  ClearRect(rect->Get_X(exception_state), rect->Get_Y(exception_state),
            rect->Get_Width(exception_state), rect->Get_Height(exception_state),
            exception_state);
}

scoped_refptr<Color> SurfaceImpl::GetPixel(int32_t x,
                                           int32_t y,
                                           ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  auto* pixel_detail = SDL_GetPixelFormatDetails(surface_->format);
  int32_t bpp = pixel_detail->bytes_per_pixel;
  uint8_t* pixel = static_cast<uint8_t*>(surface_->pixels) +
                   static_cast<size_t>(y) * surface_->pitch +
                   static_cast<size_t>(x) * bpp;

  uint8_t color[4];
  SDL_GetRGBA(*reinterpret_cast<uint32_t*>(pixel), pixel_detail, nullptr,
              &color[0], &color[1], &color[2], &color[3]);

  return new ColorImpl(base::Vec4(color[0], color[1], color[2], color[3]));
}

void SurfaceImpl::SetPixel(int32_t x,
                           int32_t y,
                           scoped_refptr<Color> color,
                           ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  scoped_refptr<ColorImpl> color_obj = ColorImpl::From(color.get());
  const SDL_Color color_unorm = color_obj->AsSDLColor();
  auto* pixel_detail = SDL_GetPixelFormatDetails(surface_->format);
  int bpp = pixel_detail->bytes_per_pixel;
  uint8_t* pixel =
      static_cast<uint8_t*>(surface_->pixels) + y * surface_->pitch + x * bpp;
  *reinterpret_cast<uint32_t*>(pixel) =
      SDL_MapRGBA(pixel_detail, nullptr, color_unorm.r, color_unorm.g,
                  color_unorm.b, color_unorm.a);
}

std::string SurfaceImpl::DumpData(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return std::string();

  std::string data;
  size_t data_size = surface_->pitch * surface_->h;
  data.resize(sizeof(uint32_t) * 2 + data_size);

  auto* raw_data = reinterpret_cast<uint32_t*>(data.data());
  *(raw_data + 0) = surface_->w;
  *(raw_data + 1) = surface_->h;
  std::memcpy(data.data(), surface_->pixels, data_size);

  return data;
}

void SurfaceImpl::SavePNG(const std::string& filename,
                          ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  IMG_SavePNG(surface_, filename.c_str());
}

void SurfaceImpl::OnObjectDisposed() {
  SDL_DestroySurface(surface_);
  surface_ = nullptr;
}

}  // namespace content
