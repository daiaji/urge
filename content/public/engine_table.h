// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_TABLE_H_
#define CONTENT_PUBLIC_ENGINE_TABLE_H_

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
// Interface referrence: RPGVXAce.chm
/*--urge(type=class)--*/
class URGE_RUNTIME_API Table : public base::RefCounted<Table> {
 public:
  virtual ~Table() = default;

  /*--urge()--*/
  static scoped_refptr<Table> New(uint32_t xsize,
                                  uint32_t ysize,
                                  uint32_t zsize,
                                  ExceptionState& exception_state);

  /*--urge()--*/
  static scoped_refptr<Table> Copy(ExecutionContext* execution_context,
                                   scoped_refptr<Table> other,
                                   ExceptionState& exception_state);

  /*--urge()--*/
  URGE_EXPORT_SERIALIZABLE(Table);

  /*--urge()--*/
  virtual void Resize(uint32_t xsize, ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void Resize(uint32_t xsize,
                      uint32_t ysize,
                      ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual void Resize(uint32_t xsize,
                      uint32_t ysize,
                      uint32_t zsize,
                      ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual uint32_t Xsize(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual uint32_t Ysize(ExceptionState& exception_state) = 0;

  /*--urge()--*/
  virtual uint32_t Zsize(ExceptionState& exception_state) = 0;

  /*--urge(alias_name:[])--*/
  virtual int16_t Get(uint32_t x,
                      uint32_t y,
                      uint32_t z,
                      ExceptionState& exception_state) = 0;

  /*--urge(alias_name:[]=)--*/
  virtual void Put(uint32_t x,
                   uint32_t y,
                   uint32_t z,
                   int16_t value,
                   ExceptionState& exception_state) = 0;
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_RECT_H_
