// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/components/font_context.h"

namespace content {

namespace {

std::pair<int64_t, void*> ReadFontToMemory(SDL_IOStream* io) {
  int64_t font_size = SDL_GetIOSize(io);
  void* font_ptr = SDL_malloc(font_size);
  if (font_ptr)
    SDL_ReadIO(io, font_ptr, font_size);

  return std::make_pair(font_size, font_ptr);
}

}  // namespace

ScopedFontData::ScopedFontData(filesystem::IO* io,
                               const std::string& default_font_name)
    : default_color(new ColorImpl(base::Vec4(255.0f, 255.0f, 255.0f, 255.0f))),
      default_out_color(new ColorImpl(base::Vec4(0, 0, 0, 255.0f))) {
  // Get font load dir and default font
  std::string filename(default_font_name);
  std::string dir("."), file;

  size_t last_slash_pos = filename.find_last_of('/');
  if (last_slash_pos != std::string::npos) {
    dir = filename.substr(0, last_slash_pos);
    file = filename.substr(last_slash_pos + 1);
  } else
    file = filename;

  dir.push_back('/');
  default_name.push_back(file);
  default_font = file;

  LOG(INFO) << "[Font] Search Path: " << dir;
  LOG(INFO) << "[Font] Default Font: " << file;

  // Load all font to memory as cache
  std::vector<std::string> font_files;
  io->EnumDirectory(dir, font_files);
  for (auto& it : font_files) {
    std::string filepath = dir + it;
    SDL_IOStream* font_stream = io->OpenFile(filepath, nullptr);
    if (font_stream) {
      // Cached in memory
      data_cache.emplace(it, ReadFontToMemory(font_stream));

      // Close i/o stream
      SDL_CloseIO(font_stream);

      LOG(INFO) << "[Font] Loaded Font: " << it;
    }
  }
}

ScopedFontData::~ScopedFontData() {
  for (auto& it : data_cache)
    SDL_free(it.second.second);
}

bool ScopedFontData::IsFontExisted(const std::string& name) {
  return data_cache.find(name) != data_cache.end();
}

}  // namespace content
