// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD - style license that can be
// found in the LICENSE file.

#include "binding/mri/mri_util.h"
#include "ruby/version.h"

#include "binding/mri/mri_main.h"

#include <iostream>

#include "SDL3/SDL_messagebox.h"
#include "zlib/zlib.h"

#include "binding/mri/binding_patch.h"
#include "binding/mri/mri_file.h"
#include "binding/mri/urge_binding.h"

#include "binding/mri/autogen_audio_binding.h"
#include "binding/mri/autogen_bitmap_binding.h"
#include "binding/mri/autogen_color_binding.h"
#include "binding/mri/autogen_font_binding.h"
#include "binding/mri/autogen_graphics_binding.h"
#include "binding/mri/autogen_input_binding.h"
#include "binding/mri/autogen_mouse_binding.h"
#include "binding/mri/autogen_plane_binding.h"
#include "binding/mri/autogen_rect_binding.h"
#include "binding/mri/autogen_sprite_binding.h"
#include "binding/mri/autogen_table_binding.h"
#include "binding/mri/autogen_tilemap2_binding.h"
#include "binding/mri/autogen_tilemap_binding.h"
#include "binding/mri/autogen_tilemapautotile_binding.h"
#include "binding/mri/autogen_tilemapbitmap_binding.h"
#include "binding/mri/autogen_tone_binding.h"
#include "binding/mri/autogen_viewport_binding.h"
#include "binding/mri/autogen_window2_binding.h"
#include "binding/mri/autogen_window_binding.h"

extern "C" {
void rb_call_builtin_inits();
void Init_zlib();
}

namespace binding {

content::ExecutionContext* g_current_execution_context = nullptr;
extern filesystem::IOService* g_io_service;

namespace {

using EvalParamater = struct {
  VALUE string;
  VALUE filename;
};

// eval script, nil, title
VALUE EvalInternal(EvalParamater* arg) {
  VALUE argv[] = {arg->string, Qnil, arg->filename};
  return rb_funcall2(Qnil, rb_intern("eval"), 3, argv);
}

VALUE EvalString(VALUE string, VALUE filename, int* state) {
  EvalParamater arg = {string, filename};
  return rb_protect((VALUE(*)(VALUE))EvalInternal, (VALUE)&arg, state);
}

std::string InsertNewLines(const std::string& input, size_t interval) {
  std::string result;
  size_t length = input.length();

  for (size_t i = 0; i < length; i += interval) {
    result += input.substr(i, interval);
    if (i + interval < length)
      result += '\n';
  }

  return result;
}

void ParseExeceptionInfo(VALUE exc,
                         const BindingEngineMri::BacktraceData& btData) {
  VALUE exeception_name = rb_class_path(rb_obj_class(exc));
  VALUE backtrace = rb_funcall2(exc, rb_intern("backtrace"), 0, NULL);
  VALUE backtrace_front = rb_ary_entry(backtrace, 0);

  VALUE ds = rb_sprintf("%" PRIsVALUE ": %" PRIsVALUE " (%" PRIsVALUE ")",
                        backtrace_front, exc, exeception_name);

  std::string error_info(StringValueCStr(ds));
  LOG(INFO) << "[Binding] " << error_info;

  error_info = InsertNewLines(error_info, 128);
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Engine Error",
                           error_info.c_str(), nullptr);
}

VALUE RgssMainCb(VALUE block) {
  rb_funcall2(block, rb_intern("call"), 0, 0);
  return Qnil;
}

VALUE RgssMainRescue(VALUE arg, VALUE exc) {
  VALUE* excRet = (VALUE*)arg;

  *excRet = exc;

  return Qnil;
}

void MriProcessReset() {
  content::ExceptionState exception_state;
  MriGetGlobalModules()->Graphics->Reset(exception_state);
  MriGetGlobalModules()->Audio->Reset(exception_state);
}

}  // namespace

MRI_METHOD(MRI_RGSSMain) {
  bool gc_required = false;

  while (true) {
    VALUE exc = Qnil;
    if (gc_required) {
      rb_gc_start();
      gc_required = false;
    }

    rb_rescue2((VALUE(*)(ANYARGS))RgssMainCb, rb_block_proc(),
               (VALUE(*)(ANYARGS))RgssMainRescue, (VALUE)&exc, rb_eException,
               (VALUE)0);

    if (NIL_P(exc))
      break;

    if (rb_obj_class(exc) ==
        MriGetException(content::ExceptionCode::CODE_NUMS)) {
      gc_required = true;
      MriProcessReset();
    } else
      rb_exc_raise(exc);
  }

  return Qnil;
}

MRI_METHOD(MRI_RGSSSTOP) {
  scoped_refptr<content::Graphics> screen = MriGetGlobalModules()->Graphics;

  content::ExceptionState exception_state;
  while (true)
    screen->Update(exception_state);

  return Qnil;
}

BindingEngineMri::BindingEngineMri() = default;

BindingEngineMri::~BindingEngineMri() = default;

void BindingEngineMri::PreEarlyInitialization(
    content::ContentProfile* profile,
    filesystem::IOService* io_service) {
  profile_ = profile;
  g_io_service = io_service;

  int argc = 0;
  char** argv = 0;
  ruby_sysinit(&argc, &argv);

  RUBY_INIT_STACK;
  ruby_init();
  ruby_init_loadpath();
  rb_call_builtin_inits();

  rb_enc_set_default_internal(rb_enc_from_encoding(rb_utf8_encoding()));
  rb_enc_set_default_external(rb_enc_from_encoding(rb_utf8_encoding()));

  MriInitException(profile->api_version >=
                   content::ContentProfile::APIVersion::RGSS3);

  Init_zlib();
  InitURGEBinding();
  InitCoreFileBinding();

  InitAudioBinding();
  InitBitmapBinding();
  InitColorBinding();
  InitFontBinding();
  InitGraphicsBinding();
  InitInputBinding();
  InitMouseBinding();
  InitPlaneBinding();
  InitRectBinding();
  InitSpriteBinding();
  InitTableBinding();
  InitTilemapBinding();
  InitTilemapAutotileBinding();
  InitTilemapBitmapBinding();
  InitTilemap2Binding();
  InitToneBinding();
  InitViewportBinding();
  InitWindowBinding();
  InitWindow2Binding();

  MriApplyBindingPatch();

  if (profile->api_version < content::ContentProfile::APIVersion::RGSS3) {
    if (sizeof(void*) == 4) {
      MriDefineMethod(rb_cNilClass, "id", MriReturnInt<4>);
      MriDefineMethod(rb_cTrueClass, "id", MriReturnInt<2>);
    } else if (sizeof(void*) == 8) {
      MriDefineMethod(rb_cNilClass, "id", MriReturnInt<8>);
      MriDefineMethod(rb_cTrueClass, "id", MriReturnInt<20>);
    } else {
      NOTREACHED();
    }

    rb_const_set(rb_cObject, rb_intern("TRUE"), Qtrue);
    rb_const_set(rb_cObject, rb_intern("FALSE"), Qfalse);
    rb_const_set(rb_cObject, rb_intern("NIL"), Qnil);
  }

  MriDefineModuleFunction(rb_mKernel, "rgss_main", MRI_RGSSMain);
  MriDefineModuleFunction(rb_mKernel, "rgss_stop", MRI_RGSSSTOP);

  LOG(INFO) << "[Binding] CRuby Interpreter Version: " << RUBY_API_VERSION_CODE;
  LOG(INFO) << "[Binding] CRuby Interpreter Platform: " << RUBY_PLATFORM;
}

void BindingEngineMri::OnMainMessageLoopRun(
    content::ExecutionContext* execution,
    ScopedModuleContext* module_context) {
  // Set global execution context
  g_current_execution_context = execution;

  // Define running modules
  MriGetGlobalModules()->Graphics = module_context->graphics;
  MriGetGlobalModules()->Input = module_context->input;
  MriGetGlobalModules()->Audio = module_context->audio;
  MriGetGlobalModules()->Mouse = module_context->mouse;

  // Run packed scripts
  content::ExceptionState exception_state;
  LoadPackedScripts(profile_, exception_state);
  if (exception_state.HadException()) {
    std::string error_message;
    exception_state.FetchException(error_message);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Engine Error",
                             error_message.c_str(), nullptr);
  }
}

void BindingEngineMri::PostMainLoopRunning() {
  bool pause_required = false;

  VALUE exc = rb_errinfo();
  if (!NIL_P(exc) && !rb_obj_is_kind_of(exc, rb_eSystemExit)) {
    ParseExeceptionInfo(exc, backtrace_);
    pause_required = true;
  }

  ruby_cleanup(0);
  g_current_execution_context = nullptr;
  *MriGetGlobalModules() = GlobalModules();

  if (pause_required)
    std::cin.get();

  LOG(INFO) << "[Binding] Quit mri binding engine.";
}

void BindingEngineMri::ExitSignalRequired() {
  rb_raise(rb_eSystemExit, "");
}

void BindingEngineMri::ResetSignalRequired() {
  VALUE rb_eReset = MriGetException(content::ExceptionCode::CODE_NUMS);
  rb_raise(rb_eReset, "");
}

void BindingEngineMri::LoadPackedScripts(
    content::ContentProfile* profile,
    content::ExceptionState& exception_state) {
  VALUE packed_scripts = MriLoadData(profile->script_path, exception_state);
  if (exception_state.HadException())
    return;

  if (!RB_TYPE_P(packed_scripts, RUBY_T_ARRAY)) {
    LOG(INFO) << "[Binding] Failed to read script data.";
    return;
  }

  rb_gv_set("$RGSS_SCRIPTS", packed_scripts);

  long scripts_count = RARRAY_LEN(packed_scripts);
  std::string zlib_decode_buffer(0x1000, 0);

  for (long i = 0; i < scripts_count; ++i) {
    VALUE script = rb_ary_entry(packed_scripts, i);

    // 0 -> ScriptID for Editor
    VALUE script_name = rb_ary_entry(script, 1);
    VALUE script_src = rb_ary_entry(script, 2);
    unsigned long buffer_size;

    int zlib_result = Z_OK;

    while (true) {
      uint8_t* buffer_ptr =
          reinterpret_cast<uint8_t*>(zlib_decode_buffer.data());
      buffer_size = zlib_decode_buffer.length();

      const uint8_t* source_ptr =
          reinterpret_cast<const uint8_t*>(RSTRING_PTR(script_src));
      const uint32_t source_size = RSTRING_LEN(script_src);

      zlib_result =
          uncompress(buffer_ptr, &buffer_size, source_ptr, source_size);

      buffer_ptr[buffer_size] = '\0';

      if (zlib_result != Z_BUF_ERROR)
        break;

      zlib_decode_buffer.resize(zlib_decode_buffer.size() * 2);
    }

    if (zlib_result != Z_OK) {
      static char buffer[256];
      snprintf(buffer, sizeof(buffer), "Error decoding script %ld: '%s'", i,
               RSTRING_PTR(script_name));

      LOG(INFO) << buffer;
      break;
    }

    rb_ary_store(script, 3, rb_str_new_cstr(zlib_decode_buffer.c_str()));
  }

  VALUE exc = rb_gv_get("$!");
  if (exc != Qnil)
    return;

  while (true) {
    for (long i = 0; i < scripts_count; ++i) {
      VALUE script = rb_ary_entry(packed_scripts, i);
      const char* script_name = RSTRING_PTR(rb_ary_entry(script, 1));
      VALUE script_src = rb_ary_entry(script, 3);

      VALUE utf8_string =
          MriStringUTF8(RSTRING_PTR(script_src), RSTRING_LEN(script_src));

      char filename_buffer[512] = {0};
      int len = snprintf(filename_buffer, sizeof(filename_buffer), "%03ld: %s",
                         i, script_name);

      VALUE filename = MriStringUTF8(filename_buffer, len);
      backtrace_.emplace(filename_buffer, script_name);

      int state;
      EvalString(utf8_string, filename, &state);
      if (state)
        break;
    }

    VALUE exc = rb_gv_get("$!");
    if (rb_obj_class(exc) != MriGetException(content::ExceptionCode::CODE_NUMS))
      break;

    MriProcessReset();
  }
}

}  // namespace binding
