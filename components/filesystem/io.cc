// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/filesystem/io.h"

#include "SDL3/SDL_filesystem.h"

namespace filesystem {

namespace {

// Windows is case-insensitive, Unix-like is case-sensitive.
void ToLower(std::string& str) {
  for (size_t i = 0; i < str.size(); ++i)
    str[i] = tolower(str[i]);
}

const char* FindFileExtName(const char* filename) {
  for (size_t i = strlen(filename); i > 0; --i) {
    if (filename[i] == '/')
      break;
    if (filename[i] == '.')
      return filename + i + 1;
  }

  return nullptr;
}

SDL_EnumerationResult SDLCALL EnumDirectoryCallback(void* userdata,
                                                    const char* dirname,
                                                    const char* fname) {
  auto* out_files = static_cast<std::vector<std::string>*>(userdata);
  out_files->push_back(fname);

  return SDL_ENUM_CONTINUE;
}

}  // namespace

IO::IO() {}

IO::~IO() {}

std::unique_ptr<IO> IO::Create() {
  return std::unique_ptr<IO>(new IO());
}

SDL_IOStream* IO::OpenFile(const std::string& filename, bool raw) {
  return nullptr;
}

bool IO::IsFileExisted(const std::string& filename) {
  return false;
}

bool IO::EnumDirectory(const std::string& path,
                       std::vector<std::string>& out_files) {
  out_files.clear();

  return SDL_EnumerateDirectory(path.c_str(), EnumDirectoryCallback,
                                &out_files);
}

}  // namespace filesystem
