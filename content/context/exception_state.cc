// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/context/exception_state.h"

#include <stdarg.h>
#include <stdio.h>

namespace content {

void ExceptionState::ThrowError(ExceptionCode exception_code,
                                const char* format,
                                ...) {
  had_exception_ = true;
  code_ = exception_code;

  va_list ap;
  va_start(ap, format);
  message_.resize(1024);
  vsnprintf(message_.data(), message_.size(), format, ap);
  va_end(ap);
}

}  // namespace content
