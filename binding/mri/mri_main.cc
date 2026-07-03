// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD - style license that can be
// found in the LICENSE file.

#include "binding/mri/mri_util.h"
#include "ruby/version.h"

#include "binding/mri/mri_main.h"

#include "SDL3/SDL_messagebox.h"
#include "zlib/zlib.h"

#include "binding/mri/binding_patch.h"
#include "binding/mri/mri_file.h"
#include "binding/mri/mri_init_autogen.h"
#include "binding/mri/platform/mri_emscripten.h"

#ifdef HAVE_GET_MACHINE_HASH
#include "admenri/machineid/machineid.h"
#endif

extern "C" {
void Init_zlib(void);
void Init_ffi_c(void);
void rb_call_builtin_inits();
}

namespace binding {

content::ExecutionContext* g_current_execution_context = nullptr;
extern filesystem::IOService* g_io_service;

namespace {

VALUE EvalString(VALUE string, VALUE filename, int32_t* state) {
  using EvaluteContext = struct {
    VALUE string;
    VALUE filename;
  };

  auto evaluate = [](VALUE parameter) -> VALUE {
    EvaluteContext* context = reinterpret_cast<EvaluteContext*>(parameter);
    VALUE argv[] = {context->string, Qnil, context->filename};
    return rb_funcall2(Qnil, rb_intern("eval"), std::size(argv), argv);
  };

  EvaluteContext context = {string, filename};
  return rb_protect(evaluate, reinterpret_cast<VALUE>(&context), state);
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

std::string ParseExeceptionInfo(VALUE exception) {
  VALUE exeception_name = rb_class_path(rb_obj_class(exception));
  VALUE exception_message = rb_obj_as_string(exception);
  VALUE backtrace = rb_funcall2(exception, rb_intern("backtrace"), 0, NULL);
  VALUE backtrace_front = rb_str_new2("Unknown Location");
  if (backtrace != Qnil)
    backtrace_front = rb_ary_entry(backtrace, 0);

  VALUE format_errors =
      rb_sprintf("%" PRIsVALUE " (%" PRIsVALUE ") :\n%" PRIsVALUE,
                 backtrace_front, exeception_name, exception_message);

  std::string error_info = StringValueCStr(format_errors);
  return InsertNewLines(error_info, 128);
}

void MriProcessReset() {
  content::ExceptionState exception_state;
  MriGetGlobalModules()->Graphics->Reset(exception_state);
  MriGetGlobalModules()->Audio->Reset(exception_state);
  MriGetGlobalModules()->URGE->Reset(exception_state);
  MriProcessException(exception_state);
}

static VALUE RescueCallBlock(VALUE block) {
  return rb_funcall2(block, rb_intern("call"), 0, nullptr);
};

static VALUE RescueException(VALUE param, VALUE exception) {
  return *reinterpret_cast<VALUE*>(param) = exception;
};

MRI_METHOD(MRI_RGSSMain) {
  bool gc_required = false;

  while (true) {
    VALUE exception = Qnil;
    if (gc_required) {
      rb_gc_start();
      gc_required = false;
    }

    rb_rescue2(RescueCallBlock,
               rb_block_proc(),
               RescueException,
               (VALUE)&exception, rb_eException, nullptr);

    if (NIL_P(exception))
      break;

    if (rb_obj_class(exception) ==
        MriGetException(content::ExceptionCode::CODE_NUMS)) {
      gc_required = true;
      MriProcessReset();
    } else {
      gc_required = false;
      rb_exc_raise(exception);
    }
  }

  return Qnil;
}

MRI_METHOD(MRI_RGSSStop) {
  scoped_refptr<content::Graphics> screen = MriGetGlobalModules()->Graphics;

  content::ExceptionState exception_state;
  while (true)
    screen->Update(exception_state);

  return Qnil;
}

#ifdef HAVE_GET_MACHINE_HASH
MRI_METHOD(ADMENRI_getMachineHash) {
  auto hash = admenri::GetMachineHash();
  return rb_str_new(hash.data(), hash.size());
}
#endif

}  // namespace

BindingEngineMri::BindingEngineMri() : profile_(nullptr) {}

BindingEngineMri::~BindingEngineMri() = default;

void BindingEngineMri::PreEarlyInitialization(
    content::ContentProfile* profile,
    filesystem::IOService* io_service) {
  profile_ = profile;
  g_io_service = io_service;

  int32_t argc = 0;
  char** argv = nullptr;
  ruby_sysinit(&argc, &argv);

  RUBY_INIT_STACK;
  ruby_init();
  ruby_init_loadpath();

  rb_enc_set_default_internal(rb_enc_from_encoding(rb_utf8_encoding()));
  rb_enc_set_default_external(rb_enc_from_encoding(rb_utf8_encoding()));

  // rb_call_builtin_inits is hidden in Ruby shared library on Linux
  // (not in .dynsym due to -fvisibility=hidden).
  // Try dlsym first; fall back to dladdr + build-time extracted offset.
#if defined(OS_LINUX)
  #include <dlfcn.h>
  auto fn = (void (*)())dlsym(RTLD_DEFAULT, "rb_call_builtin_inits");
  if (!fn) {
    #include "mri_builtin_offset.h"
    #ifdef MRI_BUILTIN_INITS_OFFSET
      Dl_info info;
      dladdr((void*)ruby_init, &info);
      fn = (void (*)())((uintptr_t)info.dli_fbase + MRI_BUILTIN_INITS_OFFSET);
    #else
      #error "MRI_BUILTIN_INITS_OFFSET undefined. Install binutils or configure manually."
    #endif
  }
  if (fn)
    fn();
#elif RAPI_FULL >= 300
  rb_call_builtin_inits();
#endif

  // internal exception
  MriInitException(profile->api_version >=
                   content::ContentProfile::APIVersion::RGSS3);

  // marshal data reader
  InitCoreFileBinding();

  // URGE autogen bindings
  InitMriAutogen();

  // C extensions
  Init_zlib();
#if defined(OS_WIN)
  Init_ffi_c();
#endif

  // Autogen binding patching
  MriApplyBindingPatch();

#if defined(OS_EMSCRIPTEN)
  InitEmscriptenBinding();
#endif  // OS_EMSCRIPTEN

  MriDefineModuleFunction(rb_mKernel, "rgss_main", MRI_RGSSMain);
  MriDefineModuleFunction(rb_mKernel, "rgss_stop", MRI_RGSSStop);

#ifdef HAVE_GET_MACHINE_HASH
  VALUE class_admenri = rb_define_module("Admenri");
  MriDefineModuleFunction(class_admenri, "get_machine_hash",
                          ADMENRI_getMachineHash);
#endif

  LOG(INFO) << "[Binding] CRuby Interpreter Version: " << RUBY_API_VERSION_CODE;
#ifndef OS_MACOSX   // RUBY_PLATFORM macro causes compile error on macos
  LOG(INFO) << "[Binding] CRuby Interpreter Platform: " << RUBY_PLATFORM;
#endif

  VALUE debug = MRI_BOOL_VALUE(profile->game_debug);
  if (profile->api_version < content::ContentProfile::APIVersion::RGSS2)
    rb_gv_set("DEBUG", debug);
  else
    rb_gv_set("TEST", debug);
  rb_gv_set("BTEST", MRI_BOOL_VALUE(profile->game_battle_test));
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
  MriGetGlobalModules()->URGE = module_context->engine;

  // Console eval callback (evaluates Ruby code, returns result as string)
  execution->eval_ruby = [](const std::string& code) -> std::string {
    // Create explicit UTF-8 source string to avoid rb_str_new_cstr ASCII-8BIT tagging
    VALUE code_utf8 = rb_utf8_str_new(code.c_str(), code.length());
    // Use TOPLEVEL_BINDING so def/class/variables persist between eval calls
    VALUE top_binding =
        rb_const_get(rb_cObject, rb_intern("TOPLEVEL_BINDING"));
    VALUE eval_argv[] = {code_utf8, top_binding,
                         rb_utf8_str_new_cstr("(console)")};

    auto eval_proc = [](VALUE args) -> VALUE {
      VALUE* argv = reinterpret_cast<VALUE*>(args);
      return rb_funcall2(Qnil, rb_intern("eval"), 3, argv);
    };

    int state = 0;
    VALUE result = rb_protect(eval_proc, reinterpret_cast<VALUE>(eval_argv), &state);
    if (state) {
      VALUE err = rb_errinfo();
      VALUE msg = rb_funcall(err, rb_intern("message"), 0);
      VALUE full = rb_str_plus(msg, rb_utf8_str_new_cstr("\n"));
      VALUE bt = rb_funcall(err, rb_intern("backtrace"), 0);
      if (!NIL_P(bt)) {
        VALUE bt_text = rb_funcall(bt, rb_intern("first"), 1, INT2FIX(5));
        full = rb_str_plus(full, rb_funcall(bt_text, rb_intern("join"), 1, rb_utf8_str_new_cstr("\n")));
      }
      rb_set_errinfo(Qnil);
      return std::string(RSTRING_PTR(full), RSTRING_LEN(full));
    }
    if (result == Qnil)
      return "=> nil";
    VALUE str = rb_obj_as_string(result);
    return "=> " + std::string(RSTRING_PTR(str), RSTRING_LEN(str));
  };

  // Run packed scripts
  content::ExceptionState exception_state;
  LoadPackedScripts(profile_, exception_state);
  if (exception_state.HadException()) {
    std::string error_message;
    exception_state.FetchException(error_message);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "URGE",
                             error_message.c_str(), nullptr);
    if (execution) {
      execution->console.Push("[Ruby Error] Script load error:");
      execution->console.Push("[Ruby Error] " + error_message);
    }
  }

  // Debug: log any Ruby exception info
  VALUE rb_ex = rb_errinfo();
  if (!NIL_P(rb_ex) && !rb_obj_is_kind_of(rb_ex, rb_eSystemExit)) {
    VALUE msg = rb_funcall(rb_ex, rb_intern("message"), 0);
    VALUE bt  = rb_funcall(rb_ex, rb_intern("backtrace"), 0);
    std::string msg_str = StringValueCStr(msg);
    std::string bt_str;
    if (!NIL_P(bt)) {
      VALUE bt_val = rb_funcall(bt, rb_intern("join"), 1, rb_str_new_cstr("\n"));
      bt_str.assign(RSTRING_PTR(bt_val), RSTRING_LEN(bt_val));
    }
    LOG(ERROR) << "[Binding] Ruby exception: " << msg_str;
    LOG(ERROR) << "[Binding] Backtrace:\n" << bt_str;
  }
}

void BindingEngineMri::PostMainLoopRunning() {
  // Show exception info
  VALUE exception = rb_errinfo();
  std::string exception_message;
  if (!NIL_P(exception) && !rb_obj_is_kind_of(exception, rb_eSystemExit))
    exception_message = ParseExeceptionInfo(exception);

  // Push error to console overlay and log file if available
  if (!exception_message.empty() && g_current_execution_context) {
    g_current_execution_context->console.Push(
        "[Ruby Error] Unhandled exception:");
    g_current_execution_context->console.Push("[Ruby Error] " +
                                              exception_message);
  }

  // Clean up ruby vm
  ruby_finalize();

  // Release binding reference
  g_current_execution_context = nullptr;
  GlobalModules empty_modules;
  std::swap(*MriGetGlobalModules(), empty_modules);

  // Show message box for errors
  if (!exception_message.empty())
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Scripts Error",
                             exception_message.c_str(), nullptr);

  LOG(INFO) << "[Binding] Quit CRuby binding engine.";
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

  int32_t scripts_count = RARRAY_LEN(packed_scripts);
  std::string zlib_decode_buffer(0x1000, 0);

  for (int32_t i = 0; i < scripts_count; ++i) {
    VALUE script = rb_ary_entry(packed_scripts, i);

    // 0 -> ScriptID for Editor
    VALUE script_name = rb_ary_entry(script, 1);
    VALUE script_src = rb_ary_entry(script, 2);

    unsigned long buffer_size;

    int32_t zlib_result = Z_OK;

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
      LOG(INFO) << "[Binding] Error when decoding: "
                << StringValueCStr(script_name);
      break;
    }

    rb_ary_store(script, 3, rb_str_new_cstr(zlib_decode_buffer.c_str()));
  }

  for (;;) {
    for (int32_t i = 0; i < scripts_count; ++i) {
      VALUE script = rb_ary_entry(packed_scripts, i);
      VALUE script_name = rb_ary_entry(script, 1);
      VALUE script_source = rb_ary_entry(script, 3);

      std::stringstream format_filename;
      format_filename << std::to_string(i + 1) << " - "
                      << StringValueCStr(script_name);

      int32_t state = 0;
      std::string filename = format_filename.str().c_str();
      EvalString(
          MriStringUTF8(RSTRING_PTR(script_source), RSTRING_LEN(script_source)),
          MriStringUTF8(filename.data(), filename.size()), &state);
      if (state)
        break;
    }

    VALUE exception = rb_errinfo();
    if (rb_obj_class(exception) !=
        MriGetException(content::ExceptionCode::CODE_NUMS))
      break;

    MriProcessReset();
    rb_gc_start();
  }
}

}  // namespace binding
