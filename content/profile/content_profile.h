// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PROFILE_CONTENT_PROFILE_H_
#define CONTENT_PROFILE_CONTENT_PROFILE_H_

#include "SDL3/SDL_iostream.h"

#include <memory>
#include <string>
#include <vector>

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

  static std::unique_ptr<ContentProfile> MakeFrom(SDL_IOStream* stream);

  void LoadCommandLine(int argc, char** argv);
  bool LoadConfigure(const std::string& app);

  std::vector<std::string> args;
  std::string program_name;

  std::string window_title = "Engine Widget";
  std::string script_path = "Data/Scripts.rxdata";

  bool game_debug = false;
  bool game_battle_test = false;

  APIVersion api_version = APIVersion::UNKNOWN;
  std::string default_font_path = "Fonts/Default.ttf";
  std::string wgpu_backend = "";

 private:
  ContentProfile(SDL_IOStream* stream);

  SDL_IOStream* ini_stream_;
};

}  // namespace content

#endif  //! CONTENT_PROFILE_CONTENT_PROFILE_H_
