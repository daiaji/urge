// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/components/disposable.h"

namespace content {

Disposable::Disposable(DisposableCollection* parent)
    : parent_(parent), disposed_(0) {
  parent_->AddDisposable(this);
}

Disposable::~Disposable() {
  parent_->RemoveDisposable(this);
}

void Disposable::Dispose(ExceptionState& exception_state) {
  if (!disposed_) {
    OnObjectDisposed();
    disposed_ = true;
  }
}

bool Disposable::IsDisposed(ExceptionState& exception_state) {
  return disposed_;
}

bool Disposable::CheckDisposed(ExceptionState& exception_state) {
  if (disposed_)
    exception_state.ThrowContentError(
        ExceptionCode::CONTENT_ERROR,
        "disposed object: " + DisposedObjectName());
  return disposed_;
}

}  // namespace content
