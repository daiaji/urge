// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_WORKER_EVENT_CONTROLLER_H_
#define CONTENT_WORKER_EVENT_CONTROLLER_H_

#include <vector>

#include "SDL3/SDL_events.h"

#include "ui/widget/widget.h"

namespace content {

class EventController {
 public:
  using KeyEvent = struct {
    uint64_t timestamp;
    SDL_KeyboardID keyboard_id;

    enum {
      KEY_DOWN = 0,
      KEY_UP,
    } type;

    SDL_Scancode scancode;
    SDL_Keycode keycode;
    SDL_Keymod modifier;
    bool down;
    bool repeat;
  };

  using MouseEvent = struct {
    uint64_t timestamp;
    SDL_MouseID mouse_id;

    enum {
      MOUSE_MOTION = 0,
      MOUSE_BUTTON_EVENT,
      MOUSE_WHEEL_EVENT,
    } type;

    int32_t x;
    int32_t y;

    int32_t button_id;
    int32_t button_down;
    int32_t button_clicks;

    int32_t motion_state;
    int32_t motion_relx;
    int32_t motion_rely;

    int32_t wheel_dir;
    int32_t wheel_x;
    int32_t wheel_y;
  };

  using TouchEvent = struct {
    uint64_t timestamp;
    SDL_TouchID touch_id;

    enum {
      FINGER_DOWN = 0,
      FINGER_UP,
      FINGER_MOTION,
      FINGER_CANCELED,
    } type;

    SDL_FingerID finger;
    int32_t x;
    int32_t y;
    int32_t dx;
    int32_t dy;
    float pressure;
  };

  EventController(base::WeakPtr<ui::Widget> window);
  ~EventController();

  EventController(const EventController&) = delete;
  EventController& operator=(const EventController&) = delete;

  void DispatchEvent(SDL_Event* event);

  void PollKeyEvents(std::vector<KeyEvent>& out);
  void PollMouseEvents(std::vector<MouseEvent>& out);
  void PollTouchEvents(std::vector<TouchEvent>& out);

 private:
  base::WeakPtr<ui::Widget> window_;
  std::vector<KeyEvent> key_events_;
  std::vector<MouseEvent> mouse_events_;
  std::vector<TouchEvent> touch_events_;
};

}  // namespace content

#endif  //! CONTENT_WORKER_EVENT_CONTROLLER_H_
