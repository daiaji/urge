// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/profile/content_profile.h"

#include "inih/INIReader.h"

#include "base/debug/logging.h"

namespace content {

namespace {

void ReplaceStringWidth(std::string& str, char before, char after) {
  for (size_t i = 0; i < str.size(); ++i)
    if (str[i] == before)
      str[i] = after;
}

char* IniStreamReader(char* str, int num, void* stream) {
  SDL_IOStream* io = static_cast<SDL_IOStream*>(stream);

  memset(str, 0, num);
  char c;
  int i = 0;

  while (i < num - 1 && SDL_ReadIO(io, &c, 1)) {
    str[i++] = c;
    if (c == '\n')
      break;
  }

  str[i] = '\0';
  return i ? str : nullptr;
}

}  // namespace

ContentProfile::ContentProfile(SDL_IOStream* stream) : ini_stream_(stream) {}

ContentProfile::~ContentProfile() = default;

std::unique_ptr<ContentProfile> ContentProfile::MakeFrom(SDL_IOStream* stream) {
  return std::unique_ptr<ContentProfile>(new ContentProfile(stream));
}

void ContentProfile::LoadCommandLine(int argc, char** argv) {
  for (int i = 0; i < argc; ++i)
    args.push_back(argv[i]);

  for (int i = 0; i < argc; i++) {
    if (std::string(argv[i]) == "test" || std::string(argv[i]) == "debug")
      game_debug = true;
    if (std::string(argv[i]) == "btest")
      game_battle_test = true;
  }

  if (game_debug)
    LOG(INFO) << "[App] Running debug test.";
  if (game_battle_test)
    LOG(INFO) << "[App] Running battle test.";
}

bool ContentProfile::LoadConfigure(const std::string& app) {
  std::unique_ptr<INIReader> reader(new INIReader);
  if (ini_stream_) {
    reader = std::make_unique<INIReader>(ini_stream_, IniStreamReader);
    if (reader->ParseError())
      return false;
  } else {
    LOG(INFO) << "[App] Warning: No configure file found.";
  }

  // RGSS part
  window_title = reader->Get("Game", "Title", window_title);
  script_path = reader->Get("Game", "Scripts", script_path);
  ReplaceStringWidth(script_path, '\\', '/');

  // Engine part
  default_font_path =
      reader->Get("Engine", "DefaultFontPath", default_font_path);
  ReplaceStringWidth(default_font_path, '\\', '/');
  api_version = static_cast<ContentProfile::APIVersion>(reader->GetInteger(
      "Engine", "APIVersion", static_cast<int32_t>(api_version)));
  wgpu_backend = reader->Get("Engine", "WGPUBackend", wgpu_backend);

  if (ini_stream_)
    SDL_CloseIO(ini_stream_);

  return true;
}

}  // namespace content
