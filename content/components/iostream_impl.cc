// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/components/iostream_impl.h"

#include "components/filesystem/io_service.h"

namespace content {

#define CHECK_VALID(ret)                                                     \
  if (!stream_) {                                                            \
    exception_state.ThrowError(ExceptionCode::IO_ERROR, "Closed iostream."); \
    return ret;                                                              \
  }

scoped_refptr<IOStream> IOStream::FromFileSystem(
    ExecutionContext* execution_context,
    const std::string& filename,
    const std::string& mode,
    ExceptionState& exception_state) {
  auto* stream = SDL_IOFromFile(filename.c_str(), mode.c_str());
  if (!stream) {
    exception_state.ThrowError(ExceptionCode::IO_ERROR,
                               "Failed to open file: %s", SDL_GetError());
    return nullptr;
  }

  return new IOStreamImpl(stream, std::string());
}

scoped_refptr<IOStream> IOStream::FromIOSystem(
    ExecutionContext* execution_context,
    const std::string& filename,
    ExceptionState& exception_state) {
  filesystem::IOState io_state;
  auto* stream =
      execution_context->io_service->OpenReadRaw(filename.c_str(), &io_state);
  if (io_state.error_count) {
    exception_state.ThrowError(ExceptionCode::IO_ERROR,
                               "Failed to load from iosystem: %s",
                               io_state.error_message.c_str());
    return nullptr;
  }

  return new IOStreamImpl(stream, std::string());
}

scoped_refptr<IOStream> IOStream::FromMemory(
    ExecutionContext* execution_context,
    const std::string& buffer,
    ExceptionState& exception_state) {
  std::string new_buffer = buffer;
  auto* stream = SDL_IOFromConstMem(new_buffer.data(), new_buffer.size());
  if (!stream) {
    exception_state.ThrowError(ExceptionCode::IO_ERROR,
                               "Failed to open file: %s", SDL_GetError());
    return nullptr;
  }

  return new IOStreamImpl(stream, std::move(new_buffer));
}

IOStreamImpl::IOStreamImpl(SDL_IOStream* stream, std::string buffer)
    : stream_(stream), buffer_(std::move(buffer)) {}

IOStreamImpl::~IOStreamImpl() {
  if (stream_)
    SDL_CloseIO(stream_);
}

scoped_refptr<IOStreamImpl> IOStreamImpl::From(scoped_refptr<IOStream> host) {
  return static_cast<IOStreamImpl*>(host.get());
}

bool IOStreamImpl::Close(ExceptionState& exception_state) {
  CHECK_VALID(false);

  auto result = SDL_CloseIO(stream_);
  stream_ = nullptr;
  return result;
}

IOStream::IOStatus IOStreamImpl::GetStatus(ExceptionState& exception_state) {
  CHECK_VALID(STATUS_ERROR);

  return static_cast<IOStatus>(SDL_GetIOStatus(stream_));
}

int64_t IOStreamImpl::GetSize(ExceptionState& exception_state) {
  CHECK_VALID(0);

  return SDL_GetIOSize(stream_);
}

int64_t IOStreamImpl::Seek(int64_t offset,
                           IOWhence whence,
                           ExceptionState& exception_state) {
  CHECK_VALID(0);

  return SDL_SeekIO(stream_, offset, static_cast<SDL_IOWhence>(whence));
}

int64_t IOStreamImpl::Tell(ExceptionState& exception_state) {
  CHECK_VALID(0);

  return SDL_TellIO(stream_);
}

std::string IOStreamImpl::Read(int64_t size, ExceptionState& exception_state) {
  CHECK_VALID(std::string());

  std::string buffer(size, 0);
  size_t read_size = SDL_ReadIO(stream_, buffer.data(), size);
  buffer.resize(read_size);

  return buffer;
}

int64_t IOStreamImpl::Write(const std::string& buffer,
                            ExceptionState& exception_state) {
  CHECK_VALID(0);

  return static_cast<int64_t>(
      SDL_WriteIO(stream_, buffer.data(), buffer.size()));
}

bool IOStreamImpl::Flush(ExceptionState& exception_state) {
  CHECK_VALID(false);

  return SDL_FlushIO(stream_);
}

}  // namespace content
