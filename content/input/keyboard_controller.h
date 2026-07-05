// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_INPUT_KEYBOARD_CONTROLLER_H_
#define CONTENT_INPUT_KEYBOARD_CONTROLLER_H_

#include <optional>
#include <unordered_map>

#include "SDL3/SDL_gamepad.h"

#include "content/context/engine_object.h"
#include "content/profile/content_profile.h"
#include "content/profile/i18n_profile.h"
#include "content/public/engine_input.h"
#include "content/worker/event_controller.h"
#include "ui/widget/widget.h"

namespace content {

class KeyboardControllerImpl : public Input, public EngineObject {
 public:
  // Unified binding source (keyboard + gamepad, mkxp-z style)
  struct BindingSource {
    enum class Type : uint8_t { Invalid, Key, GamepadButton, GamepadAxis };
    Type type = Type::Invalid;
    uint16_t code = 0;  // SDL_Scancode or SDL_GamepadButton or SDL_GamepadAxis
    int8_t dir = 0;     // for axis: -1 or +1

    bool operator==(const BindingSource& o) const {
      return type == o.type && code == o.code && dir == o.dir;
    }
    bool operator!=(const BindingSource& o) const { return !(*this == o); }
  };

  struct BindingEntry {
    std::string sym;
    BindingSource source;
  };
  using BindingList = std::vector<BindingEntry>;

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
  void ApplyKeyBinding(const BindingList& bindings);

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

  std::vector<uint8_t> GetRawKeyStates(
      ExceptionState& exception_state) override;

  bool Emulate(int32_t scancode,
               bool down,
               int32_t modifier,
               bool repeat,
               ExceptionState& exception_state) override;

  // Gamepad support (event-driven, reads from EventController)
  bool IsGamepadConnected(ExceptionState& exception_state) override;

  // Rumble / force feedback
  void RumbleGamepad(uint16_t low_freq,
                     uint16_t high_freq,
                     uint32_t duration_ms,
                     ExceptionState& exception_state) override;

  // ImGui settings page
  void CreateButtonGUISettings();

  // Configurable bindings (unified)
  void SetBindingList(const BindingList& bindings);
  const BindingList& GetBindingList() const;
  BindingList GetDefaultBindings() const;
  void ResetBindingsToDefault();

  // Input capture (called from main thread after event dispatch)
  void PollCapture();
  void StartCaptureFor(const std::string& sym, int32_t next_idx = -1);

  // mkxp-z style raw gamepad access
  int16_t GetGamepadAxisValue(int32_t axis);
  std::string GetGamepadAxisName(int32_t axis);
  std::string GetGamepadButtonName(int32_t button);
  std::string GetGamepadName(ExceptionState& exception_state) override;
  int32_t GetGamepadPowerLevel(ExceptionState& exception_state) override;
  std::vector<float> GetGamepadAxisLeft(ExceptionState& exception_state) override;
  std::vector<float> GetGamepadAxisRight(ExceptionState& exception_state) override;
  std::vector<float> GetGamepadAxisTrigger(ExceptionState& exception_state) override;
  bool GamepadPressEx(int32_t button, ExceptionState& exception_state) override;
  bool GamepadTriggerEx(int32_t button, ExceptionState& exception_state) override;
  bool GamepadRepeatEx(int32_t button, ExceptionState& exception_state) override;
  bool GamepadReleaseEx(int32_t button, ExceptionState& exception_state) override;
  int32_t GamepadRepeatCountEx(int32_t button, ExceptionState& exception_state) override;
  double GamepadButtonTimeEx(int32_t button, ExceptionState& exception_state) override;
  std::vector<uint8_t> GetGamepadRawButtonStates(
      ExceptionState& exception_state) override;
  std::vector<float> GetGamepadRawAxes(
      ExceptionState& exception_state) override;

 private:
  void UpdateDir4Internal();
  void UpdateDir8Internal();

  void LoadAllBindings();
  void SaveAllBindings();

  BindingList bindings_;

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

  // Gamepad state (copied from EventController each frame for edge detection)
  int16_t gp_axes_[SDL_GAMEPAD_AXIS_COUNT] = {};
  int16_t gp_axes_prev_[SDL_GAMEPAD_AXIS_COUNT] = {};
  bool gp_buttons_[SDL_GAMEPAD_BUTTON_COUNT] = {};
  bool gp_buttons_prev_[SDL_GAMEPAD_BUTTON_COUNT] = {};
  int32_t gp_repeat_count_[SDL_GAMEPAD_BUTTON_COUNT] = {};
  uint64_t gp_button_time_[SDL_GAMEPAD_BUTTON_COUNT] = {};  // press timestamp (ms)
  int32_t gp_dir_repeat_[4] = {};  // DOWN, LEFT, RIGHT, UP repeat frame count (-1 if inactive)
  bool gp_connected_ = false;

  // Input capture state (for ImGui rebind UI)
  std::string capture_target_;
  BindingSource capture_slot_;
  bool is_capturing_ = false;
  int32_t capture_next_idx_ = -1;

  // Edge-detection baselines for capture
  std::array<bool, SDL_SCANCODE_COUNT> capture_kb_baseline_{};
  std::array<bool, SDL_GAMEPAD_BUTTON_COUNT> capture_gp_btn_baseline_{};
  std::array<int16_t, SDL_GAMEPAD_AXIS_COUNT> capture_gp_axis_baseline_{};
};

}  // namespace content

#endif  //! CONTENT_INPUT_CONTROLLER_KEYBOARD_H_
