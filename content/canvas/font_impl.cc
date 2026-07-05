// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/canvas/font_impl.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

#include "content/context/execution_context.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TAGS_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_IDS_H

namespace content {

constexpr int32_t kOutlineSize = 1;

namespace {

// Byte-order helpers for big-endian VDMX fields
inline uint16_t GetBEWord(const uint8_t* p) {
  return (static_cast<uint16_t>(p[0]) << 8) | p[1];
}

inline int16_t GetBEShort(const uint8_t* p) {
  return static_cast<int16_t>(GetBEWord(p));
}

// Try to load ppem from the VDMX table (matches Windows GDI behavior).
// Returns 0 if no VDMX table or no matching entry.
int LoadVDMX(FT_Face ft_face, int height) {
  FT_ULong length = 0;
  FT_Error err = FT_Load_Sfnt_Table(ft_face, TTAG_VDMX, 0, nullptr, &length);
  if (err || length == 0)
    return 0;

  std::vector<uint8_t> buf(length);
  err = FT_Load_Sfnt_Table(ft_face, TTAG_VDMX, 0, buf.data(), &length);
  if (err)
    return 0;

  // VDMX_Header: version(2) + numRecs(2) + numRatios(2) = 6 bytes
  if (length < 6)
    return 0;

  const uint8_t* data = buf.data();
  uint16_t numRatios = GetBEWord(data + 4);

  // Each Ratio is 4 bytes: bCharSet(1) + xRatio(1) + yStartRatio(1) + yEndRatio(1)
  size_t ratio_table_offset = 6;
  size_t ratio_table_size = numRatios * 4;
  if (length < ratio_table_offset + ratio_table_size + numRatios * 2)
    return 0;

  // Find the matching ratio (device aspect ratio 1:1)
  int found_idx = -1;
  for (int i = 0; i < numRatios; ++i) {
    const uint8_t* r = data + ratio_table_offset + i * 4;
    uint8_t bCharSet = r[0];
    uint8_t xRatio = r[1];
    uint8_t yStartRatio = r[2];
    uint8_t yEndRatio = r[3];

    if (!bCharSet)
      continue;

    if ((xRatio == 0 && yStartRatio == 0 && yEndRatio == 0) ||
        (xRatio == 1 && yStartRatio <= 1 && yEndRatio >= 1)) {
      found_idx = i;
      break;
    }
  }

  if (found_idx < 0)
    return 0;

  // Read the group offset for this ratio
  size_t offset_table_offset = ratio_table_offset + ratio_table_size;
  uint16_t group_offset = GetBEWord(data + offset_table_offset + found_idx * 2);

  // VDMX_group: recs(2) + startsz(1) + endsz(1) = 4 bytes
  size_t group_start = group_offset;
  if (group_start + 4 > length)
    return 0;

  const uint8_t* group_data = data + group_start;
  uint16_t recs = GetBEWord(group_data);

  // Each VDMX_vTable entry: yPelHeight(2) + yMax(2) + yMin(2) = 6 bytes
  size_t vtable_start = group_start + 4;
  if (vtable_start + recs * 6 > length)
    return 0;

  int ppem = 0;
  for (int i = 0; i < recs; ++i) {
    const uint8_t* entry = data + vtable_start + i * 6;
    uint16_t yPelHeight = GetBEWord(entry);
    int16_t yMax = GetBEShort(entry + 2);
    int16_t yMin = GetBEShort(entry + 4);
    int font_height = yMax + (-yMin);

    if (font_height == height) {
      ppem = yPelHeight;
      break;
    }
    if (font_height > height) {
      if (i > 0) {
        const uint8_t* prev = data + vtable_start + (i - 1) * 6;
        ppem = GetBEWord(prev);
      }
      break;
    }
  }

  return ppem;
}

// Calculate ppem from font metrics (matches Windows GDI / Wine).
// ppem = units_per_EM * height / (winAscent + winDescent)
int CalcPPEMForHeight(FT_Face ft_face, int height) {
  if (height == 0)
    height = 16;

  if (height < 0)
    return -height;

  TT_OS2* os2 = reinterpret_cast<TT_OS2*>(
      FT_Get_Sfnt_Table(ft_face, FT_SFNT_OS2));
  TT_HoriHeader* hori = reinterpret_cast<TT_HoriHeader*>(
      FT_Get_Sfnt_Table(ft_face, FT_SFNT_HHEA));

  int units;
  if (os2 && (os2->usWinAscent + os2->usWinDescent) != 0) {
    units = os2->usWinAscent + abs(static_cast<short>(os2->usWinDescent));
  } else if (hori) {
    units = hori->Ascender - hori->Descender;
  } else {
    return height;
  }

  if (units <= 0)
    return height;

  int ppem = static_cast<int>(
      FT_MulDiv(ft_face->units_per_EM, height, units));

  // If rounding overshoots, reduce ppem
  if (ppem > 1 && FT_MulDiv(units, ppem, ft_face->units_per_EM) > height)
    --ppem;

  return std::max(ppem, 1);
}

// Calculate ppem for a font, with VDMX -> formula fallback.
// Applies font_scale after ppem calculation (not before).
int CalculateFontPPEM(const void* font_data, int64_t font_size, int size, float font_scale) {
  static FT_Library cached_ft_lib = nullptr;
  if (!cached_ft_lib)
    if (FT_Init_FreeType(&cached_ft_lib) != 0)
      return std::max(static_cast<int>(size * font_scale), 1);

  FT_Face ft_face;
  FT_Library ft_lib = cached_ft_lib;
  if (FT_New_Memory_Face(ft_lib, static_cast<const FT_Byte*>(font_data),
                         static_cast<FT_Long>(font_size), 0, &ft_face) != 0) {
    FT_Done_FreeType(ft_lib);
    return std::max(static_cast<int>(size * font_scale), 1);
  }

  int ppem = 0;
  if (FT_IS_SCALABLE(ft_face)) {
    ppem = LoadVDMX(ft_face, size);
    if (!ppem)
      ppem = CalcPPEMForHeight(ft_face, size);
  } else {
    ppem = size;
  }

  FT_Done_Face(ft_face);

  ppem = std::max(static_cast<int>(ppem * font_scale), 1);
  const int MAX_PPEM = (1 << 16) - 1;
  if (ppem > MAX_PPEM)
    ppem = MAX_PPEM;
  return ppem;
}

// Blend text surface onto outline surface with proper alpha compositing.
// Matches RGSS behavior: prevents overlapping glyph edges from becoming
// overly opaque, which would make outlines appear too heavy.
void BlendTextSurface(SDL_Surface* txt_srf, const SDL_Rect& in_rect,
                      const SDL_Color& txt_color,
                      SDL_Surface* out_srf, const SDL_Rect& out_rect,
                      const SDL_Color& out_color) {
  if (in_rect.w <= 0 || in_rect.h <= 0)
    return;

  auto* txt_fmt = SDL_GetPixelFormatDetails(txt_srf->format);
  auto* out_fmt = SDL_GetPixelFormatDetails(out_srf->format);

  uint8_t* txt_start = static_cast<uint8_t*>(txt_srf->pixels) +
                        in_rect.x * txt_fmt->bytes_per_pixel +
                        in_rect.y * txt_srf->pitch;
  uint8_t* out_start = static_cast<uint8_t*>(out_srf->pixels) +
                        out_rect.x * out_fmt->bytes_per_pixel +
                        out_rect.y * out_srf->pitch;

  const float out_r = out_color.r;
  const float out_g = out_color.g;
  const float out_b = out_color.b;

  const uint32_t full_out_pixel = SDL_MapRGBA(out_fmt, nullptr,
      out_color.r, out_color.g, out_color.b, out_color.a);

  for (int i = 0; i < in_rect.h; ++i) {
    uint32_t* txt_pixel = reinterpret_cast<uint32_t*>(txt_start + i * txt_srf->pitch);
    uint32_t* out_pixel = reinterpret_cast<uint32_t*>(out_start + i * out_srf->pitch);
    for (int j = 0; j < in_rect.w; ++j) {
      const uint8_t txt_a = (*txt_pixel >> txt_fmt->Ashift) & 0xFF;
      const uint8_t out_a = (*out_pixel >> out_fmt->Ashift) & 0xFF;

      if (txt_a >= txt_color.a) {
        // Text pixel is fully opaque — use it directly
        *out_pixel = *txt_pixel;
      } else if (out_a == 0) {
        // Outline pixel is transparent — use text pixel
        *out_pixel = *txt_pixel;
      } else if (txt_a != 0) {
        // Both have partial alpha — blend using RGSS formula
        const int32_t co1 = static_cast<int32_t>(txt_a) * txt_color.a;
        const int32_t co2 = static_cast<int32_t>(
            std::min(out_a, out_color.a)) * (txt_color.a - txt_a);

        const int32_t fa = co1 + co2;

        const float fa_inv = 1.0f / fa;
        const float co3 = co1 * fa_inv;
        const float co4 = co2 * fa_inv;

        const float txt_r_px = (*txt_pixel >> txt_fmt->Rshift) & 0xFF;
        const float txt_g_px = (*txt_pixel >> txt_fmt->Gshift) & 0xFF;
        const float txt_b_px = (*txt_pixel >> txt_fmt->Bshift) & 0xFF;

        const uint8_t r = std::min<int>((txt_r_px * co3 + out_r * co4) + 0.001f, 255);
        const uint8_t g = std::min<int>((txt_g_px * co3 + out_g * co4) + 0.001f, 255);
        const uint8_t b = std::min<int>((txt_b_px * co3 + out_b * co4) + 0.001f, 255);
        const uint8_t a = fa / txt_color.a;

        *out_pixel = SDL_MapRGBA(out_fmt, nullptr, r, g, b, a);
      } else if (out_a > out_color.a) {
        // Text pixel is transparent but outline is over-opaque (SDL_ttf artifact)
        *out_pixel = full_out_pixel;
      }

      ++txt_pixel;
      ++out_pixel;
    }
  }
}

void RenderShadowSurface(SDL_Surface*& in, const SDL_Color& text_color) {
  if (in->w < 4 || in->h < 4)
    return;

  auto* dest = SDL_CreateSurface(in->w, in->h, in->format);

  SDL_Rect dest_rect{1, 1, 0, 0};
  SDL_SetSurfaceBlendMode(dest, SDL_BLENDMODE_NONE);
  SDL_BlitSurface(in, nullptr, dest, &dest_rect);

  auto* fmt = SDL_GetPixelFormatDetails(dest->format);
  auto* pixels = static_cast<uint32_t*>(dest->pixels);
  auto pitch = dest->pitch / 4;
  for (int32_t y = 0; y < dest->h; ++y)
    for (int32_t x = 0; x < dest->w; ++x) {
      uint8_t r, g, b, a;
      SDL_GetRGBA(pixels[x + y * pitch], fmt, nullptr, &r, &g, &b, &a);
      r = a * text_color.r / 255;
      g = a * text_color.g / 255;
      b = a * text_color.b / 255;
      pixels[x + y * pitch] = SDL_MapRGBA(fmt, nullptr, r, g, b, a);
    }

  SDL_SetSurfaceBlendMode(dest, SDL_BLENDMODE_BLEND);
  SDL_BlitSurface(in, nullptr, dest, nullptr);

  SDL_DestroySurface(in);
  in = dest;
}

TTF_Font* ReadFontFromMemory(
    const std::map<std::string, std::pair<int64_t, void*>>& mem_fonts,
    const std::string& path,
    int32_t size,
    float font_scale,
    int32_t* out_ppem = nullptr) {
  auto it = mem_fonts.find(path);
  if (it == mem_fonts.end())
    return nullptr;

  // Calculate ppem using FreeType
  int ppem = CalculateFontPPEM(it->second.second, it->second.first,
                                size, font_scale);

  // Open font with SDL_ttf at calculated ppem
  SDL_IOStream* io = SDL_IOFromConstMem(it->second.second, it->second.first);
  TTF_Font* font = TTF_OpenFontIO(io, true, ppem);

  if (out_ppem)
    *out_ppem = ppem;
  return font;
}

}  // namespace

// static
scoped_refptr<Font> Font::New(ExecutionContext* execution_context,
                              const std::string& name,
                              uint32_t size,
                              ExceptionState& exception_state) {
  return base::MakeRefCounted<FontImpl>(name, size,
                                        execution_context->font_context);
}

// static
scoped_refptr<Font> Font::Copy(ExecutionContext* execution_context,
                               scoped_refptr<Font> other,
                               ExceptionState& exception_state) {
  return base::MakeRefCounted<FontImpl>(*static_cast<FontImpl*>(other.get()));
}

// static
bool Font::IsExisted(ExecutionContext* execution_context,
                     const std::string& name,
                     ExceptionState& exception_state) {
  return execution_context->font_context->IsFontExisted(name);
}

URGE_DEFINE_STATIC_ATTRIBUTE(
    Font,
    DefaultName,
    std::vector<std::string>,
    { return execution_context->font_context->default_name; },
    { execution_context->font_context->default_name = value; });

URGE_DEFINE_STATIC_ATTRIBUTE(
    Font,
    DefaultSize,
    uint32_t,
    { return execution_context->font_context->default_size; },
    {
      execution_context->font_context->default_size =
          std::max<int32_t>(6, value);
    });

URGE_DEFINE_STATIC_ATTRIBUTE(
    Font,
    DefaultBold,
    bool,
    { return execution_context->font_context->default_bold; },
    { execution_context->font_context->default_bold = value; });

URGE_DEFINE_STATIC_ATTRIBUTE(
    Font,
    DefaultItalic,
    bool,
    { return execution_context->font_context->default_italic; },
    { execution_context->font_context->default_italic = value; });

URGE_DEFINE_STATIC_ATTRIBUTE(
    Font,
    DefaultShadow,
    bool,
    { return execution_context->font_context->default_shadow; },
    { execution_context->font_context->default_shadow = value; });

URGE_DEFINE_STATIC_ATTRIBUTE(
    Font,
    DefaultOutline,
    bool,
    { return execution_context->font_context->default_outline; },
    { execution_context->font_context->default_outline = value; });

URGE_DEFINE_STATIC_ATTRIBUTE(
    Font,
    DefaultSolid,
    bool,
    { return execution_context->font_context->default_solid; },
    { execution_context->font_context->default_solid = value; });

URGE_DEFINE_STATIC_ATTRIBUTE(
    Font,
    DefaultColor,
    scoped_refptr<Color>,
    { return execution_context->font_context->default_color; },
    {
      CHECK_ATTRIBUTE_VALUE;
      *execution_context->font_context->default_color = *ColorImpl::From(value);
    });

URGE_DEFINE_STATIC_ATTRIBUTE(
    Font,
    DefaultOutColor,
    scoped_refptr<Color>,
    { return execution_context->font_context->default_out_color; },
    {
      CHECK_ATTRIBUTE_VALUE;
      *execution_context->font_context->default_out_color =
          *ColorImpl::From(value);
    });

URGE_DEFINE_STATIC_ATTRIBUTE(
    Font,
    DefaultGradientColor,
    scoped_refptr<Color>,
    { return execution_context->font_context->default_gradient_color; },
    {
      CHECK_ATTRIBUTE_VALUE;
      *execution_context->font_context->default_gradient_color =
          *ColorImpl::From(value);
    });

URGE_DEFINE_STATIC_ATTRIBUTE(
    Font,
    DefaultShadowColor,
    scoped_refptr<Color>,
    { return execution_context->font_context->default_shadow_color; },
    {
      CHECK_ATTRIBUTE_VALUE;
      *execution_context->font_context->default_shadow_color =
          *ColorImpl::From(value);
    });

///////////////////////////////////////////////////////////////////////////////
// Font Implement

FontImpl::FontImpl(ScopedFontData* parent)
    : name_(parent->default_name),
      size_(parent->default_size),
      bold_(parent->default_bold),
      italic_(parent->default_italic),
      outline_(parent->default_outline),
      shadow_(parent->default_shadow),
      solid_(parent->default_solid),
      color_(base::MakeRefCounted<ColorImpl>(base::Vec4())),
      out_color_(base::MakeRefCounted<ColorImpl>(base::Vec4())),
      gradient_color_(base::MakeRefCounted<ColorImpl>(base::Vec4())),
      shadow_color_(base::MakeRefCounted<ColorImpl>(base::Vec4())),
      parent_(parent),
      font_(nullptr) {
  *color_ = *parent->default_color;
  *out_color_ = *parent->default_out_color;
  *gradient_color_ = *parent->default_gradient_color;
  *shadow_color_ = *parent->default_shadow_color;
}

FontImpl::FontImpl(const std::string& name,
                   uint32_t size,
                   ScopedFontData* parent)
    : name_({name}),
      size_(size),
      bold_(parent->default_bold),
      italic_(parent->default_italic),
      outline_(parent->default_outline),
      shadow_(parent->default_shadow),
      solid_(parent->default_solid),
      color_(base::MakeRefCounted<ColorImpl>(base::Vec4())),
      out_color_(base::MakeRefCounted<ColorImpl>(base::Vec4())),
      gradient_color_(base::MakeRefCounted<ColorImpl>(base::Vec4())),
      shadow_color_(base::MakeRefCounted<ColorImpl>(base::Vec4())),
      parent_(parent),
      font_(nullptr) {
  *color_ = *parent->default_color;
  *out_color_ = *parent->default_out_color;
  *gradient_color_ = *parent->default_gradient_color;
  *shadow_color_ = *parent->default_shadow_color;
}

FontImpl::FontImpl(const FontImpl& other)
    : name_(other.name_),
      size_(other.size_),
      bold_(other.bold_),
      italic_(other.italic_),
      outline_(other.outline_),
      shadow_(other.shadow_),
      solid_(other.solid_),
      color_(base::MakeRefCounted<ColorImpl>(base::Vec4())),
      out_color_(base::MakeRefCounted<ColorImpl>(base::Vec4())),
      gradient_color_(base::MakeRefCounted<ColorImpl>(base::Vec4())),
      shadow_color_(base::MakeRefCounted<ColorImpl>(base::Vec4())),
      parent_(other.parent_),
      font_(nullptr) {
  *color_ = *other.color_;
  *out_color_ = *other.out_color_;
  *gradient_color_ = *other.gradient_color_;
  *shadow_color_ = *other.shadow_color_;
}

FontImpl& FontImpl::operator=(const FontImpl& other) {
  name_ = other.name_;
  size_ = other.size_;
  bold_ = other.bold_;
  italic_ = other.italic_;
  outline_ = other.outline_;
  shadow_ = other.shadow_;
  solid_ = other.solid_;
  *color_ = *other.color_;
  *out_color_ = *other.out_color_;
  *gradient_color_ = *other.gradient_color_;
  *shadow_color_ = *other.shadow_color_;
  font_ = nullptr;
  return *this;
}

scoped_refptr<FontImpl> FontImpl::From(scoped_refptr<Font> host) {
  return static_cast<FontImpl*>(host.get());
}

TTF_Font* FontImpl::GetCanonicalFont(ExceptionState& exception_state) {
  if (!font_) {
    LoadFontInternal(exception_state);
    if (exception_state.HadException())
      return nullptr;

    // Setup font rendering fallback
    SetupFontFallbackInternal();
  }

  int32_t font_style = TTF_STYLE_NORMAL;
  if (bold_)
    font_style |= TTF_STYLE_BOLD;
  if (italic_)
    font_style |= TTF_STYLE_ITALIC;
  TTF_SetFontStyle(font_, font_style);

  TTF_SetFontKerning(font_, parent_->font_kerning ? 1 : 0);
  TTF_SetFontHinting(font_, parent_->font_hinting);

  return font_;
}

SDL_Surface* FontImpl::RenderText(const std::string& text,
                                  uint8_t* font_opacity,
                                  ExceptionState& exception_state) {
  TTF_Font* font = GetCanonicalFont(exception_state);
  if (!font)
    return nullptr;

  auto color_impl = ColorImpl::From(color_);
  auto out_color_impl = ColorImpl::From(out_color_);
  auto shadow_color_impl = ColorImpl::From(shadow_color_);

  SDL_Color font_color = color_impl->AsSDLColor();
  SDL_Color outline_color = out_color_impl->AsSDLColor();
  SDL_Color shadow_sdl_color = shadow_color_impl->AsSDLColor();
  if (font_opacity)
    *font_opacity = font_color.a;

  SDL_Surface* text_surface =
      solid_
          ? TTF_RenderText_Solid(font, text.c_str(), text.size(), font_color)
          : TTF_RenderText_Blended(font, text.c_str(), text.size(), font_color);
  if (!text_surface)
    return nullptr;

  // Gradient effect
  EnsureFontSurfaceFormatInternal(text_surface);
  const SDL_Color gradient_top_color = color_->AsSDLColor();
  const SDL_Color gradient_bottom_color = gradient_color_->AsSDLColor();
  if (gradient_bottom_color.a) {
    if (gradient_top_color.r != gradient_bottom_color.r ||
        gradient_top_color.g != gradient_bottom_color.g ||
        gradient_top_color.b != gradient_bottom_color.b) {
      const auto pitch = text_surface->pitch / 4;
      const auto* pixel_detail =
          SDL_GetPixelFormatDetails(text_surface->format);
      auto* pixels = static_cast<uint32_t*>(text_surface->pixels);
      const float gradient_alpha = gradient_bottom_color.a / 255.0f;

      for (int32_t y = 0; y < text_surface->h; ++y) {
        for (int32_t x = 0; x < text_surface->w; ++x) {
          uint8_t r, g, b, a;
          SDL_GetRGBA(pixels[x + y * pitch], pixel_detail, nullptr, &r, &g, &b,
                      &a);

          if (a) {
            const float progress =
                (static_cast<float>(y) / text_surface->h) * gradient_alpha;
            r = gradient_bottom_color.r * progress +
                gradient_top_color.r * (1.0f - progress);
            g = gradient_bottom_color.g * progress +
                gradient_top_color.g * (1.0f - progress);
            b = gradient_bottom_color.b * progress +
                gradient_top_color.b * (1.0f - progress);

            pixels[x + y * pitch] =
                SDL_MapRGBA(pixel_detail, nullptr, r, g, b, a);
          }
        }
      }
    }
  }

  if (outline_) {
    // Outline shape render
    SDL_Surface* outline_surface = nullptr;
    TTF_SetFontOutline(font, kOutlineSize);

    // Per-instance solid attribute or global font family preset
    bool use_solid = solid_ || kSolidFontFamilies.count(name_.front()) > 0;
    outline_surface = use_solid
                          ? TTF_RenderText_Solid(font, text.c_str(),
                                                 text.size(), outline_color)
                          : TTF_RenderText_Blended(font, text.c_str(),
                                                   text.size(), outline_color);
    TTF_SetFontOutline(font, 0);

    if (!outline_surface) {
      SDL_DestroySurface(text_surface);
      return nullptr;
    }

    EnsureFontSurfaceFormatInternal(outline_surface);

    // Blend text onto outline with proper alpha compositing,
    // preventing overlapping glyph edges from becoming overly opaque.
    int crop = parent_->font_outline_crop ? kOutlineSize : 0;
    int double_outline = kOutlineSize * 2;
    SDL_Rect in_rect = {kOutlineSize - crop, kOutlineSize - crop,
                        text_surface->w - kOutlineSize,
                        text_surface->h - kOutlineSize};
    SDL_Rect out_rect = {double_outline - crop, double_outline - crop, 0, 0};
    BlendTextSurface(text_surface, in_rect, font_color,
                     outline_surface, out_rect, outline_color);

    SDL_DestroySurface(text_surface);
    text_surface = outline_surface;
  }

  EnsureFontSurfaceFormatInternal(text_surface);
  if (shadow_)
    RenderShadowSurface(text_surface, shadow_sdl_color);

  return text_surface;
}

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    Name,
    std::vector<std::string>,
    FontImpl,
    { return name_; },
    {
      name_ = value;
      font_ = nullptr;
    });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    Size,
    uint32_t,
    FontImpl,
    { return size_; },
    {
      size_ = std::max<int32_t>(6, value);
      font_ = nullptr;
    });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    Bold,
    bool,
    FontImpl,
    { return bold_; },
    { bold_ = value; });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    Italic,
    bool,
    FontImpl,
    { return italic_; },
    { italic_ = value; });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    Outline,
    bool,
    FontImpl,
    { return outline_; },
    { outline_ = value; });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    Shadow,
    bool,
    FontImpl,
    { return shadow_; },
    { shadow_ = value; });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    Solid,
    bool,
    FontImpl,
    { return solid_; },
    { solid_ = value; });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    Color,
    scoped_refptr<Color>,
    FontImpl,
    { return color_; },
    {
      CHECK_ATTRIBUTE_VALUE;
      *color_ = *ColorImpl::From(value);
    });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    OutColor,
    scoped_refptr<Color>,
    FontImpl,
    { return out_color_; },
    {
      CHECK_ATTRIBUTE_VALUE;
      *out_color_ = *ColorImpl::From(value);
    });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    GradientColor,
    scoped_refptr<Color>,
    FontImpl,
    { return gradient_color_; },
    {
      CHECK_ATTRIBUTE_VALUE;
      *gradient_color_ = *ColorImpl::From(value);
    });

URGE_DEFINE_OVERRIDE_ATTRIBUTE(
    ShadowColor,
    scoped_refptr<Color>,
    FontImpl,
    { return shadow_color_; },
    {
      CHECK_ATTRIBUTE_VALUE;
      *shadow_color_ = *ColorImpl::From(value);
    });

void FontImpl::LoadFontInternal(ExceptionState& exception_state) {
  std::vector<std::string> load_names(name_);
  load_names.push_back(parent_->default_font);

  // Apply font substitution from INI FontSubs config
  auto& subs = parent_->font_subs;
  for (auto& name : load_names) {
    std::string lower(name);
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    auto sub_it = subs.find(lower);
    if (sub_it != subs.end()) {
      name = sub_it->second;
    }
  }

  // Resolve names to filenames: no extension → family name only
  auto& name_cache = parent_->family_name_cache;
  for (auto& name : load_names) {
    if (name.empty())
      continue;
    // Already a filename with extension
    if (name.find('.') != std::string::npos)
      continue;
    // Treat as family name → look up in family_name_cache
    std::string lower(name);
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    auto cached = name_cache.find(lower);
    if (cached != name_cache.end())
      name = cached->second;
  }

  auto& font_cache = parent_->font_cache;
  if (size_ >= 6 && size_ <= 96) {
    for (const auto& font_name : load_names) {
      // Find existed font object
      auto it = font_cache.find(std::make_pair(font_name, size_));
      if (it != font_cache.end()) {
        font_ = it->second;
        return;
      }

      // Load new font object with ppem calculation
      int32_t ppem = 0;
      TTF_Font* font_obj =
          ReadFontFromMemory(parent_->data_cache, font_name, size_,
                             parent_->font_scale, &ppem);
      if (font_obj) {
        font_ = font_obj;
        font_cache.emplace(std::make_pair(font_name, size_), font_);
        parent_->size_to_ppem.emplace(
            std::make_pair(font_name, size_), ppem);
        return;
      }
    }
  }

  // Fallback: try internal embedded font
  int64_t emb_size = 0;
  const void* emb_data = parent_->GetUIDefaultFont(&emb_size);
  if (emb_data && emb_size > 0) {
    int32_t emb_ppem = CalculateFontPPEM(emb_data, emb_size, size_,
                                          parent_->font_scale);
    SDL_IOStream* io = SDL_IOFromConstMem(emb_data, emb_size);
    TTF_Font* font_obj = TTF_OpenFontIO(io, true, emb_ppem);
    if (font_obj) {
      font_ = font_obj;
      font_cache.emplace(std::make_pair(parent_->default_font, size_), font_);
      parent_->size_to_ppem.emplace(
          std::make_pair(parent_->default_font, size_), emb_ppem);
      return;
    }
  }

  std::string font_names;
  for (auto& it : name_)
    font_names = font_names + it + " ";

  // Throw font not find error
  exception_state.ThrowError(ExceptionCode::CONTENT_ERROR,
                             "failed to load font: %s", font_names.c_str());
}

void FontImpl::EnsureFontSurfaceFormatInternal(SDL_Surface*& surf) {
  SDL_Surface* format_surf = nullptr;
  if (surf->format != SDL_PIXELFORMAT_ABGR8888) {
    format_surf = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(surf);
    surf = format_surf;
  }
}

void FontImpl::SetupFontFallbackInternal() {
  TTF_ClearFallbackFonts(font_);
  auto& font_cache = parent_->font_cache;
  auto it = font_cache.find(std::make_pair(parent_->default_font, size_));
  if (it != font_cache.end()) {
    if (it->second == font_)
      return;

    // Add fallback option
    TTF_AddFallbackFont(font_, it->second);
  } else {
    // Create new size font object with ppem
    int32_t ppem = 0;
    TTF_Font* font_obj =
        ReadFontFromMemory(parent_->data_cache, parent_->default_font, size_,
                           parent_->font_scale, &ppem);
    if (font_obj) {
      TTF_AddFallbackFont(font_, font_obj);
      font_cache.emplace(std::make_pair(parent_->default_font, size_),
                         font_obj);
      parent_->size_to_ppem.emplace(
          std::make_pair(parent_->default_font, size_), ppem);
    }
  }
}

}  // namespace content
