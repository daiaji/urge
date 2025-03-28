// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_INPUT_MOUSE_CONTROLLER_H_
#define CONTENT_INPUT_MOUSE_CONTROLLER_H_

#include <array>

#include "content/profile/content_profile.h"
#include "content/public/engine_mouse.h"
#include "ui/widget/widget.h"

namespace content {

class MouseImpl : public Mouse {
 public:
  enum Button {
    Left = SDL_BUTTON_LEFT,
    Middle = SDL_BUTTON_MIDDLE,
    Right = SDL_BUTTON_RIGHT,
    X1 = SDL_BUTTON_X1,
    X2 = SDL_BUTTON_X2,
  };

  MouseImpl(base::WeakPtr<ui::Widget> window, ContentProfile* profile);
  ~MouseImpl() override;

  MouseImpl(const MouseImpl&) = delete;
  MouseImpl& operator=(const MouseImpl&) = delete;

  void SetUpdateEnable(bool enable) { enable_update_ = enable; }

  void Update(ExceptionState& exception_state) override;

  int32_t GetX(ExceptionState& exception_state) override;
  int32_t GetY(ExceptionState& exception_state) override;
  void SetPosition(int32_t x,
                   int32_t y,
                   ExceptionState& exception_state) override;

  bool IsDown(int32_t button, ExceptionState& exception_state) override;
  bool IsUp(int32_t button, ExceptionState& exception_state) override;
  bool IsDouble(int32_t button, ExceptionState& exception_state) override;
  bool IsPressed(int32_t button, ExceptionState& exception_state) override;
  bool IsMoved(ExceptionState& exception_state) override;

  int32_t GetScrollX(ExceptionState& exception_state) override;
  int32_t GetScrollY(ExceptionState& exception_state) override;

  void SetCursor(scoped_refptr<Bitmap> cursor,
                 int32_t hot_x,
                 int32_t hot_y,
                 ExceptionState& exception_state) override;

  bool Get_Visible(ExceptionState& exception_state) override;
  void Put_Visible(const bool& value, ExceptionState& exception_state) override;

 private:
  struct BindingState {
    bool up = false;
    bool down = false;
    int click_count = 0;
    bool pressed = false;
  };

  struct MouseState {
    float last_x = 0, last_y = 0;
    bool moved = false;
    int scroll_x = 0, scroll_y = 0;
    int last_scroll_x = 0, last_scroll_y = 0;
  } entity_state_;

  base::WeakPtr<ui::Widget> window_;
  ContentProfile* profile_;
  bool enable_update_;

  std::array<BindingState, sizeof(ui::Widget::MouseState::states)> states_;
};

}  // namespace content

#endif  //! CONTENT_INPUT_MOUSE_CONTROLLER_H_
