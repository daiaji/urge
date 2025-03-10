// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FILESYSTEM_IO_H_
#define COMPONENTS_FILESYSTEM_IO_H_

#include <memory>
#include <string>
#include <vector>

#include "SDL3/SDL_iostream.h"

#include "base/bind/callback.h"

namespace filesystem {

class IO {
 public:
  ~IO();

  IO(const IO&) = delete;
  IO& operator=(const IO&) = delete;

  struct ExceptionFrame {
    int error_count = 0;
    std::string error_msg;
  };

  // Make filesystem io instance
  static std::unique_ptr<IO> Create();

  // i/o file abstract interface

  // Open an iostream based on filename,
  // io reader will match extname automatically.
  // When |raw| is true, it will open file with name directly.
  SDL_IOStream* OpenFile(const std::string& filename,
                         ExceptionFrame* exception_state);

  using OpenCallback =
      base::RepeatingCallback<bool(SDL_IOStream*, const std::string&)>;
  void OpenRead(const std::string& file_path,
                OpenCallback callback,
                ExceptionFrame* exception_state);

  // Check file status in matched path.
  bool IsFileExisted(const std::string& filename);

  // Enumerate content of specific directory.
  bool EnumDirectory(const std::string& path,
                     std::vector<std::string>& out_files);

 private:
  IO();
};

}  // namespace filesystem

#endif  //! COMPONENTS_FILESYSTEM_IO_H_
