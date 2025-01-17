// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_COLOR_H_
#define CONTENT_PUBLIC_ENGINE_COLOR_H_

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
// Interface referrence: RPGVXAce.chm
/*--urge(type=class)--*/
class URGE_RUNTIME_API Color : public base::RefCounted<Color> {
 public:
  virtual ~Color() = default;

  /*--urge()--*/
  static scoped_refptr<Color> New(ExceptionState& exception_state);

  /*--urge()--*/
  static scoped_refptr<Color> New(float red,
                                  float green,
                                  float blue,
                                  float alpha,
                                  ExceptionState& exception_state);

  /*--urge()--*/
  static scoped_refptr<Color> Copy(ExecutionContext* execution_context,
                                   scoped_refptr<Color> other,
                                   ExceptionState& exception_state);

  /*--urge()--*/
  URGE_EXPORT_SERIALIZABLE(Color);

  /*--urge()--*/
  virtual void Set(float red,
                   float green,
                   float blue,
                   float alpha,
                   ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void Set(scoped_refptr<Color> other,
                   ExceptionState& exception_state) = 0;

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Red, float);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Green, float);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Blue, float);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Alpha, float);
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_COLOR_H_
