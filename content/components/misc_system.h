// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMPONENTS_MISC_SYSTEM_H_
#define CONTENT_COMPONENTS_MISC_SYSTEM_H_

#include "content/public/engine_urge.h"
#include "ui/widget/widget.h"

namespace content {

class MiscSystem : public URGE {
 public:
  MiscSystem(base::WeakPtr<ui::Widget> window);
  ~MiscSystem() override;

  MiscSystem(const MiscSystem&) = delete;
  MiscSystem& operator=(const MiscSystem&) = delete;

 public:
  std::string GetPlatform(ExceptionState& exception_state) override;
  void OpenURL(const std::string& path,
               ExceptionState& exception_state) override;
  std::string Gets(ExceptionState& exception_state) override;
  void Puts(const std::string& message,
            ExceptionState& exception_state) override;
  void Alert(const std::string& message,
             ExceptionState& exception_state) override;
  bool Confirm(const std::string& message,
               ExceptionState& exception_state) override;

 private:
  base::WeakPtr<ui::Widget> window_;
};

}  // namespace content

#endif  //! CONTENT_COMPONENTS_MISC_SYSTEM_H_
