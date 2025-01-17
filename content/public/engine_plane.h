// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_PLANE_H_
#define CONTENT_PUBLIC_ENGINE_PLANE_H_

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"
#include "content/public/engine_bitmap.h"
#include "content/public/engine_viewport.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
// Interface referrence: RPGVXAce.chm
/*--urge(type=class)--*/
class URGE_RUNTIME_API Plane : public base::RefCounted<Plane> {
 public:
  virtual ~Plane() = default;

  /*--urge()--*/
  static scoped_refptr<Plane> New(ExecutionContext* execution_context,
                                  scoped_refptr<Viewport> viewport,
                                  ExceptionState& exception_state);

  /*--urge()--*/
  virtual void Dispose(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual bool IsDisposed(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Bitmap, scoped_refptr<Bitmap>);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Viewport, scoped_refptr<Viewport>);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Visible, bool);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Z, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Ox, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Oy, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(ZoomX, float);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(ZoomY, float);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Opacity, uint32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(BlendType, uint32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Color, scoped_refptr<Color>);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Tone, scoped_refptr<Tone>);
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_PLANE_H_
