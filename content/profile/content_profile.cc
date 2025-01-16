// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/profile/content_profile.h"

namespace content {

ContentProfile::~ContentProfile() = default;

std::unique_ptr<ContentProfile> ContentProfile::MakeFrom(SDL_IOStream* stream) {
  auto* profile = new ContentProfile;
  return std::unique_ptr<ContentProfile>(profile);
}

}  // namespace content
