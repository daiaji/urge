// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_INPUTEVENT_H_
#define CONTENT_PUBLIC_ENGINE_INPUTEVENT_H_

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
/*--urge(name:InputEvent)--*/
class URGE_RUNTIME_API InputEvent : public base::RefCounted<InputEvent> {
 public:
  virtual ~InputEvent() = default;

  /*--urge(name:update)--*/
  static std::vector<scoped_refptr<InputEvent>> Update(
      ExecutionContext* execution_context,
      ExceptionState& exception_state) = 0;

  /*--urge(name:keyboard_id)--*/
  virtual int32_t GetKeyboardID(ExceptionState& exception_state) = 0;

  /*--urge(name:scancode)--*/
  virtual int32_t GetScancode(ExceptionState& exception_state) = 0;

  /*--urge(name:key)--*/
  virtual int32_t GetKey(ExceptionState& exception_state) = 0;

  /*--urge(name:modifier)--*/
  virtual int32_t GetModifier(ExceptionState& exception_state) = 0;

  /*--urge(name:down?)--*/
  virtual bool GetDown(ExceptionState& exception_state) = 0;

  /*--urge(name:repeat?)--*/
  virtual bool GetRepeat(ExceptionState& exception_state) = 0;
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_INPUTEVENT_H_
