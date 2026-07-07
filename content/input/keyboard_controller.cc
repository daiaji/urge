// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/input/keyboard_controller.h"

#include "components/filesystem/io_service.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_timer.h"
#include "base/debug/logging.h"
#include "imgui/imgui.h"

#include "content/context/execution_context.h"
#include "content/worker/event_controller.h"
#include "content/profile/command_ids.h"

// SDL_GamepadButton names for UI display
static const char* kGamepadButtonNames[] = {
    "A", "B", "X", "Y", "Back", "Guide", "Start",
    "LS", "RS", "LB", "RB",
    "DPad_Up", "DPad_Down", "DPad_Left", "DPad_Right",
    "Misc1", "RightPaddle1", "LeftPaddle1", "RightPaddle2", "LeftPaddle2",
    "Touchpad", "Misc2", "Misc3", "Misc4", "Misc5", "Misc6",
};
static_assert(sizeof(kGamepadButtonNames) / sizeof(kGamepadButtonNames[0]) ==
              SDL_GAMEPAD_BUTTON_COUNT);

// SDL_GamepadAxis names for UI display
static const char* kGamepadAxisNames[] = {
    "LStick_X", "LStick_Y", "RStick_X", "RStick_Y",
    "LTrigger", "RTrigger",
};

namespace content {

using BindingSource = KeyboardControllerImpl::BindingSource;
using BindingEntry = KeyboardControllerImpl::BindingEntry;
using BindingList = KeyboardControllerImpl::BindingList;
using BS = BindingSource;

namespace {

const BindingEntry kDefaultGamepadBindings[] = {
    {"UP",    {BS::Type::GamepadButton, SDL_GAMEPAD_BUTTON_DPAD_UP, 0}},
    {"DOWN",  {BS::Type::GamepadButton, SDL_GAMEPAD_BUTTON_DPAD_DOWN, 0}},
    {"LEFT",  {BS::Type::GamepadButton, SDL_GAMEPAD_BUTTON_DPAD_LEFT, 0}},
    {"RIGHT", {BS::Type::GamepadButton, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 0}},
    {"C",     {BS::Type::GamepadButton, SDL_GAMEPAD_BUTTON_SOUTH, 0}},
    {"B",     {BS::Type::GamepadButton, SDL_GAMEPAD_BUTTON_EAST, 0}},
    {"A",     {BS::Type::GamepadButton, SDL_GAMEPAD_BUTTON_WEST, 0}},
    {"X",     {BS::Type::GamepadButton, SDL_GAMEPAD_BUTTON_NORTH, 0}},
    {"L",     {BS::Type::GamepadButton, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, 0}},
    {"R",     {BS::Type::GamepadButton, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 0}},
    {"Y",     {BS::Type::GamepadButton, SDL_GAMEPAD_BUTTON_LEFT_STICK, 0}},
    {"Z",     {BS::Type::GamepadButton, SDL_GAMEPAD_BUTTON_RIGHT_STICK, 0}},
    {"SHIFT", {BS::Type::GamepadButton, SDL_GAMEPAD_BUTTON_BACK, 0}},
    {"UP",    {BS::Type::GamepadAxis, SDL_GAMEPAD_AXIS_LEFTY, -1}},
    {"DOWN",  {BS::Type::GamepadAxis, SDL_GAMEPAD_AXIS_LEFTY, 1}},
    {"LEFT",  {BS::Type::GamepadAxis, SDL_GAMEPAD_AXIS_LEFTX, -1}},
    {"RIGHT", {BS::Type::GamepadAxis, SDL_GAMEPAD_AXIS_LEFTX, 1}},
};

static const int16_t kGamepadAxisThreshold = 0x4000;

static void LoadGameControllerDB() {
  SDL_IOStream* db = SDL_IOFromFile("gamecontrollerdb.txt", "r");
  if (db) {
    const int count = SDL_AddGamepadMappingsFromIO(db, true);
    if (count < 0) {
      LOG(WARNING) << "[Gamepad] Failed to load gamecontrollerdb.txt: "
                   << SDL_GetError();
    } else {
      LOG(INFO) << "[Gamepad] Loaded " << count
                << " mappings from gamecontrollerdb.txt";
    }
  }
}

const BindingEntry kDefaultKeyboardBindings[] = {
    {"DOWN",  {BS::Type::Key, SDL_SCANCODE_DOWN, 0}},
    {"LEFT",  {BS::Type::Key, SDL_SCANCODE_LEFT, 0}},
    {"RIGHT", {BS::Type::Key, SDL_SCANCODE_RIGHT, 0}},
    {"UP",    {BS::Type::Key, SDL_SCANCODE_UP, 0}},
    {"F5",    {BS::Type::Key, SDL_SCANCODE_F5, 0}},
    {"F6",    {BS::Type::Key, SDL_SCANCODE_F6, 0}},
    {"F7",    {BS::Type::Key, SDL_SCANCODE_F7, 0}},
    {"F8",    {BS::Type::Key, SDL_SCANCODE_F8, 0}},
    {"F9",    {BS::Type::Key, SDL_SCANCODE_F9, 0}},
    {"SHIFT", {BS::Type::Key, SDL_SCANCODE_LSHIFT, 0}},
    {"SHIFT", {BS::Type::Key, SDL_SCANCODE_RSHIFT, 0}},
    {"CTRL",  {BS::Type::Key, SDL_SCANCODE_LCTRL, 0}},
    {"CTRL",  {BS::Type::Key, SDL_SCANCODE_RCTRL, 0}},
    {"ALT",   {BS::Type::Key, SDL_SCANCODE_LALT, 0}},
    {"ALT",   {BS::Type::Key, SDL_SCANCODE_RALT, 0}},
    {"A",     {BS::Type::Key, SDL_SCANCODE_LSHIFT, 0}},
    {"B",     {BS::Type::Key, SDL_SCANCODE_ESCAPE, 0}},
    {"B",     {BS::Type::Key, SDL_SCANCODE_KP_0, 0}},
    {"B",     {BS::Type::Key, SDL_SCANCODE_X, 0}},
    {"C",     {BS::Type::Key, SDL_SCANCODE_SPACE, 0}},
    {"C",     {BS::Type::Key, SDL_SCANCODE_RETURN, 0}},
    {"X",     {BS::Type::Key, SDL_SCANCODE_A, 0}},
    {"Y",     {BS::Type::Key, SDL_SCANCODE_S, 0}},
    {"Z",     {BS::Type::Key, SDL_SCANCODE_D, 0}},
    {"L",     {BS::Type::Key, SDL_SCANCODE_Q, 0}},
    {"R",     {BS::Type::Key, SDL_SCANCODE_W, 0}},
};

const BindingEntry kKeyboardBindings1[] = {
    {"A", {BS::Type::Key, SDL_SCANCODE_Z, 0}},
    {"C", {BS::Type::Key, SDL_SCANCODE_C, 0}},
};

const BindingEntry kKeyboardBindings2[] = {
    {"C", {BS::Type::Key, SDL_SCANCODE_Z, 0}},
};

const std::string kArrowDirsSymbol[] = {
    "DOWN", "LEFT", "RIGHT", "UP",
};

const std::array<std::string, 12> kButtonItems = {
    "A", "B", "C", "X", "Y", "Z", "L", "R", "DOWN", "LEFT", "RIGHT", "UP",
};

// All symbols that should always appear in the binding UI,
// even when all their bindings have been cleared.
static const char* kAllSymbols[] = {
    "A", "B", "C", "X", "Y", "Z", "L", "R",
    "UP", "DOWN", "LEFT", "RIGHT",
    "SHIFT", "CTRL", "ALT",
    "F5", "F6", "F7", "F8", "F9",
    "START", "BACK",
};

static bool IsKeyPressed(const BindingSource& src,
                         const std::array<bool, SDL_SCANCODE_COUNT>& raw) {
  return src.type == BS::Type::Key && src.code < SDL_SCANCODE_COUNT &&
         raw[src.code];
}

static bool IsGamepadButtonActive(const BindingSource& src,
                                  const bool* buttons, size_t btn_count) {
  return src.type == BS::Type::GamepadButton &&
         src.code < btn_count && buttons[src.code];
}

static bool IsGamepadAxisActive(const BindingSource& src,
                                const int16_t* axes, size_t ax_count,
                                int16_t threshold) {
  if (src.type != BS::Type::GamepadAxis || src.code >= ax_count)
    return false;
  int16_t val = axes[src.code];
  return src.dir < 0 ? (val < -threshold) : (val > threshold);
}

static bool IsValidBindingSource(const BindingSource& source) {
  switch (source.type) {
    case BS::Type::Key:
      return source.code < SDL_SCANCODE_COUNT;
    case BS::Type::GamepadButton:
      return source.code < SDL_GAMEPAD_BUTTON_COUNT;
    case BS::Type::GamepadAxis:
      return source.code < SDL_GAMEPAD_AXIS_COUNT &&
             (source.dir == -1 || source.dir == 1);
    default:
      return false;
  }
}

}  // namespace

KeyboardControllerImpl::KeyboardControllerImpl(
    ExecutionContext* execution_context)
    : EngineObject(execution_context) {
  LoadGameControllerDB();

  for (size_t i = 0; i < std::size(kDefaultKeyboardBindings); ++i)
    bindings_.push_back(kDefaultKeyboardBindings[i]);

  if (execution_context->engine_profile->api_version ==
      ContentProfile::APIVersion::RGSS1)
    for (size_t i = 0; i < std::size(kKeyboardBindings1); ++i)
      bindings_.push_back(kKeyboardBindings1[i]);

  if (execution_context->engine_profile->api_version >=
      ContentProfile::APIVersion::RGSS2)
    for (size_t i = 0; i < std::size(kKeyboardBindings2); ++i)
      bindings_.push_back(kKeyboardBindings2[i]);

  for (size_t i = 0; i < std::size(kDefaultGamepadBindings); ++i)
    bindings_.push_back(kDefaultGamepadBindings[i]);

  LoadAllBindings();
}

KeyboardControllerImpl::~KeyboardControllerImpl() = default;

void KeyboardControllerImpl::ProcessEvent(
    const std::optional<EventController::KeyEventData>& event) {
  if (event) {
    if (event->is_repeat)
      return;
    raw_states_[event->scancode] = event->is_down;
  } else {
    for (auto& it : raw_states_)
      it = false;
  }
}

void KeyboardControllerImpl::ApplyKeyBinding(const BindingList& bindings) {
  bindings_ = bindings;
}

void KeyboardControllerImpl::Update(ExceptionState& exception_state) {
  auto* ec = context()->event_controller;
  const auto& gs = ec->gamepad_state();

  gp_connected_ = gs.connected;

  for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; ++a) {
    gp_axes_prev_[a] = gp_axes_[a];
    gp_axes_[a] = gs.axes[a];
  }
  for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
    gp_buttons_prev_[b] = gp_buttons_[b];
    gp_buttons_[b] = gs.buttons[b];
  }

  for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
    if (gp_buttons_[b]) {
      if (!gp_buttons_prev_[b]) {
        gp_repeat_count_[b] = 0;
        gp_button_time_[b] = SDL_GetTicks();
      } else {
        ++gp_repeat_count_[b];
      }
    } else {
      gp_repeat_count_[b] = 0;
      gp_button_time_[b] = 0;
    }
  }

  for (int d = 0; d < 4; ++d) {
    bool active = false;
    for (const auto& entry : bindings_) {
      if (entry.sym != kArrowDirsSymbol[d])
        continue;
      if (IsGamepadAxisActive(entry.source, gp_axes_,
                              SDL_GAMEPAD_AXIS_COUNT, kGamepadAxisThreshold)) {
        active = true;
        break;
      }
    }
    if (active)
      gp_dir_repeat_[d] = (gp_dir_repeat_[d] >= 0) ? (gp_dir_repeat_[d] + 1) : 0;
    else
      gp_dir_repeat_[d] = -1;
  }

  for (int32_t i = 0; i < SDL_SCANCODE_COUNT; ++i) {
    bool key_pressed = raw_states_[static_cast<SDL_Scancode>(i)];

    key_states_[i].trigger = !key_states_[i].pressed && key_pressed;
    key_states_[i].pressed = key_pressed;
    key_states_[i].repeat = false;

    if (key_states_[i].pressed) {
      ++key_states_[i].repeat_count;
      bool repeated = key_states_[i].repeat_count == 1 ||
                     (key_states_[i].repeat_count >= 23 &&
                      (key_states_[i].repeat_count + 1) % 6 == 0);
      key_states_[i].repeat = repeated;
    } else {
      key_states_[i].repeat_count = 0;
    }

    recent_key_states_[i].pressed = key_states_[i].pressed;
    recent_key_states_[i].trigger = key_states_[i].trigger;
    recent_key_states_[i].repeat = key_states_[i].repeat;
  }

  UpdateDir4Internal();
  UpdateDir8Internal();
}

bool KeyboardControllerImpl::IsPressed(const std::string& sym,
                                       ExceptionState& exception_state) {
  if (sym.empty())
    return false;

  for (const auto& entry : bindings_) {
    if (entry.sym != sym)
      continue;
    if (IsKeyPressed(entry.source, raw_states_))
      return true;
    if (IsGamepadButtonActive(entry.source, gp_buttons_,
                              SDL_GAMEPAD_BUTTON_COUNT))
      return true;
    if (IsGamepadAxisActive(entry.source, gp_axes_, SDL_GAMEPAD_AXIS_COUNT,
                            kGamepadAxisThreshold))
      return true;
  }
  return false;
}

bool KeyboardControllerImpl::IsTriggered(const std::string& sym,
                                         ExceptionState& exception_state) {
  if (sym.empty())
    return false;

  for (const auto& entry : bindings_) {
    if (entry.sym != sym)
      continue;
    if (IsKeyPressed(entry.source, raw_states_)) {
      if (key_states_[entry.source.code].trigger)
        return true;
      continue;
    }
    if (IsGamepadButtonActive(entry.source, gp_buttons_,
                              SDL_GAMEPAD_BUTTON_COUNT) &&
        entry.source.code < SDL_GAMEPAD_BUTTON_COUNT &&
        !gp_buttons_prev_[entry.source.code])
      return true;
    if (IsGamepadAxisActive(entry.source, gp_axes_, SDL_GAMEPAD_AXIS_COUNT,
                            kGamepadAxisThreshold)) {
      bool active = (entry.source.dir < 0)
                        ? (gp_axes_[entry.source.code] < -kGamepadAxisThreshold)
                        : (gp_axes_[entry.source.code] > kGamepadAxisThreshold);
      bool was_active = (entry.source.dir < 0)
                            ? (gp_axes_prev_[entry.source.code] < -kGamepadAxisThreshold)
                            : (gp_axes_prev_[entry.source.code] > kGamepadAxisThreshold);
      if (active && !was_active)
        return true;
    }
  }
  return false;
}

bool KeyboardControllerImpl::IsRepeated(const std::string& sym,
                                        ExceptionState& exception_state) {
  if (sym.empty())
    return false;

  for (const auto& entry : bindings_) {
    if (entry.sym != sym)
      continue;
    if (IsKeyPressed(entry.source, raw_states_)) {
      if (key_states_[entry.source.code].repeat)
        return true;
      continue;
    }
    if (IsGamepadButtonActive(entry.source, gp_buttons_,
                              SDL_GAMEPAD_BUTTON_COUNT)) {
      int rc = gp_repeat_count_[entry.source.code];
      if (rc == 0 || (rc >= 23 && (rc + 1) % 6 == 0))
        return true;
      continue;
    }
    if (IsGamepadAxisActive(entry.source, gp_axes_, SDL_GAMEPAD_AXIS_COUNT,
                            kGamepadAxisThreshold)) {
      for (int d = 0; d < 4; ++d) {
        if (sym == kArrowDirsSymbol[d] && gp_dir_repeat_[d] >= 0) {
          int rc = gp_dir_repeat_[d];
          if (rc == 0 || (rc >= 23 && (rc + 1) % 6 == 0))
            return true;
        }
      }
    }
  }
  return false;
}

int32_t KeyboardControllerImpl::Dir4(ExceptionState& exception_state) {
  return dir4_state_.active;
}

int32_t KeyboardControllerImpl::Dir8(ExceptionState& exception_state) {
  return dir8_state_.active;
}

bool KeyboardControllerImpl::KeyPressed(int32_t scancode,
                                        ExceptionState& exception_state) {
  if (scancode < 0 || scancode >= SDL_SCANCODE_COUNT)
    return false;
  return key_states_[scancode].pressed;
}

bool KeyboardControllerImpl::KeyTriggered(int32_t scancode,
                                          ExceptionState& exception_state) {
  if (scancode < 0 || scancode >= SDL_SCANCODE_COUNT)
    return false;
  return key_states_[scancode].trigger;
}

bool KeyboardControllerImpl::KeyRepeated(int32_t scancode,
                                         ExceptionState& exception_state) {
  if (scancode < 0 || scancode >= SDL_SCANCODE_COUNT)
    return false;
  return key_states_[scancode].repeat;
}

std::string KeyboardControllerImpl::GetKeyName(
    int32_t scancode,
    ExceptionState& exception_state) {
  if (scancode < 0 || scancode >= SDL_SCANCODE_COUNT)
    return "";
  SDL_Keycode key = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(scancode),
                                           SDL_KMOD_NONE, false);
  return std::string(SDL_GetKeyName(key));
}

std::vector<int32_t> KeyboardControllerImpl::GetKeysFromFlag(
    const std::string& flag,
    ExceptionState& exception_state) {
  std::vector<int32_t> out;
  if (flag.empty())
    return out;
  for (const auto& entry : bindings_)
    if (entry.sym == flag && entry.source.type == BS::Type::Key)
      out.push_back(entry.source.code);
  return out;
}

void KeyboardControllerImpl::SetKeysFromFlag(
    const std::string& flag,
    const std::vector<int32_t>& keys,
    ExceptionState& exception_state) {
  if (flag.empty())
    return;

  auto iter = std::remove_if(
      bindings_.begin(), bindings_.end(),
      [&](const BindingEntry& entry) {
        return entry.sym == flag && entry.source.type == BS::Type::Key;
      });
  bindings_.erase(iter, bindings_.end());

  for (const auto& key : keys) {
    if (key < 0 || key >= SDL_SCANCODE_COUNT) {
      LOG(WARNING) << "[Bindings] Ignoring invalid scancode: " << key;
      continue;
    }
    BindingEntry entry;
    entry.sym = flag;
    entry.source.type = BS::Type::Key;
    entry.source.code = static_cast<uint16_t>(key);
    entry.source.dir = 0;
    bindings_.push_back(std::move(entry));
  }

  SaveAllBindings();
}

std::vector<int32_t> KeyboardControllerImpl::GetRecentPressed(
    ExceptionState& exception_state) {
  std::vector<int32_t> out;
  for (size_t i = 0; i < recent_key_states_.size(); ++i)
    if (recent_key_states_[i].pressed)
      out.push_back(i);
  return out;
}

std::vector<int32_t> KeyboardControllerImpl::GetRecentTriggered(
    ExceptionState& exception_state) {
  std::vector<int32_t> out;
  for (size_t i = 0; i < recent_key_states_.size(); ++i)
    if (recent_key_states_[i].trigger)
      out.push_back(i);
  return out;
}

std::vector<int32_t> KeyboardControllerImpl::GetRecentRepeated(
    ExceptionState& exception_state) {
  std::vector<int32_t> out;
  for (size_t i = 0; i < recent_key_states_.size(); ++i)
    if (recent_key_states_[i].repeat)
      out.push_back(i);
  return out;
}

std::vector<uint8_t> KeyboardControllerImpl::GetRawKeyStates(
    ExceptionState& exception_state) {
  std::vector<uint8_t> out;
  out.reserve(raw_states_.size());
  for (bool pressed : raw_states_)
    out.push_back(pressed ? 1 : 0);
  return out;
}

bool KeyboardControllerImpl::Emulate(int32_t scancode,
                                     bool down,
                                     int32_t modifier,
                                     bool repeat,
                                     ExceptionState& exception_state) {
  SDL_Event emulate_event;
  emulate_event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
  emulate_event.key.timestamp = SDL_GetTicksNS();
  emulate_event.key.windowID = context()->window->GetWindowID();
  emulate_event.key.which = 0;
  emulate_event.key.scancode = static_cast<SDL_Scancode>(scancode);
  emulate_event.key.mod = modifier;
  emulate_event.key.raw = 0;
  emulate_event.key.down = down;
  emulate_event.key.repeat = repeat;
  emulate_event.key.key = SDL_GetKeyFromScancode(emulate_event.key.scancode,
                                                 emulate_event.key.mod, true);
  return SDL_PushEvent(&emulate_event);
}

void KeyboardControllerImpl::UpdateDir4Internal() {
  bool key_states_arr[std::size(kArrowDirsSymbol)] = {0};
  for (const auto& entry : bindings_) {
    if (entry.source.type != BS::Type::Key)
      continue;
    if (entry.source.code >= SDL_SCANCODE_COUNT)
      continue;
    for (size_t i = 0; i < std::size(kArrowDirsSymbol); ++i)
      if (entry.sym == kArrowDirsSymbol[i])
        key_states_arr[i] |= key_states_[entry.source.code].pressed;
  }

  for (size_t i = 0; i < std::size(kArrowDirsSymbol); ++i) {
    for (const auto& entry : bindings_) {
      if (entry.sym != kArrowDirsSymbol[i])
        continue;
      if (IsGamepadButtonActive(entry.source, gp_buttons_,
                                SDL_GAMEPAD_BUTTON_COUNT) ||
          IsGamepadAxisActive(entry.source, gp_axes_, SDL_GAMEPAD_AXIS_COUNT,
                              kGamepadAxisThreshold)) {
        key_states_arr[i] = true;
        break;
      }
    }
  }

  int32_t dir_flag = 0;
  const int32_t dir_flags_fix[] = {1 << 1, 1 << 2, 1 << 3, 1 << 4};
  const int32_t block_dir_flags[] = {dir_flags_fix[0] | dir_flags_fix[3],
                                     dir_flags_fix[1] | dir_flags_fix[2]};
  const int32_t other_dirs[][3] = {
      {1, 2, 3}, {0, 3, 2}, {0, 3, 1}, {1, 2, 0},
  };

  for (size_t i = 0; i < 4; ++i)
    dir_flag |= (key_states_arr[i] ? dir_flags_fix[i] : 0);

  if (dir_flag == block_dir_flags[0] || dir_flag == block_dir_flags[1]) {
    dir4_state_.active = 0;
    return;
  }

  if (dir4_state_.previous) {
    if (key_states_arr[dir4_state_.previous / 2 - 1]) {
      for (size_t i = 0; i < 3; ++i) {
        int32_t other_key = other_dirs[dir4_state_.previous / 2 - 1][i];
        if (!key_states_arr[other_key])
          continue;
        dir4_state_.active = (other_key + 1) * 2;
        return;
      }
    }
  }

  for (size_t i = 0; i < 4; ++i) {
    if (!key_states_arr[i])
      continue;
    dir4_state_.active = (i + 1) * 2;
    dir4_state_.previous = (i + 1) * 2;
    return;
  }

  dir4_state_.active = 0;
  dir4_state_.previous = 0;
}

void KeyboardControllerImpl::UpdateDir8Internal() {
  bool key_states_arr[std::size(kArrowDirsSymbol)] = {0};
  for (const auto& entry : bindings_) {
    if (entry.source.type != BS::Type::Key)
      continue;
    for (size_t i = 0; i < std::size(kArrowDirsSymbol); ++i)
      if (entry.sym == kArrowDirsSymbol[i])
        key_states_arr[i] |= key_states_[entry.source.code].pressed;
  }

  for (size_t i = 0; i < std::size(kArrowDirsSymbol); ++i) {
    for (const auto& entry : bindings_) {
      if (entry.sym != kArrowDirsSymbol[i])
        continue;
      if (IsGamepadButtonActive(entry.source, gp_buttons_,
                                SDL_GAMEPAD_BUTTON_COUNT) ||
          IsGamepadAxisActive(entry.source, gp_axes_, SDL_GAMEPAD_AXIS_COUNT,
                              kGamepadAxisThreshold)) {
        key_states_arr[i] = true;
        break;
      }
    }
  }

  static const int32_t combos[4][4] = {
      {2, 1, 3, 0}, {1, 4, 0, 7}, {3, 0, 6, 9}, {0, 7, 9, 8}};
  const int32_t other_dirs[][3] = {
      {1, 2, 3}, {0, 3, 2}, {0, 3, 1}, {1, 2, 0},
  };

  dir8_state_.active = 0;

  for (size_t i = 0; i < 4; ++i) {
    if (!key_states_arr[i])
      continue;
    for (int32_t j = 0; j < 3; ++j) {
      int32_t other_key = other_dirs[i][j];
      if (!key_states_arr[other_key])
        continue;
      dir8_state_.active = combos[i][other_key];
      return;
    }
    dir8_state_.active = (i + 1) * 2;
    return;
  }
}

// --- Unified binding persistence (text format) ---

#define BIND_CONFIG_SUFFIX "_bind.txt"

static std::string BindConfigPath(ExecutionContext* ctx) {
  std::string path = ctx->engine_profile->program_name;
  path += BIND_CONFIG_SUFFIX;
  path += std::to_string(
      static_cast<int32_t>(ctx->engine_profile->api_version));
  return path;
}

void KeyboardControllerImpl::LoadAllBindings() {
  std::string filepath = BindConfigPath(context());

  LOG(INFO) << "[Bindings] Reading bindings from " << filepath;

  filesystem::IOState io_state;
  auto* fstream = context()->io_service->OpenReadRaw(filepath, &io_state);
  if (!fstream)
    return;

  std::string content;
  {
    char buf[4096];
    size_t n;
    while ((n = SDL_ReadIO(fstream, buf, sizeof(buf))) > 0)
      content.append(buf, n);
  }
  SDL_CloseIO(fstream);

  BindingList loaded_bindings;

  size_t pos = 0;
  while (pos < content.size()) {
    size_t eol = content.find('\n', pos);
    if (eol == std::string::npos) eol = content.size();
    std::string line = content.substr(pos, eol - pos);
    pos = eol + 1;

    size_t start = line.find_first_not_of(" \t\r");
    if (start == std::string::npos) continue;
    size_t end = line.find_last_not_of(" \t\r");
    line = line.substr(start, end - start + 1);

    if (line.empty() || line[0] == '#') continue;

    size_t eq = line.find('=');
    if (eq == std::string::npos) {
      LOG(WARNING) << "[Bindings] Skipping malformed line: " << line;
      continue;
    }

    std::string sym = line.substr(0, eq);
    std::string val = line.substr(eq + 1);
    sym = sym.substr(0, sym.find_last_not_of(" \t") + 1);
    const size_t val_start = val.find_first_not_of(" \t");
    if (val_start == std::string::npos) {
      LOG(WARNING) << "[Bindings] Skipping empty binding value: " << line;
      continue;
    }
    val = val.substr(val_start);

    BindingEntry entry;
    entry.sym = sym;

    if (val.starts_with("Key:") || val.starts_with("key:")) {
      entry.source.type = BS::Type::Key;
      try {
        int code = std::stoi(val.substr(4));
        if (code < 0 || code >= SDL_SCANCODE_COUNT) {
          LOG(WARNING) << "[Bindings] Key scancode out of range: " << val;
          continue;
        }
        entry.source.code = static_cast<uint16_t>(code);
      } catch (...) {
        LOG(WARNING) << "[Bindings] Invalid key scancode: " << val;
        continue;
      }
      entry.source.dir = 0;
    } else if (val.starts_with("Btn:") || val.starts_with("btn:")) {
      entry.source.type = BS::Type::GamepadButton;
      try {
        int code = std::stoi(val.substr(4));
        if (code < 0 || code >= SDL_GAMEPAD_BUTTON_COUNT) {
          LOG(WARNING) << "[Bindings] Button index out of range: " << val;
          continue;
        }
        entry.source.code = static_cast<uint8_t>(code);
      } catch (...) {
        LOG(WARNING) << "[Bindings] Invalid button index: " << val;
        continue;
      }
      entry.source.dir = 0;
    } else if (val.starts_with("Axis:") || val.starts_with("axis:")) {
      entry.source.type = BS::Type::GamepadAxis;
      size_t colon2 = val.rfind(':');
      if (colon2 == std::string::npos || colon2 <= 5) {
        LOG(WARNING) << "[Bindings] Skipping malformed Axis entry: "
                     << sym << " = " << val;
        continue;
      }
      try {
        int code = std::stoi(val.substr(5, colon2 - 5));
        if (code < 0 || code >= SDL_GAMEPAD_AXIS_COUNT) {
          LOG(WARNING) << "[Bindings] Axis index out of range: " << val;
          continue;
        }
        entry.source.code = static_cast<uint8_t>(code);
      } catch (...) {
        LOG(WARNING) << "[Bindings] Invalid axis index: " << val;
        continue;
      }
      if (val.size() <= colon2 + 1 ||
          (val[colon2 + 1] != '-' && val[colon2 + 1] != '+')) {
        LOG(WARNING) << "[Bindings] Invalid axis direction: " << val;
        continue;
      }
      entry.source.dir = val[colon2 + 1] == '-' ? -1 : 1;
    } else {
      LOG(WARNING) << "[Bindings] Skipping unrecognized format: "
                   << sym << " = " << val;
      continue;
    }

    if (!IsValidBindingSource(entry.source)) {
      LOG(WARNING) << "[Bindings] Skipping invalid binding: " << sym;
      continue;
    }
    loaded_bindings.push_back(std::move(entry));
  }

  if (loaded_bindings.empty()) {
    LOG(WARNING) << "[Bindings] No valid bindings loaded; keeping defaults.";
    return;
  }

  bindings_ = std::move(loaded_bindings);
  LOG(INFO) << "[Bindings] Loaded " << bindings_.size() << " bindings.";
}

void KeyboardControllerImpl::SaveAllBindings() {
  std::string filepath = BindConfigPath(context());

  LOG(INFO) << "[Bindings] Saving bindings to " << filepath;

  filesystem::IOState io_state;
  auto* fstream = context()->io_service->OpenWrite(filepath, &io_state);
  if (!fstream)
    return;

  std::string out;
  out += "# URGE Bindings\n";
  out += "# Format: SYMBOL = Key:SCANCODE  or  SYMBOL = Btn:INDEX  or  SYMBOL = Axis:INDEX:[+-]\n";
  out += "# Lines starting with # are ignored.\n\n";

  for (const auto& entry : bindings_) {
    out += entry.sym;
    out += " = ";
    switch (entry.source.type) {
      case BS::Type::Key:
        out += "Key:";
        out += std::to_string(entry.source.code);
        break;
      case BS::Type::GamepadButton:
        out += "Btn:";
        out += std::to_string(entry.source.code);
        break;
      case BS::Type::GamepadAxis:
        out += "Axis:";
        out += std::to_string(entry.source.code);
        out += ':';
        out += (entry.source.dir < 0) ? '-' : '+';
        break;
      default:
        continue;
    }
    out += '\n';
  }

  SDL_WriteIO(fstream, out.data(), out.size());
  SDL_CloseIO(fstream);

  LOG(INFO) << "[Bindings] Saved " << bindings_.size() << " bindings.";
}

// --- Configurable gamepad bindings API ---

void KeyboardControllerImpl::SetBindingList(const BindingList& bindings) {
  for (const auto& entry : bindings) {
    auto it = std::find_if(bindings_.begin(), bindings_.end(),
        [&entry](const BindingEntry& e) {
          return e.sym == entry.sym && e.source.type == entry.source.type &&
                 e.source.code == entry.source.code &&
                 e.source.dir == entry.source.dir;
        });
    if (it == bindings_.end())
      bindings_.push_back(entry);
  }

  SaveAllBindings();
}

const KeyboardControllerImpl::BindingList&
KeyboardControllerImpl::GetBindingList() const {
  return bindings_;
}

KeyboardControllerImpl::BindingList
KeyboardControllerImpl::GetDefaultBindings() const {
  BindingList defaults;
  for (size_t i = 0; i < std::size(kDefaultKeyboardBindings); ++i)
    defaults.push_back(kDefaultKeyboardBindings[i]);
  if (context()->engine_profile->api_version ==
      ContentProfile::APIVersion::RGSS1)
    for (size_t i = 0; i < std::size(kKeyboardBindings1); ++i)
      defaults.push_back(kKeyboardBindings1[i]);
  if (context()->engine_profile->api_version >=
      ContentProfile::APIVersion::RGSS2)
    for (size_t i = 0; i < std::size(kKeyboardBindings2); ++i)
      defaults.push_back(kKeyboardBindings2[i]);
  for (size_t i = 0; i < std::size(kDefaultGamepadBindings); ++i)
    defaults.push_back(kDefaultGamepadBindings[i]);
  return defaults;
}

void KeyboardControllerImpl::ResetBindingsToDefault() {
  bindings_.clear();

  for (size_t i = 0; i < std::size(kDefaultKeyboardBindings); ++i)
    bindings_.push_back(kDefaultKeyboardBindings[i]);
  if (context()->engine_profile->api_version ==
      ContentProfile::APIVersion::RGSS1)
    for (size_t i = 0; i < std::size(kKeyboardBindings1); ++i)
      bindings_.push_back(kKeyboardBindings1[i]);
  if (context()->engine_profile->api_version >=
      ContentProfile::APIVersion::RGSS2)
    for (size_t i = 0; i < std::size(kKeyboardBindings2); ++i)
      bindings_.push_back(kKeyboardBindings2[i]);
  for (size_t i = 0; i < std::size(kDefaultGamepadBindings); ++i)
    bindings_.push_back(kDefaultGamepadBindings[i]);

  SaveAllBindings();
}

// --- Input capture for rebind UI ---

static const char* kGamepadConfigurableSyms[] = {
    "A", "B", "C", "X", "Y", "Z", "L", "R",
    "UP", "DOWN", "LEFT", "RIGHT",
    "SHIFT", "CTRL", "START", "BACK",
};

static const char* NextCaptureSym(int32_t idx) {
  if (idx < 0 || static_cast<size_t>(idx) >= std::size(kGamepadConfigurableSyms))
    return nullptr;
  return kGamepadConfigurableSyms[idx];
}

void KeyboardControllerImpl::StartCaptureFor(const std::string& sym,
                                             int32_t next_idx) {
  capture_target_ = sym;
  capture_next_idx_ = next_idx;
  is_capturing_ = true;
  capture_slot_ = BindingSource();
  for (int i = 0; i < SDL_SCANCODE_COUNT; ++i)
    capture_kb_baseline_[i] = raw_states_[i];
  for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; ++i)
    capture_gp_btn_baseline_[i] = gp_buttons_[i];
  for (int i = 0; i < SDL_GAMEPAD_AXIS_COUNT; ++i)
    capture_gp_axis_baseline_[i] = gp_axes_[i];
}

void KeyboardControllerImpl::PollCapture() {
  if (!is_capturing_)
    return;

  if (gp_connected_) {
    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
      if (gp_buttons_[b] && !capture_gp_btn_baseline_[b]) {
        capture_slot_.type = BS::Type::GamepadButton;
        capture_slot_.code = static_cast<uint8_t>(b);
        capture_slot_.dir = 0;

        auto it = std::find_if(bindings_.begin(), bindings_.end(),
            [this](const BindingEntry& e) {
              return e.sym == capture_target_ && e.source == capture_slot_;
            });
        if (it == bindings_.end())
          bindings_.push_back({capture_target_, capture_slot_});

        is_capturing_ = false;
        SaveAllBindings();

        const char* next = NextCaptureSym(capture_next_idx_);
        if (next)
          StartCaptureFor(next, capture_next_idx_ + 1);
        return;
      }
    }

    for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; ++a) {
      int16_t val = gp_axes_[a];
      int16_t base = capture_gp_axis_baseline_[a];
      bool now_active = (val < -kGamepadAxisThreshold ||
                         val > kGamepadAxisThreshold);
      bool was_active = (base < -kGamepadAxisThreshold ||
                         base > kGamepadAxisThreshold);
      if (now_active && !was_active) {
        capture_slot_.type = BS::Type::GamepadAxis;
        capture_slot_.code = static_cast<uint8_t>(a);
        capture_slot_.dir = (val < 0) ? -1 : 1;

        auto it = std::find_if(bindings_.begin(), bindings_.end(),
            [this](const BindingEntry& e) {
              return e.sym == capture_target_ && e.source == capture_slot_;
            });
        if (it == bindings_.end())
          bindings_.push_back({capture_target_, capture_slot_});

        is_capturing_ = false;
        SaveAllBindings();

        const char* next = NextCaptureSym(capture_next_idx_);
        if (next)
          StartCaptureFor(next, capture_next_idx_ + 1);
        return;
      }
    }
  }

  for (int s = 0; s < SDL_SCANCODE_COUNT; ++s) {
    if (raw_states_[s] && !capture_kb_baseline_[s]) {
      capture_slot_.type = BS::Type::Key;
      capture_slot_.code = static_cast<uint16_t>(s);
      capture_slot_.dir = 0;

      auto it = std::find_if(bindings_.begin(), bindings_.end(),
          [this](const BindingEntry& e) {
            return e.sym == capture_target_ && e.source == capture_slot_;
          });
      if (it == bindings_.end())
        bindings_.push_back({capture_target_, capture_slot_});

      is_capturing_ = false;
      SaveAllBindings();

      const char* next = NextCaptureSym(capture_next_idx_);
      if (next)
        StartCaptureFor(next, capture_next_idx_ + 1);
      return;
    }
  }
}

// --- mkxp-z style raw gamepad API ---

int16_t KeyboardControllerImpl::GetGamepadAxisValue(int32_t axis) {
  if (!gp_connected_ || axis < 0 || axis >= SDL_GAMEPAD_AXIS_COUNT)
    return 0;
  return gp_axes_[axis];
}

std::string KeyboardControllerImpl::GetGamepadAxisName(int32_t axis) {
  if (axis >= 0 && axis < SDL_GAMEPAD_AXIS_COUNT)
    return kGamepadAxisNames[axis];
  return "Unknown";
}

std::string KeyboardControllerImpl::GetGamepadButtonName(int32_t button) {
  if (button >= 0 && button < static_cast<int32_t>(
          sizeof(kGamepadButtonNames) / sizeof(kGamepadButtonNames[0])))
    return kGamepadButtonNames[button];
  return "Unknown";
}

bool KeyboardControllerImpl::IsGamepadConnected(
    ExceptionState& exception_state) {
  return gp_connected_;
}

bool KeyboardControllerImpl::RumbleGamepad(uint16_t low_freq,
                                            uint16_t high_freq,
                                            uint32_t duration_ms,
                                            ExceptionState& exception_state) {
  auto* pad = context()->event_controller->gamepad_handle();
  if (!pad)
    return false;
  return SDL_RumbleGamepad(pad, low_freq, high_freq, duration_ms);
}

std::string KeyboardControllerImpl::GetGamepadName(
    ExceptionState& exception_state) {
  auto* pad = context()->event_controller->gamepad_handle();
  if (!pad)
    return "";
  const char* name = SDL_GetGamepadName(pad);
  return name ? std::string(name) : "";
}

int32_t KeyboardControllerImpl::GetGamepadPowerLevel(
    ExceptionState& exception_state) {
  auto* pad = context()->event_controller->gamepad_handle();
  if (!pad)
    return SDL_POWERSTATE_UNKNOWN;
  int percent;
  return SDL_GetGamepadPowerInfo(pad, &percent);
}

std::vector<float> KeyboardControllerImpl::GetGamepadAxisLeft(
    ExceptionState& exception_state) {
  if (!gp_connected_)
    return {0.0f, 0.0f};
  return {
      static_cast<float>(gp_axes_[SDL_GAMEPAD_AXIS_LEFTX]) / 32767.0f,
      static_cast<float>(gp_axes_[SDL_GAMEPAD_AXIS_LEFTY]) / 32767.0f,
  };
}

std::vector<float> KeyboardControllerImpl::GetGamepadAxisRight(
    ExceptionState& exception_state) {
  if (!gp_connected_)
    return {0.0f, 0.0f};
  return {
      static_cast<float>(gp_axes_[SDL_GAMEPAD_AXIS_RIGHTX]) / 32767.0f,
      static_cast<float>(gp_axes_[SDL_GAMEPAD_AXIS_RIGHTY]) / 32767.0f,
  };
}

std::vector<float> KeyboardControllerImpl::GetGamepadAxisTrigger(
    ExceptionState& exception_state) {
  if (!gp_connected_)
    return {0.0f, 0.0f};
  return {
      static_cast<float>(gp_axes_[SDL_GAMEPAD_AXIS_LEFT_TRIGGER]) / 32767.0f,
      static_cast<float>(gp_axes_[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER]) / 32767.0f,
  };
}

bool KeyboardControllerImpl::GamepadPressEx(
    int32_t button, ExceptionState& exception_state) {
  return gp_connected_ && button >= 0 &&
         button < SDL_GAMEPAD_BUTTON_COUNT &&
         gp_buttons_[button];
}

bool KeyboardControllerImpl::GamepadTriggerEx(
    int32_t button, ExceptionState& exception_state) {
  return gp_connected_ && button >= 0 &&
         button < SDL_GAMEPAD_BUTTON_COUNT &&
         gp_buttons_[button] && !gp_buttons_prev_[button];
}

bool KeyboardControllerImpl::GamepadRepeatEx(
    int32_t button, ExceptionState& exception_state) {
  if (!gp_connected_ || button < 0 || button >= SDL_GAMEPAD_BUTTON_COUNT ||
      !gp_buttons_[button])
    return false;
  int rc = gp_repeat_count_[button];
  return rc == 0 || (rc >= 23 && (rc + 1) % 6 == 0);
}

bool KeyboardControllerImpl::GamepadReleaseEx(
    int32_t button, ExceptionState& exception_state) {
  return button >= 0 &&
         button < SDL_GAMEPAD_BUTTON_COUNT &&
         !gp_buttons_[button] && gp_buttons_prev_[button];
}

int32_t KeyboardControllerImpl::GamepadRepeatCountEx(
    int32_t button, ExceptionState& exception_state) {
  if (!gp_connected_ || button < 0 || button >= SDL_GAMEPAD_BUTTON_COUNT)
    return 0;
  return gp_repeat_count_[button];
}

double KeyboardControllerImpl::GamepadButtonTimeEx(
    int32_t button, ExceptionState& exception_state) {
  if (!gp_connected_ || button < 0 || button >= SDL_GAMEPAD_BUTTON_COUNT ||
      !gp_buttons_[button])
    return 0.0;
  return static_cast<double>(SDL_GetTicks() - gp_button_time_[button]) / 1000.0;
}

std::vector<uint8_t> KeyboardControllerImpl::GetGamepadRawButtonStates(
    ExceptionState& exception_state) {
  std::vector<uint8_t> out(SDL_GAMEPAD_BUTTON_COUNT);
  for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; ++i)
    out[i] = gp_buttons_[i] ? 1 : 0;
  return out;
}

std::vector<float> KeyboardControllerImpl::GetGamepadRawAxes(
    ExceptionState& exception_state) {
  std::vector<float> out(SDL_GAMEPAD_AXIS_COUNT);
  for (int i = 0; i < SDL_GAMEPAD_AXIS_COUNT; ++i)
    out[i] = static_cast<float>(gp_axes_[i]) / 32767.0f;
  return out;
}

// --- ImGui settings UI ---

static const char* SourceTypeToString(BindingSource::Type type) {
  switch (type) {
    case BS::Type::GamepadButton:
      return "Btn";
    case BS::Type::GamepadAxis:
      return "Axis";
    default:
      return "?";
  }
}

static const char* SourceDescription(const BindingSource& src) {
  static char buf[64];
  switch (src.type) {
    case BS::Type::Key: {
      const char* name = SDL_GetScancodeName(
          static_cast<SDL_Scancode>(src.code));
      return name ? name : "?";
    }
    case BS::Type::GamepadButton:
      if (src.code < sizeof(kGamepadButtonNames) / sizeof(kGamepadButtonNames[0]))
        return kGamepadButtonNames[src.code];
      snprintf(buf, sizeof(buf), "Btn#%u", src.code);
      return buf;
    case BS::Type::GamepadAxis:
      if (src.code < SDL_GAMEPAD_AXIS_COUNT) {
        snprintf(buf, sizeof(buf), "%s%c",
                 kGamepadAxisNames[src.code], src.dir < 0 ? '-' : '+');
        return buf;
      }
      snprintf(buf, sizeof(buf), "Axis#%u%c", src.code, src.dir < 0 ? '-' : '+');
      return buf;
    default:
      return "";
  }
}

void KeyboardControllerImpl::CreateButtonGUISettings() {
  auto* i18n = context()->i18n_profile;

  if (ImGui::CollapsingHeader(
          i18n->GetI18NString(IDS_SETTINGS_INPUT, "Input").c_str())) {
    ImGui::TextWrapped("%s",
        i18n->GetI18NString(IDS_INPUT_REBIND_HINT,
            "Left click a slot to rebind, right click to clear.")
            .c_str());

    if (!gp_connected_) {
      ImGui::TextColored(ImVec4(1,1,0,1), "%s",
          i18n->GetI18NString(IDS_INPUT_NO_GAMEPAD,
              "No gamepad connected. Connect a gamepad to configure bindings.")
              .c_str());
    }

    if (is_capturing_)
      ImGui::OpenPopup("##GamepadCapture");

    if (ImGui::BeginPopupModal("##GamepadCapture", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Press a gamepad button or move an axis for \"%s\"...",
                   capture_target_.c_str());
      ImGui::Text("(or press ESC to cancel)");

      if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        is_capturing_ = false;
        capture_next_idx_ = -1;
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    if (ImGui::BeginTable("##GPBindings", 3,
                          ImGuiTableFlags_Borders |
                              ImGuiTableFlags_SizingFixedFit |
                              ImGuiTableFlags_NoHostExtendX)) {
      ImGui::TableSetupColumn("Button", ImGuiTableColumnFlags_WidthFixed);
      ImGui::TableSetupColumn("Bindings", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);

      std::vector<std::string> all_syms;
      for (const auto& s : kAllSymbols)
        all_syms.push_back(s);

      for (const auto& sym : all_syms) {
        ImGui::TableNextRow();
        ImGui::PushID(sym.c_str());

        bool clear_bindings = false;

        // Column 0: Button name
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%s", sym.c_str());
        if (ImGui::IsItemHovered())
          ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                 ImColor(60, 60, 60));

        // Column 1: Bindings — right-click on individual text removes that binding
        ImGui::TableSetColumnIndex(1);

        std::vector<BindingSource> sources;
        for (const auto& entry : bindings_) {
          if (entry.sym == sym)
            sources.push_back(entry.source);
        }

        // Record column 1 cell bounds for empty-area click detection
        ImVec2 col1_min = ImGui::GetCursorScreenPos();
        float col1_w = ImGui::GetContentRegionAvail().x;

        if (sources.empty()) {
          ImGui::TextDisabled("(unbound)");
          ImVec2 mouse = ImGui::GetMousePos();
          ImVec2 col1_max(col1_min.x + col1_w, col1_min.y + ImGui::GetTextLineHeight());
          if (mouse.x >= col1_min.x && mouse.x <= col1_max.x &&
              mouse.y >= col1_min.y && mouse.y <= col1_max.y) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                   ImColor(60, 60, 60));
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
              clear_bindings = true;
          }
        } else {
          bool first_src = true;
          bool text_hovered = false;
          for (size_t si = 0; si < sources.size(); ++si) {
            if (!first_src) ImGui::SameLine();
            first_src = false;
            const auto& src = sources[si];
            const char* label = SourceDescription(src);
            ImVec2 text_pos = ImGui::GetCursorScreenPos();
            ImVec2 text_size = ImGui::CalcTextSize(label);
            ImVec2 mouse = ImGui::GetMousePos();
            bool hovered = mouse.x >= text_pos.x &&
                           mouse.x <= text_pos.x + text_size.x &&
                           mouse.y >= text_pos.y &&
                           mouse.y <= text_pos.y + text_size.y;
            if (hovered) {
              text_hovered = true;
              ImGui::GetWindowDrawList()->AddRectFilled(
                  text_pos, ImVec2(text_pos.x + text_size.x,
                                   text_pos.y + text_size.y),
                  IM_COL32(80, 80, 70, 255));
              if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                for (auto it = bindings_.begin(); it != bindings_.end(); ++it) {
                  if (it->sym == sym && it->source == src) {
                    bindings_.erase(it);
                    break;
                  }
                }
                SaveAllBindings();
              }
            }
            ImGui::Text("%s", label);
          }
          // Empty area: check mouse within column 1 cell but not on text
          if (!text_hovered) {
            ImVec2 mouse = ImGui::GetMousePos();
            ImVec2 col1_max(col1_min.x + col1_w,
                            col1_min.y + ImGui::GetTextLineHeight());
            if (mouse.x >= col1_min.x && mouse.x <= col1_max.x &&
                mouse.y >= col1_min.y && mouse.y <= col1_max.y) {
              ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                     ImColor(60, 60, 60));
              if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                for (auto it = bindings_.begin(); it != bindings_.end();) {
                  if (it->sym == sym)
                    it = bindings_.erase(it);
                  else
                    ++it;
                }
                SaveAllBindings();
              }
            }
          }
        }

        // Column 2: + button
        ImGui::TableSetColumnIndex(2);

        if (is_capturing_) {
          ImGui::BeginDisabled();
          ImGui::SmallButton("+");
          ImGui::EndDisabled();
        } else {
          if (ImGui::SmallButton("+")) {
            int32_t next = -1;
            for (size_t i = 0; i < std::size(kGamepadConfigurableSyms); ++i)
              if (sym == kGamepadConfigurableSyms[i]) {
                next = static_cast<int32_t>(i + 1);
                break;
              }
            StartCaptureFor(sym, next);
          }
        }

        // Right-click on button name clears ALL bindings for this symbol
        if (clear_bindings) {
          for (auto it = bindings_.begin(); it != bindings_.end();) {
            if (it->sym == sym)
              it = bindings_.erase(it);
            else
              ++it;
          }
          SaveAllBindings();
        }

        ImGui::PopID();
      }

      ImGui::EndTable();
    }

    ImGui::Separator();
    if (ImGui::Button(i18n->GetI18NString(IDS_BUTTON_RESET_SETTINGS, "Reset")
                           .c_str())) {
      ResetBindingsToDefault();
    }
  }
}

}  // namespace content
