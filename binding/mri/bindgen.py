import binding.build.aas_parser as aas
import json


class MriBindGen:
  def __init__(self):


  def setup(self, json):



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

      /*--urge(name:dispose)--*/
      virtual void Dispose(ExceptionState& exception_state) = 0;

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
  print(json.dumps(parser.classes, indent=2, ensure_ascii=False))
