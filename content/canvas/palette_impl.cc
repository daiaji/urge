// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/canvas/palette_impl.h"

#include "SDL3_image/SDL_image.h"

#include "components/filesystem/io_service.h"
#include "content/common/color_impl.h"
#include "content/common/rect_impl.h"
#include "content/screen/renderscreen_impl.h"

namespace content {

scoped_refptr<Palette> Palette::New(ExecutionContext* execution_context,
                                    uint32_t width,
                                    uint32_t height,
                                    ExceptionState& exception_state) {
  auto* surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_ABGR8888);
  if (!surface) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      SDL_GetError());
    return nullptr;
  }

  return new PaletteImpl(surface);
}

scoped_refptr<Palette> Palette::New(ExecutionContext* execution_context,
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
  execution_context->io_service->OpenRead(filename, file_handler, &io_state);

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

  return new PaletteImpl(memory_texture);
}

scoped_refptr<Palette> Palette::FromDump(ExecutionContext* execution_context,
                                         const std::string& dump_data,
                                         ExceptionState& exception_state) {
  if (dump_data.size() < sizeof(uint32_t) * 2) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      "Invalid palette header dump data.");
    return nullptr;
  }

  auto* raw_data = reinterpret_cast<const uint32_t*>(dump_data.data());
  uint32_t width = *(raw_data + 0), height = *(raw_data + 1);

  if (dump_data.size() < sizeof(uint32_t) * 2 + width * height * 4) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      "Invalid palette body dump data.");
    return nullptr;
  }

  auto* surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_ABGR8888);
  if (!surface) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      SDL_GetError());
    return nullptr;
  }

  std::memcpy(surface->pixels, raw_data + 2, surface->pitch * surface->h);

  return new PaletteImpl(surface);
}

scoped_refptr<Palette> Palette::Copy(ExecutionContext* execution_context,
                                     scoped_refptr<Palette> other,
                                     ExceptionState& exception_state) {
  auto* src_surf = PaletteImpl::From(other)->GetRawSurface();
  if (!src_surf) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      "Invalid copy palette target.");
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

  return new PaletteImpl(dst_surf);
}

scoped_refptr<Palette> Palette::Deserialize(ExecutionContext* execution_context,
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

  return new PaletteImpl(surface);
}

std::string Palette::Serialize(ExecutionContext* execution_context,
                               scoped_refptr<Palette> value,
                               ExceptionState& exception_state) {
  scoped_refptr<PaletteImpl> palette = PaletteImpl::From(value);
  SDL_Surface* surface = palette->GetRawSurface();

  std::string serialized_data(
      sizeof(uint32_t) * 2 + surface->pitch * surface->h, 0);

  uint32_t surface_width = surface->w, surface_height = surface->h;
  std::memcpy(serialized_data.data() + sizeof(uint32_t) * 0, &surface_width,
              sizeof(uint32_t));
  std::memcpy(serialized_data.data() + sizeof(uint32_t) * 1, &surface_height,
              sizeof(uint32_t));
  std::memcpy(serialized_data.data() + sizeof(uint32_t) * 2, surface->pixels,
              surface->pitch * surface->h);

  return serialized_data;
}

PaletteImpl::PaletteImpl(SDL_Surface* surface)
    : Disposable(nullptr), surface_(surface) {}

PaletteImpl::~PaletteImpl() {
  ExceptionState exception_state;
  Dispose(exception_state);
}

scoped_refptr<PaletteImpl> PaletteImpl::From(scoped_refptr<Palette> host) {
  return static_cast<PaletteImpl*>(host.get());
}

void PaletteImpl::Dispose(ExceptionState& exception_state) {
  Disposable::Dispose(exception_state);
}

bool PaletteImpl::IsDisposed(ExceptionState& exception_state) {
  return Disposable::IsDisposed(exception_state);
}

uint32_t PaletteImpl::Width(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return surface_->w;
}

uint32_t PaletteImpl::Height(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return surface_->h;
}

scoped_refptr<Rect> PaletteImpl::GetRect(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return nullptr;

  return new RectImpl(base::Rect(0, 0, surface_->w, surface_->h));
}

void PaletteImpl::Blt(int32_t x,
                      int32_t y,
                      scoped_refptr<Palette> src_palette,
                      scoped_refptr<Rect> src_rect,
                      uint32_t opacity,
                      ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto* src_surface = PaletteImpl::From(src_palette)->GetRawSurface();
  if (!src_surface) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      "Invalid blt target.");
    return;
  }

  auto src_region = RectImpl::From(src_rect)->AsSDLRect();
  SDL_Rect dest_pos = {x, y, 0, 0};

  SDL_BlitSurface(src_surface, &src_region, surface_, &dest_pos);
}

void PaletteImpl::StretchBlt(scoped_refptr<Rect> dest_rect,
                             scoped_refptr<Palette> src_palette,
                             scoped_refptr<Rect> src_rect,
                             uint32_t opacity,
                             ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  auto* src_surface = PaletteImpl::From(src_palette)->GetRawSurface();
  if (!src_surface) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      "Invalid blt target.");
    return;
  }

  auto src_region = RectImpl::From(src_rect)->AsSDLRect();
  auto dest_region = RectImpl::From(dest_rect)->AsSDLRect();

  SDL_BlitSurfaceScaled(src_surface, &src_region, surface_, &dest_region,
                        SDL_SCALEMODE_LINEAR);
}

void PaletteImpl::FillRect(int32_t x,
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

void PaletteImpl::FillRect(scoped_refptr<Rect> rect,
                           scoped_refptr<Color> color,
                           ExceptionState& exception_state) {
  FillRect(rect->Get_X(exception_state), rect->Get_Y(exception_state),
           rect->Get_Width(exception_state), rect->Get_Height(exception_state),
           color, exception_state);
}

void PaletteImpl::Clear(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  SDL_ClearSurface(surface_, 0, 0, 0, 0);
}

void PaletteImpl::ClearRect(int32_t x,
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

void PaletteImpl::ClearRect(scoped_refptr<Rect> rect,
                            ExceptionState& exception_state) {
  ClearRect(rect->Get_X(exception_state), rect->Get_Y(exception_state),
            rect->Get_Width(exception_state), rect->Get_Height(exception_state),
            exception_state);
}

scoped_refptr<Color> PaletteImpl::GetPixel(int32_t x,
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

void PaletteImpl::SetPixel(int32_t x,
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

std::string PaletteImpl::DumpData(ExceptionState& exception_state) {
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

void PaletteImpl::SavePNG(const std::string& filename,
                          ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  IMG_SavePNG(surface_, filename.c_str());
}

void PaletteImpl::OnObjectDisposed() {
  SDL_DestroySurface(surface_);
  surface_ = nullptr;
}

}  // namespace content
