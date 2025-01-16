// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PROFILE_CONTENT_PROFILE_H_
#define CONTENT_PROFILE_CONTENT_PROFILE_H_

#include "SDL3/SDL_iostream.h"

#include <memory>
#include <string>

namespace content {

class ContentProfile {
 public:
  enum class APIVersion {
    kRGSS1 = 0,
    kRGSS2,
    kRGSS3,
  };

  ~ContentProfile();

  ContentProfile(const ContentProfile&) = delete;
  ContentProfile& operator=(const ContentProfile&) = delete;

  static std::unique_ptr<ContentProfile> MakeFrom(SDL_IOStream* stream);

  std::string window_title;
  std::string script_path;
  APIVersion api_version;

 private:
  ContentProfile() = default;
};

}  // namespace content

#endif  //! CONTENT_PROFILE_CONTENT_PROFILE_H_
