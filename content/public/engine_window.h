// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_WINDOW_H_
#define CONTENT_PUBLIC_ENGINE_WINDOW_H_

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
class URGE_RUNTIME_API Window : public base::RefCounted<Window> {
 public:
  virtual ~Window() = default;

  /*--urge()--*/
  static scoped_refptr<Window> New(ExecutionContext* execution_context,
                                   scoped_refptr<Viewport> viewport,
                                   ExceptionState& exception_state);

  /*--urge()--*/
  virtual void Dispose(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual bool IsDisposed(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void Update(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Viewport, scoped_refptr<Viewport>);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Windowskin, scoped_refptr<Bitmap>);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Contents, scoped_refptr<Bitmap>);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Stretch, bool);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(CursorRect, scoped_refptr<Rect>);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Active, bool);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Visible, bool);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Pause, bool);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(X, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Y, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Width, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Height, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Z, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Ox, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Oy, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Opacity, uint32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(BackOpacity, uint32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(ContentsOpacity, uint32_t);
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_WINDOW_H_
