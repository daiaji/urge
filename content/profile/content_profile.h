// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PROFILE_CONTENT_PROFILE_H_
#define CONTENT_PROFILE_CONTENT_PROFILE_H_

#include <memory>
#include <string>
#include <vector>

#include "SDL3/SDL_iostream.h"

#include "base/math/vector.h"

namespace content {

class ContentProfile {
 public:
  enum class APIVersion {
    UNKNOWN = 0,
    RGSS1,
    RGSS2,
    RGSS3,
  };

  ~ContentProfile();

  ContentProfile(const ContentProfile&) = delete;
  ContentProfile& operator=(const ContentProfile&) = delete;

  static std::unique_ptr<ContentProfile> MakeFrom(const std::string& app,
                                                  SDL_IOStream* stream);

  void LoadCommandLine(int32_t argc, char** argv);
  bool LoadConfigure(const std::string& app);

  std::vector<std::string> args;
  std::string program_name;

  std::string window_title = "Engine Widget";
  std::string script_path = "Data/Scripts.rxdata";

  bool game_debug = false;
  bool game_battle_test = false;

  APIVersion api_version = APIVersion::UNKNOWN;
  std::string default_font_path = "Fonts/Default.ttf";
  std::string driver_backend;
  std::string i18n_xml_path;

  bool disable_settings = false;
  bool disable_fps_monitor = false;
  bool disable_reset = false;

  base::Vec2i window_size;
  base::Vec2i resolution;

  int32_t frame_rate = 60;

  bool smooth_scale = false;
  bool allow_skip_frame = true;
  bool fullscreen = false;

 private:
  ContentProfile(const std::string& app, SDL_IOStream* stream);

  SDL_IOStream* ini_stream_;
};

}  // namespace content

#endif  //! CONTENT_PROFILE_CONTENT_PROFILE_H_
