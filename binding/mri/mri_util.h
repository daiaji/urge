// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD - style license that can be
// found in the LICENSE file.

#ifndef BINDING_MRI_MRI_UTIL_H_
#define BINDING_MRI_MRI_UTIL_H_

#include <string>

#include "ruby.h"
#include "ruby/encoding.h"
#include "ruby/version.h"

#ifdef RUBY_API_VERSION_MAJOR
#define RAPI_MAJOR RUBY_API_VERSION_MAJOR
#define RAPI_MINOR RUBY_API_VERSION_MINOR
#define RAPI_TEENY RUBY_API_VERSION_TEENY
#else
#define RAPI_MAJOR RUBY_VERSION_MAJOR
#define RAPI_MINOR RUBY_VERSION_MINOR
#define RAPI_TEENY RUBY_VERSION_TEENY
#endif
#define RAPI_FULL ((RAPI_MAJOR * 100) + (RAPI_MINOR * 10) + RAPI_TEENY)

#if RAPI_FULL >= 270
#include "ruby/internal/arithmetic/int.h"
#include "ruby/internal/arithmetic/long_long.h"
#endif

#include "base/memory/ref_counted.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"

#include "content/public/engine_audio.h"
#include "content/public/engine_graphics.h"
#include "content/public/engine_input.h"
#include "content/public/engine_mouse.h"
#include "content/public/engine_urge.h"

namespace binding {

struct GlobalModules {
  scoped_refptr<content::Graphics> Graphics;
  scoped_refptr<content::Input> Input;
  scoped_refptr<content::Audio> Audio;
  scoped_refptr<content::Mouse> Mouse;
  scoped_refptr<content::URGE> URGE;
};

GlobalModules* MriGetGlobalModules();
content::ExecutionContext* MriGetCurrentContext();

#if RAPI_FULL >= 270
#define DEF_TYPE_RESERVED 0,
#else
#define DEF_TYPE_RESERVED
#endif

#if RAPI_FULL >= 210
#define DEF_TYPE_FLAGS 0
#else
#define DEF_TYPE_FLAGS
#endif

#define MRI_DEFINE_DATATYPE(Klass, Name, Free) \
  const rb_data_type_t k##Klass##DataType = {  \
      Name, {0, Free, 0, DEF_TYPE_RESERVED{}}, 0, 0, DEF_TYPE_FLAGS}

#define MRI_DECLARE_DATATYPE(Klass) \
  extern const rb_data_type_t k##Klass##DataType;

#define MRI_DEFINE_DATATYPE_PTR(Klass, Name, FreeTy) \
  MRI_DEFINE_DATATYPE(Klass, Name, MriFreeInstance<FreeTy>)

#define MRI_DEFINE_DATATYPE_REF(Klass, Name, FreeTy) \
  MRI_DEFINE_DATATYPE(Klass, Name, MriFreeInstanceRef<FreeTy>)

template <typename Ty>
void MriFreeInstance(void* ptr) {
  delete static_cast<Ty*>(ptr);
}

template <typename Ty>
void MriFreeInstanceRef(void* ptr) {
  static_cast<Ty*>(ptr)->Release();
}

template <const rb_data_type_t* DataType>
VALUE MriClassAllocate(VALUE klass) {
#if RAPI_FULL >= 230
  return rb_data_typed_object_wrap(klass, nullptr, DataType);
#else
  return rb_data_typed_object_alloc(klass, nullptr, DataType);
#endif
}

/// <summary>
/// Parse format args into pointer variable
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <param name="format">o s z f i b n |</param>
/// <param name=""></param>
/// <returns></returns>
int MriParseArgsTo(int argc, VALUE* argv, const char* fmt, ...);

void MriInitException(bool rgss3);
void MriProcessException(const content::ExceptionState& exception);
void MriCheckArgc(int actual, int expected);
VALUE MriGetException(content::ExceptionCode exception);

template <typename Ty>
void MriSetStructData(VALUE obj, Ty* ptr) {
  RTYPEDDATA_DATA(obj) = ptr;
}

template <typename Ty>
Ty* MriGetStructData(VALUE obj) {
  Ty* struct_data = static_cast<Ty*>(RTYPEDDATA_DATA(obj));
  if (!struct_data)
    rb_raise(rb_eRuntimeError,
             "Invalid instance data for variable: Missing call to super?");
  return struct_data;
}

template <typename Ty>
Ty* MriCheckStructData(VALUE obj, const rb_data_type_t& type) {
  if (obj == Qnil)
    return nullptr;
  return static_cast<Ty*>(Check_TypedStruct(obj, &type));
}

using RubyMethod = VALUE (*)(int argc, VALUE* argv, VALUE self);
inline void MriDefineMethod(VALUE klass, const char* name, RubyMethod func) {
  rb_define_method(klass, name, RUBY_METHOD_FUNC(func), -1);
}

inline void MriDefineClassMethod(VALUE klass,
                                 const char* name,
                                 RubyMethod func) {
  rb_define_singleton_method(klass, name, RUBY_METHOD_FUNC(func), -1);
}

inline void MriDefineModuleFunction(VALUE module,
                                    const char* name,
                                    RubyMethod func) {
  rb_define_module_function(module, name, RUBY_METHOD_FUNC(func), -1);
}

#define MRI_METHOD(name) static VALUE name(int argc, VALUE* argv, VALUE self)

inline VALUE MriStringUTF8(const char* string, long length) {
  return rb_enc_str_new(string, length, rb_utf8_encoding());
}

template <typename Ty>
VALUE MriWrapObject(scoped_refptr<Ty> ptr,
                    const rb_data_type_t& type,
                    VALUE super = rb_cObject) {
  if (!ptr)
    return Qnil;

  VALUE klass = rb_const_get(super, rb_intern(type.wrap_struct_name));
  VALUE obj = rb_obj_alloc(klass);

  ptr->AddRef();
  MriSetStructData<Ty>(obj, ptr.get());

  return obj;
}

inline void MriCollectStrings(VALUE obj, std::vector<std::string>& out) {
  if (RB_TYPE_P(obj, RUBY_T_STRING)) {
    out.push_back(RSTRING_PTR(obj));
    return;
  }

  if (RB_TYPE_P(obj, RUBY_T_ARRAY)) {
    for (long i = 0; i < RARRAY_LEN(obj); ++i) {
      VALUE str = rb_ary_entry(obj, i);
      if (!RB_TYPE_P(str, RUBY_T_STRING))
        continue;
      out.push_back(RSTRING_PTR(str));
    }
  }
}

inline VALUE MriGetEngineID(int argc, VALUE* argv, VALUE self) {
  auto engine_id = reinterpret_cast<uint64_t>(RTYPEDDATA_DATA(self));
  return ULL2NUM(engine_id);
}

template <int32_t id>
MRI_METHOD(MriReturnInt) {
  return rb_fix_new(id);
}

#define MRI_FROM_BOOL(v) (v != Qfalse)
#define MRI_FROM_STRING(v) std::string(RSTRING_PTR(v))

#define MRI_BOOL_VALUE(v) ((v) ? Qtrue : Qfalse)
#define MRI_STRING_VALUE(v) rb_utf8_str_new(v.c_str(), v.size())

///
/// Method Define Template
///

#define MRI_DECLARE_ATTRIBUTE(klass, rb_name, ktype, ctype) \
  MriDefineMethod(klass, rb_name, ktype##_Get_##ctype);     \
  MriDefineMethod(klass, rb_name "=", ktype##_Put_##ctype);

#define MRI_DECLARE_CLASS_ATTRIBUTE(klass, rb_name, ktype, ctype) \
  MriDefineClassMethod(klass, rb_name, ktype##_Get_##ctype);      \
  MriDefineClassMethod(klass, rb_name "=", ktype##_Put_##ctype);

#define MRI_DECLARE_MODULE_ATTRIBUTE(klass, rb_name, ktype, ctype) \
  MriDefineModuleFunction(klass, rb_name, ktype##_Get_##ctype);    \
  MriDefineModuleFunction(klass, rb_name "=", ktype##_Put_##ctype);

///
/// Serializable template
///

template <typename Ty>
MRI_METHOD(serializable_marshal_load) {
  std::string data;
  MriParseArgsTo(argc, argv, "s", &data);

  content::ExceptionState exception_state;
  scoped_refptr ptr =
      Ty::Deserialize(MriGetCurrentContext(), data, exception_state);
  MriProcessException(exception_state);

  ptr->AddRef();
  VALUE obj = rb_obj_alloc(self);
  MriSetStructData(obj, ptr.get());

  return obj;
}

template <typename Ty>
MRI_METHOD(serializable_marshal_dump) {
  scoped_refptr obj = MriGetStructData<Ty>(self);

  content::ExceptionState exception_state;
  std::string data =
      Ty::Serialize(MriGetCurrentContext(), obj, exception_state);
  MriProcessException(exception_state);

  return rb_str_new(data.data(), (long)data.size());
}

template <typename Ty>
void MriInitSerializableBinding(VALUE klass) {
  MriDefineClassMethod(klass, "_load", serializable_marshal_load<Ty>);
  MriDefineMethod(klass, "_dump", serializable_marshal_dump<Ty>);
}

///
/// Object compare template
///

#define MRI_OBJECT_ID_COMPARE(klass)                                     \
  MRI_METHOD(klass##_equal_to) {                                         \
    scoped_refptr obj = MriGetStructData<content::klass>(self);          \
    MriCheckArgc(argc, 1);                                               \
    scoped_refptr value_obj =                                            \
        MriCheckStructData<content::klass>(argv[0], k##klass##DataType); \
    return obj == value_obj ? Qtrue : Qfalse;                            \
  }

#define MRI_OBJECT_ID_COMPARE_CUSTOM(klass)                                 \
  MRI_METHOD(klass##_equal_to) {                                            \
    scoped_refptr obj = MriGetStructData<content::klass>(self);             \
    MriCheckArgc(argc, 1);                                                  \
    scoped_refptr value_obj =                                               \
        MriCheckStructData<content::klass>(argv[0], k##klass##DataType);    \
    content::ExceptionState exception_state;                                \
    VALUE result =                                                          \
        obj->CompareWithOther(value_obj, exception_state) ? Qtrue : Qfalse; \
    MriProcessException(exception_state);                                   \
    return result;                                                          \
  }

#define MRI_DECLARE_OBJECT_COMPARE(ty)          \
  MriDefineMethod(klass, "==", ty##_equal_to);  \
  MriDefineMethod(klass, "===", ty##_equal_to); \
  MriDefineMethod(klass, "eql?", ty##_equal_to);

}  // namespace binding

#endif  // !BINDING_MRI_MRI_UTIL_H_
