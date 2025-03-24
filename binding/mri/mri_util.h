// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD - style license that can be
// found in the LICENSE file.

#ifndef BINDING_MRI_MRI_UTIL_H_
#define BINDING_MRI_MRI_UTIL_H_

#include <string>

#include "ruby.h"
#include "ruby/encoding.h"
#include "ruby/version.h"

#include "base/memory/ref_counted.h"
#include "content/context/exception_state.h"
#include "content/context/execution_context.h"

#include "content/public/engine_audio.h"
#include "content/public/engine_graphics.h"
#include "content/public/engine_input.h"

namespace binding {

struct GlobalModules {
  scoped_refptr<content::Graphics> Graphics;
  scoped_refptr<content::Input> Input;
  scoped_refptr<content::Audio> Audio;
};

GlobalModules* MriGetGlobalModules();
content::ExecutionContext* MriGetCurrentContext();

#define MRI_DEFINE_DATATYPE(Klass, Name, Free) \
  const rb_data_type_t k##Klass##DataType = {Name, {0, Free, 0, 0, 0}, 0, 0, 0}

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
  return rb_data_typed_object_wrap(klass, nullptr, DataType);
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
  if (RB_NIL_P(obj))
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
  auto engine_id =
      std::to_string(reinterpret_cast<uint64_t>(RTYPEDDATA_DATA(self)));
  return rb_str_new(engine_id.data(), engine_id.size());
}

template <int32_t id>
MRI_METHOD(MriReturnInt) {
  return rb_fix_new(id);
}

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
/// Method template
///

#define MRI_DEFINE_ATTRIBUTE_OBJ(klass, attr, type)                  \
  MRI_METHOD(klass##_Put_##attr) {                                   \
    scoped_refptr obj = MriGetStructData<content::klass>(self);      \
    VALUE value;                                                     \
    MriParseArgsTo(argc, argv, "o", &value);                         \
    scoped_refptr value_obj =                                        \
        MriCheckStructData<content::type>(value, k##type##DataType); \
    content::ExceptionState exception_state;                         \
    obj->Put_##attr(value_obj, exception_state);                     \
    MriProcessException(exception_state);                            \
    return self;                                                     \
  }                                                                  \
  MRI_METHOD(klass##_Get_##attr) {                                   \
    scoped_refptr obj = MriGetStructData<content::klass>(self);      \
    content::ExceptionState exception_state;                         \
    scoped_refptr value_obj = obj->Get_##attr(exception_state);      \
    MriProcessException(exception_state);                            \
    return MriWrapObject(value_obj, k##type##DataType);              \
  }

#define MRI_DEFINE_ATTRIBUTE_INTEGER(klass, attr)               \
  MRI_METHOD(klass##_Put_##attr) {                              \
    scoped_refptr obj = MriGetStructData<content::klass>(self); \
    int32_t value;                                              \
    MriParseArgsTo(argc, argv, "i", &value);                    \
    content::ExceptionState exception_state;                    \
    obj->Put_##attr(value, exception_state);                    \
    MriProcessException(exception_state);                       \
    return self;                                                \
  }                                                             \
  MRI_METHOD(klass##_Get_##attr) {                              \
    scoped_refptr obj = MriGetStructData<content::klass>(self); \
    content::ExceptionState exception_state;                    \
    int32_t value = obj->Get_##attr(exception_state);           \
    MriProcessException(exception_state);                       \
    return rb_fix_new(value);                                   \
  }

#define MRI_DEFINE_ATTRIBUTE_FLOAT(klass, attr)                  \
  MRI_METHOD(klass##_Put_##attr) {                               \
    scoped_refptr obj = MriGetStructData<content::klass>(self);  \
    double value;                                                \
    MriParseArgsTo(argc, argv, "f", &value);                     \
    content::ExceptionState exception_state;                     \
    obj->Put_##attr(static_cast<float>(value), exception_state); \
    MriProcessException(exception_state);                        \
    return self;                                                 \
  }                                                              \
  MRI_METHOD(klass##_Get_##attr) {                               \
    scoped_refptr obj = MriGetStructData<content::klass>(self);  \
    content::ExceptionState exception_state;                     \
    float value = obj->Get_##attr(exception_state);              \
    MriProcessException(exception_state);                        \
    return rb_float_new(value);                                  \
  }

#define MRI_DEFINE_ATTRIBUTE_BOOLEAN(klass, attr)               \
  MRI_METHOD(klass##_Put_##attr) {                              \
    scoped_refptr obj = MriGetStructData<content::klass>(self); \
    bool value;                                                 \
    MriParseArgsTo(argc, argv, "b", &value);                    \
    content::ExceptionState exception_state;                    \
    obj->Put_##attr(value, exception_state);                    \
    MriProcessException(exception_state);                       \
    return self;                                                \
  }                                                             \
  MRI_METHOD(klass##_Get_##attr) {                              \
    scoped_refptr obj = MriGetStructData<content::klass>(self); \
    content::ExceptionState exception_state;                    \
    bool value = obj->Get_##attr(exception_state);              \
    MriProcessException(exception_state);                       \
    return value ? Qtrue : Qfalse;                              \
  }

#define MRI_DEFINE_ATTRIBUTE_STRING(klass, attr)                \
  MRI_METHOD(klass##_Put_##attr) {                              \
    scoped_refptr obj = MriGetStructData<content::klass>(self); \
    std::string value;                                          \
    MriParseArgsTo(argc, argv, "s", &value);                    \
    content::ExceptionState exception_state;                    \
    obj->Put_##attr(value, exception_state);                    \
    MriProcessException(exception_state);                       \
    return self;                                                \
  }                                                             \
  MRI_METHOD(klass##_Get_##attr) {                              \
    scoped_refptr obj = MriGetStructData<content::klass>(self); \
    content::ExceptionState exception_state;                    \
    auto value = obj->Get_##attr(exception_state);              \
    MriProcessException(exception_state);                       \
    return rb_str_new(value.c_str(), value.size());             \
  }

#define MRI_DEFINE_ATTRIBUTE_STRINGLIST(klass, attr)            \
  MRI_METHOD(klass##_Put_##attr) {                              \
    scoped_refptr obj = MriGetStructData<content::klass>(self); \
    VALUE value;                                                \
    MriParseArgsTo(argc, argv, "o", &value);                    \
    content::ExceptionState exception_state;                    \
    std::vector<std::string> collection;                        \
    MriCollectStrings(value, collection);                       \
    obj->Put_##attr(collection, exception_state);               \
    MriProcessException(exception_state);                       \
    return self;                                                \
  }                                                             \
  MRI_METHOD(klass##_Get_##attr) {                              \
    scoped_refptr obj = MriGetStructData<content::klass>(self); \
    content::ExceptionState exception_state;                    \
    auto value = obj->Get_##attr(exception_state);              \
    MriProcessException(exception_state);                       \
    VALUE ary = rb_ary_new();                                   \
    for (auto it : value)                                       \
      rb_ary_push(ary, rb_str_new(it.c_str(), it.size()));      \
    return ary;                                                 \
  }

///
/// Static template
///

#define MRI_DEFINE_STATIC_ATTRIBUTE_OBJ(klass, attr, type)                   \
  MRI_METHOD(klass##_Put_##attr) {                                           \
    VALUE value;                                                             \
    MriParseArgsTo(argc, argv, "o", &value);                                 \
    scoped_refptr value_obj =                                                \
        MriCheckStructData<content::type>(value, k##type##DataType);         \
    content::ExceptionState exception_state;                                 \
    content::klass::Put_##attr(MriGetCurrentContext(), value_obj,            \
                               exception_state);                             \
    MriProcessException(exception_state);                                    \
    return self;                                                             \
  }                                                                          \
  MRI_METHOD(klass##_Get_##attr) {                                           \
    content::ExceptionState exception_state;                                 \
    scoped_refptr value_obj =                                                \
        content::klass::Get_##attr(MriGetCurrentContext(), exception_state); \
    MriProcessException(exception_state);                                    \
    return MriWrapObject(value_obj, k##type##DataType);                      \
  }

#define MRI_DEFINE_STATIC_ATTRIBUTE_INTEGER(klass, attr)                     \
  MRI_METHOD(klass##_Put_##attr) {                                           \
    int32_t value;                                                           \
    MriParseArgsTo(argc, argv, "i", &value);                                 \
    content::ExceptionState exception_state;                                 \
    content::klass::Put_##attr(MriGetCurrentContext(), value,                \
                               exception_state);                             \
    MriProcessException(exception_state);                                    \
    return self;                                                             \
  }                                                                          \
  MRI_METHOD(klass##_Get_##attr) {                                           \
    content::ExceptionState exception_state;                                 \
    int32_t value =                                                          \
        content::klass::Get_##attr(MriGetCurrentContext(), exception_state); \
    MriProcessException(exception_state);                                    \
    return rb_fix_new(value);                                                \
  }

#define MRI_DEFINE_STATIC_ATTRIBUTE_FLOAT(klass, attr)                       \
  MRI_METHOD(klass##_Put_##attr) {                                           \
    double value;                                                            \
    MriParseArgsTo(argc, argv, "f", &value);                                 \
    content::ExceptionState exception_state;                                 \
    content::klass::Put_##attr(MriGetCurrentContext(),                       \
                               static_cast<float>(value), exception_state);  \
    MriProcessException(exception_state);                                    \
    return self;                                                             \
  }                                                                          \
  MRI_METHOD(klass##_Get_##attr) {                                           \
    content::ExceptionState exception_state;                                 \
    float value =                                                            \
        content::klass::Get_##attr(MriGetCurrentContext(), exception_state); \
    MriProcessException(exception_state);                                    \
    return rb_float_new(value);                                              \
  }

#define MRI_DEFINE_STATIC_ATTRIBUTE_BOOLEAN(klass, attr)                     \
  MRI_METHOD(klass##_Put_##attr) {                                           \
    bool value;                                                              \
    MriParseArgsTo(argc, argv, "b", &value);                                 \
    content::ExceptionState exception_state;                                 \
    content::klass::Put_##attr(MriGetCurrentContext(), value,                \
                               exception_state);                             \
    MriProcessException(exception_state);                                    \
    return self;                                                             \
  }                                                                          \
  MRI_METHOD(klass##_Get_##attr) {                                           \
    content::ExceptionState exception_state;                                 \
    bool value =                                                             \
        content::klass::Get_##attr(MriGetCurrentContext(), exception_state); \
    MriProcessException(exception_state);                                    \
    return value ? Qtrue : Qfalse;                                           \
  }

#define MRI_DEFINE_STATIC_ATTRIBUTE_STRING(klass, attr)                      \
  MRI_METHOD(klass##_Put_##attr) {                                           \
    std::string value;                                                       \
    MriParseArgsTo(argc, argv, "s", &value);                                 \
    content::ExceptionState exception_state;                                 \
    content::klass::Put_##attr(MriGetCurrentContext(), value,                \
                               exception_state);                             \
    MriProcessException(exception_state);                                    \
    return self;                                                             \
  }                                                                          \
  MRI_METHOD(klass##_Get_##attr) {                                           \
    content::ExceptionState exception_state;                                 \
    auto value =                                                             \
        content::klass::Get_##attr(MriGetCurrentContext(), exception_state); \
    MriProcessException(exception_state);                                    \
    return rb_str_new(value.c_str(), value.size());                          \
  }

#define MRI_DEFINE_STATIC_ATTRIBUTE_STRINGLIST(klass, attr)                  \
  MRI_METHOD(klass##_Put_##attr) {                                           \
    VALUE value;                                                             \
    MriParseArgsTo(argc, argv, "o", &value);                                 \
    content::ExceptionState exception_state;                                 \
    std::vector<std::string> collection;                                     \
    MriCollectStrings(value, collection);                                    \
    content::klass::Put_##attr(MriGetCurrentContext(), collection,           \
                               exception_state);                             \
    MriProcessException(exception_state);                                    \
    return self;                                                             \
  }                                                                          \
  MRI_METHOD(klass##_Get_##attr) {                                           \
    content::ExceptionState exception_state;                                 \
    auto value =                                                             \
        content::klass::Get_##attr(MriGetCurrentContext(), exception_state); \
    MriProcessException(exception_state);                                    \
    VALUE ary = rb_ary_new();                                                \
    for (auto it : value)                                                    \
      rb_ary_push(ary, rb_str_new(it.c_str(), it.size()));                   \
    return ary;                                                              \
  }

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

}  // namespace binding

#endif  // !BINDING_MRI_MRI_UTIL_H_
