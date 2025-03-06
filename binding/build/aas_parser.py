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
    if not match: return {}
    return self.parse_params_dict(match.group(1))

  @staticmethod
  def parse_params_dict(param_str):
    params = {}
    if not param_str: return params
    for item in param_str.split(','):
      item = item.strip()
      if not item: continue
      if ':' in item:
        k, v = item.split(':', 1)
        k = k.strip()
        v = v.strip()
        # 处理多值键（例如多个 optional）
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

    # 处理多行参数列表
    if self.is_processing_params:
      self.buffer += " " + line
      if ')' in line:  # 参数列表结束
        self.is_processing_params = False
        full_line = self.buffer
        self.buffer = ""
        self.process_declaration(full_line)
      return

    # 检测是否开始多行参数列表
    if '(' in line and ')' not in line:
      self.is_processing_params = True
      self.buffer = line
      return

    # 处理单行声明
    self.process_declaration(line)

  def process_declaration(self, line):
    """处理单行或多行合并后的声明"""
    # 解析urge注释
    if line.startswith('/*--urge'):
      self.current_comment = self.parse_urge_comment(line)
      self.expect_decl = True
      return

    # 类声明解析
    if self.expect_decl and line.startswith('class'):
      if class_info := self.parse_class_decl(line):
        # 如果当前类已经存在，则保存并重置
        if self.current_class:
          self.classes.append(self.current_class)
        self.current_class = class_info
        self.expect_decl = False
      return

    # 其他声明处理
    if self.expect_decl and self.current_class:
      # 序列化标记处理
      if 'URGE_EXPORT_SERIALIZABLE' in line:
        self.current_class['is_serializable'] = True
        self.expect_decl = False
        return

      # 属性声明解析
      if attr := self.parse_attribute(line):
        self.current_class['attributes'].append(attr)
        self.expect_decl = False
        return

      # 方法声明解析
      if method := self.parse_method(line):
        self.current_class['methods'].append(method)
        self.expect_decl = False

  def parse_class_decl(self, line):
    """解析类声明"""
    match = re.match(r'class\s+URGE_RUNTIME_API\s+(\w+)', line)
    if not match: return None

    # 检查是否包含 is_module 参数
    is_module = self.current_comment.get('is_module', False)

    return {
      "class_name": self.current_comment.get('name', match.group(1)),
      "methods": [],
      "attributes": [],
      "is_serializable": False,
      "is_module": is_module  # 新增 is_module 属性
    }

  def parse_method(self, decl):
    """解析方法声明"""
    # 统一处理声明格式
    decl = re.sub(r'\s+', ' ', decl).replace(' ;', ';')

    # 方法签名正则（增强版）
    method_re = re.compile(
      r'^(?P<static>static\s+)?'
      r'(?P<virtual>virtual\s+)?'
      r'(?P<return_type>.+?)\s+'
      r'(?P<name>\w+)'
      r'\s*\((?P<params>.*?)\)'
      r'\s*(=\s*0)?\s*;?'
    )
    if not (match := method_re.search(decl)):
      return None

    # 参数过滤规则
    def should_keep_param(param):
      return not any(
        excluded in param['type']
        for excluded in ['ExceptionState', 'ExecutionContext']
      )

    # 解析 optional 参数
    optional_params = {}
    if 'optional' in self.current_comment:
      optional_list = self.current_comment['optional']
      # 统一处理为列表
      if not isinstance(optional_list, list):
        optional_list = [optional_list]
      for optional_param in optional_list:
        if '=' in optional_param:
          optional_name, default_value = optional_param.split('=', 1)
          optional_params[optional_name.strip()] = default_value.strip()

    # 解析参数列表（支持模板类型）
    params = []
    raw_params = re.split(r',(?![^<>]*>)', match.group('params'))
    for param in raw_params:
      param = param.strip()
      if not param: continue

      # 处理指针和引用
      param = re.sub(r'(\w+)(\*+|\&+)', r'\1 \2', param).strip()

      # 分离类型和名称
      parts = re.split(r'\s+(?=\w+$)', param)
      if len(parts) > 1:
        p_type, p_name = parts[0], parts[1]
      else:
        p_type, p_name = parts[0], ''

      # 检查是否是 optional 参数
      is_optional = p_name.strip() in optional_params
      default_value = optional_params.get(p_name.strip(), None)

      param_info = {
        "type": p_type,
        "name": p_name,
        "optional": is_optional,
        "default_value": default_value
      }
      if should_keep_param(param_info):
        params.append(param_info)

    return {
      "name": self.current_comment.get('name', match.group('name')),
      "return_type": match.group('return_type').strip(),
      "parameters": params,
      "is_static": bool(match.group('static')),
      "is_virtual": bool(match.group('virtual')),
    }

  def parse_attribute(self, decl):
    """解析属性声明（增强版）"""
    # 支持多行属性声明
    decl = decl.replace(' ', '')

    # 匹配普通属性
    attr_match = re.match(
      r'URGE_EXPORT_ATTRIBUTE\((\w+),\s*(.*?)\);?',
      decl
    )
    if attr_match:
      return {
        "name": self.current_comment.get('name', attr_match.group(1)),
        "type": attr_match.group(2).strip(),
        "is_static": False  # 普通属性默认为非静态
      }

    # 匹配静态属性
    static_attr_match = re.match(
      r'URGE_EXPORT_STATIC_ATTRIBUTE\((\w+),\s*(.*?)\);?',
      decl
    )
    if static_attr_match:
      return {
        "name": self.current_comment.get('name', static_attr_match.group(1)),
        "type": static_attr_match.group(2).strip(),
        "is_static": True  # 静态属性标记为 true
      }

    return None

  def parse(self, code):
    """主解析方法"""
    for line in code.split('\n'):
      self.process_line(line)

    # 确保最后一个类被保存
    if self.current_class:
      self.classes.append(self.current_class)

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

  parser = APIParser()
  parser.parse(cpp_code)
  print(json.dumps(parser.classes, indent=2, ensure_ascii=False))
