# Copyright 2018-2025 Admenri.
# Use of this source code is governed by a BSD - style license that can be
# found in the LICENSE file.

import re

class APIParser:
  def __init__(self):
    self.classes = []
    self.current_class = None
    self.expect_decl = False
    self.current_comment = {}
    self.buffer = ""
    self.is_processing_params = False
    self.is_processing_enum = False
    self.enum_buffer = []

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
          if isinstance(params[k], list):
            params[k].append(v)
          else:
            params[k] = [params[k], v]
        else:
          params[k] = v
      else:
        params[item] = True
    return params

  def process_line(self, line):
    line = line.strip()
    if self.is_processing_enum:
      self.enum_buffer.append(line)
      if '};' in line:
        self.process_enum_body(' '.join(self.enum_buffer))
        self.is_processing_enum = False
      return
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

    if self.expect_decl and line.startswith('enum'):
      self.enum_buffer = [line]
      self.is_processing_enum = '{' not in line or '}' not in line
      if not self.is_processing_enum:
        self.process_enum_body(line)
      self.expect_decl = False
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

      if 'URGE_EXPORT_COMPARABLE' in line:
        self.current_class['is_comparable'] = True
        self.expect_decl = False
        return

      if attr := self.parse_attribute(line):
        self.current_class['attributes'].append(attr)
        self.expect_decl = False
        return

      if method := self.parse_method(line):
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
      "parent": self.current_comment.get('parent', ''),
      "is_serializable": False,
      "is_comparable": False,
      "is_module": self.current_comment.get('is_module', False),
      "dependency": [],
      "enums": []
    }

  def process_enum_body(self, body):
    enum_decl = re.search(r'enum\s+(\w+)(?:\s*:\s*(\w+))?', body)
    if not enum_decl: return
        
    enum_data = {
      "name": enum_decl.group(1),
      "base_type": enum_decl.group(2),
      "constants": [],
      "comment": self.current_comment.copy()
    }
    
    constants = re.findall(r'(\w+)(?:\s*=.*?)?(?=,|})', body.split('{')[-1])
    enum_data["constants"] = [c.strip() for c in constants if c.strip()]
    self.current_class['enums'].append(enum_data)

  def _extract_scoped_ref_types(self, type_str):
    pattern = r'scoped_refptr\s*<\s*([\w:]+)\s*>'
    return re.findall(pattern, type_str)

  def _update_class_dependency(self, type_str):
    if not self.current_class: return
    types = self._extract_scoped_ref_types(type_str)
    for dep in types:
      self.current_class['dependency'].append(dep)

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
      p_type, p_name = (parts[0], parts[1]) if len(parts) > 1 else (parts[0], '')
      params.append({
        "type": p_type,
        "name": p_name,
        "optional": p_name.strip() in optional_params,
        "default_value": optional_params.get(p_name.strip(), None)
      })

    filtered_params = [p for p in params if not any(
      excl in p['type'] for excl in ['ExceptionState', 'ExecutionContext']
    )]

    return_type = match.group('return_type').strip()
    self._update_class_dependency(return_type)

    for param in params:
      self._update_class_dependency(param['type'])

    return {
      "name": self.current_comment.get('name', match.group('name')),
      "func": match.group('name'),
      "return_type": return_type,
      "parameters": [filtered_params],
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
        attr_type = match.group(2).strip()
        self._update_class_dependency(attr_type)

        return {
          "func": match.group(1),
          "name": self.current_comment.get('name', match.group(1)),
          "type": attr_type,
          "is_static": is_static
        }
    return None

  def parse(self, code):
    for line in code.split('\n'): self.process_line(line)

    for cls in self.classes:
      cls['dependency'] = sorted(list(cls['dependency']))

    if self.current_class:
      self.classes.append(self.current_class)

if __name__ == "__main__":
  cpp_code = """
    /*--urge(name:Graphics,is_module)--*/
      class URGE_RUNTIME_API Graphics : public base::RefCounted<Graphics> {
       public:
        virtual ~Graphics() = default;

        /*--urge(name:Test)--*/
        enum TestConstants {
          TEST_VALUE1 = 0,
          TEST_VALUE2,
          TEST_VALUE3,
          TEST_VALUE4,
          TEST_VALUE5,
        };

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
  print(parser.classes)
