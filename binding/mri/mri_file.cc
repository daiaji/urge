// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD - style license that can be
// found in the LICENSE file.

#include "binding/mri/mri_file.h"

#include "SDL3/SDL_iostream.h"
#include "components/filesystem/io_service.h"

namespace binding {

filesystem::IOService* g_io_service = nullptr;

namespace {

VALUE StringForceUTF8(RB_BLOCK_CALL_FUNC_ARGLIST(yieldarg, procarg)) {
  if (RB_TYPE_P(yieldarg, RUBY_T_STRING) && ENCODING_IS_ASCII8BIT(yieldarg))
    rb_enc_associate_index(yieldarg, rb_utf8_encindex());
  return NIL_P(procarg) ? yieldarg
                        : rb_funcall2(procarg, rb_intern("call"), 1, &yieldarg);
}

}  // namespace

VALUE MriLoadData(const std::string& filename,
                  content::ExceptionState& exception_state) {
  filesystem::IOState io_state;
  SDL_IOStream* ops = g_io_service->OpenReadRaw(filename, &io_state);
  if (io_state.error_count) {
    exception_state.ThrowError(content::ExceptionCode::IO_ERROR, "%s",
                               io_state.error_message.c_str());
    return Qnil;
  }

  Sint64 size = SDL_SeekIO(ops, 0, SDL_IO_SEEK_END);
  if (size < 0) {
    SDL_CloseIO(ops);
    exception_state.ThrowError(content::ExceptionCode::IO_ERROR,
                               "Failed to seek file: %s", filename.c_str());
    return Qnil;
  }
  SDL_SeekIO(ops, 0, SDL_IO_SEEK_SET);
  VALUE data = rb_str_new(NULL, size);
  size_t bytes_read = SDL_ReadIO(ops, RSTRING_PTR(data), size);
  SDL_CloseIO(ops);

  if (bytes_read != (size_t)size) {
    LOG(WARNING) << "[Binding] Short read: " << bytes_read << " vs " << size;
    rb_str_resize(data, bytes_read);
  }

  rb_enc_associate_index(data, rb_ascii8bit_encindex());

  VALUE marshal_klass = rb_const_get(rb_cObject, rb_intern("Marshal"));
  VALUE marshal_args[] = {data};
  return rb_funcallv(marshal_klass, rb_intern("load"), 1, marshal_args);
}

MRI_METHOD(kernel_load_data) {
  std::string filename;
  MriParseArgsTo(argc, argv, "s", &filename);

  content::ExceptionState exception_state;
  VALUE data = MriLoadData(filename, exception_state);
  MriProcessException(exception_state);

  return data;
}

MRI_METHOD(kernel_save_data) {
  MriCheckArgc(argc, 2);

  VALUE obj = argv[0];
  VALUE filename = argv[1];

  VALUE file = rb_file_open_str(filename, "wb");
  VALUE marshal_klass = rb_const_get(rb_cObject, rb_intern("Marshal"));
  VALUE v[] = {obj, file};
  rb_funcall2(marshal_klass, rb_intern("dump"), 2, v);
  rb_io_close(file);

  return Qnil;
}

MRI_METHOD(marshal_load_utf8) {
  VALUE port, proc = Qnil;
  MriParseArgsTo(argc, argv, "o|o", &port, &proc);

  VALUE marshal_klass = rb_const_get(rb_cObject, rb_intern("Marshal"));
  VALUE block = rb_proc_new((VALUE (*)(ANYARGS))StringForceUTF8, proc);
  VALUE v[] = {port, block};
  return rb_funcall2(marshal_klass, rb_intern("load"), 2, v);
}

void InitCoreFileBinding() {
  MriDefineModuleFunction(rb_mKernel, "load_data", kernel_load_data);
  MriDefineModuleFunction(rb_mKernel, "save_data", kernel_save_data);

  VALUE marshal_klass = rb_const_get(rb_cObject, rb_intern("Marshal"));
  MriDefineModuleFunction(marshal_klass, "load_utf8", marshal_load_utf8);
}

}  // namespace binding
