import binding.build.aas_parser as aas
import json


class MriBindGen:
  def __init__(self):
    self.class_data = {}

  def setup(self, klass_data: dict):
    self.class_data = klass_data

  def gen_source_declaration(self):
    template = self.class_data
    kname = template["class_name"]
    is_module = template["is_module"]

    func_body = "void Init{}Binding() {{\n".format(kname)

    if is_module:
      func_body += "VALUE klass = rb_define_module(\"{}\");\n".format(kname)
    else:
      func_body += "VALUE klass = rb_define_class(\"{}\", rb_cObject);\n".format(kname)
      func_body += "rb_define_alloc_func(klass, MriClassAllocate<&k{}DataType>);\n".format(kname)

    func_body += "\n"

    attrs = template["attributes"]
    for attr in attrs:
      aname = attr["name"]
      fname = attr["func"]
      is_static = attr["is_static"]

      preprocessor = "MRI_DECLARE_ATTRIBUTE"
      if is_module:
        preprocessor = "MRI_DECLARE_MODULE_ATTRIBUTE"
      else:
        if is_static:
          preprocessor = "MRI_DECLARE_CLASS_ATTRIBUTE"

      func_body += "{}(klass, \"{}\", {}, {});\n".format(preprocessor, aname, kname, fname)

    func_body += "\n"

    methods = template["methods"]
    defined_methods = []
    for method in methods:
      mname = method["name"]
      fname = method["func"]
      is_static = method["is_static"]

      if defined_methods.count(mname) > 0:
        continue
      defined_methods.append(mname)

      preprocessor = "MriDefineMethod"
      if is_module:
        preprocessor = "MriDefineModuleFunction"
      else:
        if is_static:
          preprocessor = "MriDefineClassMethod"

      func_body += "{}(klass, \"{}\", {}_{});\n".format(preprocessor, mname, kname, fname)

    func_body += "}\n"
    return func_body

  def gen_source_defination(self):
    template = self.class_data
    kname = template["class_name"]
    is_module = template["is_module"]

    func_body = ""
    if not is_module:
      func_body += "MRI_DEFINE_DATATYPE_REF({}, \"{}\", content::{});\n".format(kname, kname, kname)

    func_body += "\n"

    attrs = template["attributes"]
    for attr in attrs:
      fname = attr["func"]
      attr_type = attr["type"]
      is_static = attr["is_static"]

      preprocessor_prefix = ""
      if is_static:
        preprocessor_prefix = "STATIC_"

      preprocessor_suffix = ""
      if attr_type.startswith("scoped_refptr"):
        preprocessor_suffix = "OBJ"
      elif attr_type.startswith("float"):
        preprocessor_suffix = "FLOAT"
      elif attr_type.startswith("bool"):
        preprocessor_suffix = "BOOLEAN"
      else:
        preprocessor_suffix = "INTEGER"

      func_body += "MRI_DEFINE_{}ATTRIBUTE_{}({}, {});\n".format(preprocessor_prefix, preprocessor_suffix, kname, fname)

    func_body += "\n"

    methods = template["methods"]
    defined_methods = []
    for method in methods:
      defined_methods.append(method["func"])

    generated_methods = []
    for method in methods:
      func_name = method["func"]
      if generated_methods.count(func_name) > 0:
        continue
      generated_methods.append(func_name)

      func_body += "MRI_METHOD({}_{}) {{\n".format(kname, func_name)

      func_body += "}\n"

    return func_body


if __name__ == "__main__":
  cpp_code = """
      /*--urge(name:Graphics,is_module)--*/
      class URGE_RUNTIME_API Graphics : public base::RefCounted<Graphics> {
       public:
        virtual ~Graphics() = default;

        /*--urge(name:initialize)--*/
        static scoped_refptr<Graphics> New(ExecutionContext* execution_context,
                                         const std::string& filename,
                                         ExceptionState& exception_state);

        /*--urge(name:initialize)--*/
        static scoped_refptr<Graphics> New(ExecutionContext* execution_context,
                                         int32_t width,
                                         int32_t height,
                                         ExceptionState& exception_state);

        /*--urge(name:dispose)--*/
        virtual void Dispose(ExceptionState& exception_state) = 0;

        /*--urge(name:set,optional:value=0,optional:opacity=255)--*/
        virtual void Set(int32_t value, ExceptionState& exception_state) = 0;

        /*--urge(name:set,optional:value=0,optional:opacity=255)--*/
        virtual void Set(int32_t value, int32_t opacity, ExceptionState& exception_state) = 0;

        /*--urge(name:font)--*/
        URGE_EXPORT_ATTRIBUTE(Font, scoped_refptr<Font>);

        /*--urge(name:static_font)--*/
        URGE_EXPORT_STATIC_ATTRIBUTE(StaticFont, scoped_refptr<Font>);
      };

      /*--urge(name:Bitmap)--*/
      class URGE_RUNTIME_API Bitmap : public base::RefCounted<Bitmap> {
       public:
        virtual ~Bitmap() = default;

        /*--urge(name:initialize)--*/
        static scoped_refptr<Bitmap> New(ExecutionContext* execution_context,
                                       const std::string& filename,
                                       ExceptionState& exception_state);

        /*--urge(name:dispose)--*/
        virtual void Dispose(ExceptionState& exception_state) = 0;

        /*--urge(name:font)--*/
        URGE_EXPORT_ATTRIBUTE(Font, scoped_refptr<Font>);

        /*--urge(name:static_font)--*/
        URGE_EXPORT_STATIC_ATTRIBUTE(StaticFont, scoped_refptr<Font>);
      };
      """

  parser = aas.APIParser()
  parser.parse(cpp_code)

  for item in parser.classes:
    gen = MriBindGen()
    gen.setup(item)
    print(gen.gen_source_declaration())
    print(gen.gen_source_defination())
