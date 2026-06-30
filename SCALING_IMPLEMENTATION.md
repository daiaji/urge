# 超分辨率 / 缩放后处理管线 — 实施状态

> **核心原则：零 CNN 模型，纯数学算法，按需渐进实现。**

## 参考源码

| 项目 | 许可证 | 用途 | 文件 |
|------|:------:|------|------|
| [Anime4K](https://github.com/bloc97/Anime4K) (bloc97) | MIT | DoG 锐化算法来源 | `glsl/Upscale/Anime4K_Upscale_DoG_x2.glsl` |
| [Magpie](https://github.com/Blinue/Magpie) (Blinue) | GPL-3.0 | Lanczos3、Bicubic HLSL 移植来源 | `src/Effects/Lanczos.hlsl`、`src/Effects/Bicubic.hlsl` |
| [mkxp-z](https://github.com/mkxp-z/mkxp-z) | GPL-2.0 | RGSS 渲染管线架构参考 | `src/display/graphics.cpp`、`src/display/gl/shader.cpp` |

---

## 实施状态总览

| Phase | 内容 | 状态 | 备注 |
|:-----:|------|:----:|------|
| 0 | 后处理管线基础设施 | ✅ **完成** | 全屏 Quad 通路、upscale_buffer 生命周期 |
| 1.1 | Lanczos3 缩放 | ✅ **完成** | |
| 1.2 | Bicubic 缩放 | ✅ **完成** | |
| 1.3 | Anime4K DoG | ✅ **完成** | **单 Pass 内联版**（非 5 Pass 分离版） |
| 1.4 | CAS 锐化 | ✅ **完成** | AMD FidelityFX CAS 5-tap 十字形，绿色通道统一权重 |
| 2 | Sobel 自适应混合 | ✅ **完成** | Anime4K Enhance PS 内联 Sobel 3x3, lerp 按边缘强度混合 |
| 3 | ImGui UI + Ruby API | ✅ **完成** | ImGui 下拉已有；Ruby API `Graphics.scaling_mode` 整数属性 |

---

## 当前管线

```
Game → screen_buffer (640×480)
                    ↓
     [Scaling Pass: 全屏 Quad]
                    ↓
          upscale_buffer (窗口分辨率)
                    ↓
          ImGui::Image(upscale_buffer)
                    ↓
               Swapchain
```

`upscale_buffer` 在窗口 resize 时自动重建。Scaling Pass 通过 `scaling_mode` 切换算法（0=Bilinear, 1=Nearest, 2=Lanczos3, 3=Bicubic, 4=Anime4K）。

---

## 已完成变更

### Phase 0 — 后处理管线基础设施
- 全屏 Quad VS（SV_VertexID，无顶点缓冲）
- upscale_buffer 创建与重建（跟随 swapchain resize）
- `GPUScalingPassInternal` 缩放 Pass
- `FrameProcessInternal` 中自动插入缩放 Pass
- scaling_mode INI 配置解析
- ImGui 设置页缩放算法下拉框

### Phase 1.1 — Lanczos3
- 6-tap Lanczos 核 + Gather 优化（来自 Magpie）
- 抗振铃 clamp (`AntiRingStrength` 参数)

### Phase 1.2 — Bicubic
- Mitchell-Netravali 核（`B`/`C` 参数，默认 1/3, 1/3）

### Phase 1.3 — Anime4K DoG（单 Pass 内联版）
- 7×7 内联高斯模糊 + 全邻域 min/max 跟踪
- DoG 增强 + min/max clamp
- 收尾用 Lanczos3 缩放到窗口分辨率

---

## 未完成

### Phase 1.4 — CAS 锐化
已实现（2025-06-30）：独立 Pass 后处理，5-tap 十字形 AMD FidelityFX CAS。
- 5 次 Sample 采样（上/左/中/右/下），绿色通道统一权重
- `sharpness` 参数 0~1 可调（默认 0.4），通过 INI `[Renderer] CASEnabled/CASSharpness` 配置
- ImGui 界面含 CAS 开关和锐度滑块
- 流程：`Upscale Pass → upscale_buffer → [CAS Pass] → sharpened_buffer`

### Phase 2 — Sobel 自适应混合
已实现（2025-06-30）：在 Anime4K Enhance PS 中内联 Sobel 3×3 边缘检测。
- 复用 7×7 循环的中间 3×3 luma 值，零额外纹理采样
- Sobel 梯度 → norm → `dval = pow(norm, strength)`
- `result = lerp(original, enhanced, dval)` — 仅边缘区域应用 DoG 锐化
- 新 mode=5（ImGui 显示 "Anime4K+Sobel"），Sobel 强度滑块（0~2）
- 配置 INI: `[Renderer] ScalingSobelStrength=1.0`

### Phase 3 — Ruby API
已实现：auto-gen 绑定生成器已导出 `scaling_mode` 整数属性。
- `Graphics.scaling_mode` → 返回当前模式整数 (0-5)
- `Graphics.scaling_mode = n` → 设置模式
- 绑定代码在 `autogen_graphics_binding.cc`

---

## 颜色出错的根本原因

### 错误 1：Gaussian 权重数组顺序反了（单 Pass 简化版）

```hlsl
// ❌ 错误
static const float kWeight[4] = { 0.06136, 0.24477, 0.38774, 0.0 };
//    kWeight[0] = 0.06136 (最小权重) → abs(0) = 0 → 中心像素被赋予最小权重
//    kWeight[2] = 0.38774 (最大权重) → abs(2) = 2 → 距离 2 的像素被赋予最大权重

// ✅ 正确
static const float kWeight[4] = { 0.38774, 0.24477, 0.06136, 0.0 };
//    kWeight[0] = 0.38774 (最大权重) → 中心像素正确
//    kWeight[2] = 0.06136 (最小权重) → 距离 2 的像素正确
```

`kWeight[abs(i)]` 用 `abs(i)` 做索引，所以 `kWeight[0]` **必须**是中心权重（最大），而不是边缘权重。

### 错误 2：gauss_tex 用 2 通道格式丢弃了 mx（多 Pass 版）

GaussX/GaussY 输出 `float4(gauss, min, max, 0)`，共 3 个有效值。但用了 `TEX_FORMAT_RG16_UNORM`（2 通道），`.z`（mx）被丢弃。导致 Enhance Pass 读取 `mx = 0`，`clamp(c + luma, min, 0)` 永远把正数 clamp 到 ≤0，产生负的 `cc`，颜色错乱。

**修复**：用 `TEX_FORMAT_RGBA16_UNORM`（4 通道）存储全部 3 个值。

### 错误 3：绑定跨管线使用导致崩溃（多 Pass 版）

`gpu_.upscale_binding` 从 `upscale` 管线的 `IPipelineResourceSignature` 创建，但被用在 `anime4k_enhance` 管线上。Diligent 中每个管线有独立的资源签名对象，交叉使用导致描述符不匹配 → `CommitShaderResources` 崩溃。

**修复**：每个管线用各自的 `CreateBinding()` 创建独立绑定。

### 结论

由于多 Pass 分离版引入了中间纹理、格式选择、资源签名匹配等额外复杂度，最终采用 **单 Pass 内联版**（在同一个 PS 中完成 7×7 高斯 + min/max + DoG），避免了所有多 Pass 相关的状态管理问题。性能影响可忽略（49 次纹理采样 vs 多 Pass 的 ~45 次，差距 < 0.02ms）。

### 正确的多 Pass 实施策略

之前一次性实现所有 4 个 Pass 导致难以定位问题。正确的推进方式是**逐个 Pass 实现、逐个验证**：

```
Step 1: Luma  → lum_tex → upscale 输出灰度图  ✓ 验证灰度正确
Step 2: +GaussX → gauss_tex → upscale 输出模糊图  ✓ 验证模糊正确
Step 3: +GaussY → gauss_tex(2D) → upscale 输出2D模糊  ✓ 验证2D模糊正确
Step 4: +Enhance → screen+gauss_tex → enhanced_tex → upscale 完整DoG
```

每步的 GPUScalingPassInternal 只执行到当前 Pass，然后把中间结果 upscale 到窗口显示。这样一旦颜色不对，立刻知道是哪个 Pass 的问题。

### 调试纹理预览机制

在 ImGui 设置页添加「调试纹理预览」下拉框，运行时实时切换显示任意中间 Pass 的输出：

| 选项 | 显示内容 | 验证目标 |
|------|---------|---------|
| `:final` | upscale_buffer（最终输出） | 正常渲染 |
| `:debug_luma` | lum_tex → upscale | Luma 提取是否正确（应为灰度图） |
| `:debug_gaussx` | gauss_tex.R → upscale | 水平高斯模糊是否正确 |
| `:debug_gaussy` | gauss_tex.R(2D) → upscale | 垂直高斯传播后是否正确 |
| `:debug_enhanced` | enhanced_tex → upscale | DoG 增强效果 |

实现方式：在 `content_profile` 添加 `debug_preview` 字段，`GPUScalingPassInternal` 根据该字段决定：
- `:final`（默认）— 走完整 Anime4K 管线或普通缩放
- `:debug_luma` — 只跑 Luma Pass，将 lum_tex 经 Lanczos3 放大到 upscale_buffer
- 以此类推

这样无需修改代码、无需重新编译，运行时就能逐个 Pass 验收。

---

## 验证标准

| 检查项 | 方法 | 状态 |
|--------|------|:----:|
| 各算法切换画面正确 | ImGui 下拉切换，目视检查边缘和色块 | ✅ |
| 窗口 resize 后 upscale_buffer 重建 | 拖拽窗口边缘，画面正常适应 | ✅ |
| 性能 | Lanczos ~0.03ms, Anime4K ~0.08ms → 可忽略 | ✅ |
| 不降级时退化为 Nearest | `scaling_mode=1` 时行为不变 | ✅ |
| CAS 锐化开关+滑块 | ImGui 勾选/拖动，画面锐度变化 | ✅ |
| CAS 颜色无偏移 | RGB 同用 wG 权重，saturate 钳位 | ✅ |
| Sobel 自适应混合边缘检测 | mode=5, 瓦片边缘 DoG 生效，平直区域无振铃 | ✅ |
| Ruby API `Graphics.scaling_mode` | `Graphics.scaling_mode = 5` → 模式切换 | ✅ |
