// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD - style license that can be
// found in the LICENSE file.

#include "binding/mri/input_binding_patch.h"

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

const int kKeyboardBindingsSize =
    sizeof(kKeyboardBindings) / sizeof(kKeyboardBindings[0]);

std::string GetButtonSymbol(int argc, VALUE* argv) {
  MriCheckArgc(argc, 1);

  std::string sym;
  if (FIXNUM_P(*argv)) {
    int key_id = FIX2INT(*argv);
    for (int i = 0; i < kKeyboardBindingsSize; ++i)
      if (kKeyboardBindings[i].key_id == key_id)
        return kKeyboardBindings[i].name;
  } else if (SYMBOL_P(*argv)) {
    MriParseArgsTo(argc, argv, "n", &sym);
  }

  return sym;
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

void ApplyInputBindingPatch() {
  VALUE klass = rb_const_get(rb_cObject, rb_intern("Input"));

  MriDefineModuleFunction(klass, "press?", input_is_pressed);
  MriDefineModuleFunction(klass, "trigger?", input_is_triggered);
  MriDefineModuleFunction(klass, "repeat?", input_is_repeated);

  for (int i = 0; i < kKeyboardBindingsSize; ++i) {
    auto& binding_set = kKeyboardBindings[i];
    ID key = rb_intern(binding_set.name.c_str());
    rb_const_set(klass, key, INT2FIX(binding_set.key_id));
  }
}

}  // namespace binding
