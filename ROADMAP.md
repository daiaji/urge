# URGE 后续开发路线图

> 本文档记录了 URGE 引擎计划移植 / 实现的功能目标，基于对 mkxp-z 参考实现的分析。
> 标注 ✅ 的为已完成项。

## 项目定位

URGE 定位与 **RGD** 类似，面向**游戏作者**而非终端玩家。核心原则：

| 原则 | 说明 |
|------|------|
| **引擎只提供通用能力** | C++ 层内置基础渲染/输入/音频/配置，不做 RGSS 特化 |
| **RGSS 兼容在 Ruby 脚本层** | 类似 `rgss_patch`，由游戏作者在脚本编辑器中主动插入 |
| **不内置 RGSS 数据结构** | 规避法律风险，不直接嵌入 Enterbrain 的加密/数据格式 |
| **不追求免修改运行** | 不考虑"玩家下载原版游戏直接用 URGE 打开"的场景 |

据此，ROADMAP 中标注为 **「脚本层」** 的功能应由 Ruby 扩展实现，不属于引擎 C++ 端开发目标。

### Ruby 版本差异与兼容补丁

官方 RGSS 各版本捆绑的 Ruby 版本不同，URGE 统一使用 Ruby 3.3，通过独立的兼容补丁文件逐层恢复行为差异：

| 文件 | 目标版本 | 覆盖内容 | 适用 RGSS |
|------|---------|---------|-----------|
| `ruby18_compat.rb` | Ruby 1.8 → 3.x | `Hash#index`/`Object::TRUE`/`FALSE`/`NIL`/`Object#id`/`#type` 等 1.8 特有 API | RGSS1 (XP) |
| `ruby19_compat.rb` | Ruby 1.9.2 → 3.x | Array 子类保留（DUPSETUP 宏）、`BasicObject#initialize(*args)` | RGSS2/3 (VX/VXA) |
| `rgss3_compat.rb` | RGSS 行为 | 引擎通用 API → VX Ace 标准行为（如 `resize_screen` 顺带调窗口） | RGSS3 (VXA) |
| 内嵌通用 | Ruby 2.4+ / 2.7+ | `Fixnum`/`Bignum` → `Integer` 别名、`taint` 安全模型桩 | 全部 |

补丁文件位于 `binding/mri/third_party/rgss/`，通过 `rm-toolkit --inject-script` 按顺序注入到脚本数组。
`rgss3_compat.rb` 作为独立脚本条目紧跟 `rgss3_patch.rb` 之后注入。

**关键历史：**

> RGSS3 的 `RGSS301.dll` 并非 Enterbrain 魔改 Ruby——他们只是选了 2010 年的 Ruby 1.9.2p0，
> 之后从未升级。1.9.3 删除了 `DUPSETUP` 宏（Ruby 核心团队的性能优化），导致 Array 子类在
> `reverse`/`sort`/`uniq` 等方法中丢失。URGE 通过 `ruby19_compat.rb` 在 Ruby 层恢复这些行为。
>
> 详情见 `binding/mri/third_party/rgss/ruby19_compat.rb`，三方证据（1.9.2 源码 / 3.3.8 源码 / VX Ace 反射）。

| 时间线 | |
|--------|---|
| 2005+ | RGSS1 (RMXP) 基于 Ruby 1.8.x，`Object#id`/`Hash#index` 等 API 在后续版本移除 |
| 2010.08 | Ruby 1.9.2p0 发布，Enterbrain 开始绑进 RGSS3 |
| 2011.10 | Ruby 1.9.3p0 发布，删除 `DUPSETUP`（子类保留失效） |
| 2011.12 | VX Ace 日本发售，`RGSS301.dll` 冻结 1.9.2p0 |
| 2025+ | URGE 用 Ruby 3.3，`ruby18_compat.rb` + `ruby19_compat.rb` 恢复差异 |

---

## 1. 画质增强 / 超分辨率

### 已变更 ✅

| 功能 | 说明 |
|------|------|
| **整数倍缩放** | `IntegerScaling=true/false`，`Graphics.integer_scaling=`，ImGui 复选框。启用时保持整数倍放大，剩余区域黑边 |
| **分离上下采样滤镜** | `SmoothScaling=N`（放大滤镜）、`SmoothScalingDown=N`（缩小滤镜），`Graphics.smooth_scaling=` / `Graphics.smooth_scaling_down=`。向后兼容 `SmoothScalePresent=true` |
| **画面比例保持可切换** | `KeepRatio=true`（保持比例+黑边） / `false`（拉伸填满），INI + Ruby + ImGui 全覆盖 |
| **Lanczos3 / Bicubic 缩放** | 6-tap Lanczos 核（抗振铃 clamp） + Mitchell-Netravali Bicubic，全屏 Quad 后处理 Pass |
| **Anime4K DoG 单 Pass 内联** | 7×7 高斯 + min/max 跟踪 + DoG clamp，零额外 Pass 开销 |
| **CAS 锐化** | AMD FidelityFX CAS 5-tap 十字形，sharpness 0~1 可调，独立 Pass 后处理 |
| **Sobel 自适应混合** | Anime4K Enhance PS 内联 Sobel 3×3，按边缘强度 lerp 混合，零额外采样 |
| **Anime4K+Sobel 细调** | Thin warp 线条变细 + Darken line 线条加深，Sobel/Warp/Dark 三滑块 |
| **缩放算法 Ruby API** | `Graphics.scaling_mode` 整数属性（0=Bilinear ~ 5=Anime4K+Sobel），auto-gen 绑定 |
| **设置持久化** | 所有图形/音频设置自动写入 Game.ini，退出重启保留；三区域 Reset 按钮 + 全局 Reset All |

### 背景

RGSS 游戏原生分辨率为 640×480，在 1080p/4K 显示器上需要放大 2~4 倍。
URGE 当前仅支持 Nearest / Bilinear 两种采样，无法满足高清显示需求。

### 目标算法

| 算法 | 类型 | 适用场景 | 速度 | HLSL | 优先级 |
|------|------|----------|:----:|:----:|:------:|
| **Nearest** | 最近邻插值 | 像素风原比例显示 | 零开销 | 原生 | ★ 已有 |
| **Bilinear** | 双线性插值 | 通用平滑 | 零开销 | 原生 | ★ 已有 |
| **Bicubic** | 三次卷积插值 | 通用平滑放大（比 Bilinear 锐利） | ~0.02ms | 简单 | ✅ 已完成 |
| **Lanczos3** | 3-lobe 正弦重采样 | 文字/UI 边缘最优 | ~0.03ms | 简单 | ✅ 已完成 |
| **Anime4K DoG (单Pass内联)** | 7×7 高斯 + DoG clamp | 线条画、立绘、动画风格 | ~0.08ms | GLSL 已移植 | ✅ 已完成 |
| **CAS 锐化** | AMD FidelityFX 5-tap 十字形 | 通用后处理锐化 | ~0.02ms | HLSL 已移植 | ✅ 已完成 |
| **Sobel 自适应混合** | Anime4K 内联 Sobel 3×3 + lerp | 边缘自适应锐化 | ~0.03ms | 内联实现 | ✅ 已完成 |
| **FSR 1.0 (EASU+RCAS)** | 边缘自适应放大 + 锐化 | 照片、CG、3D 渲染 | ~0.1ms | ✅ 开源 | ☆ 可选 |
| **xBRZ** | 像素艺术规则放大 | RM 默认像素风（色块保持） | ~0.2ms | 需翻译 | ☆ 可选 |

### 滤镜控制 ✅ 部分已实现

| 配置 | 说明 | URGE |
|------|------|:----:|
| `SmoothScaling` | 放大时使用的滤镜（0=Nearest / 1=Bilinear） | ✅ `smooth_scaling` |
| `SmoothScalingDown` | 缩小时独立控制的滤镜 | ✅ `smooth_scaling_down` |
| `bitmapSmoothScaling` | 单个 Sprite/Bitmap 纹理放大滤镜 | 待实现 |
| `bitmapSmoothScalingDown` | 单个 Sprite/Bitmap 纹理缩小滤镜 | 待实现 |

### HiRes 模式

| 配置 | 说明 |
|------|------|
| `framebufferScalingFactor` | 帧缓冲缩放倍率（如 2.0 = 渲染到 2x 尺寸再缩小显示） |
| `textureScalingFactor` | 纹理加载缩放倍率（控制贴图本身的分辨率） |

### 整数倍缩放 ✅ 已实现

| 配置 | 说明 | URGE |
|------|------|:----:|
| `IntegerScaling` | 启用时强制保持整数比例放大，多余区域黑边 | ✅ `integer_scaling` |
| `integerScaling.lastMileScaling` | 是否允许最后一档用非整数缩放填满屏幕 | 待实现（非关键） |

### 实现方案

```
当前管线:
  Game → screen_buffer → ImGui::Image() → Swapchain (Point/Linear 二选一)

目标管线:
  Game → screen_buffer → [后处理 Pass] → upscale_buffer → ImGui::Image() → Swapchain
                           ↑
                     新增中间 RT (窗口分辨率)
                     全屏 Quad + 自定义 HLSL Pipeline
```

- 新增中间渲染目标 `upscale_buffer`（窗口分辨率）
- Diligent 全屏 Quad Pipeline，绑定 screen_buffer 为 SRV
- ImGui 纹理源从 screen_buffer 改为 upscale_buffer
- 设置 UI 可切换全部算法

### 双入口设计 ✅ 已实现

| 入口 | 操作者 | 生效 | 优先级 |
|------|--------|------|:------:|
| **ImGui 设置页** | 玩家 | 写入引擎配置，持久化 | 低（默认） |
| **`Graphics.scaling_mode=`** | 游戏脚本 | 覆盖玩家设置，立即生效 | 高 |

当前模式映射：0=Bilinear / 1=Nearest / 2=Lanczos3 / 3=Bicubic / 4=Anime4K / 5=Anime4K+Sobel

```ruby
Graphics.scaling_mode          # => 0~5
Graphics.scaling_mode = 5      # Anime4K+Sobel
```

### 自动适配方案 ✅ Anime4K+Sobel 已实现

RM 游戏素材类型混杂（立绘线条 / 像素地图块 / 照片远景），单一算法无法全局最优。
当前实现在 Anime4K Enhance PS 中**内联 Sobel 3×3 边缘检测**，按边缘强度 lerp 混合：

```
Anime4K DoG 锐化 → Sobel 3×3 梯度 → norm → pow(strength)
                                    ↓
            result = lerp(原始, 锐化, dval)
                                    ↓
                    输出 (窗口分辨率)
```

- mode=5（ImGui 显示 "Anime4K+Sobel"）
- Sobel 强度 0~2、Warp 强度 0~1、Darken 强度 0~2 三滑块微调
- 零额外纹理采样（复用 7×7 循环的中间 3×3 luma 值）

### 算法与素材适配

| 素材类型 | 最优算法 |
|----------|---------|
| 立绘 / 对话框 / UI 线条 | Anime4K Mode C |
| 像素地图块 / 远景模糊 | Lanczos3 / Bicubic |
| 照片级 CG / 3D 背景 | FSR 1.0 EASU+RCAS |
| 纯像素风（不放大） | Nearest（最近邻） |

### 备注

- FSR 1.0 是唯一适合 2D 的 FSR 版本（FSR 2/3/4 需深度/运动矢量，3D only）
- FSR 1.0 已停更，但作为单帧空域算法不存在技术过时
- Anime4K Mode C 最后提交 2024-04，纯数学后处理无需持续更新
- DLSS / XeSS 需要专用硬件或深度缓冲，2D RGSS 无法接入
- NIS 质量 ≈ FSR 1.0 且更冷门，无独立 HLSL 散包

---

## 2. 帧率控制与补帧

### 帧率解锁 / VSync ✅ 已完成

参考 mkxp-z 设计：

| 配置 (INI) | 效果 | 状态 |
|------|------|:----:|
| `VSync=0/1` | 关闭/开启垂直同步 | ✅ |
| `FrameRate=0` | 不限帧，跑满硬件 | ✅ |
| `FrameRate=144` | 锁 144fps | ✅ |
| `SyncToRefreshRate=true` | 自动跟随显示器刷新率（开启时强制 VSync=1 并禁用 FPSLimiter） | ✅ |

### 双入口

| 入口 | 操作者 | 优先级 |
|------|--------|:------:|
| **ImGui 设置页** | 玩家 | 低（默认） |
| **`Graphics.frame_rate=`** | 脚本 | 高（覆盖） |

- `Graphics.frame_rate` 已有（通过 bindgen 自动生成），`frame_rate=0` 不限帧
- `Graphics.vsync` 已有（通过 bindgen 自动生成），`vsync=0` 关闭
- ImGui 设置页有 VSync 复选框 + FrameRate 滑块

FreeSync 场景下建议 `VSync=0` + `SyncToRefreshRate=true`，既无撕裂也无需垂直同步延迟。

### 帧率解锁与 RGSS 基建脚本兼容性

> ⚠️ RGSS 基建脚本（Game_CharacterBase、Game_Timer、Game_Screen 等）硬绑定固定帧率，
> 所有移动/计时/过渡效果均按"每帧"而非"每秒"计算。
> 帧率解锁后基建脚本会整体加速，需在 Ruby 脚本层归一化或引擎启用补帧。

**受影响的基建类**（VX Ace 为例）：

| 类 | 表现 |
|------|------|
| `Game_CharacterBase` | `distance_per_frame` 每帧固定像素，移速正比帧率 |
| `Game_Character` | 动画计数 `@anime_count`、事件 `@wait_count` 帧累计 |
| `Game_Timer` | 计时 `@count / Graphics.frame_rate`（引擎已确保不返回 0） |
| `Game_Screen` | 淡入淡出/画面渐变按帧数，高帧率下转瞬即逝 |

**脚本归一化示例**：

```ruby
class Game_CharacterBase
  def distance_per_frame
    (2 ** real_move_speed / 256.0) * (60.0 / Graphics.frame_rate)
  end
end
```

**引擎层终极方案**：补帧（见下方）——逻辑跑 60fps，GPU 插帧输出高帧率画面，零脚本改动。

> 关 VSync 时菜单 400fps 但游戏内仅 200fps：非引擎限制，是 Ruby 脚本计算量压住了 CPU，
> 恰好印证补帧必要性——CPU 瓶颈不变，GPU 空闲时间刚好用来插帧。

### 补帧（Frame Generation）

> 长期目标

**原理**：在两帧之间用光流法（Optical Flow）插值中间帧，让画面在视觉上跑在更高帧率。
类似 SVP / LSFG 3.0 / DLSS 3 的思路，但纯 2D 不需要运动矢量缓冲。

| 方案 | 原理 | 质量 | 实现成本 |
|------|------|:----:|:--------:|
| **前帧混合** | 前后帧线性混合 + 边缘感知 warp | 一般，有残影 | 低 |
| **简化光流（推荐）** | Compute shader: Sobel → 块匹配 → warp → 混合 | 好，延迟低 | 中 |
| **完整光流** | Pyramidal optical flow + 遮挡处理 | 好，但复杂 | 高 |

推荐**简化光流**方案，4 个 compute shader 实现：

```
输入: 帧 A, 帧 B
Pass 1 — Sobel 边缘检测（3×3 梯度，求方向）
Pass 2 — 局部块匹配（8×8 block, 搜索 ±4px）→ 运动向量场
Pass 3 — 按运动向量 warp 帧 A → 中间位置
Pass 4 — 与帧 B 加权混合 + 遮挡处理
```

- 每帧新增 ~0.3ms（1080p）
- Diligent `DispatchCompute` 直接实现
- 不依赖第三方库（SVP 需剥离 OpenCL 播放框架，RIFE/DAIN 跑不动实时）

### 为什么 2D 补帧性价比高

RGSS 的性能瓶颈在 **CPU（Ruby 脚本执行）**，GPU 大量闲置：

```
一帧时间分解:
  ┌────── CPU (脚本) ──────┬────── GPU (渲染) ────┐
  │  ████████████████████   │  ██████              │
  │  ≈8ms 跑脚本            │  ≈0.2ms 渲染 640×480 │
  └─────────────────────────┴──────────────────────┘
  ← 60fps 预算 16.6ms →    ← GPU 空闲 ~8ms →
```

GPU 补帧正好填入 GPU 空闲时间：

```
  ┌────── CPU (脚本) ──────┬───────── GPU ────────────┐
  │  ████████████████████   │  ██████  ███████████████  │
  │  ≈8ms 跑脚本            │  render  ↑ 光流 4 pass ↑ │
  └─────────────────────────┴──────────────────────────┘
  ← 输出 120fps →           ← 空闲时间完全利用 →
```

结果：**不增加延迟，白送一倍帧率**。CPU 跑 60fps → GPU 补到 120fps。
与 DLSS 3 不同（GPU 满载硬挤补帧，延迟升高），RGSS 的补帧是真正零成本。

### 暴露脚本 API

```ruby
Graphics.frame_rate            # => 当前帧率目标（0 = 不限）
Graphics.frame_rate = 144       # 锁定
Graphics.frame_rate = 0         # 解锁
Graphics.frame_rate = :default  # 回退到玩家设置
Graphics.vsync_enabled = false  # 关闭 VSync（让 FreeSync 接管）
```

---

## 3. 显示 / 窗口 ✅ 已完成

| 功能 | 配置 (INI) | 说明 | 实现 |
|------|-------------|------|:----:|
| **VSync 可配置** | `VSync=0/1` | INI 可配置 + ImGui 复选框切换 | ✅ |
| **窗口可调整大小** | `WinResizable=true/false` | 允许/禁止用户拖拽调整窗口 | ✅ |
| **固定宽高比** | `FixedAspectRatio=true/false` | `SDL_SetWindowAspectRatio` 保持游戏比例，随 `resize_screen` 同步更新 | ✅ |
| **画面比例保持可切换** | `KeepRatio=true/false` | `true` 黑边保持比例 / `false` 拉伸填满，INI + Ruby + ImGui 全覆盖 | ✅ |
| **窗口分辨率变更同步** | — | `resize_screen` 同步重建帧缓冲 + 更新 aspect ratio 锁，mkxp-z 行为对齐 | ✅ |
| **`CheckResizeInternal` 原子化** | — | 每帧统一检测窗口尺寸 + 更新 `backingScaleFactor` + 缩交换链 | ✅ |

---

## 4. 单手柄支持

- **状态**: ✅ 已完成（同 mkxp-z，仅第一个手柄）
- 单 `SDL_Gamepad*` 指针，仅认第一个连接的控制器
- 按 `SDL_GAMEPAD_BUTTON_COUNT` / `SDL_GAMEPAD_AXIS_COUNT` 数组轮询

---

## 5. 手柄震动反馈

- **状态**: ⏳ C++ 实现完成，未绑定 Ruby
- `RumbleGamepad(low, high, ms)` 调用 `SDL_RumbleGamepad()`
- 暴露给脚本层后可实现受击/反馈等效果

---

## 6. 手柄按键 / 轴映射可配置 + 持久化

- **状态**: ✅ 已完成
- 移植自 mkxp-z 的 `SettingsMenu`
- `GamepadBindingEntry` 扁平列表（Button / Axis 联合）
- ImGui 设置页：表格展示 + PopupModal 捕获弹窗
  - 左键添加绑定，右键清除绑定
  - 捕获后自动跳转下一项
  - 支持符号：A/B/C/X/Y/Z/L/R/方向/SHIFT/CTRL/START/BACK
- 边缘检测捕获（基线法，只捕捉新按下的输入）
- 独立 `_gp.txt` 持久化文件，纯文本可编辑格式
  - `SYMBOL = Btn:INDEX` / `SYMBOL = Axis:INDEX:[+-]`
  - 支持 `#` 注释，任意符号可手写配置（含 F 键、精英手柄拨片等）

---

## 7. 键位动作名称配置

| 配置 | 参考 mkxp-z | 说明 |
|------|-------------|------|
| `kbActionNames.a/b/c/x/y/z/l/r` | `config.kbActionNames` | 设置 UI 里按钮标签可自定义（如把 "C" 显示为"确认"、"攻击"） |

---

## 8. `Net` 网络模块（脚本层 HTTP）

### 当前状态

URGE 有 `components/network/` 但仅是内部任务调度器，**没有暴露给 Ruby 的 HTTP API**。

### 目标 API

| 方法 | 说明 |
|------|------|
| `Net::HTTP.get(url)` | GET 请求，返回 body 字符串 |
| `Net::HTTP.post(url, body)` | POST 请求 |
| `Net::HTTP.get_json(url)` | GET 并解析 JSON |
| `Net::HTTP.post_json(url, data)` | POST JSON |
| `Net::HTTP.headers` | 自定义 Header 支持 |

### 实现方案

- 捆绑 `cpp-httplib`（mkxp-z 同款，header-only，轻量）
- 可选 OpenSSL 支持 HTTPS
- 异步回调防止阻塞主线程
- 参考 mkxp-z 的 `src/net/net.cpp`

---

## 9. 字体系统增强 ✅ 已对齐

### 引擎层（C++）

| 配置 (INI) | 默认值 | 说明 |
|------|:------:|------|
| `DefaultFontPath` | — | 字体目录和默认字体文件 |
| `FontScale` | 0.9 | 字体缩放系数 |
| `FontKerning` | true | `TTF_SetFontKerning`，字符间距自然 |
| `FontHinting` | 3 | `TTF_HINTING_NONE` |
| `FontOutlineCrop` | true | 裁切描边与正文交接处的透明缝 |
| `FontSubs` | — | 字体替换表，`"From>To"` 逗号分隔，C++ `LoadFontInternal` 中按原名查找替换 |

其他 C++ 增强：

| 功能 | 说明 |
|------|------|
| **SFNT 全量族名索引** | 启动时通过 FreeType 遍历字体的 SFNT name table，索引所有族名。`Font.default_name="LGBaseFont"` 等族名可自动匹配到对应字体文件。支持 UTF-16BE / UTF-32BE (UCS-4) 解码，跨平台。 |
| **阴影颜色** | 阴影 RGB 取自文本颜色而非纯黑，与 mkxp-z 行为一致（`FontImpl::RenderShadowSurface`）。 |
| **透明文本 alpha** | 移除 `font_color.a = outline_color.a = 255` 强制不透明，TTF 渲染使用实际 alpha（`FontImpl::RenderText`）。 |
| **不区分大小写回退** | Windows 上文件名大小写与 INI 配置不匹配时的回退。 |
| **内嵌字体兜底** | 找不到任何字体文件时使用内嵌字体。 |

### 脚本层（rgss3_compat.rb）

| 功能 | 说明 |
|------|------|
| **textSize 空格补偿** | 追加空格再减宽度，补偿 FreeType 末字偏大。 |
| **draw_text outline 偏移** | Center/Right 对齐补偿 outline 边框宽度。 |
| **draw_text 超长挤压** | 文本宽度超过 rect 时水平缩放适应。 |
| **resize_screen 窗口同步** | 调用 `resize_screen` 后同步移动窗口位置。 |

### 待实现

| 配置 | 参考 mkxp-z | 说明 | 原因 |
|------|-------------|------|------|
| `font_height_reporting` | `config.fontHeightReporting` | 行高报告模式 | URGE 无 `Font.height` 挂载点 |

### 已修复

- **`require_relative` 不可用问题**：`rgss3_patch.rb` 原用 `require_relative` 加载 `rgss3_compat.rb`，但 RGSS 脚本通过 `eval()` 执行无文件系统上下文。已移除 `require_relative`，改为通过 `rm-toolkit --inject-script` 分别注入补丁文件。

---

## 10. 控制台 / 开发调试（REPL + 输出显示）

### 已实现 ✅

- **ImGui REPL 控制台**：按 `~` 打开，多行输入 Ruby 代码，Enter 执行，Shift+Enter 换行
  - 输出区显示输入回显 + 执行结果/错误
  - 历史记录（Up/Down 导航）
  - 每次打开独立执行，不影响游戏状态

### Console Ruby 模块 ✅

| 方法 | 说明 |
|------|------|
| `Console.write(msg)` | 输出文本到控制台 |
| `Console.puts(msg)` | 输出文本并换行 |
| `Console.clear` | 清空控制台输出 |

mkxp-z **没有** Ruby 层面的 Console 模块（仅 Windows 原生 `AllocConsole` 调试输出窗口）。

---

## 11. `System` 模块扩展

### 当前状态

URGE 的 `EngineImpl` 仅暴露基础平台信息，缺少 mkxp-z 的丰富运行时 API。

### 目标 API（参考 mkxp-z `binding-mri.cpp`）

| 方法 | 说明 | 优先级 |
|------|------|:------:|
| `System.platform` | 返回 `"Windows"` / `"macOS"` / `"Linux"` | ★★★ |
| `System.is_wine?` / `System.is_rosetta?` | 检测兼容层运行（Wine 下需特殊路径处理） | ★★★ |
| `System.user_language` | 系统语言（`"zh-CN"` 等），自动本地化 | ★★★ |
| `System.user_name` | 操作系统用户名 | ★★ |
| `System.game_title` | 游戏窗口标题 | ★★ |
| `System.mount(path)` / `System.unmount(path)` | 动态挂载/卸载资源路径，天然支持 mod 系统 | ★★★ |
| `System.launch(path, args='')` | 用系统默认程序打开文件/URL，支持参数 | ★★ |
| `System::CONFIG` | 将引擎配置以 JSON 对象暴露给脚本只读 | ★★ |
| `System.parse_csv(string)` | 内置 CSV 解析器，解析游戏数据 | ★★ |
| `System.nproc` | CPU 核心数 | ★ |
| `System.memory` | 系统总内存 (MB) | ★ |
| `System.power_state` | 电池电量/剩余秒数/是否放电 | ★ |
| `System.puts(msg)` | 输出到控制台/日志（等价 `Console.write`） | ★ |
| `System.desensitize(path)` | 不区分大小写的文件路径查找 | ★ |
| `System.default_font_family=` | 全局默认字体族设置 | ★ |
| `System.delta` / `System.uptime` | 引擎运行时间（秒） | ★ |

### CFG 模块（运行时配置读写） ★★★

```
CFG["windowTitle"]          # => "My Game"
CFG["windowTitle"] = "New"  # 运行时修改 + 持久化到用户目录 JSON
CFG.to_hash                  # 所有配置的哈希表
```

- 写入立即生效，重启后保留
- 未写入的键回退到引擎默认值
- 参考 `mkxp-z/binding/binding-mri.cpp:725-787`

---

## 12. 音频增强

| 功能 | 参考 mkxp-z | 说明 | 优先级 |
|------|-------------|------|:------:|
| **MIDI 合成 (FluidSynth)** | `config.midi.soundFont` / `chorus` / `reverb` | 游戏内 MIDI 播放 + SoundFont 配置 | ☆ 低 |
| **SE 并发数可配置** | `config.SE.sourceCount` | 同时播放音效数量上限 | ☆ 低 |
| **BGM 轨道数可配置** | `config.BGM.trackCount` | 同时播放 BGM 数量上限 | ☆ 低 |

---

## 13. 其他可移植特性

| 功能 | 参考 mkxp-z | 描述 | 优先级 | 层 |
|------|-------------|------|:------:|:----:|
| **Theora 视频播放** | `src/theoraplay/` | OGV 视频回放（RGSS Movie 模块），小众需求 | ☆ 低 | 🔧 脚本层 |
| **preload/postload 脚本** | `config.preloadScripts` / `postloadScripts` | 主脚本前后注入 | ☆ 低 | 🔧 脚本层 |
| **自定义加密（原版 RGSS）** | `src/crypto/` | 原版 RGSS 加密格式解包 | ☆ 低 | 🔧 脚本层 |
| **改进加密方案** | 引擎自主设计 | 更安全的资源加密，算法不公开 | ☆ 低 | 引擎 |
| **JIT 选项** | `config.jit` / `config.yjit` | Ruby MJIT / YJIT 开关 | ☆ 低 | 引擎 |
| **鼠标光标定制** | `etc.cpp` | 隐藏/显示/自定义光标图形 | ☆ 低 | 引擎 |
| **自定义脚本入口** | `config.customScript` | 替换默认脚本加载路径 | ☆ 低 | 引擎 |

---

## 14. Bitmap 动画与图像处理（GIF / APNG）

### 动画支持 ★★★

参考 mkxp-z `bitmap.h:210-229`、`bitmap-binding.cpp:488-814`：

| 方法 | 说明 |
|------|------|
| `animated?` | 是否包含多帧动画 |
| `play` / `stop` | 播放 / 停止动画 |
| `playing` / `playing=` | 查询 / 设置播放状态 |
| `goto_and_play(n)` / `goto_and_stop(n)` | 跳转到第 n 帧并播放/停止 |
| `frame_count` / `current_frame` | 总帧数 / 当前帧 |
| `frame_rate` / `frame_rate=` | 动画帧率（FPS） |
| `looping` / `looping=` | 是否循环播放 |
| `add_frame(bitmap, pos)` / `remove_frame(pos)` | 动态增删帧 |
| `next_frame` / `previous_frame` | 步进/步退一帧 |
| `snap_to_bitmap` | 当前帧快照为独立位图 |

### 图像处理 ★

| 方法 | 说明 |
|------|------|
| `blur` | GPU 加速高斯模糊（5 采样，见 `shader/blur.frag`） |
| `radial_blur(angle, divisions)` | 径向模糊（旋转/缩放虚化效果） |
| `raw_data` / `raw_data=` | RGBA 原始字节缓冲区读写，像素级操作 |
| `to_file(filename)` | 保存为 PNG 文件 |
| `max_size`（类方法） | 查询硬件最大纹理尺寸 |

### 配置

| 配置 | 说明 |
|------|------|
| `bitmapSmoothScaling` / `bitmapSmoothScalingDown` | 单个 Bitmap/Sprite 纹理的放大/缩小滤镜独立控制 |

---

## 15. 精灵增强

### 图案叠加（Pattern）★★

参考 mkxp-z `sprite.h:65-73`、`sprite-binding.cpp:55-82`：

在原有精灵之上叠加可平铺、可滚动、可缩放的图案位图。

| 属性 | 说明 |
|------|------|
| `pattern` | 叠加图案位图 |
| `pattern_blend_type` | 叠加混合模式（加法/减法/正常） |
| `pattern_tile` | 是否平铺 |
| `pattern_opacity` | 叠加层透明度 (0–255) |
| `pattern_scroll_x` / `pattern_scroll_y` | 图案滚动速度 |
| `pattern_zoom_x` / `pattern_zoom_y` | 图案缩放倍数 |

典型用途：受击红色闪光、水面波动纹理、角色特效叠加，无需额外精灵对象。

### 反色（Invert）★

| 属性 | 说明 |
|------|------|
| `invert` | 精灵反色渲染，简单的视觉效果 |

---

## 16. 输入系统增强

### 鼠标输入 ★★★

参考 mkxp-z `input-binding.cpp:279-301,540-569`：

| 方法 | 说明 |
|------|------|
| `Input.mouse_x` / `Input.mouse_y` | 鼠标相对于游戏画面的坐标 |
| `Input.scroll_v` | 鼠标滚轮值 |
| `Input.mouse_in_window?` | 鼠标是否在窗口内 |
| `Input::MOUSELEFT` / `Input::MOUSEMIDDLE` / `Input::MOUSERIGHT` | 鼠标按钮常量 |
| `Input::MOUSEX1` / `Input::MOUSEX2` | 扩展鼠标按钮 |

### 剪贴板 ★★★

| 方法 | 说明 |
|------|------|
| `Input.clipboard` / `Input.clipboard=` | 读写系统剪贴板 |

### 扩展键盘 ★★

| 方法 | 说明 |
|------|------|
| `Input.pressex?(sym)` / `Input.triggerex?(sym)` | 接受 `:space`、`:return` 等符号，自定义键位绑定 |
| `Input.text_input` / `Input.text_input=` | 文本输入模式（IM）开关 |
| `Input.gets` | 读取文本输入缓冲区 |

### 手柄原始数据 ★

| 子模块 | 说明 | 状态 |
|------|------|:----:|
| `Input::Controller.connected?` | 手柄连接状态 | ✅ |
| `Input::Controller.name` | 手柄名称 | ✅ |
| `Input::Controller.axes_left` / `axes_right` / `axes_trigger` | 左/右摇杆、扳机原始值 | ✅ |
| `Input::Controller.raw_button_states` / `raw_axes` | 原始按键/轴数据数组 | ✅ |
| `Input::Controller.power_level` | 手柄电量 | ✅ |
| `Input::Controller.pressex?` / `triggerex?` / `repeatex?` / `releaseex?` | 扩展按键查询（按扫描码） | ✅ |
| `Input::Controller.repeat_count` / `button_time` | 按键重复计数 / 按下时长 | ✅ |

---

## 17. 其他配置与 API 扩展

### 新增配置选项

| 配置 (INI) | 参考 mkxp-z | 说明 | 优先级 |
|------|-------------|------|:------:|
| `patches` | `config.patches` | mod 补丁目录列表，在游戏资源之前加载 | ★★★ |
| `fontScale` | `config.fontScale` | 全局字体大小缩放倍数 | ★★ |
| `bicubicSharpness` | `config.bicubicSharpness` | Bicubic 缩放锐度参数（0–100+） | ★ |
| `enableBlitting` | `config.enableBlitting` | 帧缓冲 Blitting 模式（部分平台可提升性能） | ★ |
| `iconPath` | `config.iconPath` | 自定义窗口图标路径（Linux） | ★ |
| `anyAltToggleFS` | `config.anyAltToggleFS` | 左右 Alt+Enter 均可切换全屏 | ★ |
| `titleLanguage` | `config.titleLanguage` | 标题栏语言（未在 mkxp.json 文档化） | ★ |

### Graphics API 扩展 ★★

参考 mkxp-z `graphics-binding.cpp:425-462`：

| 方法 | 说明 |
|------|------|
| `Graphics.screenshot(filename)` | 脚本触发截图，保存为 PNG |
| `Graphics.resize_window(w, h)` | 脚本调整窗口（非游戏内分辨率） |
| `Graphics.center` | 窗口居中到屏幕 |
| `Graphics.display_width` / `Graphics.display_height` | 显示器分辨率 |
| `Graphics.average_frame_rate` | 当前实际平均帧率 |
| `Graphics.show_cursor` / `Graphics.show_cursor=` | 显示/隐藏系统鼠标光标 |
| `Graphics.scale` / `Graphics.scale=` | 动态画面缩放倍数 |
| `Graphics.smooth_scaling` / `Graphics.smooth_scaling=` | 运行时切换缩放滤镜 | ✅ 已实现 |
| `Graphics.integer_scaling` / `Graphics.integer_scaling=` | 运行时切换整数倍缩放 | ✅ 已实现 |
| `Graphics.thread_safe` / `Graphics.thread_safe=` | 关闭 GVL 释放，线程安全绘制 |

---

## 架构迁移评估（WGPU / origin/master）

> 以下分析基于 `origin/master`（WGPU 重写分支）与 `origin/legacy`（当前 `main` 基础）的对比。
> `origin/master` 与 `origin/legacy` 无共同祖先，是作者从头重写的架构骨架。

### 新架构现状

| 模块 | 文件行数 | 状态 |
|------|:--------:|:----:|
| WGPU GPU 设备层（gpu_device/command/resource） | ~1600 | ✅ 已实现 |
| 场景图（Node/Transform/World） | ~150 | ⚠️ 基本骨架，World/Renderer 空壳 |
| Camera / Light / Viewport | ~200 | ⚠️ Camera 有实现，其余空壳 |
| ImGui / Widget 窗口管理 | ~300 | ✅ 已实现 |
| CoreProfile 配置 | ~55 | ⚠️ 基本实现 |
| CRuby 绑定框架 | ~500 | ⚠️ IDL 生成器 + 入口桩，未绑定实际 API |
| ExternalBinding | ~19 | ❌ 空壳，不执行任何脚本 |
| Material / Mesh | ~14 | ❌ 空壳（7 行） |
| **字体/Canvas/2D 渲染** | **0** | ❌ **不存在** |
| **输入系统（键盘/鼠标/手柄）** | **0** | ❌ **不存在** |
| **Ruby RGSS 绑定** | **0** | ❌ **不存在** |
| **音频/视频** | **0** | ❌ **不存在** |
| **网络** | **0** | ❌ **不存在** |

**整体完成度：~15%**。WGPU 底层磨好了，上层游戏引擎全是空地。

### 从 `main` 迁移的工作量估算

假设作者完成新架构底座（WGPU 管线 + 场景图 + CRuby + IDL 生成），我们从 `main` 移植功能的工作量：

| 功能 | 行数 | 迁移难度 | 说明 |
|------|:----:|:--------:|------|
| **手柄支持** | ~1200 | 🟢 低 | 纯 SDL3 API，零渲染依赖，直接复制 |
| **ImGui REPL 控制台** | ~200 | 🟢 低 | 纯 ImGui + Ruby eval，不依赖渲染 |
| **RGSS 兼容补丁** | ~80 | 🟢 低 | Ruby 脚本文件，直接放入 |
| **音频服务（miniaudio）** | ~1000 | 🟢 低 | 平台无关音频库 |
| **字体增强（FreeType）** | ~500 | 🟡 中 | 算法可复用，纹理上传需适配 WGPU |
| **窗口/缩放管线** | ~300 | 🟡 中 | 逻辑可复用，渲染后端换 WGPU |
| **Canvas 改进** | ~100 | 🟡 中 | 同上 |
| **FPS 限制器** | ~50 | 🟢 低 | 纯计时逻辑 |

**总量估算：~3400 行**
- 🟢 低难度（直接复制）: ~2500 行（73%）
- 🟡 中难度（适配 API）: ~900 行（27%）

### 当前扩展的性价比

`main`（旧架构）是一个**完整可运行的 RGSS 引擎**。在新架构完工前：

| 要加的功能 | 在 `main` 上加 | 等新架构再移植 |
|-----------|:-------------:|:-------------:|
| **手柄支持** | ✅ 即时可用 | ➡ 低难度，~1 天 |
| **字体/渲染** | ✅ 即时可用 | ➡ 中难度，需要 WGPU 适配 |
| **音频/网络** | ✅ 即时可用 | ➡ 低难度，API 无关 |
| **Ruby 绑定** | ✅ 即时可用 | ➡ 需要等 CRuby 绑定框架就绪 |

**结论：加的功能越独立于渲染管线，性价比越高。** 手柄/音频/网络这类纯逻辑功能在 `main` 上扩展毫无问题，未来迁移成本很低。字体/Canvas 类改动因为新架构也没有，在 `main` 上做反而是最有价值的时间点——先把功能打磨好，将来直接搬运算法。

**不建议等新架构**。作者的个人项目进度不可预测，而手柄等缺失功能对 `main` 的可用性是切实的短板。


## 进度总览

> 分类依据：**Ruby 脚本能否自己实现？**
> - 「引擎」— Ruby 做不到，必须 C++ 层提供（GPU 管线、二进制解码、字体渲染等）
> - 「钩子」— 引擎暴露薄 API，Ruby 层组合逻辑（剪贴板、CFG 读写、平台常量等）
> - 「🔧 脚本层」— 引擎已提供基础能力，Ruby 完全能自己做

### 引擎必须做（Ruby 做不到）

| 功能 | 为什么必须引擎 | 状态 |
|------|--------------|:----:|
| **超分辨率画质增强**（Anime4K / Lanczos3 / Bicubic / CAS / Sobel） | HLSL/GLSL 着色器 + GPU 渲染管线 + 中间 RT | ✅ 已完成 |
| **字体 Fallback + 排版控制** | HarfBuzz 字形回退 + FreeType 描边裁剪 | 待实现 |
| | ✅ kerning/hinting/outline_crop/font_scale 已完成 | 已实现 |
| **补帧（光流法 Frame Generation）** | Compute Shader 光流计算 + DispatchCompute，纯 GPU 端 | 待实现 |
| **HiRes 模式**（framebufferScalingFactor） | 帧缓冲超采样，需要 GPU 纹理缩放 Pass | 待实现 |
| **MIDI 合成 (FluidSynth)** | SoundFont → PCM 音频缓冲区合成，Ruby 无实时音频管线 | 待实现 |
| **Bitmap 动画**（GIF / APNG 播放） | 二进制动画解码 + 帧间纹理更新 | 待实现 |
| **整数倍缩放** | GPU 纹理过滤模式 + 黑边/裁切逻辑 | ✅ 已实现 |
| **Sprite 图案叠加**（Pattern） | 着色器叠加混合（平铺/滚动/缩放），额外 GPU Pass | 待实现 |
| **Bitmap 模糊**（blur / radial_blur） | GPU 高斯/径向模糊着色器 | 待实现 |
| **Net 网络模块**（脚本层 HTTP） | 异步 I/O + 线程调度，Ruby 线程受 GVL 限制 | 待实现 |
| **滤镜控制**（smoothScaling / bitmapSmoothScaling） | GPU 采样器模式切换 | ✅ 部分已实现（smoothScaling / smoothScalingDown 分离配置） |
| **设置持久化**（Game.ini 读写 + Reset 按钮） | 全量 INI 写回，帧尾 auto-save + 析构 fallback | ✅ 已完成 |

### 引擎提供钩子，Ruby 扩展（引擎工作量小）

| 功能 | 引擎提供什么 | 状态 |
|------|-------------|:----:|
| **鼠标输入 API**（坐标/滚轮/按钮） | SDL 原始事件数据暴露为 Ruby 常量；Win32API 兼容层依赖 `Input.mouse_x/y` 实现 `GetCursorPos` | 待实现 |
| **剪贴板访问** | SDL 剪贴板 API 绑定 | 待实现 |
| **CFG 模块**（运行时配置读写） | 用户目录路径 + JSON 文件 I/O 暴露 | 待实现 |
| **System 平台检测** | SDL/预编译宏的字符串常量暴露；Win32API 兼容层依赖 `System.is_windows?`/`is_wine?` 决定是否走原生 Win32API | 待实现 |
| **System.mount / unmount**（mod 支持） | PhysFS 挂载点动态增删 API | 待实现 |
| **Graphics.screenshot** | `snap_to_bitmap` → `to_file` PNG 导出链 | 待实现 |
| **Graphics API 扩展**（resize / center / scale / show_cursor / fullscreen） | SDL 窗口操作 + GPU 缩放参数绑定；Win32API 兼容层依赖 `Graphics.show_cursor`/`fullscreen` 实现 `ShowCursor`/Alt+Enter 切换 | 待实现 |
| **扩展键盘输入**（扫描码 / 文本输入） | SDL 扫描码常量 + IME 缓冲区暴露；Win32API 兼容层依赖 `Input.raw_key_states` 实现 `GetKeyboardState`/`GetAsyncKeyState` | 待实现 |
| **Input::Controller 子模块** | SDL 手柄原始数据数组暴露（connected? / name / axes / raw_states / power_level / pressex? 等） | ✅ 已实现 |
| **patches 配置**（mod 注入） | PhysFS 挂载优先级在资源加载前插入 | 待实现 |
| **Console 控制台 + Ruby 模块** | ImGui REPL + `Console.write`/`puts`/`clear` | ✅ 已实现 |
| **System.launch** | `SDL_OpenURL` 绑定 | 待实现 |

### 🔧 脚本层（Ruby 直接能做，非引擎开发目标）

| 功能 | 为什么引擎不用做 |
|------|-----------------|
| 键位动作名称可配置 | 字符串映射表，Ruby Hash 就够了 |
| System.parse_csv | CSV 解析 Ruby 标准库就有 |
| MiniFFI / Win32API 兼容 | 纯桩（`class Win32API; def call(*); 0; end`）三行 Ruby 即可防御 NameError。完整的跨平台实现（含 User32/GetKeyState/KGL2 支持）参考 mkxp-z `win32_wrap.rb` + `kgl2_wrap.rb`，但依赖引擎提供 `Input.raw_key_states`/`Input.mouse_x`/`Graphics.show_cursor` 等钩子（见「引擎提供钩子」节），需先实现这些底层 API |
| 原版 RGSS 加密解密 | 算法公开，Ruby 实现解包器 |
| preload/postload 脚本 | 本身就是 Ruby 脚本加载顺序概念 |
| Theora 视频回放 | 小众需求，FFI 绑 libtheora 即可 |
| System 杂项（nproc / memory / power_state） | 非核心，引擎需提供 Ruby 常量但优先级极低 |
| fontScale 配置 | 引擎提供字体缩放 API，Ruby 调就行 |
| 鼠标光标定制 | SDL 光标 API，极少数游戏需要 |
| SE/BGM 音源数配置 | 引擎配置项，无技术难点 |
| JIT 选项 | MRI 启动参数，引擎透传即可 |
| 改进加密方案 | 算法内容不公开，非引擎基础设施 |

### ✅ 已实现

| 功能 | 说明 |
|------|------|
| **帧率解锁 / FreeSync / VSync 可配置** | INI + ImGui UI + `Graphics.frame_rate`/`Graphics.vsync` Ruby API 全覆盖 |
| **窗口可调整大小 / 固定宽高比** | `WinResizable` + `FixedAspectRatio` + `KeepRatio` |
| **单手柄支持** | 同 mkxp-z，仅第一个手柄，无合并逻辑 |
| **手柄震动反馈（C++ 层）** | `RumbleGamepad(low, high, ms)` 调用 `SDL_RumbleGamepad()`，待绑 Ruby |
| **手柄映射配置 + 持久化** | ImGui 设置页表格 + PopupModal 捕获弹窗，独立 `_gp.txt` 纯文本持久化，右键清除，自动跳转 |
| **REPL 控制台 + Console 模块** | ImGui 控制台（`~` 开关），多行 Ruby 代码执行、历史记录、`Console.write`/`puts`/`clear` |
| **画面比例保持可切换** | `KeepRatio=true/false`，INI + Ruby + ImGui 全覆盖，`false` 时拉伸填满无黑边 |
| **整数倍缩放** | `IntegerScaling=true/false`，`Graphics.integer_scaling=`，ImGui 复选框 |
| **分离上下采样滤镜** | `SmoothScaling=N` / `SmoothScalingDown=N`，`Graphics.smooth_scaling=` / `smooth_scaling_down=` |
| **resize_screen 同步重建缓冲** | 对齐 mkxp-z，立即重建帧缓冲 + 更新 aspect ratio 锁，消除过渡残影 |
| **按键绑定右键清除** | ImGui 设置页右键点击 Selectable 清除整行绑定 |
| **绑定表清理** | 移除多余的第 4 列 |
| **`CheckResizeInternal` 原子化** | 统一检测窗口尺寸 + 更新 `backingScaleFactor` + 缩交换链 |
| **Input::Controller 子模块** | 完整暴露手柄原始数据（connected? / name / axes / raw_states / power_level / pressex? 等） |
| **SFNT UCS-4 (UTF-32BE) 名称解码** | 处理 `TT_MS_ID_UCS_4` 编码族名，UTF-16BE 添加 surrogate pair 支持 |
| **VDMX / OS2 ppem 精确计算** | Wine 移植算法，`MAX_PPEM` 上限检查，缓存 `FT_Library` |
| **内嵌字体兜底** | 字体文件全部缺失时自动回退内嵌字体，不抛异常 |
| **ImGui 字体 HiDPI 缩放** | `FontGlobalScale` 跟随 backing scale，基础字体 16→18px |
| **Lanczos3 / Bicubic 缩放** | 6-tap Lanczos + Mitchell-Netravali Bicubic，全屏 Quad 后处理 |
| **Anime4K DoG 单 Pass 内联** | 7×7 高斯 + min/max + DoG clamp，49 纹理采样零额外 Pass |
| **CAS 锐化** | AMD FidelityFX CAS 5-tap 十字形，sharpness 滑块 0~1 |
| **Sobel 自适应混合 + 细调** | Anime4K Enhance PS 内联 Sobel 3×3，Warp/Dark 三滑块 |
| **缩放算法 Ruby API** | `Graphics.scaling_mode` auto-gen 绑定（0~5） |
| **设置持久化 + UI 统一** | 全量 INI 写回 SaveConfigure()，帧尾 auto-save + 析构 fallback，三区 Reset + 全局 Reset All |
