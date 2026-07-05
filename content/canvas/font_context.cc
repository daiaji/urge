// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/canvas/font_context.h"

#include "content/resource/embed.ttf.bin"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H

#include <vector>
#include <string>

namespace {

// Minimal UTF-16BE to UTF-8 conversion for SFNT name records
std::string Utf16BeToUtf8(const char* data, size_t len) {
  std::string out;
  for (size_t i = 0; i + 1 < len; i += 2) {
    uint32_t cp = ((unsigned char)data[i] << 8) | (unsigned char)data[i + 1];
    if (cp >= 0xD800 && cp <= 0xDBFF && i + 3 < len) {
      uint32_t hi = cp - 0xD800;
      uint32_t lo = ((unsigned char)data[i + 2] << 8) | (unsigned char)data[i + 3];
      if (lo >= 0xDC00 && lo <= 0xDFFF) {
        cp = 0x10000 + (hi << 10) + (lo - 0xDC00);
        i += 2;
      }
    }
    if (cp < 0x80)       out += (char)cp;
    else if (cp < 0x800) { out += 0xC0 | (cp >> 6); out += 0x80 | (cp & 0x3F); }
    else if (cp < 0x10000) { out += 0xE0 | (cp >> 12); out += 0x80 | ((cp >> 6) & 0x3F); out += 0x80 | (cp & 0x3F); }
    else                 { out += 0xF0 | (cp >> 18); out += 0x80 | ((cp >> 12) & 0x3F); out += 0x80 | ((cp >> 6) & 0x3F); out += 0x80 | (cp & 0x3F); }
  }
  return out;
}

// UTF-32BE to UTF-8 conversion for SFNT UCS-4 name records
std::string Utf32BeToUtf8(const char* data, size_t len) {
  std::string out;
  for (size_t i = 0; i + 3 < len; i += 4) {
    uint32_t cp = ((unsigned char)data[i] << 24) | ((unsigned char)data[i + 1] << 16) |
                  ((unsigned char)data[i + 2] << 8) | (unsigned char)data[i + 3];
    if (cp > 0x10FFFF)
      continue;
    if (cp < 0x80)       out += (char)cp;
    else if (cp < 0x800) { out += 0xC0 | (cp >> 6); out += 0x80 | (cp & 0x3F); }
    else if (cp < 0x10000) { out += 0xE0 | (cp >> 12); out += 0x80 | ((cp >> 6) & 0x3F); out += 0x80 | (cp & 0x3F); }
    else                 { out += 0xF0 | (cp >> 18); out += 0x80 | ((cp >> 12) & 0x3F); out += 0x80 | ((cp >> 6) & 0x3F); out += 0x80 | (cp & 0x3F); }
  }
  return out;
}

std::pair<int64_t, void*> ReadFontToMemory(SDL_IOStream* io) {
  int64_t font_size = SDL_GetIOSize(io);
  void* font_ptr = SDL_malloc(font_size);
  SDL_ReadIO(io, font_ptr, font_size);
  return std::make_pair(font_size, font_ptr);
}

}  // namespace

namespace content {

ScopedFontData::ScopedFontData(filesystem::IOService* io,
                               const std::string& default_font_name)
    : default_color(base::MakeRefCounted<ColorImpl>(
          base::Vec4(255.0f, 255.0f, 255.0f, 255.0f))),
      default_out_color(
          base::MakeRefCounted<ColorImpl>(base::Vec4(0, 0, 0, 255.0f))),
      default_gradient_color(
          base::MakeRefCounted<ColorImpl>(base::Vec4(0.0f, 0.0f, 0.0f, 0.0f))),
      default_shadow_color(
          base::MakeRefCounted<ColorImpl>(base::Vec4(0.0f, 0.0f, 0.0f, 128.0f))) {
  // Get font load dir and default font
  std::string filename = default_font_name;
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
  LOG(INFO) << "[Font] Default Font: \"" << file << "\" (from path: \"" << filename << "\")";

  // Load all font to memory as cache
  std::vector<std::string> font_files = io->EnumDir(dir);
  if (font_files.empty())
    font_files.push_back(default_font);

  for (auto& it : font_files) {
    std::string filepath = dir + it;
    SDL_IOStream* font_stream = io->OpenReadRaw(filepath, nullptr);
    if (font_stream) {
      // Cached in memory
      auto& entry = data_cache.emplace(it, ReadFontToMemory(font_stream)).first->second;

      // Close i/o stream
      SDL_CloseIO(font_stream);

      // Read all SFNT family names using FreeType for robust name→filename lookup
      FT_Library ft_lib;
      if (FT_Init_FreeType(&ft_lib) == 0) {
        FT_Face ft_face;
        if (FT_New_Memory_Face(ft_lib, (FT_Byte*)entry.second, entry.first, 0, &ft_face) == 0) {
          if (FT_IS_SFNT(ft_face)) {
            for (unsigned int i = 0, n = FT_Get_Sfnt_Name_Count(ft_face); i < n; ++i) {
              FT_SfntName sfnt;
              if (FT_Get_Sfnt_Name(ft_face, i, &sfnt))
                continue;
              if (sfnt.name_id != TT_NAME_ID_FONT_FAMILY)
                continue;
              std::string family;
              if ((sfnt.platform_id == TT_PLATFORM_MICROSOFT &&
                   sfnt.encoding_id == TT_MS_ID_UNICODE_CS) ||
                  sfnt.platform_id == TT_PLATFORM_APPLE_UNICODE)
                family = Utf16BeToUtf8((const char*)sfnt.string, sfnt.string_len);
              else if (sfnt.platform_id == TT_PLATFORM_MICROSOFT &&
                       sfnt.encoding_id == TT_MS_ID_UCS_4)
                family = Utf32BeToUtf8((const char*)sfnt.string, sfnt.string_len);
              else
                family = std::string((const char*)sfnt.string, sfnt.string_len);
              if (family.empty())
                continue;
              std::string lower(family);
              std::transform(lower.begin(), lower.end(), lower.begin(),
                             [](unsigned char c) { return std::tolower(c); });
              family_name_cache.emplace(lower, it);
              LOG(INFO) << "[Font] Family name: \"" << family << "\" → " << it;
            }
          }
          FT_Done_Face(ft_face);
        }
        FT_Done_FreeType(ft_lib);
      }

      LOG(INFO) << "[Font] Loaded Font: " << it;
    }
  }

  // Load internal font if default missing
  // Try exact match first, then case-insensitive fallback (for Windows)
  auto default_font_it = data_cache.find(default_font);
  if (default_font_it == data_cache.end()) {
    // Case-insensitive fallback (Windows filesystem may differ from INI case)
    if (!default_font.empty()) {
      std::string lower_default(default_font);
      std::transform(lower_default.begin(), lower_default.end(), lower_default.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      for (auto it = data_cache.begin(); it != data_cache.end(); ++it) {
        std::string lower_key(it->first);
        std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (lower_key == lower_default) {
          default_font_it = it;
          default_font = it->first;
          break;
        }
      }
    }
  }

  if (default_font_it == data_cache.end()) {
    LOG(INFO) << "[Font] Default font missing, use internal font for instead.";

    void* internal_duplicate_data = SDL_malloc(embed_ttf_len);
    std::memcpy(internal_duplicate_data, embed_ttf, embed_ttf_len);

    std::pair<int64_t, void*> cache_pair =
        std::make_pair(embed_ttf_len, internal_duplicate_data);
    data_cache.emplace(default_font, std::move(cache_pair));
  }
}

ScopedFontData::~ScopedFontData() {
  for (auto& it : font_cache)
    TTF_CloseFont(it.second);
  for (auto& it : data_cache)
    SDL_free(it.second.second);
}

bool ScopedFontData::IsFontExisted(const std::string& name) {
  if (data_cache.find(name) != data_cache.end())
    return true;
  std::string lower(name);
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return family_name_cache.find(lower) != family_name_cache.end();
}

const void* ScopedFontData::GetUIDefaultFont(int64_t* font_size) {
  auto it = data_cache.find(default_font);
  if (it != data_cache.end()) {
    *font_size = it->second.first;
    return it->second.second;
  }

  *font_size = embed_ttf_len;
  return embed_ttf;
}

}  // namespace content
