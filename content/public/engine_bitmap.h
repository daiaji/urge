// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_BITMAP_H_
#define CONTENT_PUBLIC_ENGINE_BITMAP_H_

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"
#include "content/public/engine_color.h"
#include "content/public/engine_font.h"
#include "content/public/engine_rect.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
// Interface referrence: RPGVXAce.chm
/*--urge(type=class)--*/
class URGE_RUNTIME_API Bitmap : public base::RefCounted<Bitmap> {
 public:
  virtual ~Bitmap() = default;

  /*--urge()--*/
  static scoped_refptr<Bitmap> New(ExecutionContext* execution_context,
                                   const std::string& filename,
                                   ExceptionState& exception_state);

  /*--urge()--*/
  static scoped_refptr<Bitmap> New(ExecutionContext* execution_context,
                                   uint32_t width,
                                   uint32_t height,
                                   ExceptionState& exception_state);

  /*--urge()--*/
  static scoped_refptr<Bitmap> Copy(ExecutionContext* execution_context,
                                    scoped_refptr<Bitmap> other,
                                    ExceptionState& exception_state);

  /*--urge()--*/
  URGE_EXPORT_SERIALIZABLE(Bitmap);

  /*--urge()--*/
  virtual void Dispose(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual bool IsDisposed(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual uint32_t Width(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual uint32_t Height(ExceptionState& exception_state) = 0;

  /*--urge(alias_name:rect)--*/
  virtual scoped_refptr<Rect> GetRect(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void Blt(int32_t x,
                   int32_t y,
                   scoped_refptr<Bitmap> src_bitmap,
                   scoped_refptr<Rect> src_rect,
                   uint32_t opacity,
                   ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void StretchBlt(scoped_refptr<Rect> dest_rect,
                          scoped_refptr<Bitmap> src_bitmap,
                          scoped_refptr<Rect> src_rect,
                          uint32_t opacity,
                          ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void FillRect(int32_t x,
                        int32_t y,
                        uint32_t width,
                        uint32_t height,
                        scoped_refptr<Color> color,
                        ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void FillRect(scoped_refptr<Rect> rect,
                        scoped_refptr<Color> color,
                        ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void GradientFillRect(int32_t x,
                                int32_t y,
                                uint32_t width,
                                uint32_t height,
                                scoped_refptr<Color> color1,
                                scoped_refptr<Color> color2,
                                bool vertical,
                                ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void GradientFillRect(scoped_refptr<Rect> rect,
                                scoped_refptr<Color> color1,
                                scoped_refptr<Color> color2,
                                bool vertical,
                                ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void Clear(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void ClearRect(int32_t x,
                         int32_t y,
                         uint32_t width,
                         uint32_t height,
                         ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void ClearRect(scoped_refptr<Rect> rect,
                         ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual scoped_refptr<Color> GetPixel(int32_t x,
                                        int32_t y,
                                        ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void SetPixel(int32_t x,
                        int32_t y,
                        scoped_refptr<Color> color,
                        ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void HueChange(int32_t hue, ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void Blur(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void RadialBlur(int32_t angle,
                          int32_t division,
                          ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void DrawText(int32_t x,
                        int32_t y,
                        uint32_t width,
                        uint32_t height,
                        const std::string& str,
                        int32_t align,
                        ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void DrawText(scoped_refptr<Rect> rect,
                        const std::string& str,
                        int32_t align,
                        ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual scoped_refptr<Rect> TextSize(const std::string& str,
                                       ExceptionState& exception_state) = 0;

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Font, scoped_refptr<Font>);
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_BITMAP_H_
