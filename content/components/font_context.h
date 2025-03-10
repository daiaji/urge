// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMPONENTS_FONT_CONTEXT_H_
#define CONTENT_COMPONENTS_FONT_CONTEXT_H_

#include <map>
#include <string>
#include <vector>

#include "SDL3/SDL_surface.h"
#include "SDL3_ttf/SDL_ttf.h"

#include "components/filesystem/io.h"
#include "content/common/color_impl.h"
#include "content/content_config.h"

namespace content {

struct ScopedFontData {
  std::string default_font;
  std::vector<std::string> default_name;
  int default_size = 24;
  bool default_bold = false;
  bool default_italic = false;
  bool default_outline = true;
  bool default_shadow = false;
  scoped_refptr<ColorImpl> default_color = nullptr;
  scoped_refptr<ColorImpl> default_out_color = nullptr;

  std::map<std::pair<std::string, int>, TTF_Font*> font_cache;
  std::map<std::string, std::pair<int64_t, void*>> data_cache;

  ScopedFontData(filesystem::IO* io, const std::string& default_font_name);
  ~ScopedFontData();

  ScopedFontData(const ScopedFontData&) = delete;
  ScopedFontData& operator=(const ScopedFontData&) = delete;

  bool IsFontExisted(const std::string& name);
};

}  // namespace content

#endif  //! CONTENT_COMPONENTS_FONT_CONTEXT_H_
