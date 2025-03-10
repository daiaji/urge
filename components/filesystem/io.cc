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

struct OpenReadEnumData {
  IO::OpenCallback callback;
  std::string full;
  std::string dir;
  std::string file;
  std::string ext;

  bool search_complete = false;
  int match_count = 0;

  std::string errormsg;

  OpenReadEnumData() = default;
};

SDL_EnumerationResult SDLCALL OpenFileEnumCallback(void* userdata,
                                                   const char* dirname,
                                                   const char* fname) {
  OpenReadEnumData* enum_data = static_cast<OpenReadEnumData*>(userdata);
  std::string filename(fname);
  ToLower(filename);

  if (enum_data->search_complete)
    return SDL_ENUM_SUCCESS;

  size_t it = filename.rfind(enum_data->file);
  if (it == std::string::npos)
    return SDL_ENUM_CONTINUE;

  const char last = filename[enum_data->file.size()];
  if (last != '.' && last != '/')
    return SDL_ENUM_CONTINUE;

  std::string fullpath;
  if (*dirname) {
    fullpath += std::string(dirname);
    fullpath += "/";
  }
  fullpath += fname;

  SDL_IOStream* ops = SDL_IOFromFile(fullpath.c_str(), "r");
  if (!ops) {
    enum_data->search_complete = true;
    enum_data->errormsg = SDL_GetError();

    return SDL_ENUM_FAILURE;
  }

  // Free on user callback side
  if (enum_data->callback.Run(ops, FindFileExtName(filename.c_str())))
    enum_data->search_complete = true;

  enum_data->match_count++;
  return SDL_ENUM_CONTINUE;
}

SDL_EnumerationResult SDLCALL EnumDirectoryCallback(void* userdata,
                                                    const char* dirname,
                                                    const char* fname) {
  auto* out_files = static_cast<std::vector<std::string>*>(userdata);
  out_files->push_back(fname);

  return SDL_ENUM_CONTINUE;
}

}  // namespace

IO::IO() = default;

IO::~IO() = default;

std::unique_ptr<IO> IO::Create() {
  return std::unique_ptr<IO>(new IO());
}

SDL_IOStream* IO::OpenFile(const std::string& filename,
                           ExceptionFrame* exception_state) {
  SDL_IOStream* ops = SDL_IOFromFile(filename.c_str(), "r");
  if (!ops && exception_state) {
    exception_state->error_count++;
    exception_state->error_msg = "Failed to load file: " + filename;
    return nullptr;
  }

  return ops;
}

void IO::OpenRead(const std::string& file_path,
                  OpenCallback callback,
                  ExceptionFrame* exception_state) {
  std::string filename(file_path);
  std::string dir, file, ext;

  size_t last_slash_pos = filename.find_last_of('/');
  if (last_slash_pos != std::string::npos) {
    dir = filename.substr(0, last_slash_pos);
    file = filename.substr(last_slash_pos + 1);
  } else
    file = filename;

  size_t last_dot_pos = file.find_last_of('.');
  if (last_dot_pos != std::string::npos) {
    ext = file.substr(last_dot_pos + 1);
    file = file.substr(0, last_dot_pos);
  }

  OpenReadEnumData data;
  data.callback = callback;
  data.full = filename;
  data.dir = dir;
  data.file = file;
  ToLower(data.file);
  data.ext = ext;

  if (!SDL_EnumerateDirectory(dir.c_str(), OpenFileEnumCallback, &data))
    LOG(INFO) << "[Filesystem] " << SDL_GetError();

  if (!data.errormsg.empty()) {
    exception_state->error_count++;
    exception_state->error_msg = data.errormsg;
    return;
  }

  if (data.match_count <= 0) {
    exception_state->error_count++;
    exception_state->error_msg = "No file match: " + file_path;
  }
}

bool IO::IsFileExisted(const std::string& filename) {
  SDL_PathInfo info;
  return SDL_GetPathInfo(filename.c_str(), &info) &&
         info.type == SDL_PATHTYPE_FILE;
}

bool IO::EnumDirectory(const std::string& path,
                       std::vector<std::string>& out_files) {
  out_files.clear();
  return SDL_EnumerateDirectory(path.c_str(), EnumDirectoryCallback,
                                &out_files);
}

}  // namespace filesystem
