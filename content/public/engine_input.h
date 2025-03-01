// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_INPUT_H_
#define CONTENT_PUBLIC_ENGINE_INPUT_H_

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
// Interface referrence: RPGVXAce.chm
/*--urge(name:Input,is_module)--*/
class URGE_RUNTIME_API Input : public base::RefCounted<Input> {
 public:
  virtual ~Input() = default;

  /*--urge(name:update)--*/
  virtual void Update(ExceptionState& exception_state) = 0;

  /*--urge(name:press?)--*/
  virtual bool IsPressed(const std::string& sym,
                         ExceptionState& exception_state) = 0;

  /*--urge(name:trigger?)--*/
  virtual bool IsTriggered(const std::string& sym,
                           ExceptionState& exception_state) = 0;

  /*--urge(name:repeat?)--*/
  virtual bool IsRepeated(const std::string& sym,
                          ExceptionState& exception_state) = 0;

  /*--urge(name:dir4)--*/
  virtual int32_t Dir4(ExceptionState& exception_state) = 0;

  /*--urge(name:dir8)--*/
  virtual int32_t Dir8(ExceptionState& exception_state) = 0;
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_INPUT_H_
