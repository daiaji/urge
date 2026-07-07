// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_WORKER_EVENT_CONTROLLER_H_
#define CONTENT_WORKER_EVENT_CONTROLLER_H_

#include <array>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gamepad.h"

#include "base/math/rectangle.h"
#include "content/public/engine_keyevent.h"
#include "content/public/engine_mouseevent.h"
#include "content/public/engine_textinputevent.h"
#include "content/public/engine_touchevent.h"

namespace content {

#define MOUSE_BUTTON_COUNT 5

class EventController {
 public:
  // Custom event handler data
  struct CommonData {
    uint64_t timestamp;
  };

  // Keyboard event data
  struct KeyEventData : public CommonData {
    KeyEvent::Type type;
    SDL_KeyboardID keyboard_id;
    SDL_Scancode scancode;
    SDL_Keycode keycode;
    SDL_Keymod modifier;
    int32_t is_down;
    int32_t is_repeat;
  };

  // Mouse event data
  struct MouseEventData : public CommonData {
    MouseEvent::Type type;
    SDL_MouseID mouse_id;
    base::Vec2 relative_position;
    int32_t button_id;
    int32_t button_is_down;
    int32_t button_clicks;
    int32_t motion_state;
    base::Vec2 relative_offset;
    MouseEvent::WheelState wheel_dir;
    base::Vec2 wheel_offset;
  };

  // Touch event data
  struct TouchEventData : public CommonData {
    TouchEvent::Type type;
    SDL_TouchID touch_id;
    SDL_FingerID finger;
    base::Vec2 relative_position;
    base::Vec2 delta_offset;
    float pressure;
  };

  // IME input event data
  struct TextInputEventData : public CommonData {
    TextInputEvent::Type type;
    std::string text;
    int32_t select_start;
    int32_t select_length;
  };

  // Event-driven gamepad state (mkxp-z style)
  struct GamepadState {
    bool connected = false;
    int16_t axes[SDL_GAMEPAD_AXIS_COUNT] = {};
    bool buttons[SDL_GAMEPAD_BUTTON_COUNT] = {};
  };

  EventController();
  ~EventController();

  EventController(const EventController&) = delete;
  EventController& operator=(const EventController&) = delete;

  // Process game window
  void ClearPendingEvents();
  void DispatchEvent(const SDL_Event* event,
                     SDL_WindowID window_id,
                     const base::Vec2i& window_size,
                     const base::Vec2i& screen_size,
                     const base::Rect& bound_in_screen);

  // For event reference (readonly)
  const std::vector<KeyEventData>& key_events() const { return key_events_; }
  const std::vector<MouseEventData>& mouse_events() const {
    return mouse_events_;
  }
  const std::vector<TouchEventData>& touch_events() const {
    return touch_events_;
  }
  const std::vector<TextInputEventData>& text_input_events() const {
    return text_input_events_;
  }

  // Gamepad state (updated by events, read by input system)
  const GamepadState& gamepad_state() const { return gamepad_state_; }
  void SetGamepadConnected(bool connected) { gamepad_state_.connected = connected; }
  void UpdateGamepadButton(int button, bool down) {
    if (button >= 0 && button < SDL_GAMEPAD_BUTTON_COUNT)
      gamepad_state_.buttons[button] = down;
  }
  void UpdateGamepadAxis(int axis, int16_t value) {
    if (axis >= 0 && axis < SDL_GAMEPAD_AXIS_COUNT)
      gamepad_state_.axes[axis] = value;
  }
  void ResetGamepadState() {
    gamepad_state_ = GamepadState{};
  }

  // Gamepad handle lifecycle (managed by ContentRunner)
  SDL_Gamepad* gamepad_handle() const { return gamepad_handle_; }
  void set_gamepad_handle(SDL_Gamepad* handle) { gamepad_handle_ = handle; }
  SDL_JoystickID gamepad_id() const { return gamepad_id_; }
  void set_gamepad_id(SDL_JoystickID id) { gamepad_id_ = id; }

 private:
  bool IsCurrentGamepadEvent(SDL_JoystickID id) const;

  std::vector<KeyEventData> key_events_;
  std::vector<MouseEventData> mouse_events_;
  std::vector<TouchEventData> touch_events_;
  std::vector<TextInputEventData> text_input_events_;

  GamepadState gamepad_state_;
  SDL_Gamepad* gamepad_handle_ = nullptr;
  SDL_JoystickID gamepad_id_ = 0;
};

}  // namespace content

#endif  //! CONTENT_WORKER_EVENT_CONTROLLER_H_
