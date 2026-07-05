// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD - style license that can be
// found in the LICENSE file.

#include "binding/mri/binding_patch.h"

#include <fstream>
#include <unordered_map>

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

static int GetControllerButtonArg(VALUE* argv) {
  const char* name = rb_id2name(SYM2ID(*argv));
  auto it = kStrToGPButton.find(name);
  if (it == kStrToGPButton.end())
    rb_raise(rb_eRuntimeError, "%s is not a valid gamepad button name.", name);
  return it->second;
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
