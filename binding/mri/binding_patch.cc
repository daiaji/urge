// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD - style license that can be
// found in the LICENSE file.

#include "binding/mri/binding_patch.h"

#include <cctype>
#include <cstring>
#include <fstream>
#include <unordered_map>

#include "SDL3/SDL_scancode.h"

#include "content/input/mouse_controller.h"
#include "content/public/engine_input.h"

namespace binding {

namespace {

struct BindingSet {
  std::string name;
  int key_id;
};

const BindingSet kKeyboardBindings[] = {
    {"DOWN", 2},   {"LEFT", 4},  {"RIGHT", 6}, {"UP", 8},

    {"A", 11},     {"B", 12},    {"C", 13},    {"X", 14},  {"Y", 15},
    {"Z", 16},     {"L", 17},    {"R", 18},

    {"SHIFT", 21}, {"CTRL", 22}, {"ALT", 23},

    {"F5", 25},    {"F6", 26},   {"F7", 27},   {"F8", 28}, {"F9", 29},
};

const BindingSet kScancodeBindings[] = {
    {"KEY_A", SDL_SCANCODE_A},
    {"KEY_B", SDL_SCANCODE_B},
    {"KEY_C", SDL_SCANCODE_C},
    {"KEY_D", SDL_SCANCODE_D},
    {"KEY_E", SDL_SCANCODE_E},
    {"KEY_F", SDL_SCANCODE_F},
    {"KEY_G", SDL_SCANCODE_G},
    {"KEY_H", SDL_SCANCODE_H},
    {"KEY_I", SDL_SCANCODE_I},
    {"KEY_J", SDL_SCANCODE_J},
    {"KEY_K", SDL_SCANCODE_K},
    {"KEY_L", SDL_SCANCODE_L},
    {"KEY_M", SDL_SCANCODE_M},
    {"KEY_N", SDL_SCANCODE_N},
    {"KEY_O", SDL_SCANCODE_O},
    {"KEY_P", SDL_SCANCODE_P},
    {"KEY_Q", SDL_SCANCODE_Q},
    {"KEY_R", SDL_SCANCODE_R},
    {"KEY_S", SDL_SCANCODE_S},
    {"KEY_T", SDL_SCANCODE_T},
    {"KEY_U", SDL_SCANCODE_U},
    {"KEY_V", SDL_SCANCODE_V},
    {"KEY_W", SDL_SCANCODE_W},
    {"KEY_X", SDL_SCANCODE_X},
    {"KEY_Y", SDL_SCANCODE_Y},
    {"KEY_Z", SDL_SCANCODE_Z},
    {"KEY_0", SDL_SCANCODE_0},
    {"KEY_1", SDL_SCANCODE_1},
    {"KEY_2", SDL_SCANCODE_2},
    {"KEY_3", SDL_SCANCODE_3},
    {"KEY_4", SDL_SCANCODE_4},
    {"KEY_5", SDL_SCANCODE_5},
    {"KEY_6", SDL_SCANCODE_6},
    {"KEY_7", SDL_SCANCODE_7},
    {"KEY_8", SDL_SCANCODE_8},
    {"KEY_9", SDL_SCANCODE_9},
    {"KEY_SPACE", SDL_SCANCODE_SPACE},
    {"KEY_RETURN", SDL_SCANCODE_RETURN},
    {"KEY_ESCAPE", SDL_SCANCODE_ESCAPE},
    {"KEY_BACKSPACE", SDL_SCANCODE_BACKSPACE},
    {"KEY_TAB", SDL_SCANCODE_TAB},
    {"KEY_LSHIFT", SDL_SCANCODE_LSHIFT},
    {"KEY_RSHIFT", SDL_SCANCODE_RSHIFT},
    {"KEY_LCTRL", SDL_SCANCODE_LCTRL},
    {"KEY_RCTRL", SDL_SCANCODE_RCTRL},
    {"KEY_LALT", SDL_SCANCODE_LALT},
    {"KEY_RALT", SDL_SCANCODE_RALT},
    {"KEY_GRAVE", SDL_SCANCODE_GRAVE},
    {"KEY_MINUS", SDL_SCANCODE_MINUS},
    {"KEY_EQUALS", SDL_SCANCODE_EQUALS},
    {"KEY_LEFTBRACKET", SDL_SCANCODE_LEFTBRACKET},
    {"KEY_RIGHTBRACKET", SDL_SCANCODE_RIGHTBRACKET},
    {"KEY_BACKSLASH", SDL_SCANCODE_BACKSLASH},
    {"KEY_SEMICOLON", SDL_SCANCODE_SEMICOLON},
    {"KEY_APOSTROPHE", SDL_SCANCODE_APOSTROPHE},
    {"KEY_COMMA", SDL_SCANCODE_COMMA},
    {"KEY_PERIOD", SDL_SCANCODE_PERIOD},
    {"KEY_SLASH", SDL_SCANCODE_SLASH},
    {"KEY_UP", SDL_SCANCODE_UP},
    {"KEY_DOWN", SDL_SCANCODE_DOWN},
    {"KEY_LEFT", SDL_SCANCODE_LEFT},
    {"KEY_RIGHT", SDL_SCANCODE_RIGHT},
    {"KEY_F1", SDL_SCANCODE_F1},
    {"KEY_F2", SDL_SCANCODE_F2},
    {"KEY_F3", SDL_SCANCODE_F3},
    {"KEY_F4", SDL_SCANCODE_F4},
    {"KEY_F5", SDL_SCANCODE_F5},
    {"KEY_F6", SDL_SCANCODE_F6},
    {"KEY_F7", SDL_SCANCODE_F7},
    {"KEY_F8", SDL_SCANCODE_F8},
    {"KEY_F9", SDL_SCANCODE_F9},
    {"KEY_F10", SDL_SCANCODE_F10},
    {"KEY_F11", SDL_SCANCODE_F11},
    {"KEY_F12", SDL_SCANCODE_F12},
};

std::string GetButtonSymbol(int argc, VALUE* argv) {
  MriCheckArgc(argc, 1);

  std::string sym;
  if (FIXNUM_P(*argv)) {
    int key_id = FIX2INT(*argv);
    for (size_t i = 0; i < std::size(kKeyboardBindings); ++i)
      if (kKeyboardBindings[i].key_id == key_id)
        return kKeyboardBindings[i].name;
  } else if (SYMBOL_P(*argv)) {
    MriParseArgsTo(argc, argv, "n", &sym);
  }

  return sym;
}

std::string NormalizeScancodeName(const std::string& input) {
  std::string key;
  key.reserve(input.size());
  for (char c : input) {
    if (c == '-' || c == ' ')
      key.push_back('_');
    else
      key.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
  }

  if (key.rfind("KEY_", 0) == 0)
    key.erase(0, 4);
  if (key.rfind("SDL_SCANCODE_", 0) == 0)
    key.erase(0, 13);
  if (key.rfind("SCANCODE_", 0) == 0)
    key.erase(0, 9);

  return key;
}

int GetScancodeArg(VALUE value) {
  if (FIXNUM_P(value))
    return FIX2INT(value);

  std::string raw;
  if (SYMBOL_P(value)) {
    raw = rb_id2name(SYM2ID(value));
  } else if (RB_TYPE_P(value, T_STRING)) {
    raw = StringValueCStr(value);
  } else {
    rb_raise(rb_eTypeError, "expected Integer, Symbol, or String");
  }

  const std::string key = NormalizeScancodeName(raw);
  if (key.size() == 1) {
    char c = key[0];
    if (c >= 'A' && c <= 'Z')
      return SDL_SCANCODE_A + (c - 'A');
    if (c >= '1' && c <= '9')
      return SDL_SCANCODE_1 + (c - '1');
    if (c == '0')
      return SDL_SCANCODE_0;
  }

  static const std::unordered_map<std::string, int> kNameToScancode = {
      {"NUMBER_0", SDL_SCANCODE_0},
      {"NUMBER_1", SDL_SCANCODE_1},
      {"NUMBER_2", SDL_SCANCODE_2},
      {"NUMBER_3", SDL_SCANCODE_3},
      {"NUMBER_4", SDL_SCANCODE_4},
      {"NUMBER_5", SDL_SCANCODE_5},
      {"NUMBER_6", SDL_SCANCODE_6},
      {"NUMBER_7", SDL_SCANCODE_7},
      {"NUMBER_8", SDL_SCANCODE_8},
      {"NUMBER_9", SDL_SCANCODE_9},
      {"NUM_0", SDL_SCANCODE_0},
      {"NUM_1", SDL_SCANCODE_1},
      {"NUM_2", SDL_SCANCODE_2},
      {"NUM_3", SDL_SCANCODE_3},
      {"NUM_4", SDL_SCANCODE_4},
      {"NUM_5", SDL_SCANCODE_5},
      {"NUM_6", SDL_SCANCODE_6},
      {"NUM_7", SDL_SCANCODE_7},
      {"NUM_8", SDL_SCANCODE_8},
      {"NUM_9", SDL_SCANCODE_9},
      {"SPACE", SDL_SCANCODE_SPACE},
      {"RETURN", SDL_SCANCODE_RETURN},
      {"ENTER", SDL_SCANCODE_RETURN},
      {"KP_ENTER", SDL_SCANCODE_KP_ENTER},
      {"ESC", SDL_SCANCODE_ESCAPE},
      {"ESCAPE", SDL_SCANCODE_ESCAPE},
      {"BACKSPACE", SDL_SCANCODE_BACKSPACE},
      {"TAB", SDL_SCANCODE_TAB},
      {"SHIFT", SDL_SCANCODE_LSHIFT},
      {"LSHIFT", SDL_SCANCODE_LSHIFT},
      {"LEFT_SHIFT", SDL_SCANCODE_LSHIFT},
      {"RSHIFT", SDL_SCANCODE_RSHIFT},
      {"RIGHT_SHIFT", SDL_SCANCODE_RSHIFT},
      {"CTRL", SDL_SCANCODE_LCTRL},
      {"LCTRL", SDL_SCANCODE_LCTRL},
      {"LEFT_CTRL", SDL_SCANCODE_LCTRL},
      {"RCTRL", SDL_SCANCODE_RCTRL},
      {"RIGHT_CTRL", SDL_SCANCODE_RCTRL},
      {"ALT", SDL_SCANCODE_LALT},
      {"LALT", SDL_SCANCODE_LALT},
      {"LEFT_ALT", SDL_SCANCODE_LALT},
      {"RALT", SDL_SCANCODE_RALT},
      {"RIGHT_ALT", SDL_SCANCODE_RALT},
      {"GRAVE", SDL_SCANCODE_GRAVE},
      {"TILDE", SDL_SCANCODE_GRAVE},
      {"BACKQUOTE", SDL_SCANCODE_GRAVE},
      {"MINUS", SDL_SCANCODE_MINUS},
      {"EQUALS", SDL_SCANCODE_EQUALS},
      {"EQUAL", SDL_SCANCODE_EQUALS},
      {"LEFTBRACKET", SDL_SCANCODE_LEFTBRACKET},
      {"LEFT_BRACKET", SDL_SCANCODE_LEFTBRACKET},
      {"RIGHTBRACKET", SDL_SCANCODE_RIGHTBRACKET},
      {"RIGHT_BRACKET", SDL_SCANCODE_RIGHTBRACKET},
      {"BACKSLASH", SDL_SCANCODE_BACKSLASH},
      {"SEMICOLON", SDL_SCANCODE_SEMICOLON},
      {"APOSTROPHE", SDL_SCANCODE_APOSTROPHE},
      {"COMMA", SDL_SCANCODE_COMMA},
      {"PERIOD", SDL_SCANCODE_PERIOD},
      {"DOT", SDL_SCANCODE_PERIOD},
      {"SLASH", SDL_SCANCODE_SLASH},
      {"UP", SDL_SCANCODE_UP},
      {"DOWN", SDL_SCANCODE_DOWN},
      {"LEFT", SDL_SCANCODE_LEFT},
      {"RIGHT", SDL_SCANCODE_RIGHT},
      {"F1", SDL_SCANCODE_F1},
      {"F2", SDL_SCANCODE_F2},
      {"F3", SDL_SCANCODE_F3},
      {"F4", SDL_SCANCODE_F4},
      {"F5", SDL_SCANCODE_F5},
      {"F6", SDL_SCANCODE_F6},
      {"F7", SDL_SCANCODE_F7},
      {"F8", SDL_SCANCODE_F8},
      {"F9", SDL_SCANCODE_F9},
      {"F10", SDL_SCANCODE_F10},
      {"F11", SDL_SCANCODE_F11},
      {"F12", SDL_SCANCODE_F12},
  };

  auto it = kNameToScancode.find(key);
  if (it != kNameToScancode.end())
    return it->second;

  rb_raise(rb_eRuntimeError, "%s is not a valid keyboard scancode name.",
           raw.c_str());
}

// mkxp-z style: string -> SDL_GamepadButton mapping
static const std::unordered_map<std::string, int> kStrToGPButton = {
    {"A", SDL_GAMEPAD_BUTTON_SOUTH},
    {"B", SDL_GAMEPAD_BUTTON_EAST},
    {"X", SDL_GAMEPAD_BUTTON_WEST},
    {"Y", SDL_GAMEPAD_BUTTON_NORTH},
    {"BACK", SDL_GAMEPAD_BUTTON_BACK},
    {"GUIDE", SDL_GAMEPAD_BUTTON_GUIDE},
    {"START", SDL_GAMEPAD_BUTTON_START},
    {"LEFT_STICK", SDL_GAMEPAD_BUTTON_LEFT_STICK},
    {"RIGHT_STICK", SDL_GAMEPAD_BUTTON_RIGHT_STICK},
    {"LEFT_SHOULDER", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
    {"RIGHT_SHOULDER", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
    {"DPAD_UP", SDL_GAMEPAD_BUTTON_DPAD_UP},
    {"DPAD_DOWN", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
    {"DPAD_LEFT", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
    {"DPAD_RIGHT", SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
};

bool TryGetGamepadButtonName(const std::string& key, std::string* button_name) {
  const char* prefixes[] = {"GAMEPAD_", "PAD_", "CONTROLLER_"};
  for (const char* prefix : prefixes) {
    size_t prefix_len = std::strlen(prefix);
    if (key.rfind(prefix, 0) == 0 && key.size() > prefix_len) {
      *button_name = key.substr(prefix_len);
      return true;
    }
  }
  return false;
}

int GetGamepadButtonByName(const std::string& name) {
  auto it = kStrToGPButton.find(name);
  if (it == kStrToGPButton.end())
    rb_raise(rb_eRuntimeError, "%s is not a valid gamepad button name.",
             name.c_str());
  return it->second;
}

static int GetControllerButtonArg(VALUE* argv) {
  const char* name = rb_id2name(SYM2ID(*argv));
  return GetGamepadButtonByName(name);
}

}  // namespace

MRI_METHOD(input_is_pressed) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  std::string key = GetButtonSymbol(argc, argv);
  content::ExceptionState exception_state;
  bool v = input->IsPressed(key, exception_state);
  MriProcessException(exception_state);
  return v ? Qtrue : Qfalse;
}

MRI_METHOD(input_is_triggered) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  std::string key = GetButtonSymbol(argc, argv);
  content::ExceptionState exception_state;
  bool v = input->IsTriggered(key, exception_state);
  MriProcessException(exception_state);
  return v ? Qtrue : Qfalse;
}

MRI_METHOD(input_is_repeated) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  std::string key = GetButtonSymbol(argc, argv);
  content::ExceptionState exception_state;
  bool v = input->IsRepeated(key, exception_state);
  MriProcessException(exception_state);
  return v ? Qtrue : Qfalse;
}

MRI_METHOD(input_gamepad_connected) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;
  bool v = input->IsGamepadConnected(exception_state);
  MriProcessException(exception_state);
  return v ? Qtrue : Qfalse;
}

MRI_METHOD(input_gamepad_rumble) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  uint16_t low_freq, high_freq;
  uint32_t duration_ms;
  content::ExceptionState exception_state;

  if (argc < 3)
    return Qnil;

  low_freq = static_cast<uint16_t>(NUM2INT(argv[0]));
  high_freq = static_cast<uint16_t>(NUM2INT(argv[1]));
  duration_ms = static_cast<uint32_t>(NUM2UINT(argv[2]));

  input->RumbleGamepad(low_freq, high_freq, duration_ms, exception_state);
  MriProcessException(exception_state);
  return Qtrue;
}

MRI_METHOD(input_press_ex) {
  MriCheckArgc(argc, 1);
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;

  bool v;
  if (FIXNUM_P(argv[0])) {
    v = input->KeyPressed(FIX2INT(argv[0]), exception_state);
  } else {
    std::string raw = SYMBOL_P(argv[0]) ? rb_id2name(SYM2ID(argv[0]))
                                        : StringValueCStr(argv[0]);
    std::string button_name;
    if (TryGetGamepadButtonName(NormalizeScancodeName(raw), &button_name))
      v = input->GamepadPressEx(GetGamepadButtonByName(button_name), exception_state);
    else
      v = input->KeyPressed(GetScancodeArg(argv[0]), exception_state);
  }
  MriProcessException(exception_state);
  return MRI_BOOL_VALUE(v);
}

MRI_METHOD(input_trigger_ex) {
  MriCheckArgc(argc, 1);
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;

  bool v;
  if (FIXNUM_P(argv[0])) {
    v = input->KeyTriggered(FIX2INT(argv[0]), exception_state);
  } else {
    std::string raw = SYMBOL_P(argv[0]) ? rb_id2name(SYM2ID(argv[0]))
                                        : StringValueCStr(argv[0]);
    std::string button_name;
    if (TryGetGamepadButtonName(NormalizeScancodeName(raw), &button_name))
      v = input->GamepadTriggerEx(GetGamepadButtonByName(button_name), exception_state);
    else
      v = input->KeyTriggered(GetScancodeArg(argv[0]), exception_state);
  }
  MriProcessException(exception_state);
  return MRI_BOOL_VALUE(v);
}

MRI_METHOD(input_repeat_ex) {
  MriCheckArgc(argc, 1);
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;

  bool v;
  if (FIXNUM_P(argv[0])) {
    v = input->KeyRepeated(FIX2INT(argv[0]), exception_state);
  } else {
    std::string raw = SYMBOL_P(argv[0]) ? rb_id2name(SYM2ID(argv[0]))
                                        : StringValueCStr(argv[0]);
    std::string button_name;
    if (TryGetGamepadButtonName(NormalizeScancodeName(raw), &button_name))
      v = input->GamepadRepeatEx(GetGamepadButtonByName(button_name), exception_state);
    else
      v = input->KeyRepeated(GetScancodeArg(argv[0]), exception_state);
  }
  MriProcessException(exception_state);
  return MRI_BOOL_VALUE(v);
}

MRI_METHOD(input_raw_key_states) {
  MriCheckArgc(argc, 0);
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;
  auto states = input->GetRawKeyStates(exception_state);
  MriProcessException(exception_state);

  VALUE ret = rb_ary_new();
  for (size_t i = 0; i < states.size(); ++i)
    rb_ary_push(ret, MRI_BOOL_VALUE(states[i] != 0));
  return ret;
}

// --- Input::Controller module (mkxp-z style) ---

MRI_METHOD(input_controller_connected) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;
  bool v = input->IsGamepadConnected(exception_state);
  MriProcessException(exception_state);
  return MRI_BOOL_VALUE(v);
}

MRI_METHOD(input_controller_name) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;
  std::string name = input->GetGamepadName(exception_state);
  MriProcessException(exception_state);
  return rb_utf8_str_new_cstr(name.c_str());
}

MRI_METHOD(input_controller_power_level) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;
  int level = input->GetGamepadPowerLevel(exception_state);
  MriProcessException(exception_state);

  VALUE ret;
  switch (static_cast<SDL_PowerState>(level)) {
    case SDL_POWERSTATE_CHARGED:    ret = ID2SYM(rb_intern("CHARGED")); break;
    case SDL_POWERSTATE_CHARGING:   ret = ID2SYM(rb_intern("CHARGING")); break;
    case SDL_POWERSTATE_ON_BATTERY: ret = ID2SYM(rb_intern("ON_BATTERY")); break;
    case SDL_POWERSTATE_NO_BATTERY: ret = ID2SYM(rb_intern("NO_BATTERY")); break;
    default:                        ret = ID2SYM(rb_intern("UNKNOWN")); break;
  }
  return ret;
}

MRI_METHOD(input_controller_axes_left) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;
  auto axes = input->GetGamepadAxisLeft(exception_state);
  MriProcessException(exception_state);

  VALUE ret = rb_ary_new();
  rb_ary_push(ret, rb_float_new(axes[0]));
  rb_ary_push(ret, rb_float_new(axes[1]));
  return ret;
}

MRI_METHOD(input_controller_axes_right) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;
  auto axes = input->GetGamepadAxisRight(exception_state);
  MriProcessException(exception_state);

  VALUE ret = rb_ary_new();
  rb_ary_push(ret, rb_float_new(axes[0]));
  rb_ary_push(ret, rb_float_new(axes[1]));
  return ret;
}

MRI_METHOD(input_controller_axes_trigger) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;
  auto axes = input->GetGamepadAxisTrigger(exception_state);
  MriProcessException(exception_state);

  VALUE ret = rb_ary_new();
  rb_ary_push(ret, rb_float_new(axes[0]));
  rb_ary_push(ret, rb_float_new(axes[1]));
  return ret;
}

MRI_METHOD(input_controller_press_ex) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;

  int button;
  if (SYMBOL_P(argv[0])) {
    button = GetControllerButtonArg(argv);
  } else {
    button = NUM2INT(argv[0]);
  }

  bool v = input->GamepadPressEx(button, exception_state);
  MriProcessException(exception_state);
  return MRI_BOOL_VALUE(v);
}

MRI_METHOD(input_controller_trigger_ex) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;

  int button;
  if (SYMBOL_P(argv[0])) {
    button = GetControllerButtonArg(argv);
  } else {
    button = NUM2INT(argv[0]);
  }

  bool v = input->GamepadTriggerEx(button, exception_state);
  MriProcessException(exception_state);
  return MRI_BOOL_VALUE(v);
}

MRI_METHOD(input_controller_repeat_ex) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;

  int button;
  if (SYMBOL_P(argv[0])) {
    button = GetControllerButtonArg(argv);
  } else {
    button = NUM2INT(argv[0]);
  }

  bool v = input->GamepadRepeatEx(button, exception_state);
  MriProcessException(exception_state);
  return MRI_BOOL_VALUE(v);
}

MRI_METHOD(input_controller_release_ex) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;

  int button;
  if (SYMBOL_P(argv[0])) {
    button = GetControllerButtonArg(argv);
  } else {
    button = NUM2INT(argv[0]);
  }

  bool v = input->GamepadReleaseEx(button, exception_state);
  MriProcessException(exception_state);
  return MRI_BOOL_VALUE(v);
}

MRI_METHOD(input_controller_repeat_count) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;

  int button;
  if (SYMBOL_P(argv[0])) {
    button = GetControllerButtonArg(argv);
  } else {
    button = NUM2INT(argv[0]);
  }

  int count = input->GamepadRepeatCountEx(button, exception_state);
  MriProcessException(exception_state);
  return INT2NUM(count);
}

MRI_METHOD(input_controller_repeat_time_ex) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;

  int button;
  if (SYMBOL_P(argv[0])) {
    button = GetControllerButtonArg(argv);
  } else {
    button = NUM2INT(argv[0]);
  }

  double t = input->GamepadButtonTimeEx(button, exception_state);
  MriProcessException(exception_state);
  return rb_float_new(t);
}

MRI_METHOD(input_controller_raw_button_states) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;
  auto states = input->GetGamepadRawButtonStates(exception_state);
  MriProcessException(exception_state);

  VALUE ret = rb_ary_new();
  for (size_t i = 0; i < states.size(); ++i)
    rb_ary_push(ret, MRI_BOOL_VALUE(states[i] != 0));
  return ret;
}

MRI_METHOD(input_controller_raw_axes) {
  scoped_refptr<content::Input> input = MriGetGlobalModules()->Input;
  content::ExceptionState exception_state;
  auto axes = input->GetGamepadRawAxes(exception_state);
  MriProcessException(exception_state);

  VALUE ret = rb_ary_new();
  for (size_t i = 0; i < axes.size(); ++i)
    rb_ary_push(ret, rb_float_new(axes[i]));
  return ret;
}

void ApplyInputPatch() {
  VALUE klass = rb_const_get(rb_cObject, rb_intern("Input"));

  MriDefineModuleFunction(klass, "press?", input_is_pressed);
  MriDefineModuleFunction(klass, "trigger?", input_is_triggered);
  MriDefineModuleFunction(klass, "repeat?", input_is_repeated);
  MriDefineModuleFunction(klass, "gamepad_connected?", input_gamepad_connected);
  MriDefineModuleFunction(klass, "gamepad_rumble", input_gamepad_rumble);
  MriDefineModuleFunction(klass, "pressex?", input_press_ex);
  MriDefineModuleFunction(klass, "triggerex?", input_trigger_ex);
  MriDefineModuleFunction(klass, "repeatex?", input_repeat_ex);
  MriDefineModuleFunction(klass, "raw_key_states", input_raw_key_states);

  // mkxp-z Input::Controller module
  VALUE controller_mod = rb_define_module_under(klass, "Controller");
  MriDefineModuleFunction(controller_mod, "connected?", input_controller_connected);
  MriDefineModuleFunction(controller_mod, "name", input_controller_name);
  MriDefineModuleFunction(controller_mod, "power_level", input_controller_power_level);
  MriDefineModuleFunction(controller_mod, "axes_left", input_controller_axes_left);
  MriDefineModuleFunction(controller_mod, "axes_right", input_controller_axes_right);
  MriDefineModuleFunction(controller_mod, "axes_trigger", input_controller_axes_trigger);
  MriDefineModuleFunction(controller_mod, "pressex?", input_controller_press_ex);
  MriDefineModuleFunction(controller_mod, "triggerex?", input_controller_trigger_ex);
  MriDefineModuleFunction(controller_mod, "repeatex?", input_controller_repeat_ex);
  MriDefineModuleFunction(controller_mod, "releaseex?", input_controller_release_ex);
  MriDefineModuleFunction(controller_mod, "repeatcount", input_controller_repeat_count);
  MriDefineModuleFunction(controller_mod, "timeex?", input_controller_repeat_time_ex);
  MriDefineModuleFunction(controller_mod, "raw_button_states", input_controller_raw_button_states);
  MriDefineModuleFunction(controller_mod, "raw_axes", input_controller_raw_axes);

  for (size_t i = 0; i < std::size(kKeyboardBindings); ++i) {
    auto& binding_set = kKeyboardBindings[i];
    ID key = rb_intern(binding_set.name.c_str());
    rb_const_set(klass, key, INT2FIX(binding_set.key_id));
  }

  for (size_t i = 0; i < std::size(kScancodeBindings); ++i) {
    auto& binding_set = kScancodeBindings[i];
    ID key = rb_intern(binding_set.name.c_str());
    rb_const_set(klass, key, INT2FIX(binding_set.key_id));
  }
}

struct MouseButtonSet {
  std::string name;
  int button_id;
};

const MouseButtonSet kMouseButtonSets[] = {
    {"LEFT", content::MouseImpl::Button::Left},
    {"MIDDLE", content::MouseImpl::Button::Middle},
    {"RIGHT", content::MouseImpl::Button::Right},
    {"X1", content::MouseImpl::Button::X1},
    {"X2", content::MouseImpl::Button::X2},
};

void ApplyMousePatch() {
  VALUE klass = rb_const_get(rb_cObject, rb_intern("Mouse"));

  for (size_t i = 0; i < std::size(kMouseButtonSets); ++i)
    rb_const_set(klass, rb_intern(kMouseButtonSets[i].name.c_str()),
                 INT2FIX(kMouseButtonSets[i].button_id));
}

MRI_METHOD(console_write) {
  if (argc < 1)
    return Qnil;

  std::string text;
  for (int i = 0; i < argc; ++i) {
    VALUE str = rb_obj_as_string(argv[i]);
    if (!text.empty()) text += "\t";
    text += std::string(RSTRING_PTR(str), RSTRING_LEN(str));
  }

  auto* ctx = MriGetCurrentContext();
  if (ctx)
    ctx->console.Push("[Script] " + text);

  return Qnil;
}

MRI_METHOD(console_puts) {
  if (argc < 1)
    return Qnil;

  for (int i = 0; i < argc; ++i) {
    VALUE str = rb_obj_as_string(argv[i]);
    std::string s(RSTRING_PTR(str), RSTRING_LEN(str));
    auto* ctx = MriGetCurrentContext();
    if (ctx)
      ctx->console.Push("[Script] " + s);
  }

  return Qnil;
}

MRI_METHOD(console_clear) {
  auto* ctx = MriGetCurrentContext();
  if (ctx)
    ctx->console.output.clear();
  return Qnil;
}

MRI_METHOD(console_save) {
  VALUE path = argv[0];
  std::string path_str(StringValueCStr(path));

  auto* ctx = MriGetCurrentContext();
  if (!ctx)
    return Qfalse;

  std::ofstream log_file(path_str);
  if (!log_file.is_open())
    return Qfalse;

  for (const auto& line : ctx->console.output)
    log_file << line << "\n";

  log_file.close();
  return Qtrue;
}

void ApplyConsolePatch() {
  VALUE klass = rb_define_module("Console");

  MriDefineModuleFunction(klass, "write", console_write);
  MriDefineModuleFunction(klass, "puts", console_puts);
  MriDefineModuleFunction(klass, "clear", console_clear);
  MriDefineModuleFunction(klass, "save", console_save);
}

void MriApplyBindingPatch() {
  ApplyInputPatch();
  ApplyMousePatch();
  ApplyConsolePatch();

  // mkxp-z style Controller constants
  VALUE controller_mod = rb_const_get(
      rb_const_get(rb_cObject, rb_intern("Input")),
      rb_intern("Controller"));
  rb_const_set(controller_mod, rb_intern("A"), INT2FIX(SDL_GAMEPAD_BUTTON_SOUTH));
  rb_const_set(controller_mod, rb_intern("B"), INT2FIX(SDL_GAMEPAD_BUTTON_EAST));
  rb_const_set(controller_mod, rb_intern("X"), INT2FIX(SDL_GAMEPAD_BUTTON_WEST));
  rb_const_set(controller_mod, rb_intern("Y"), INT2FIX(SDL_GAMEPAD_BUTTON_NORTH));
  rb_const_set(controller_mod, rb_intern("BACK"), INT2FIX(SDL_GAMEPAD_BUTTON_BACK));
  rb_const_set(controller_mod, rb_intern("GUIDE"), INT2FIX(SDL_GAMEPAD_BUTTON_GUIDE));
  rb_const_set(controller_mod, rb_intern("START"), INT2FIX(SDL_GAMEPAD_BUTTON_START));
  rb_const_set(controller_mod, rb_intern("LEFT_STICK"), INT2FIX(SDL_GAMEPAD_BUTTON_LEFT_STICK));
  rb_const_set(controller_mod, rb_intern("RIGHT_STICK"), INT2FIX(SDL_GAMEPAD_BUTTON_RIGHT_STICK));
  rb_const_set(controller_mod, rb_intern("LEFT_SHOULDER"), INT2FIX(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
  rb_const_set(controller_mod, rb_intern("RIGHT_SHOULDER"), INT2FIX(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER));
  rb_const_set(controller_mod, rb_intern("DPAD_UP"), INT2FIX(SDL_GAMEPAD_BUTTON_DPAD_UP));
  rb_const_set(controller_mod, rb_intern("DPAD_DOWN"), INT2FIX(SDL_GAMEPAD_BUTTON_DPAD_DOWN));
  rb_const_set(controller_mod, rb_intern("DPAD_LEFT"), INT2FIX(SDL_GAMEPAD_BUTTON_DPAD_LEFT));
  rb_const_set(controller_mod, rb_intern("DPAD_RIGHT"), INT2FIX(SDL_GAMEPAD_BUTTON_DPAD_RIGHT));
}

}  // namespace binding
