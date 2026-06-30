# URGE 引擎 Linux 移植 DEBUG 报告

## 一、环境

- 系统：Manjaro Linux (Arch-based), X11/Wayland 混合环境
- 编译器：GCC 15.2.1, C++20
- Ruby：rbenv 3.3.8 + 系统 Ruby 3.4.8
- 图形：Vulkan (AMD Radeon RX 550 / RADV)
- 构建：CMake 4.3.2 + Ninja

## 二、编译期问题

### 1. websocketpp C++20 兼容

**症状**：`content/network/websocket_impl.cc` 编译失败，`template-id not allowed for destructor in C++20`

**根因**：websocketpp 在 C++20 中使用了弃用的 `~endpoint<connection,config>(){}` 语法，GCC 15 默认开启 `-Wtemplate-id-cdtor`，项目又用了 `-Werror`

**修复**：`content/CMakeLists.txt` 对 GCC + C++20 加 `-Wno-error=template-id-cdtor`

### 2. Widget::InitParams 缺少 opengl 字段

**症状**：`app/urge_main.cc` Linux 分支 `widget_params.opengl = ...` 编译失败

**根因**：`InitParams` 结构体没有 `opengl` 成员

**修复**：`widget.h` 加 `bool opengl = false;`，`widget.cc` 中在创建窗口时据此设置 `SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN`

## 三、运行时崩溃

### 3. X11 Display 空指针 → SIGSEGV

**症状**：Wayland 会话下段错误在 `XGetXCBConnection(NULL)`

**根因**：`render_device.cc` 在 Linux 分支无条件读取 `SDL_PROP_WINDOW_X11_DISPLAY_POINTER` 并传给 `XGetXCBConnection`。Wayland 窗口没有 X11 属性，返回 NULL

**修复**：检查 `xdisplay` 是否为 NULL，是则跳过 X11 特定代码并提前返回错误

### 4. Marshal.load 不存在 → 静默回退到 Kernel.load

**症状**：`rb_funcallv(marshal_klass, "load", data)` 报 "string contains null byte"

**根因**：此错误历时最长。`Marshal.load` 并非通过传统的 `rb_define_module_function` 在 C 中注册，而是通过 Ruby 的 builtin 机制在 `marshal.rb` 中用 `def self.load` + `Primitive.marshal_load` 定义。这个 builtin 初始化函数 `rb_call_builtin_inits()` 在 Ruby 3.3 的共享库中编译为 **static 符号**（`nm` 显示 `t`），引擎无法链接调用它。结果 `Marshal.load` 从未被定义，`rb_funcallv` 沿继承链找到 `Kernel.load`，把二进制 marshal 数据当作文件路径传给 `rb_find_file`，触发 "string contains null byte"

**弯路记录**：
- `rb_funcallv` ↔ `rb_funcallv_public` 来回切换（Marshal.load 压根不存在，换啥都没用）
- `rb_eval_string_protect("::Marshal.load($var)")`（绕开方法查找但没解决根因）
- `rb_protect` 包裹 `LoadPackedScripts`（捕获异常但没解决根因）
- `dlsym` 查找 static 符号（本地符号不在 `.dynsym` 中，找不到）
- `ruby_process_options` 触发 builtin 加载（太重，重复初始化导致 SIGABRT）
- 手动 `rb_define_module_function` 注册 `Marshal.load`（有效但只修了一个方法）

**最终修复**：`dladdr(ruby_init, &info)` 获取 `libruby.so` 加载基址，加上 `readelf -s` 得到的 `rb_call_builtin_inits` 符号偏移 `0x1656f0`，算出函数实际地址并调用。一次调用加载所有 builtin 模块（marshal.rb、array.rb、kernel.rb 等）

### 5. ec->tag 为空 → Ruby 内部 SIGSEGV

**症状**：Ruby `rb_raise` 触发 `rb_ec_tag_jump(ec, TAG_RAISE)`，`ec->tag` 为空导致 `ec->tag->state = st` 解引用崩溃

**根因**：这是崩溃 #4 的并发症状。当 `Marshal.load` 不存在时，回退到 `Kernel.load`，它要求参数是文件路径（C 字符串），而二进制数据包含 `\0`。`rb_get_path_check_to_string` → `rb_string_value_cstr` 失败 → `rb_raise` → `rb_longjmp` → `ec->tag` 为空（因为 `Kernel.load` 不是由 `rb_protect` 设置 tag 的 C 函数调用的）

**注意**：`ec->tag` 为空只在未启动的 fiber 上下文中发生（Ruby 源码 cont.c:2275），正常线程永不为空。这里为空是因为 `Kernel.load` 是从 C API (`rb_funcallv`) 调用的，没有正确的 VM 帧栈标签

**修复**：不需要单独修复，崩溃 #4 根因解决后此问题自然消失

## 四、关于 Ruby Builtin 机制的发现

Ruby 3.3 中，核心类的许多方法不再在 C 源文件中用 `rb_define_method` 注册，而是通过 builtin 机制：

```
marshal.rb → def self.load  → Primitive.marshal_load  → marshal.c: marshal_load()
array.rb   → def shuffle    → Primitive.rb_ary_shuffle → array.c: rb_ary_shuffle()
```

这些 `.rb` 文件在构建时被编译为 `.rbinc` 二进制数据，嵌入 `libruby.so`。初始化函数 `rb_call_builtin_inits()` 在 `inits.c:85` 定义，负责调用所有 `Init_builtin_*` 函数。

正常 Ruby 执行流程：`main → ruby_init() → ruby_options() → process_options() → rb_call_builtin_inits()`，但 `process_options()` 还做了解析参数、编译脚本等工作，对嵌入引擎太重。

Windows 上无此问题是因为 MinGW/MSVC 编译 Ruby 时不加 `-fvisibility=hidden`，所有符号包括 static 函数都对链接器可见。

## 五、最终改动

```
app/urge_main.cc                 | 11 +++++++++-
binding/mri/mri_file.cc          | 37 +++++++++++++++++++++++----------
binding/mri/mri_main.cc          | 20 +++++++++++++++----
content/CMakeLists.txt           |  3 +++
renderer/device/render_device.cc | 45 +++++++++++++++++++++++-----------------
ui/widget/widget.cc              |  4 ++++
ui/widget/widget.h               |  1 +
7 files changed, 88 insertions(+), 33 deletions(-)
```

三个层面的修复：

| 层面 | 改动 | 修复 |
|------|------|------|
| 编译 | `widget.h/cc` | `InitParams` 加 `opengl` 字段 |
| 编译 | `content/CMakeLists.txt` | websocketpp C++20 模板-id 警告降级 |
| 崩溃 | `render_device.cc` | X11 Display 空指针保护 + 提前返回 |
| 崩溃 | `app/urge_main.cc` | SDL_Init 失败检查 + Linux 强制 X11 驱动 |
| 功能 | `mri_main.cc` | `dladdr` + 偏移量调 `rb_call_builtin_inits()` |
| 功能 | `mri_file.cc` | `MriLoadData` 改为读字节串直接 `Marshal.load` |

## 六、遗留

- 游戏兼容脚本 `ruby19_compat` 对 Ruby 3.3 的适配可能需要额外调整
- 内置 builtin 加载的符号偏移量 `0x1656f0` 绑定具体的 Ruby 3.3.8 (rbenv) 构建
