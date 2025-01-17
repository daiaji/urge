// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_RECT_H_
#define CONTENT_PUBLIC_ENGINE_RECT_H_

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
// Interface referrence: RPGVXAce.chm
/*--urge(type=class)--*/
class URGE_RUNTIME_API Rect : public base::RefCounted<Rect> {
 public:
  virtual ~Rect() = default;

  /*--urge()--*/
  static scoped_refptr<Rect> New(ExceptionState& exception_state);

  /*--urge()--*/
  static scoped_refptr<Rect> New(int32_t x,
                                 int32_t y,
                                 int32_t width,
                                 int32_t height,
                                 ExceptionState& exception_state);

  /*--urge()--*/
  static scoped_refptr<Rect> Copy(ExecutionContext* execution_context,
                                  scoped_refptr<Rect> other,
                                  ExceptionState& exception_state);

  /*--urge()--*/
  URGE_EXPORT_SERIALIZABLE(Rect);

  /*--urge()--*/
  virtual void Set(int32_t x,
                   int32_t y,
                   int32_t width,
                   int32_t height,
                   ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void Set(scoped_refptr<Rect> other,
                   ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void Empty(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(X, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Y, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Width, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Height, int32_t);
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_RECT_H_
