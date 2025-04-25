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

base::Vec2i GetVec2iFromString(std::string in,
                               const base::Vec2i& default_value) {
  size_t sep = in.find("|");
  if (sep != std::string::npos) {
    int32_t num1 = std::stoi(in.substr(0, sep));
    int32_t num2 = std::stoi(in.substr(sep + 1));
    return base::Vec2i(num1, num2);
  }

  return default_value;
}

char* IniStreamReader(char* str, int32_t num, void* stream) {
  SDL_IOStream* io = static_cast<SDL_IOStream*>(stream);

  memset(str, 0, num);
  char c;
  int32_t i = 0;

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

void ContentProfile::LoadCommandLine(int32_t argc, char** argv) {
  for (int32_t i = 0; i < argc; ++i)
    args.push_back(argv[i]);

  for (int32_t i = 0; i < argc; i++) {
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
  api_version = static_cast<APIVersion>(reader->GetInteger(
      "Engine", "APIVersion", static_cast<int32_t>(api_version)));
  if (api_version == APIVersion::UNKNOWN) {
    if (!script_path.empty()) {
      api_version = APIVersion::RGSS1;

      const char* p = &script_path[script_path.size()];
      const char* head = &script_path[0];

      while (--p != head)
        if (*p == '.')
          break;

      if (!strcmp(p, ".rvdata"))
        api_version = APIVersion::RGSS2;
      else if (!strcmp(p, ".rvdata2"))
        api_version = APIVersion::RGSS3;
    }
  }

  default_font_path =
      reader->Get("Engine", "DefaultFontPath", default_font_path);
  ReplaceStringWidth(default_font_path, '\\', '/');
  driver_backend = reader->Get("Engine", "GraphicsAPI", driver_backend);
  i18n_xml_path = reader->Get("Engine", "I18nXMLPath", app + ".xml");

  disable_settings =
      reader->GetBoolean("Engine", "DisableSettings", disable_settings);
  disable_fps_monitor =
      reader->GetBoolean("Engine", "DisableFPSMonitor", disable_fps_monitor);
  disable_reset = reader->GetBoolean("Engine", "DisableReset", disable_reset);

  if (api_version == APIVersion::RGSS1)
    resolution = base::Vec2i(640, 480);
  if (api_version >= APIVersion::RGSS2)
    resolution = base::Vec2i(544, 416);

  resolution =
      GetVec2iFromString(reader->Get("Engine", "Resolution", ""), resolution);
  window_size =
      GetVec2iFromString(reader->Get("Engine", "WindowSize", ""), resolution);

  smooth_scale = reader->GetBoolean("Engine", "SmoothScale", smooth_scale);
  allow_skip_frame =
      reader->GetBoolean("Engine", "AllowSkipFrame", allow_skip_frame);
  fullscreen = reader->GetBoolean("Engine", "Fullscreen", fullscreen);

  if (ini_stream_)
    SDL_CloseIO(ini_stream_);

  return true;
}

}  // namespace content
