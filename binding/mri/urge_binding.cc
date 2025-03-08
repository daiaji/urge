// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD - style license that can be
// found in the LICENSE file.

#include "binding/mri/urge_binding.h"

#include <iostream>

#include "SDL3/SDL_locale.h"
#include "SDL3/SDL_messagebox.h"
#include "SDL3/SDL_misc.h"
#include "SDL3/SDL_platform.h"
#include "SDL3/SDL_timer.h"

namespace binding {

content::ExecutionContext* g_current_execution_context = nullptr;

namespace {

MRI_METHOD(URGEOpenURL) {
  std::string url;
  MriParseArgsTo(argc, argv, "s", &url);

  SDL_OpenURL(url.c_str());

  return Qnil;
}

MRI_METHOD(URGEGetCounter) {
  return rb_uint2big(SDL_GetPerformanceCounter());
}

MRI_METHOD(URGEGetCounterFreq) {
  return rb_uint2big(SDL_GetPerformanceFrequency());
}

MRI_METHOD(URGEMsgbox) {
  std::string msg, title = "Info";
  int msgbox_flag = SDL_MESSAGEBOX_INFORMATION;
  MriParseArgsTo(argc, argv, "s|si", &msg, &title, &msgbox_flag);

  SDL_ShowSimpleMessageBox(msgbox_flag, title.c_str(), msg.c_str(), nullptr);
  return Qnil;
}

MRI_METHOD(URGEConsolePuts) {
  VALUE buf = rb_str_buf_new(128);
  for (int i = 0; i < argc; ++i) {
    VALUE inspect_str = rb_funcall2(argv[i], rb_intern("inspect"), 0, nullptr);
    rb_str_buf_append(buf, inspect_str);

    if (i < argc)
      rb_str_buf_cat2(buf, " ");
  }

  LOG(INFO) << "[Console] " << RSTRING_PTR(buf);

  return Qnil;
}

MRI_METHOD(URGEConsoleGets) {
  std::string str;
  std::cin >> str;

  return rb_utf8_str_new(str.c_str(), str.size());
}

MRI_METHOD(URGEDelay) {
  MriCheckArgc(argc, 1);
  SDL_Delay(FIX2INT(argv[0]));
  return Qnil;
}

void InitURGEBinding() {
  VALUE module = rb_define_module("URGE");

  // Open url
  MriDefineModuleFunction(module, "open_url", URGEOpenURL);

  // Console operate
  VALUE console = rb_define_module_under(module, "Console");
  MriDefineModuleFunction(console, "puts", URGEConsolePuts);
  MriDefineModuleFunction(console, "gets", URGEConsoleGets);

  // Etc
  MriDefineModuleFunction(module, "msgbox", URGEMsgbox);

  // Graphics Etc
  MriDefineModuleFunction(module, "get_counter", URGEGetCounter);
  MriDefineModuleFunction(module, "get_counter_freq", URGEGetCounterFreq);
  MriDefineModuleFunction(module, "delay", URGEDelay);
}

}  // namespace binding
