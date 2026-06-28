// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_INPUT_KEYBOARD_CONTROLLER_H_
#define CONTENT_INPUT_KEYBOARD_CONTROLLER_H_

#include <optional>

#include "content/context/engine_object.h"
#include "content/profile/content_profile.h"
#include "content/profile/i18n_profile.h"
#include "content/public/engine_input.h"
#include "content/worker/event_controller.h"
#include "ui/widget/widget.h"

namespace content {

class KeyboardControllerImpl : public Input, public EngineObject {
 public:
  struct KeyBinding {
    std::string sym;
    SDL_Scancode scancode;

    bool operator==(const KeyBinding& other) const {
      return sym == other.sym && scancode == other.scancode;
    }
  };

  using KeySymMap = std::vector<KeyBinding>;
  struct KeyState {
    bool pressed = false;
    bool trigger = false;
    bool repeat = false;
    int32_t repeat_count = 0;
  };

  KeyboardControllerImpl(ExecutionContext* execution_context);
  ~KeyboardControllerImpl() override;

  KeyboardControllerImpl(const KeyboardControllerImpl&) = delete;
  KeyboardControllerImpl& operator=(const KeyboardControllerImpl&) = delete;

  void ProcessEvent(const std::optional<EventController::KeyEventData>& event);
  void ApplyKeySymBinding(const KeySymMap& keysyms);

 public:
  void Update(ExceptionState& exception_state) override;
  bool IsPressed(const std::string& sym,
                 ExceptionState& exception_state) override;
  bool IsTriggered(const std::string& sym,
                   ExceptionState& exception_state) override;
  bool IsRepeated(const std::string& sym,
                  ExceptionState& exception_state) override;
  int32_t Dir4(ExceptionState& exception_state) override;
  int32_t Dir8(ExceptionState& exception_state) override;

  bool KeyPressed(int32_t scancode, ExceptionState& exception_state) override;
  bool KeyTriggered(int32_t scancode, ExceptionState& exception_state) override;
  bool KeyRepeated(int32_t scancode, ExceptionState& exception_state) override;
  std::string GetKeyName(int32_t scancode,
                         ExceptionState& exception_state) override;
  std::vector<int32_t> GetKeysFromFlag(
      const std::string& flag,
      ExceptionState& exception_state) override;
  void SetKeysFromFlag(const std::string& flag,
                       const std::vector<int32_t>& keys,
                       ExceptionState& exception_state) override;

  std::vector<int32_t> GetRecentPressed(
      ExceptionState& exception_state) override;
  std::vector<int32_t> GetRecentTriggered(
      ExceptionState& exception_state) override;
  std::vector<int32_t> GetRecentRepeated(
      ExceptionState& exception_state) override;

  bool Emulate(int32_t scancode,
               bool down,
               int32_t modifier,
               bool repeat,
               ExceptionState& exception_state) override;

  // Gamepad support
  void UpdateGamepad();
  void PollGamepadState();
  bool GamepadIsPressed(const std::string& sym);
  int16_t GetGamepadAxisValue(int32_t axis);
  std::string GetGamepadAxisName(int32_t axis);
  std::string GetGamepadButtonName(int32_t button);
  bool IsGamepadConnected(ExceptionState& exception_state) override;

  // Rumble / force feedback
  void RumbleGamepad(uint16_t low_freq,
                     uint16_t high_freq,
                     uint32_t duration_ms,
                     ExceptionState& exception_state) override;

  // ImGui settings page
  void CreateButtonGUISettings();

  // Ported from mkxp-z: configurable gamepad bindings
  void SetGamepadBindingList(const GamepadBindingList& bindings);
  const GamepadBindingList& GetGamepadBindingList() const;
  GamepadBindingList GetDefaultGamepadBindings() const;
  void ResetGamepadBindingsToDefault();

  // Input capture (called from main thread after event dispatch)
  void PollCapture();
  void StartCaptureFor(const std::string& sym, int32_t next_idx = -1);

 private:
  void UpdateDir4Internal();
  void UpdateDir8Internal();

  void TryReadBindingsInternal();
  void StorageBindingsInternal();

  KeySymMap key_bindings_;

  std::array<bool, SDL_SCANCODE_COUNT> raw_states_;
  std::array<KeyState, SDL_SCANCODE_COUNT> key_states_;
  std::array<KeyState, SDL_SCANCODE_COUNT> recent_key_states_;

  struct {
    int32_t active = 0;
    int32_t previous = 0;
  } dir4_state_;

  struct {
    int32_t active = 0;
  } dir8_state_;

  // Single gamepad support (mkxp-z compatible, first gamepad only)
  struct GamepadHandle {
    SDL_Gamepad* pad = nullptr;
    SDL_JoystickID id = 0;
  };
  std::vector<GamepadHandle> gamepads_;

  // Merged state (OR across all connected gamepads)
  int16_t gp_axes_[SDL_GAMEPAD_AXIS_COUNT] = {};
  int16_t gp_axes_prev_[SDL_GAMEPAD_AXIS_COUNT] = {};
  bool gp_buttons_[SDL_GAMEPAD_BUTTON_COUNT] = {};
  bool gp_buttons_prev_[SDL_GAMEPAD_BUTTON_COUNT] = {};
  int32_t gp_repeat_count_[SDL_GAMEPAD_BUTTON_COUNT] = {};
  int32_t gp_axis_repeat_[4] = {};  // DOWN, LEFT, RIGHT, UP

  // Configurable gamepad bindings (flat list, mkxp-z style)
  GamepadBindingList gp_bindings_;

  // Input capture state (for ImGui rebind UI)
  std::string capture_target_;
  GamepadSource capture_slot_;
  bool is_capturing_ = false;
  int32_t capture_next_idx_ = -1;

  // Edge-detection baselines for capture
  std::array<bool, SDL_SCANCODE_COUNT> capture_kb_baseline_{};
  std::array<bool, SDL_GAMEPAD_BUTTON_COUNT> capture_gp_btn_baseline_{};
  std::array<int16_t, SDL_GAMEPAD_AXIS_COUNT> capture_gp_axis_baseline_{};
};

}  // namespace content

#endif  //! CONTENT_INPUT_CONTROLLER_KEYBOARD_H_
