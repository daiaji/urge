# Anime4K Mode A 集成

## 目标

为レトリアの大冒険（RMXP, 640×480 原生）在 URGE 中实现 Anime4K Mode A 多 Pass 放大链，将渲染分辨率提升至 **1280×960（2x 整数倍）**，并结合 Clamp_Highlights + Restore_CNN 后处理提升线条质量。

## 总体思路

```
Game Render (640×480)
  → Clamp_Highlights (de-ring, 5x5 邻域 luma 钳位)
  → Restore_CNN (Conv-4x3x3x3 → Conv-4x3x3x8 → Conv-4x3x3x8 → Conv-4x3x3x4, ReLU)
  → Upscale_CNN_x2 (640×480 → 1280×960)
  → Anime4K_Enhance(DoG) 现有 Pass (线条加深)
  → CAS 锐化 (可选)
  → 最终输出
```

不取 AutoDownscalePre_x2/_x4 和第二次 Upscale_CNN_x2（那是为 4x 链设计的）。

## 阶段 1：预处理工具链

### ShaderTranspiler

位置：`/home/daiaji/repo/ShaderTranspiler`
状态：已编译，可生成 HLSL
API：`ShaderTranspiler::CompileTo(FileCompileTask, TargetAPI::HLSL, Options)`

**已验证**：标准 GLSL 420 + `layout(binding=N)` 可以成功转换到 HLSL。

**已知限制**：
- 输入 GLSL 必须是标准 GLSL，不能有 mpv 宏（`MAIN_texOff`, `!HOOK` 等）
- 输出 HLSL 是 spirv-cross 风格（`Texture2D<float4> _13 register(t0, space0)`，全局变量 + main 包装器）
- 输出需要手动适配 URGE 的 HLSL 模式（`PSInput`/`PSOutput` 结构体 + `PSMain` 入口）

### mpv→GLSL 预处理器

位置：`/home/daiaji/repo/ShaderTranspiler/tools/mpv2glsl.py`
用途：将 Anime4K mpv 风格 GLSL（含 `!HOOK`、`!BIND`、`_texOff`、`_pos` 等宏）拆分为标准 GLSL pass，供 ShaderTranspiler 使用。

处理规则：
| mpv 宏 | 标准 GLSL 展开 |
|--------|---------------|
| `MAIN_texOff(vec2(x,y))` | `texture(tex_main, vTexCoord + vec2(x,y) * u_TexelSize)` |
| `HOOKED_tex(pos)` | `texture(tex_hooked, pos)` |
| `HOOKED_pos` | `vTexCoord` |
| `HOOKED_pt` | `u_TexelSize` |

### 转换流程

```bash
python3 tools/mpv2glsl.py /path/to/Anime4K/glsl/Restore/Anime4K_Clamp_Highlights.glsl /tmp/glsl/
build/convert /tmp/glsl/Anime4K_Clamp_Highlights_pass0.glsl /tmp/hlsl/pass0.hlsl fragment main
```

### CLI 转换工具

位置：`/home/daiaji/repo/ShaderTranspiler/tools/convert.cpp`
CMake target: `convert`
已加入 `CMakeLists.txt`。

## 阶段 2：Shader 转换

### Pass 1：Clamp_Highlights (de-ring)

**源文件**：`Anime4K/glsl/Restore/Anime4K_Clamp_Highlights.glsl`
**逻辑**：5x5 邻域水平→垂直 max luma 统计 → 当前像素 luma 钳位 ≤ 邻域 max
**输出**：4-component RGBA（修改后的像素）
**成本**：~0.02ms

mpv 里分 3 个子 pass（horiz stat → vert stat → clamp），可合并为 1 个 pass 直接采样 5x5 邻域。

### Pass 2：Restore_CNN (M 系)

**源文件**：`Anime4K/glsl/Restore/Anime4K_Restore_CNN_M.glsl`
**结构**：4 层 3×3 卷积，权重以 `mat4 * go_N(x_off, y_off)` 形式硬编码
```
Conv-4x3x3x3  (3→4 channels, 9 个 mat4)
Conv-4x3x3x8  (4→8 channels, 18 个 mat4, ReLU 正负分叉)
Conv-4x3x3x8  (8→8 channels, 18 个 mat4, ReLU 正负分叉)
Conv-4x3x3x4  (8→4 channels, 9 个 mat4)
```
**成本**：~0.1ms

**转化难点**：
- `mat4` 乘法链需要手动转换为 HLSL 的 `float4x4` 矩阵乘法（或直接展开为 dot）
- ReLU 通过 `max(x, 0)` / `max(-x, 0)` 正负分路实现
- 需要 `#define go_0` 宏 → 内联 texture sample 函数

### Pass 3：Upscale_CNN_x2 (S 系)

**源文件**：`Anime4K/glsl/Upscale/Anime4K_Upscale_CNN_x2_S.glsl`
**结构**：类似 Restore_CNN 的卷积层，但输出为 2x 分辨率
**输出分辨率**：640×480 → 1280×960
**成本**：~0.15ms

**与 Restore_CNN 区别**：需要处理像素偏移（upscale 输出的每个像素映射回输入的不同位置）。

## 阶段 3：URGE Pipeline 集成

### 现有 Shader 嵌入模式

位置：`/home/daiaji/repo/urge/renderer/pipeline/builtin_hlsl.cc`

每个 shader 以 `const std::string kHLSL_<Name>_<Stage> = R"(..."` 嵌入。

约定模式：
```hlsl
struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

cbuffer ScalingParamsBuffer {
  float2 u_InputSize;
  float2 u_OutputSize;
  float2 u_InputPt;
  float2 u_OutputPt;
  uint   u_Mode;
  float  u_ARStrength;
  float  u_BicubicB;
  float  u_BicubicC;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

void PSMain(in PSInput PSIn, out PSOutput PSOut) {
  // ...
}
```

多 Pass 时：第一个 Pass 读 `u_Texture`，后续 Pass 读前一个 Pass 的输出（也是 `Texture2D`），通过管线中的 `ITextureView` 绑定传递。

### Pipeline Pass 注册

位置：`/home/daiaji/repo/urge/renderer/pipeline/render_pipeline.cc`

每个 Pass 需要：
1. 创建 `PipelineState`（VS + PS shader）
2. 创建 Render Target（输出 texture）
3. 注册 `RenderPass` 到 pass 链

### 多 Pass 数据流

```
Pass 0: Clamp_Highlights
  Input:  game RT (640×480)
  Output: temp RT A (640×480)

Pass 1: Restore_CNN
  Input:  temp RT A (640×480)
  Output: temp RT B (640×480)

Pass 2: Upscale_CNN_x2
  Input:  temp RT B (640×480)
  Output: temp RT C (1280×960)

Pass 3: Anime4K_Enhance(DoG) [已有]
  Input:  temp RT C (1280×960)
  Output: temp RT D (1280×960)

Pass 4: Final blit + CAS
  Output: swapchain (1280×960)
```

### UI/Config

在 `urge.json` 中添加：
```json
{
  "anime4k_mode_a": {
    "enabled": false,
    "clamp_highlights": true,
    "restore_cnn": true,
    "upscale_cnn_x2": true
  }
}
```

每个 Pass 可单独开关以调试性能/画质平衡。

## 已踩过的坑（HLSL → GLSL 兼容性）

以下坑来自 URGE 历史提交，Diligent 使用 HLSL2GLSL 转换器将 HLSL 编译为 GLSL SPIR-V（OpenGL 后端），部分 HLSL 特性会踩雷。

### 1. `rcp()` 不是 GLSL 函数

```
// ❌ HLSL only
float inv = rcp(x);
// ✅ 通用写法
float inv = 1.0 / x;
```

Diligent 的 HLSL→GLSL 转换器不会翻译 `rcp()`（无对应 GLSL intrinsic），会编译报错。所有 `rcp()` 必须替换为 `1.0 / x`。
提交：`3553e48`

### 2. `Gather()` 通道顺序：HLSL vs GLSL

```
// HLSL: .wxyz (Green 在 .w, Red 在 .x)
gather.r = sr.w;  // Red 在 GatherRed 的 .w
gather.g = sg.w;  // Green

// GLSL: .wzyx (Red 在 .w, Blue 在 .z, Green 在 .y, Alpha 在 .x)
// Diligent 转换后会得到 GLSL 的 wzyx 顺序
// 解决方案：定义 URGE_GLSL_GATHER 宏，调换赋值顺序
```

通过 `#ifdef URGE_GLSL_GATHER` 做两套展开逻辑。
提交：`792fbf5`

### 3. `float3x3` 列主序构造 Bug（Diligent 上游 #800）

Diligent 的 HLSL2GLSL 转换器在处理 `float3x3` 的 column-major 构造函数时存在 Bug：

```
// HLSL: float3x3 m = float3x3(a,b,c, d,e,f, g,h,i);
// 期望的 GLSL: mat3 m = mat3(a,b,c, d,e,f, g,h,i);
// 实际 GLSL: mat3 m = mat3(a,d,g, b,e,h, c,f,i);  // ❌ transposed!
```

**解决方案**：不使用 `float3x3` 构造函数，改为显式 dot 计算：

```hlsl
// ❌ float3x3 构造会被转置
float3 result = mul(m, col);

// ✅ 直接展开成 dot
float3 result = float3(
  dot(m._m00_m01_m02, col),
  dot(m._m10_m11_m12, col),
  dot(m._m20_m21_m22, col)
);
```

或者为 OpenGL 后端单独写一个 `SampleXxx_GL()` 版本。
提交：`792fbf5`

### 4. `min4`/`max4` 宏 vs 内联展开

GLSL 没有 `min4`/`max4`（HLSL intrinsic），Diligent 可能也不翻译。URGE 的方案是用宏：

```hlsl
#define min4(a,b,c,d) min(min(a,b), min(c,d))
#define max4(a,b,c,d) max(max(a,b), max(c,d))
```

**不要**自作主张展开宏为嵌套 `min()`/`max()`——展开后混乱且无实质性益处。
提交：`b768bba`

### 5. GLSL `texture()` 的 4 分量要求

在 Diligent HLSL→GLSL 转换路径中，始终输出 `float4` 并只取 `.rgb`，不要输出 `float3`。部分 GLSL 后端对 3 分量输出支持不完整，可能导致黑屏。

### 6. 顶点布局（Vertex Layout）不能省略

OpenGL 后端对 Vertex Shader 的 Input Layout 要求严格。如果 Pipeline 缺了 `Vertex::GetLayout()`，OpenGL 会输出全黑，而 Vulkan 正常。
提交：`792fbf5`

### 总结

| 坑 | 症状 | 修复 |
|----|------|------|
| `rcp()` | GLSL 编译失败 | `1.0/x` |
| `Gather` 顺序 | OpenGL 颜色通道错乱 | `#ifdef URGE_GLSL_GATHER` |
| `float3x3` 构造 | OpenGL 矩阵转置 | 显式 dot 展开 |
| `min4`/`max4` | GLSL 编译失败 | 宏定义 |
| 3 分量输出 | OpenGL 黑屏 | 用 `float4` 输出 |
| 缺 Vertex Layout | OpenGL 黑屏 | 添加布局 |

## 已知问题与风险

### ShaderTranspiler 输出质量

输出 HLSL 是 spirv-cross 生成的机械代码，变量名被 demangle（`_13`, `_17`, `_30`）。需要手动：
- 替换为有意义的变量名
- 改为 `PSInput`/`PSOutput` 结构体模式
- 调整 `register` 绑定（`register(t0, space0)` → Diligent 自动分配）
- 入口点从 `main()` 改为 `PSMain()`

### GLSL `mat4 * vec4` 语义

GLSL 的矩阵乘法是列主序（column-major），HLSL 默认是行主序（row-major）。转换时需要确认 ShaderTranspiler 是否处理了这一差异。如果不处理，需要手动转置权重矩阵。

**验证方法**：用 ShaderTranspiler 转换一个简单的 `mat4 * vec4` GLSL，比对其 HLSL 输出中的乘法顺序。

### 多层卷积性能

Restore_CNN 每层做 3×3 卷积（9 次 texture sample × channel count），总共约 54 次 texture sample + 54 次 `mat4 * vec4` 乘加。在 640×480 上约 0.1ms，可以接受。

Upscale_CNN_x2 输出 1280×960，像素数是 4 倍，成本 ~0.15ms。

总预计：~0.3ms（Clamp 0.02 + Restore 0.1 + Upscale 0.15 + DoG 0.08 = 0.35ms），在 16.6ms 帧预算内。

## 已完成的准备工作

1. ✅ ShaderTranspiler 编译通过，可 GLSL→HLSL 转换
2. ✅ `convert` CLI 工具已编写并测试（标准 GLSL 420 可转换）
3. ✅ `mpv2glsl.py` 预处理器框架已编写（处理 Anime4K mpv 宏）
4. ✅ 验证 URGE HLSL 嵌入模式（参考 `Anime4K_Enhance` DoG shader）
5. ✅ 已确认管线 pass 注册位置和 texture 绑定机制
6. ✅ ROADMAP.md 已记录 Mode A 集成计划

## 下一步（给下个会话的 LLM）

### P0 - 核心

1. **验证 `mat4` 转置问题**：写一个最小 GLSL 测试 `mat4 * vec4`，通过 ShaderTranspiler 转 HLSL，确认列主序/行主序处理是否正确。
2. **手动 Port Clamp_Highlights**：直接用 URGE 的 HLSL 模式写，不经过转换器（结构简单，20 行逻辑，手写更快）。
3. **Port Restore_CNN**：用 mpv2glsl.py + ShaderTranspiler 转换，然后适配 URGE 模式。注意权重矩阵的转置。
4. **Port Upscale_CNN_x2**：同上，但注意 upscale 的像素偏移逻辑。
5. **注册管线 Pass**：在 `render_pipeline.cc` 中创建 pass 链。

### P1 - 验证

6. **编译并运行**：验证每个 Pass 的输出是否正确（可以用 `Console.log` 输出调试信息，或者用单独的 RT 保存中间结果）。
7. **性能测试**：测量每个 Pass 的 GPU 时间，确认在 ~0.3ms 以内。
8. **画质对比**：截取 DoG 仅 / DoG+Clamp / DoG+Clamp+Restore 的对比图。

### P2 - 优化/清理

9. **URGE 配置**：添加 UI toggle（Console 命令或 ini 文件）。
10. **Pipeline State 复用**：目前每个 Pass 创建独立的 PipelineState，可以考虑 VS 共享。
11. **HLSL 代码清理**：将转换器输出的 `_13` 等变量重命名为有意义的名称；移除 conversion-wrapper 函数，入口改为直调 PSMain。
