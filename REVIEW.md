# 代码审查报告：3706d6ea..HEAD

审查范围：commit `3706d6ea`（Revision of event filter process）之后 37 个提交，43 个文件，~3681 行新增改动。

参考实现：`/home/daiaji/repo/mkxp-z/`

---

## 全局分类

| 领域 | 提交数 | 关键文件 |
|------|--------|----------|
| 字体渲染 | 9 | `font_impl.cc`, `font_context.cc`, `rgss3_compat.rb`, `binding_patch.cc` |
| 游戏手柄 | 9 | `keyboard_controller.cc`, `keyboard_controller.h` |
| 窗口/显示管道 | 5 | `renderscreen_impl.cc`, `urge_main.cc`, `content_runner.cc` |
| Ruby 兼容性 | 7 | `binding_patch.cc`, `rgss3_compat.rb`, `ruby19_compat.rb` |
| 其他（tilemap, canvas, console等） | 7 | `tilemap_impl.cc`, `canvas_impl.cc`, `canvas_impl.h`, `mri_main.cc` |

---

## 🔴 严重问题（必须修复）

### 1. `bd66363f` refactor: align gamepad support with mkxp-z style — **编译中断**

**问题描述**：此提交更新了 `keyboard_controller.h` 的类型系统（统一 `BindingSource`/`BindingEntry` 替代旧的 `GamepadSource`/`KeyBinding`），新增了 13 个方法声明，但 **完全没有修改 `keyboard_controller.cc`**。

**具体冲突**：

| 位置 | 使用旧类型/名称 | 头文件中已改为 |
|------|----------------|----------------|
| `.cc:45,48` | `GamepadBindingEntry` | `BindingEntry` |
| `.cc:48` | `GamepadSource::Type::Button` | `BindingSource::Type::GamepadButton` |
| `.cc:957` | `capture_slot_ = GamepadSource()` | 字段类型改为 `BindingSource` |
| `.cc:974` | `GamepadSource::Type::Button` | `BindingSource::Type::GamepadButton` |
| `.cc:978,983` | `gp_bindings_` | 字段重命名为 `bindings_` |
| `.cc:979` | `GamepadBindingEntry` | `BindingEntry` |
| `.cc:986` | `SaveGamepadBindingsInternal()` | `SaveBindingsInternal()` |

新增 13 个方法在 `.h` 声明、被 `binding_patch.cc` 调用，但 `.cc` 无实现：

```
GetGamepadName, GetGamepadPowerLevel
GetGamepadAxisLeft, GetGamepadAxisRight, GetGamepadAxisTrigger
GamepadPressEx, GamepadTriggerEx, GamepadRepeatEx, GamepadReleaseEx
GamepadRepeatCountEx, GamepadButtonTimeEx
GetGamepadRawButtonStates, GetGamepadRawAxes
```

**后果**：项目无法编译（类型不存在）+ 无法链接（未定义符号）。

---

## 🟡 中隐患

### 2. `aba050d5` feat: read all SFNT family names via FreeType for robust name resolution

**问题**：SFNT name table 遍历中编码处理不完整。

- 不支持 UTF-32BE（`TT_MS_ID_UCS_4`），遇到 UCS-4 编码的族名会输出乱码
- UTF-16BE 解码器不支持 surrogate pair（>0xFFFF 的码点），极罕见但在 CJK 扩展区可能遇到
- 每次循环内独立 `FT_Init_FreeType`/`FT_Done_FreeType`，仅启动时执行，非性能热点

**mkxp-z 对照**：正确处理 UCS-4；通过 `TTF_FONT_TO_FT_FACE` hack 复用已有 FT_Face。

### 3. `d5c37fda` refactor: remove redundant data_cache fallback

**问题**：移除了 `LoadFontInternal` 中的全量 data_cache 遍历兜底。

- 若族名解析 + 文件名查找都失败，现在会抛异常
- mkxp-z 在字体不存在时使用内嵌字体（`family.empty() → openBundledFont()`），不会抛异常

**建议**：改为内嵌字体兜底而非抛异常，与 mkxp-z 行为一致。

### 4. `c0da42dd` feat: implement Windows-style ppem font size calculation

**问题**：VDMX ratio 匹配逻辑可能过于宽松。

- URGE：`(xRatio == 1 && yStartRatio <= 1 && yEndRatio >= 1)` — 硬编码 `1`
- mkxp-z：`devXRatio == ratio.xRatio && devYRatio >= ratio.yStartRatio && devYRatio <= ratio.yEndRatio` — 设备分辨率精确匹配

**其他**：
- 缺少 `MAX_PPEM`（65535）上限检查（RGSS 限制 size 6-96，理论安全）
- 每次 `CalculateFontPPEM` 都独立 `FT_Init_FreeType`/`FT_Done_FreeType`

### 5. `54b7fb9a` refactor: move font compat tweaks from C++ to rgss3_compat.rb

**问题**：commit message 与实际 diff 严重不一致。

- message 声称移除了 `content_profile font_subs`、`font_context font_subs map`、`urge_main.cc fontSubs`，但实际这些文件根本不在 diff 中。`font_subs` 仍保留在 C++
- `command->outline` 硬编码为 `0`（原来传 `font_obj->Outlined() ? 1 : 0`），该字段变成死代码

**注**：fontSubs 留在 C++ 层是正确的（mkxp-z 也在 C++ 层处理）。

### 6. `166a788e` feat: align window/resize pipeline with mkxp-z

**问题**：`backing_scale_factor_` 被计算但从未使用。

- 每个周期从 `SDL_GetWindowSizeInPixels / SDL_GetWindowSize` 计算并存储
- 但游戏显示尺寸完全基于逻辑像素计算，HiDPI 显示器上渲染内容可能与物理像素不匹配
- mkxp-z 正确使用 backing scale 进行比例/偏移计算

---

## 🟢 低隐患 / 建议

| 提交 | 问题 | 建议 |
|------|------|------|
| `861a5d4e` | textSize 空格补偿（追加空格再减宽度）是脆弱的启发式方法；与字体无关的 renderscreen 改动混在同一次提交 | 后续改用 ppem 精确计算，拆分提交 |
| `a7078d16` | 大量 `fprintf(stderr, ...)` 调试日志污染 stdout | 最终构建前移除调试日志 |
| `00352efe` | 死区阈值从 `0x4000` 降为 `0x2000`（更灵敏但易误触），与 mkxp-z (`0x4000`) 不一致 | 保持一致或文档说明差异原因 |
| `b7a69b78` | `std::stoi` 解析无 try-catch，用户手写错误格式会崩溃；`std::stoi("123abc")` 不报错 | 加 try-catch，使用完整解析检查 |
| `97071fbe` | 与 `2ed6ae83` 的 `rebuild_buffers_pending_` 机制冲突（后提交覆盖了延迟重建） | 确认 use-after-free 修复是否被保留 |
| `canvas_impl.h` | `Command_DrawText::outline` 和 `GPUCanvasDrawTextSurfaceInternal` 的 outline 参数变为死代码 | 清理死代码 |
| `2f62dfb2` | `SDL_free(sticks)` 在非游戏手柄分支未调用，造成小内存泄漏 | 添加 `SDL_free` |

---

## ✅ 通过审查（无隐患）

| 提交 | 说明 |
|------|------|
| `5f566867` | auto-resolve font family names，正确 |
| `827399c1` | 修复 namespace 包裹，纯编译修复 |
| `43937080` | text_size 非 String 参数 guard，正确 |
| `d7528d3a` | FT_TRUETYPE_TAGS_H include + BlendTextSurface，正确 |
| `399f96a2` | compare → starts_with，C++20 正确重构 |
| `af162551` | gamepad 轮询去重，有效 |
| `a85ce2fb` | namespace scope 修复，正确 |
| `28ff4e4d` | VSync/帧率控制，正确（部分配置优于 mkxp-z） |
| `d8821502` | keep_ratio 切换 stretch-fill vs letterbox，正确 |
| `e33f26af` | Array 子类保留 reverse/sort/uniq/shuffle，正确 |
| `03d11213` | 扩展到 22 方法反射，正确 |
| `2ed6ae83` | use-after-free 延迟重建修复，正确 |
| `ec57b85e` | CallbackHistory 移除 + 手动历史导航，正确 |
| `tilemap` 改动 | 仅类型转换规范，无行为变更 |
| `binding_patch.cc` | 接口与 Input 虚函数一致 |
| `mri_main.cc` | REPL eval 回调，标准 MRI API |

---

## 与 mkxp-z 架构对比

| 特性 | URGE | mkxp-z | 评价 |
|------|------|--------|------|
| 字体族名解析 | 独立 FT_New_Memory_Face | SDL_ttf TTF_FONT_TO_FT_FACE hack | URGE 更独立，但编码缺失 |
| PPEM 计算 | 公式基本一致 | 多 MAX_PPEM clamp | URGE 缺上限检查 |
| 文本兼容层 | C++ + Ruby 混合 | 全 C++ | URGE 灵活但外部依赖有风险 |
| 整数缩放 | ImGui 视口裁剪 | 中间 FBO + last-mile | mkxp-z 画质更精细 |
| 手柄绑定 | 统一 BindingSource/BindingEntry | BDescVec | 功能等价 |
| REPL console | 有（ImGui） | 无 | URGE 特色，缺发布编译禁用 |
| backing scale | 计算但不使用 | 用于比例/偏移 | URGE 有 HiDPI bug |
| 事件驱动手柄 | 从 EventController 拷贝状态 | EventThread 消息驱动 | 思路等价 |

---

## 修复优先级

| 优先级 | 项目 | 工作量 |
|--------|------|--------|
| **P0** | `bd66363f`: 补充 keyboard_controller.cc 实现，统一类型 | 中 |
| **P1** | `aba050d5`: 添加 UTF-32BE 支持 | 小 |
| **P1** | `d5c37fda`: 改为内嵌字体兜底而非抛异常 | 小 |
| **P1** | `c0da42dd`: VDMX ratio 精确匹配 + MAX_PPEM clamp | 中 |
| **P2** | `166a788e`: 使用 backing_scale_factor_ | 中 |
| **P2** | `canvas_impl.h`: 清理 outline 死代码 | 小 |
| **P3** | 调试日志清理、死区阈值一致化、解析错误处理 | 小 |
