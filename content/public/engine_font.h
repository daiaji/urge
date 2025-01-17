// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ENGINE_FONT_H_
#define CONTENT_PUBLIC_ENGINE_FONT_H_

#include <vector>

#include "base/memory/ref_counted.h"
#include "content/content_config.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"
#include "content/public/engine_color.h"

namespace content {

// IDL generator format:
// Inhert: refcounted only.
// Interface referrence: RPGVXAce.chm
/*--urge(type=class)--*/
class URGE_RUNTIME_API Font : public base::RefCounted<Font> {
 public:
  virtual ~Font() = default;

  /*--urge()--*/
  static scoped_refptr<Font> New(ExecutionContext* execution_context,
                                 const std::string& name,
                                 uint32_t size,
                                 ExceptionState& exception_state);

  /*--urge()--*/
  static scoped_refptr<Font> Copy(ExecutionContext* execution_context,
                                  scoped_refptr<Font> other,
                                  ExceptionState& exception_state);

  /*--urge()--*/
  static bool IsExisted(ExecutionContext* execution_context,
                        const std::string& name,
                        ExceptionState& exception_state);

  /*--urge()--*/
  URGE_EXPORT_STATIC_ATTRIBUTE(DefaultName, std::vector<std::string>);

  /*--urge()--*/
  URGE_EXPORT_STATIC_ATTRIBUTE(DefaultSize, uint32_t);

  /*--urge()--*/
  URGE_EXPORT_STATIC_ATTRIBUTE(DefaultBold, bool);

  /*--urge()--*/
  URGE_EXPORT_STATIC_ATTRIBUTE(DefaultItalic, bool);

  /*--urge()--*/
  URGE_EXPORT_STATIC_ATTRIBUTE(DefaultShadow, bool);

  /*--urge()--*/
  URGE_EXPORT_STATIC_ATTRIBUTE(DefaultOutline, bool);

  /*--urge()--*/
  URGE_EXPORT_STATIC_ATTRIBUTE(DefaultColor, scoped_refptr<Color>);

  /*--urge()--*/
  URGE_EXPORT_STATIC_ATTRIBUTE(DefaultOutColor, scoped_refptr<Color>);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Name, std::vector<std::string>);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Size, uint32_t);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Bold, bool);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Italic, bool);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Outline, bool);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Shadow, bool);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(Color, scoped_refptr<Color>);

  /*--urge()--*/
  URGE_EXPORT_ATTRIBUTE(OutColor, scoped_refptr<Color>);
};

}  // namespace content

#endif  //! CONTENT_PUBLIC_ENGINE_FONT_H_
