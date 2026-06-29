// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CANVAS_FONT_CONTEXT_H_
#define CONTENT_CANVAS_FONT_CONTEXT_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "SDL3/SDL_surface.h"
#include "SDL3_ttf/SDL_ttf.h"

#include "components/filesystem/io_service.h"
#include "content/common/color_impl.h"
#include "content/content_config.h"

namespace content {

constexpr float kDefaultFontScale = 0.9f;
constexpr bool kDefaultFontKerning = true;
constexpr TTF_HintingFlags kDefaultFontHinting = TTF_HINTING_NONE;
constexpr bool kDefaultFontOutlineCrop = true;

// Global font families forced to solid (non-anti-aliased) outline rendering.
// Matches mkxp-z solidFonts config. Default empty — RGSS never presets any.
const std::set<std::string> kSolidFontFamilies = {};

struct ScopedFontData {
  std::string default_font;
  std::vector<std::string> default_name;
  int32_t default_size = 24;
  bool default_bold = false;
  bool default_italic = false;
  bool default_outline = true;
  bool default_shadow = false;
  bool default_solid = false;
  scoped_refptr<ColorImpl> default_color = nullptr;
  scoped_refptr<ColorImpl> default_out_color = nullptr;
  scoped_refptr<ColorImpl> default_gradient_color = nullptr;

  // Font rendering preferences (INI overridable, RGSS defaults)
  float font_scale = kDefaultFontScale;
  bool font_kerning = kDefaultFontKerning;
  TTF_HintingFlags font_hinting = kDefaultFontHinting;
  bool font_outline_crop = kDefaultFontOutlineCrop;

  // Font substitution map: from_name → to_name (from INI FontSubs)
  std::map<std::string, std::string> font_subs;

  // Font family name → filename cache (populated at startup)
  // Maps lowercase family names like "simhei" → "SimHei.ttf"
  std::map<std::string, std::string> family_name_cache;

  std::map<std::pair<std::string, int32_t>, TTF_Font*> font_cache;
  std::map<std::string, std::pair<int64_t, void*>> data_cache;
  TTF_Font* internal_font = nullptr;

  ScopedFontData(filesystem::IOService* io,
                 const std::string& default_font_name);
  ~ScopedFontData();

  ScopedFontData(const ScopedFontData&) = delete;
  ScopedFontData& operator=(const ScopedFontData&) = delete;

  bool IsFontExisted(const std::string& name);

  // Fetch GUI font from cache
  const void* GetUIDefaultFont(int64_t* font_size);
};

}  // namespace content

#endif  //! CONTENT_CANVAS_FONT_CONTEXT_H_
