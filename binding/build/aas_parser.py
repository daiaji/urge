import re
import json

class APIParser:
  def __init__(self):
    self.classes = []
    self.current_class = None
    self.expect_decl = False
    self.current_comment = {}
    self.buffer = ""
    self.is_processing_params = False

  def parse_urge_comment(self, line):
    match = re.search(r'/\*--urge\((.*)\)--\*/', line)
    return self.parse_params_dict(match.group(1)) if match else {}

  @staticmethod
  def parse_params_dict(param_str):
    params = {}
    if not param_str: return params
    for item in param_str.split(','):
      item = item.strip()
      if not item: continue
      if ':' in item:
        k, v = item.split(':', 1)
        k, v = k.strip(), v.strip()
        if k in params:
          if isinstance(params[k], list): params[k].append(v)
          else: params[k] = [params[k], v]
        else: params[k] = v
      else: params[item] = True
    return params

  def process_line(self, line):
    line = line.strip()
    if self.is_processing_params:
      self.buffer += " " + line
      if ')' in line:
        self.is_processing_params = False
        self.process_declaration(self.buffer)
        self.buffer = ""
      return
    if '(' in line and ')' not in line:
      self.is_processing_params = True
      self.buffer = line
      return
    self.process_declaration(line)

  def process_declaration(self, line):
    if line.startswith('/*--urge'):
      self.current_comment = self.parse_urge_comment(line)
      self.expect_decl = True
      return

    if self.expect_decl and line.startswith('class'):
      if class_info := self.parse_class_decl(line):
        if self.current_class: self.classes.append(self.current_class)
        self.current_class = class_info
        self.expect_decl = False
      return

    if self.expect_decl and self.current_class:
      if 'URGE_EXPORT_SERIALIZABLE' in line:
        self.current_class['is_serializable'] = True
        self.expect_decl = False
        return

      if attr := self.parse_attribute(line):
        self.current_class['attributes'].append(attr)
        self.expect_decl = False
        return

      if method := self.parse_method(line):
        # 合并重载逻辑（统一处理所有方法）
        existing = next(
          (m for m in self.current_class['methods']
          if m['name'] == method['name']
          and m['is_static'] == method['is_static']
          and m['is_virtual'] == method['is_virtual']),
          None
        )
        if existing:
          existing['parameters'].extend(method['parameters'])
        else:
          self.current_class['methods'].append(method)
        self.expect_decl = False

  def parse_class_decl(self, line):
    match = re.match(r'class\s+URGE_RUNTIME_API\s+(\w+)', line)
    if not match: return None
    return {
      "class_name": self.current_comment.get('name', match.group(1)),
      "methods": [],
      "attributes": [],
      "is_serializable": False,
      "is_module": self.current_comment.get('is_module', False)
    }

  def parse_method(self, decl):
    decl = re.sub(r'\s+', ' ', decl).replace(' ;', ';')
    method_re = re.compile(
      r'^(?P<static>static\s+)?'
      r'(?P<virtual>virtual\s+)?'
      r'(?P<return_type>.+?)\s+'
      r'(?P<name>\w+)'
      r'\s*\((?P<params>.*?)\)'
      r'\s*(=\s*0)?\s*;?'
    )
    if not (match := method_re.search(decl)): return None

    optional_params = {}
    if 'optional' in self.current_comment:
      opts = self.current_comment['optional']
      opts = [opts] if not isinstance(opts, list) else opts
      for opt in opts:
        if '=' in opt:
          k, v = opt.split('=', 1)
          optional_params[k.strip()] = v.strip()

    params = []
    for param in re.split(r',(?![^<>]*>)', match.group('params')):
      param = re.sub(r'(\w+)(\*+|\&+)', r'\1 \2', param).strip()
      if not param: continue
      parts = re.split(r'\s+(?=\w+$)', param.strip())
      p_type, p_name = (parts[0], parts[1]) if len(parts)>1 else (parts[0], '')
      params.append({
        "type": p_type,
        "name": p_name,
        "optional": p_name.strip() in optional_params,
        "default_value": optional_params.get(p_name.strip(), None)
      })

    # 过滤排除参数
    filtered_params = [p for p in params if not any(
      excl in p['type'] for excl in ['ExceptionState', 'ExecutionContext']
    )]

    return {
      "name": self.current_comment.get('name', match.group('name')),
      "func": match.group('name'),
      "return_type": match.group('return_type').strip(),
      "parameters": [filtered_params],  # 所有方法参数统一用列表存储
      "is_static": bool(match.group('static')),
      "is_virtual": bool(match.group('virtual'))
    }

  def parse_attribute(self, decl):
    decl = decl.replace(' ', '')
    for pattern, is_static in [
      (r'URGE_EXPORT_ATTRIBUTE\((\w+),\s*(.*?)\);?', False),
      (r'URGE_EXPORT_STATIC_ATTRIBUTE\((\w+),\s*(.*?)\);?', True)
    ]:
      if match := re.match(pattern, decl):
        return {
          "func": match.group(1),
          "name": self.current_comment.get('name', match.group(1)),
          "type": match.group(2).strip(),
          "is_static": is_static
        }
    return None

  def parse(self, code):
    for line in code.split('\n'): self.process_line(line)
    if self.current_class: self.classes.append(self.current_class)

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

        /*--urge(name:test)--*/
        virtual scoped_refptr<Bitmap> Test(ExceptionState& exception_state) = 0;

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

        /*--urge(name:test)--*/
        virtual scoped_refptr<Bitmap> Test(ExceptionState& exception_state) = 0;

        /*--urge(name:set,optional:value=0,optional:opacity=255)--*/
        virtual void Set(int32_t value, ExceptionState& exception_state) = 0;

        /*--urge(name:set,optional:value=0,optional:opacity=255)--*/
        virtual void Set(int32_t value, int32_t opacity, ExceptionState& exception_state) = 0;

        /*--urge(name:font)--*/
        URGE_EXPORT_ATTRIBUTE(Font, scoped_refptr<Font>);

        /*--urge(name:static_font)--*/
        URGE_EXPORT_STATIC_ATTRIBUTE(StaticFont, scoped_refptr<Font>);
      };
    """

  parser = APIParser()
  parser.parse(cpp_code)
  print(json.dumps(parser.classes, indent=2, ensure_ascii=False))
