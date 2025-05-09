// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_PALETTE_H_
#define CONTENT_PUBLIC_ENGINE_PALETTE_H_

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"
#include "content/public/engine_color.h"
#include "content/public/engine_rect.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
/*--urge(name:Palette)--*/
class URGE_RUNTIME_API Palette : public base::RefCounted<Palette> {
 public:
  virtual ~Palette() = default;

  /*--urge(name:initialize)--*/
  static scoped_refptr<Palette> New(ExecutionContext* execution_context,
                                    uint32_t width,
                                    uint32_t height,
                                    ExceptionState& exception_state);

  /*--urge(name:initialize)--*/
  static scoped_refptr<Palette> New(ExecutionContext* execution_context,
                                    const std::string& filename,
                                    ExceptionState& exception_state);

  /*--urge(name:from_dump)--*/
  static scoped_refptr<Palette> FromDump(ExecutionContext* execution_context,
                                         const std::string& dump_data,
                                         ExceptionState& exception_state);

  /*--urge(name:initialize_copy)--*/
  static scoped_refptr<Palette> Copy(ExecutionContext* execution_context,
                                     scoped_refptr<Palette> other,
                                     ExceptionState& exception_state);

  /*--urge(serializable)--*/
  URGE_EXPORT_SERIALIZABLE(Palette);

  /*--urge(name:dispose)--*/
  virtual void Dispose(ExceptionState& exception_state) = 0;

  /*--urge(name:disposed?)--*/
  virtual bool IsDisposed(ExceptionState& exception_state) = 0;

  /*--urge(name:width)--*/
  virtual uint32_t Width(ExceptionState& exception_state) = 0;

  /*--urge(name:height)--*/
  virtual uint32_t Height(ExceptionState& exception_state) = 0;

  /*--urge(name:rect)--*/
  virtual scoped_refptr<Rect> GetRect(ExceptionState& exception_state) = 0;

  /*--urge(name:blt,optional:opacity=255)--*/
  virtual void Blt(int32_t x,
                   int32_t y,
                   scoped_refptr<Palette> src_palette,
                   scoped_refptr<Rect> src_rect,
                   uint32_t opacity,
                   ExceptionState& exception_state) = 0;

  /*--urge(name:stretch_blt,optional:opacity=255)--*/
  virtual void StretchBlt(scoped_refptr<Rect> dest_rect,
                          scoped_refptr<Palette> src_palette,
                          scoped_refptr<Rect> src_rect,
                          uint32_t opacity,
                          ExceptionState& exception_state) = 0;

  /*--urge(name:fill_rect)--*/
  virtual void FillRect(int32_t x,
                        int32_t y,
                        uint32_t width,
                        uint32_t height,
                        scoped_refptr<Color> color,
                        ExceptionState& exception_state) = 0;

  /*--urge(name:fill_rect)--*/
  virtual void FillRect(scoped_refptr<Rect> rect,
                        scoped_refptr<Color> color,
                        ExceptionState& exception_state) = 0;

  /*--urge(name:clear)--*/
  virtual void Clear(ExceptionState& exception_state) = 0;

  /*--urge(name:clear_rect)--*/
  virtual void ClearRect(int32_t x,
                         int32_t y,
                         uint32_t width,
                         uint32_t height,
                         ExceptionState& exception_state) = 0;

  /*--urge(name:clear_rect)--*/
  virtual void ClearRect(scoped_refptr<Rect> rect,
                         ExceptionState& exception_state) = 0;

  /*--urge(name:get_pixel)--*/
  virtual scoped_refptr<Color> GetPixel(int32_t x,
                                        int32_t y,
                                        ExceptionState& exception_state) = 0;

  /*--urge(name:set_pixel)--*/
  virtual void SetPixel(int32_t x,
                        int32_t y,
                        scoped_refptr<Color> color,
                        ExceptionState& exception_state) = 0;

  /*--urge(name:dump_data)--*/
  virtual std::string DumpData(ExceptionState& exception_state) = 0;

  /*--urge(name:save_png)--*/
  virtual void SavePNG(const std::string& filename,
                       ExceptionState& exception_state) = 0;
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_PALETTE_H_
