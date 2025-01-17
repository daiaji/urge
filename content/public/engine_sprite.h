// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_SPRITE_H_
#define CONTENT_PUBLIC_ENGINE_SPRITE_H_

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
class URGE_RUNTIME_API Sprite : public base::RefCounted<Sprite> {
 public:
  virtual ~Sprite() = default;

  /*--urge()--*/
  static scoped_refptr<Sprite> New(ExecutionContext* execution_context,
                                   scoped_refptr<Viewport> viewport,
                                   ExceptionState& exception_state);

  /*--urge()--*/
  virtual void Dispose(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual bool IsDisposed(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void Flash(scoped_refptr<Color> color,
                     uint32_t duration,
                     ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void Update(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual uint32_t Width(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual uint32_t Height(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Bitmap, scoped_refptr<Bitmap>);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(SrcRect, scoped_refptr<Rect>);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Viewport, scoped_refptr<Viewport>);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Visible, bool);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(X, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Y, int32_t);

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
  URGE_EXPORT_ATTRIBUTE(Angle, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(WaveAmp, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(WaveLength, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(WaveSpeed, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(WavePhase, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Mirror, bool);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(BushDepth, int32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(BushOpacity, int32_t);

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

#endif  //! CONTENT_PUBLIC_ENGINE_SPRITE_H_
