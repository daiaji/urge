// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/table_impl.h"

namespace content {

scoped_refptr<Table> Table::New(ExecutionContext* execution_context,
                                uint32_t xsize,
                                ExceptionState& exception_state) {
  return new TableImpl(xsize, 1, 1);
}

scoped_refptr<Table> Table::New(ExecutionContext* execution_context,
                                uint32_t xsize,
                                uint32_t ysize,
                                ExceptionState& exception_state) {
  return new TableImpl(xsize, ysize, 1);
}

scoped_refptr<Table> Table::New(ExecutionContext* execution_context,
                                uint32_t xsize,
                                uint32_t ysize,
                                uint32_t zsize,
                                ExceptionState& exception_state) {
  return new TableImpl(xsize, ysize, zsize);
}

scoped_refptr<Table> Table::Copy(ExecutionContext* execution_context,
                                 scoped_refptr<Table> other,
                                 ExceptionState& exception_state) {
  return new TableImpl(*static_cast<TableImpl*>(other.get()));
}

scoped_refptr<Table> Table::Deserialize(const std::string& data,
                                        ExceptionState& exception_state) {
  const uint32_t* ptr = reinterpret_cast<const uint32_t*>(data.data());
  TableImpl* impl = new TableImpl(*++ptr, *++ptr, *++ptr);
  uint32_t data_size = *++ptr;

  if (data_size != impl->x_size_ * impl->y_size_ * impl->z_size_) {
    exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                      "incorrect table serialize data");
    return nullptr;
  }

  if (data_size)
    std::memcpy(impl->data_.data(), ++ptr, data_size);

  return impl;
}

std::string Table::Serialize(scoped_refptr<Table> value,
                             ExceptionState& exception_state) {
  TableImpl* impl = static_cast<TableImpl*>(value.get());

  uint32_t dim = 0;
  if (impl->x_size_ >= 1)
    dim++;
  if (impl->y_size_ > 1)
    dim++;
  if (impl->z_size_ > 1)
    dim++;

  uint32_t data_size = impl->x_size_ * impl->y_size_ * impl->z_size_;
  std::string serial_data(sizeof(int32_t) * 5 + data_size * sizeof(int16_t), 0);

  uint32_t* ptr = reinterpret_cast<uint32_t*>(serial_data.data());
  *ptr++ = dim;
  *ptr++ = impl->x_size_;
  *ptr++ = impl->y_size_;
  *ptr++ = impl->z_size_;
  *ptr++ = data_size;
  std::memcpy(ptr, impl->data_.data(), sizeof(int16_t) * data_size);

  return serial_data;
}

TableImpl::TableImpl(uint32_t xsize, uint32_t ysize, uint32_t zsize)
    : dirty_(true),
      x_size_(xsize),
      y_size_(ysize),
      z_size_(zsize),
      data_(xsize * ysize * zsize) {}

TableImpl::TableImpl(const TableImpl& other)
    : dirty_(true),
      x_size_(other.x_size_),
      y_size_(other.y_size_),
      z_size_(other.z_size_),
      data_(other.data_) {}

void TableImpl::Resize(uint32_t xsize, ExceptionState& exception_state) {
  Resize(xsize, y_size_, z_size_, exception_state);
}

void TableImpl::Resize(uint32_t xsize,
                       uint32_t ysize,
                       ExceptionState& exception_state) {
  Resize(xsize, ysize, z_size_, exception_state);
}

void TableImpl::Resize(uint32_t xsize,
                       uint32_t ysize,
                       uint32_t zsize,
                       ExceptionState& exception_state) {
  // Allocate a new table container
  std::vector<int16_t> new_data(xsize * ysize * zsize);

  // Migrate old data to new container. (Shrink)
  for (int k = 0; k < std::min(zsize, z_size_); ++k)
    for (int j = 0; j < std::min(ysize, y_size_); ++j)
      for (int i = 0; i < std::min(xsize, x_size_); ++i)
        new_data[i + j * x_size_ + k * x_size_ * y_size_] =
            data_.at(i + x_size_ * j + x_size_ * y_size_ * k);

  // Swap data and set new size
  data_.swap(new_data);
  x_size_ = xsize;
  y_size_ = ysize;
  z_size_ = zsize;
  dirty_ = true;
}

uint32_t TableImpl::Xsize(ExceptionState& exception_state) {
  return x_size_;
}

uint32_t TableImpl::Ysize(ExceptionState& exception_state) {
  return y_size_;
}

uint32_t TableImpl::Zsize(ExceptionState& exception_state) {
  return z_size_;
}

int16_t TableImpl::Get(uint32_t x, ExceptionState& exception_state) {
  return data_.at(x);
}

int16_t TableImpl::Get(uint32_t x,
                       uint32_t y,
                       ExceptionState& exception_state) {
  return data_.at(x + x_size_ * y);
}

int16_t TableImpl::Get(uint32_t x,
                       uint32_t y,
                       uint32_t z,
                       ExceptionState& exception_state) {
  return data_.at(x + x_size_ * y + x_size_ * y_size_ * z);
}

void TableImpl::Put(uint32_t x,
                    int16_t value,
                    ExceptionState& exception_state) {
  if (x < 0 || x >= x_size_)
    return;
  data_[x] = value;
  dirty_ = true;
}

void TableImpl::Put(uint32_t x,
                    uint32_t y,
                    int16_t value,
                    ExceptionState& exception_state) {
  if (x < 0 || x >= x_size_ || y < 0 || y >= y_size_)
    return;
  data_[x + x_size_ * y] = value;
  dirty_ = true;
}

void TableImpl::Put(uint32_t x,
                    uint32_t y,
                    uint32_t z,
                    int16_t value,
                    ExceptionState& exception_state) {
  if (x < 0 || x >= x_size_ || y < 0 || y >= y_size_ || z < 0 || z >= z_size_)
    return;
  data_[x + x_size_ * y + x_size_ * y_size_ * z] = value;
  dirty_ = true;
}

bool TableImpl::FetchDirtyStatus() {
  if (dirty_) {
    dirty_ = false;
    return true;
  }
  return false;
}

}  // namespace content
