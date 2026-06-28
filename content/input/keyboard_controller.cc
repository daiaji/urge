// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/input/keyboard_controller.h"

#include "components/filesystem/io_service.h"

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"
#include "imgui/imgui.h"

#include "content/context/execution_context.h"
#include "content/profile/command_ids.h"

#define INPUT_CONFIG_SUBFIX ".cfg"

namespace content {

namespace {

const KeyboardControllerImpl::KeyBinding kDefaultKeyboardBindings[] = {
    {"DOWN", SDL_SCANCODE_DOWN},    {"LEFT", SDL_SCANCODE_LEFT},
    {"RIGHT", SDL_SCANCODE_RIGHT},  {"UP", SDL_SCANCODE_UP},

    {"F5", SDL_SCANCODE_F5},        {"F6", SDL_SCANCODE_F6},
    {"F7", SDL_SCANCODE_F7},        {"F8", SDL_SCANCODE_F8},
    {"F9", SDL_SCANCODE_F9},

    {"SHIFT", SDL_SCANCODE_LSHIFT}, {"SHIFT", SDL_SCANCODE_RSHIFT},
    {"CTRL", SDL_SCANCODE_LCTRL},   {"CTRL", SDL_SCANCODE_RCTRL},
    {"ALT", SDL_SCANCODE_LALT},     {"ALT", SDL_SCANCODE_RALT},

    {"A", SDL_SCANCODE_LSHIFT},     {"B", SDL_SCANCODE_ESCAPE},
    {"B", SDL_SCANCODE_KP_0},       {"B", SDL_SCANCODE_X},
    {"C", SDL_SCANCODE_SPACE},      {"C", SDL_SCANCODE_RETURN},
    {"X", SDL_SCANCODE_A},          {"Y", SDL_SCANCODE_S},
    {"Z", SDL_SCANCODE_D},          {"L", SDL_SCANCODE_Q},
    {"R", SDL_SCANCODE_W},
};

const KeyboardControllerImpl::KeyBinding kKeyboardBindings1[] = {
    {"A", SDL_SCANCODE_Z},
    {"C", SDL_SCANCODE_C},
};

const KeyboardControllerImpl::KeyBinding kKeyboardBindings2[] = {
    {"C", SDL_SCANCODE_Z},
};

const std::string kArrowDirsSymbol[] = {
    "DOWN",
    "LEFT",
    "RIGHT",
    "UP",
};

// Comment due to error: unused variable 'kButtonItems'
// const std::array<std::string, 12> kButtonItems = {
//     "A", "B", "C", "X", "Y", "Z", "L", "R", "DOWN", "LEFT", "RIGHT", "UP",
// };

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// KeyboardControllerImpl Implement

KeyboardControllerImpl::KeyboardControllerImpl(
    ExecutionContext* execution_context)
    : EngineObject(execution_context) {
  /* Apply default keyboard bindings */
  for (size_t i = 0; i < std::size(kDefaultKeyboardBindings); ++i)
    key_bindings_.push_back(kDefaultKeyboardBindings[i]);

  if (execution_context->engine_profile->api_version ==
      ContentProfile::APIVersion::RGSS1)
    for (size_t i = 0; i < std::size(kKeyboardBindings1); ++i)
      key_bindings_.push_back(kKeyboardBindings1[i]);

  if (execution_context->engine_profile->api_version >=
      ContentProfile::APIVersion::RGSS2)
    for (size_t i = 0; i < std::size(kKeyboardBindings2); ++i)
      key_bindings_.push_back(kKeyboardBindings2[i]);

  /* Apply default gamepad bindings */
  for (size_t i = 0; i < std::size(kDefaultGamepadBindings); ++i)
    gp_bindings_.push_back(kDefaultGamepadBindings[i]);

  /* Load user bindings from _gp.txt (overrides defaults) */
  LoadGamepadBindingsInternal();
}

KeyboardControllerImpl::~KeyboardControllerImpl() {
  for (auto& gh : gamepads_)
    if (gh.pad)
      SDL_CloseGamepad(gh.pad);
}

// Replaced by configurable gp_bindings_ + kDefaultGamepadBindings


void KeyboardControllerImpl::UpdateGamepad() {
  // Remove disconnected gamepad
  if (!gamepads_.empty() && !SDL_GamepadConnected(gamepads_[0].pad)) {
    SDL_CloseGamepad(gamepads_[0].pad);
    gamepads_.clear();
  }

  // Only open the first gamepad (same as mkxp-z behavior)
  if (gamepads_.empty()) {
    int num_joysticks = 0;
    SDL_JoystickID* sticks = SDL_GetJoysticks(&num_joysticks);
    if (num_joysticks > 0 && SDL_IsGamepad(sticks[0])) {
      SDL_Gamepad* pad = SDL_OpenGamepad(sticks[0]);
      if (pad)
        gamepads_.push_back({pad, sticks[0]});
    }
    SDL_free(sticks);
  }

  // Save previous merged states
  for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; ++a)
    gp_axes_prev_[a] = gp_axes_[a];
  for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b)
    gp_buttons_prev_[b] = gp_buttons_[b];

  PollGamepadState();
}

void KeyboardControllerImpl::PollGamepadState() {
  memset(gp_axes_, 0, sizeof(gp_axes_));
  memset(gp_buttons_, 0, sizeof(gp_buttons_));

  if (gamepads_.empty())
    return;

  auto& gh = gamepads_[0];
  for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; ++a)
    gp_axes_[a] = SDL_GetGamepadAxis(gh.pad, static_cast<SDL_GamepadAxis>(a));
  for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b)
    gp_buttons_[b] = SDL_GetGamepadButton(gh.pad, static_cast<SDL_GamepadButton>(b));
}

bool KeyboardControllerImpl::GamepadIsPressed(const std::string& sym) {
  if (gamepads_.empty())
    return false;

  for (const auto& entry : gp_bindings_) {
    if (entry.sym != sym)
      continue;
    switch (entry.source.type) {
      case GamepadSource::Type::Button:
        if (entry.source.index < SDL_GAMEPAD_BUTTON_COUNT &&
            gp_buttons_[entry.source.index])
          return true;
        break;
      case GamepadSource::Type::Axis: {
        if (entry.source.index >= SDL_GAMEPAD_AXIS_COUNT)
          break;
        int16_t val = gp_axes_[entry.source.index];
        if (entry.source.dir < 0)
          return val < -kGamepadAxisThreshold;
        else
          return val > kGamepadAxisThreshold;
      }
      default:
        break;
    }
  }
  return false;
}

void KeyboardControllerImpl::ProcessEvent(
    const std::optional<EventController::KeyEventData>& event) {
  if (event) {
    raw_states_[event->scancode] = event->is_down;
  } else {
    for (auto& it : raw_states_)
      it = false;
  }
}

void KeyboardControllerImpl::ApplyKeySymBinding(const KeySymMap& keysyms) {
  key_bindings_ = keysyms;
}

void KeyboardControllerImpl::Update(ExceptionState& exception_state) {
  for (int32_t i = 0; i < SDL_SCANCODE_COUNT; ++i) {
    bool key_pressed = raw_states_[static_cast<SDL_Scancode>(i)];

    /* Update key state with elder state */
    key_states_[i].trigger = !key_states_[i].pressed && key_pressed;

    /* After trigger set, set press state */
    key_states_[i].pressed = key_pressed;

    /* Based on press state update the repeat state */
    key_states_[i].repeat = false;
    if (key_states_[i].pressed) {
      ++key_states_[i].repeat_count;

      bool repeated = false;
      // TODO: RGSS 1/2/3 specific process
      repeated = key_states_[i].repeat_count == 1 ||
                 (key_states_[i].repeat_count >= 23 &&
                  (key_states_[i].repeat_count + 1) % 6 == 0);

      key_states_[i].repeat = repeated;
    } else {
      key_states_[i].repeat_count = 0;
    }

    /* Update recent key infos */
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

  for (auto& it : key_bindings_) {
    if (it.sym == sym)
      if (key_states_[it.scancode].pressed)
        return true;
  }

  return false;
}

bool KeyboardControllerImpl::IsTriggered(const std::string& sym,
                                         ExceptionState& exception_state) {
  if (sym.empty())
    return false;

  for (auto& it : key_bindings_) {
    if (it.sym == sym)
      if (key_states_[it.scancode].trigger)
        return true;
  }

  return false;
}

bool KeyboardControllerImpl::IsRepeated(const std::string& sym,
                                        ExceptionState& exception_state) {
  if (sym.empty())
    return false;

  for (auto& it : key_bindings_) {
    if (it.sym == sym)
      if (key_states_[it.scancode].repeat)
        return true;
  }

  return false;
}

int16_t KeyboardControllerImpl::GetGamepadAxisValue(int32_t axis) {
  if (gamepads_.empty() || axis < 0 || axis >= SDL_GAMEPAD_AXIS_COUNT)
    return 0;
  return gp_axes_[axis];
}

std::string KeyboardControllerImpl::GetGamepadAxisName(int32_t axis) {
  if (axis >= 0 && axis < SDL_GAMEPAD_AXIS_COUNT)
    return kGamepadAxisNames[axis];
  return "Unknown";
}

std::string KeyboardControllerImpl::GetGamepadButtonName(int32_t button) {
  if (button >= 0 && button < SDL_GAMEPAD_BUTTON_COUNT)
    return kGamepadButtonNames[button];
  return "Unknown";
}

bool KeyboardControllerImpl::IsGamepadConnected(ExceptionState& exception_state) {
  return !gamepads_.empty();
}

void KeyboardControllerImpl::RumbleGamepad(uint16_t low_freq,
                                           uint16_t high_freq,
                                           uint32_t duration_ms,
                                           ExceptionState& exception_state) {
  if (gamepads_.empty())
    return;
  SDL_RumbleGamepad(gamepads_[0].pad, low_freq, high_freq, duration_ms);
}

int32_t KeyboardControllerImpl::Dir4(ExceptionState& exception_state) {
  return dir4_state_.active;
}

int32_t KeyboardControllerImpl::Dir8(ExceptionState& exception_state) {
  return dir8_state_.active;
}

bool KeyboardControllerImpl::KeyPressed(int32_t scancode,
                                        ExceptionState& exception_state) {
  return key_states_[scancode].pressed;
}

bool KeyboardControllerImpl::KeyTriggered(int32_t scancode,
                                          ExceptionState& exception_state) {
  return key_states_[scancode].trigger;
}

bool KeyboardControllerImpl::KeyRepeated(int32_t scancode,
                                         ExceptionState& exception_state) {
  return key_states_[scancode].repeat;
}

std::string KeyboardControllerImpl::GetKeyName(
    int32_t scancode,
    ExceptionState& exception_state) {
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

  for (auto& it : key_bindings_)
    if (it.sym == flag)
      out.push_back(it.scancode);

  return out;
}

void KeyboardControllerImpl::SetKeysFromFlag(const std::string& flag,
                                             const std::vector<int32_t>& keys,
                                             ExceptionState& exception_state) {
  if (flag.empty())
    return;

  auto iter = std::remove_if(
      key_bindings_.begin(), key_bindings_.end(),
      [&](const KeyBinding& binding) { return binding.sym == flag; });
  key_bindings_.erase(iter, key_bindings_.end());

  for (const auto& i : keys) {
    KeyBinding binding;
    binding.sym = flag;
    binding.scancode = static_cast<SDL_Scancode>(i);

    key_bindings_.push_back(std::move(binding));
  }
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
  bool key_states[std::size(kArrowDirsSymbol)] = {0};
  for (auto& it : key_bindings_)
    for (size_t i = 0; i < std::size(kArrowDirsSymbol); ++i)
      if (it.sym == kArrowDirsSymbol[i])
        key_states[i] |= key_states_[it.scancode].pressed;

  int32_t dir_flag = 0;
  const int32_t dir_flags_fix[] = {
      1 << 1,
      1 << 2,
      1 << 3,
      1 << 4,
  };

  const int32_t block_dir_flags[] = {dir_flags_fix[0] | dir_flags_fix[3],
                                     dir_flags_fix[1] | dir_flags_fix[2]};

  const int32_t other_dirs[][3] = {
      {1, 2, 3},
      {0, 3, 2},
      {0, 3, 1},
      {1, 2, 0},
  };

  for (size_t i = 0; i < 4; ++i)
    dir_flag |= (key_states[i] ? dir_flags_fix[i] : 0);

  if (dir_flag == block_dir_flags[0] || dir_flag == block_dir_flags[1]) {
    dir4_state_.active = 0;
    return;
  }

  if (dir4_state_.previous) {
    if (key_states[dir4_state_.previous / 2 - 1]) {
      for (size_t i = 0; i < 3; ++i) {
        int32_t other_key = other_dirs[dir4_state_.previous / 2 - 1][i];
        if (!key_states[other_key])
          continue;

        dir4_state_.active = (other_key + 1) * 2;
        return;
      }
    }
  }

  for (size_t i = 0; i < 4; ++i) {
    if (!key_states[i])
      continue;

    dir4_state_.active = (i + 1) * 2;
    dir4_state_.previous = (i + 1) * 2;
    return;
  }

  dir4_state_.active = 0;
  dir4_state_.previous = 0;
}

void KeyboardControllerImpl::UpdateDir8Internal() {
  bool key_states[std::size(kArrowDirsSymbol)] = {0};
  for (auto& it : key_bindings_)
    for (size_t i = 0; i < std::size(kArrowDirsSymbol); ++i)
      if (it.sym == kArrowDirsSymbol[i])
        key_states[i] |= key_states_[it.scancode].pressed;

  static const int32_t combos[4][4] = {
      {2, 1, 3, 0}, {1, 4, 0, 7}, {3, 0, 6, 9}, {0, 7, 9, 8}};

  const int32_t other_dirs[][3] = {
      {1, 2, 3},
      {0, 3, 2},
      {0, 3, 1},
      {1, 2, 0},
  };

  dir8_state_.active = 0;

  for (size_t i = 0; i < 4; ++i) {
    if (!key_states[i])
      continue;

    for (int32_t j = 0; j < 3; ++j) {
      int32_t other_key = other_dirs[i][j];
      if (!key_states[other_key])
        continue;

      dir8_state_.active = combos[i][other_key];
      return;
    }

    dir8_state_.active = (i + 1) * 2;
    return;
  }
}

void KeyboardControllerImpl::TryReadBindingsInternal() {
  std::string filepath = context()->engine_profile->program_name;
  filepath += INPUT_CONFIG_SUBFIX;
  filepath += std::to_string(
      static_cast<int32_t>(context()->engine_profile->api_version));

  LOG(INFO) << "[Keyboard] Reading key binding table...";

  filesystem::IOState io_state;
  auto* fstream = context()->io_service->OpenReadRaw(filepath, &io_state);
  if (fstream) {
    key_bindings_.clear();

    Uint32 item_size;
    SDL_ReadIO(fstream, &item_size, sizeof(item_size));
    for (uint32_t i = 0; i < item_size; ++i) {
      Uint32 token_size;
      SDL_ReadIO(fstream, &token_size, sizeof(token_size));

      std::string token(token_size, 0);
      SDL_ReadIO(fstream, token.data(), token_size);

      SDL_Scancode scancode;
      SDL_ReadIO(fstream, &scancode, sizeof(scancode));

      key_bindings_.push_back({token, scancode});
    }

    SDL_CloseIO(fstream);
  }
}

void KeyboardControllerImpl::StorageBindingsInternal() {
  std::string filepath = context()->engine_profile->program_name;
  filepath += INPUT_CONFIG_SUBFIX;
  filepath += std::to_string(
      static_cast<int32_t>(context()->engine_profile->api_version));

  LOG(INFO) << "[Keyboard] Saving key binding table...";

  filesystem::IOState io_state;
  auto* fstream = context()->io_service->OpenWrite(filepath, &io_state);
  if (fstream) {
    Uint32 item_size = key_bindings_.size();
    SDL_WriteIO(fstream, &item_size, sizeof(item_size));
    for (const auto& it : key_bindings_) {
      uint32_t token_size = it.sym.size();
      SDL_WriteIO(fstream, &token_size, sizeof(token_size));

      SDL_WriteIO(fstream, it.sym.data(), token_size);

      SDL_Scancode scancode = it.scancode;
      SDL_WriteIO(fstream, &scancode, sizeof(scancode));
    }

    SDL_CloseIO(fstream);
  }

  // Save gamepad bindings to separate file
  SaveGamepadBindingsInternal();
}

// --- Gamepad binding persistence (text format) ---

#define GAMEPAD_CONFIG_SUFFIX "_gp.txt"

static std::string GamepadConfigPath(ExecutionContext* ctx) {
  std::string path = ctx->engine_profile->program_name;
  path += GAMEPAD_CONFIG_SUFFIX;
  path += std::to_string(
      static_cast<int32_t>(ctx->engine_profile->api_version));
  return path;
}

void KeyboardControllerImpl::LoadGamepadBindingsInternal() {
  std::string filepath = GamepadConfigPath(context());

  LOG(INFO) << "[Gamepad] Reading gamepad bindings from " << filepath;

  filesystem::IOState io_state;
  auto* fstream = context()->io_service->OpenReadRaw(filepath, &io_state);
  if (!fstream)
    return;

  // Read entire file into memory
  std::string content;
  {
    char buf[4096];
    size_t n;
    while ((n = SDL_ReadIO(fstream, buf, sizeof(buf))) > 0)
      content.append(buf, n);
  }
  SDL_CloseIO(fstream);

  // File replaces defaults entirely (same as mkxp-z behavior)
  gp_bindings_.clear();

  // Parse line by line
  size_t pos = 0;
  while (pos < content.size()) {
    size_t eol = content.find('\n', pos);
    if (eol == std::string::npos) eol = content.size();
    std::string line = content.substr(pos, eol - pos);
    pos = eol + 1;

    // Trim whitespace
    size_t start = line.find_first_not_of(" \t\r");
    if (start == std::string::npos) continue;
    size_t end = line.find_last_not_of(" \t\r");
    line = line.substr(start, end - start + 1);

    // Skip comments and empty lines
    if (line.empty() || line[0] == '#') continue;

    // Parse: SYM = Btn:INDEX  or  SYM = Axis:INDEX:[+-]
    size_t eq = line.find('=');
    if (eq == std::string::npos) {
      LOG(WARNING) << "[Gamepad] Skipping malformed line (no '='): " << line;
      continue;
    }

    std::string sym = line.substr(0, eq);
    std::string val = line.substr(eq + 1);
    sym = sym.substr(0, sym.find_last_not_of(" \t") + 1);
    val = val.substr(val.find_first_not_of(" \t"));

    GamepadBindingEntry entry;
    entry.sym = sym;

    if (val.starts_with("Btn:") || val.starts_with("btn:")) {
      entry.source.type = GamepadSource::Type::Button;
      entry.source.index = static_cast<uint8_t>(std::stoi(val.substr(4)));
      entry.source.dir = 0;
    } else if (val.starts_with("Axis:") || val.starts_with("axis:")) {
      // Format: Axis:INDEX:[+-], e.g. "Axis:1:-"
      entry.source.type = GamepadSource::Type::Axis;
      size_t colon2 = val.rfind(':');
      if (colon2 == std::string::npos || colon2 <= 5) {
        LOG(WARNING) << "[Gamepad] Skipping malformed Axis entry (need Axis:INDEX:[+-]): "
                     << sym << " = " << val;
        continue;
      }
      entry.source.index = static_cast<uint8_t>(std::stoi(val.substr(5, colon2 - 5)));
      entry.source.dir = (val.size() > colon2 + 1 && val[colon2 + 1] == '-') ? -1 : 1;
    } else {
      LOG(WARNING) << "[Gamepad] Skipping unrecognized binding format: "
                   << sym << " = " << val;
      continue;
    }

    // Append to defaults (multiple entries per sym allowed, OR logic in GamepadIsPressed)
    // To remove a binding, use GUI right-click or the default bindings won't be affected.
    gp_bindings_.push_back(std::move(entry));
  }

  LOG(INFO) << "[Gamepad] Loaded " << gp_bindings_.size() << " bindings.";
}

void KeyboardControllerImpl::SaveGamepadBindingsInternal() {
  std::string filepath = GamepadConfigPath(context());

  LOG(INFO) << "[Gamepad] Saving gamepad bindings to " << filepath;

  filesystem::IOState io_state;
  auto* fstream = context()->io_service->OpenWrite(filepath, &io_state);
  if (!fstream)
    return;

  std::string out;
  out += "# URGE Gamepad Bindings\n";
  out += "# Format: SYMBOL = Btn:INDEX  or  SYMBOL = Axis:INDEX:[+-]\n";
  out += "# Lines starting with # are ignored.\n\n";

  for (const auto& entry : gp_bindings_) {
    out += entry.sym;
    out += " = ";
    switch (entry.source.type) {
      case GamepadSource::Type::Button:
        out += "Btn:";
        out += std::to_string(entry.source.index);
        break;
      case GamepadSource::Type::Axis:
        out += "Axis:";
        out += std::to_string(entry.source.index);
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
  LOG(INFO) << "[Gamepad] Saved " << gp_bindings_.size() << " bindings.";
}

// --- Configurable gamepad bindings API ---

void KeyboardControllerImpl::SetGamepadBindingList(const GamepadBindingList& bindings) {
  gp_bindings_ = bindings;
}

const KeyboardControllerImpl::GamepadBindingList&
KeyboardControllerImpl::GetGamepadBindingList() const {
  return gp_bindings_;
}

KeyboardControllerImpl::GamepadBindingList
KeyboardControllerImpl::GetDefaultGamepadBindings() const {
  GamepadBindingList defaults;
  for (size_t i = 0; i < std::size(kDefaultGamepadBindings); ++i)
    defaults.push_back(kDefaultGamepadBindings[i]);
  return defaults;
}

void KeyboardControllerImpl::ResetGamepadBindingsToDefault() {
  gp_bindings_ = GetDefaultGamepadBindings();
  SaveGamepadBindingsInternal();
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

void KeyboardControllerImpl::StartCaptureFor(const std::string& sym, int32_t next_idx) {
  capture_target_ = sym;
  capture_next_idx_ = next_idx;
  is_capturing_ = true;
  capture_slot_ = GamepadSource();
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

  // Check gamepad buttons (edge against baseline)
  if (!gamepads_.empty()) {
    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
      if (gp_buttons_[b] && !capture_gp_btn_baseline_[b]) {
        capture_slot_.type = GamepadSource::Type::Button;
        capture_slot_.index = static_cast<uint8_t>(b);
        capture_slot_.dir = 0;

        auto it = std::find_if(gp_bindings_.begin(), gp_bindings_.end(),
            [this](const GamepadBindingEntry& e) {
              return e.sym == capture_target_ && e.source == capture_slot_;
            });
        if (it == gp_bindings_.end())
          gp_bindings_.push_back({capture_target_, capture_slot_});

        is_capturing_ = false;
        SaveGamepadBindingsInternal();

        // Auto-advance to next symbol
        const char* next = NextCaptureSym(capture_next_idx_);
        if (next) {
          StartCaptureFor(next, capture_next_idx_ + 1);
        }
        return;
      }
    }

    // Check gamepad axes (edge against baseline)
    for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; ++a) {
      int16_t val = gp_axes_[a];
      int16_t base = capture_gp_axis_baseline_[a];
      bool now_active = (val < -kGamepadAxisThreshold || val > kGamepadAxisThreshold);
      bool was_active = (base < -kGamepadAxisThreshold || base > kGamepadAxisThreshold);
      if (now_active && !was_active) {
        capture_slot_.type = GamepadSource::Type::Axis;
        capture_slot_.index = static_cast<uint8_t>(a);
        capture_slot_.dir = (val < 0) ? -1 : 1;

        auto it = std::find_if(gp_bindings_.begin(), gp_bindings_.end(),
            [this](const GamepadBindingEntry& e) {
              return e.sym == capture_target_ && e.source == capture_slot_;
            });
        if (it == gp_bindings_.end())
          gp_bindings_.push_back({capture_target_, capture_slot_});

        is_capturing_ = false;
        SaveGamepadBindingsInternal();

        // Auto-advance to next symbol
        const char* next = NextCaptureSym(capture_next_idx_);
        if (next) {
          StartCaptureFor(next, capture_next_idx_ + 1);
        }
        return;
      }
    }
  }

  // Check keyboard (edge detection against baseline)
  for (int s = 0; s < SDL_SCANCODE_COUNT; ++s) {
    if (raw_states_[s] && !capture_kb_baseline_[s]) {
      is_capturing_ = false;
      return;
    }
  }
}

// --- ImGui settings UI ---

static const char* SourceTypeToString(
    KeyboardControllerImpl::GamepadSource::Type type) {
  switch (type) {
    case KeyboardControllerImpl::GamepadSource::Type::Button:
      return "Btn";
    case KeyboardControllerImpl::GamepadSource::Type::Axis:
      return "Axis";
    default:
      return "?";
  }
}

static const char* SourceDescription(
    const KeyboardControllerImpl::GamepadSource& src) {
  static char buf[64];
  switch (src.type) {
    case KeyboardControllerImpl::GamepadSource::Type::Button:
      if (src.index < SDL_GAMEPAD_BUTTON_COUNT)
        return kGamepadButtonNames[src.index];
      snprintf(buf, sizeof(buf), "Btn#%u", src.index);
      return buf;
    case KeyboardControllerImpl::GamepadSource::Type::Axis:
      if (src.index < SDL_GAMEPAD_AXIS_COUNT) {
        snprintf(buf, sizeof(buf), "%s%c",
                 kGamepadAxisNames[src.index], src.dir < 0 ? '-' : '+');
        return buf;
      }
      snprintf(buf, sizeof(buf), "Axis#%u%c", src.index, src.dir < 0 ? '-' : '+');
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

    if (gamepads_.empty()) {
      ImGui::TextColored(ImVec4(1,1,0,1), "%s",
          i18n->GetI18NString(IDS_INPUT_NO_GAMEPAD,
              "No gamepad connected. Connect a gamepad to configure bindings.")
              .c_str());
    }

    // Capture overlay
    if (is_capturing_) {
      ImGui::OpenPopup("##GamepadCapture");
    }

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

    // Table: sym → bindings
    if (ImGui::BeginTable("##GPBindings", 4,
                          ImGuiTableFlags_Borders |
                              ImGuiTableFlags_SizingFixedFit |
                              ImGuiTableFlags_NoHostExtendX)) {
      ImGui::TableSetupColumn("Button", ImGuiTableColumnFlags_WidthFixed);
      ImGui::TableSetupColumn("Bindings", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);

      // Collect unique syms from gp_bindings_ + all configurable syms
      std::vector<std::string> all_syms;
      for (const auto& s : kGamepadConfigurableSyms)
        all_syms.push_back(s);
      for (const auto& b : gp_bindings_) {
        if (std::find(all_syms.begin(), all_syms.end(), b.sym) == all_syms.end())
          all_syms.push_back(b.sym);
      }

      for (const auto& sym : all_syms) {
        ImGui::TableNextRow();

        // Column 1: sym name
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%s", sym.c_str());

        // Column 2: binding sources
        ImGui::TableSetColumnIndex(1);

        // Collect sources for this sym
        std::vector<GamepadSource> sources;
        for (const auto& b : gp_bindings_) {
          if (b.sym == sym)
            sources.push_back(b.source);
        }

        if (sources.empty()) {
          ImGui::TextDisabled("(unbound)");
        } else {
          bool first = true;
          for (const auto& src : sources) {
            if (!first) ImGui::SameLine();
            first = false;
            ImGui::Text("%s", SourceDescription(src));
          }
        }

        // Column 2: right-click to clear bindings
        ImGui::TableSetColumnIndex(1);
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
          auto& b = gp_bindings_;
          for (auto it = b.begin(); it != b.end();) {
            if (it->sym == sym)
              it = b.erase(it);
            else
              ++it;
          }
          SaveGamepadBindingsInternal();
        }

        // Column 3: action buttons (add/remove)
        ImGui::TableSetColumnIndex(2);

        if (is_capturing_) {
          ImGui::BeginDisabled();
          ImGui::SmallButton("+");
          ImGui::EndDisabled();
        } else {
          ImGui::PushID(sym.c_str());
          if (ImGui::SmallButton("+")) {
            int32_t next = -1;
            for (size_t i = 0; i < std::size(kGamepadConfigurableSyms); ++i)
              if (sym == kGamepadConfigurableSyms[i]) {
                next = static_cast<int32_t>(i + 1);
                break;
              }
            StartCaptureFor(sym, next);
          }
          ImGui::PopID();
        }
      }

      ImGui::EndTable();
    }

    // Reset and Save buttons
    ImGui::Separator();
    if (ImGui::Button(i18n->GetI18NString(IDS_BUTTON_RESET_SETTINGS, "Reset")
                           .c_str())) {
      ResetGamepadBindingsToDefault();
    }
    ImGui::SameLine();
    if (ImGui::Button(
            i18n->GetI18NString(IDS_BUTTON_SAVE_SETTINGS, "Save").c_str())) {
      SaveGamepadBindingsInternal();
    }
  }
}

}  // namespace content
