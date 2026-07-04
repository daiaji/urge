# CuNNy-4x16-NVL / CuNNy-4x24-NVL 移植记录

## 状态：已完成 ✓（运行时已就位，待实际渲染验证）

## 目标

将 Magpie 中的 `CuNNy-4x16-NVL` 和 `CuNNy-4x24-NVL` (CNN 2x 超分) 移植到 URGE 引擎。

源文件：
- `/home/daiaji/repo/Magpie/src/Effects/CuNNy2/CuNNy-4x16-NVL.hlsl` (1062 行)
- `/home/daiaji/repo/Magpie/src/Effects/CuNNy2/CuNNy-4x24-NVL.hlsl` (1940 行)

## 管线

两个变体结构完全相同，仅中间通道数不同：

```
Game (640×480)
  → Pass1: in (3×N_chan)       (640×480, INPUT → bank A, MRT)
  → Pass2: conv1 (N×N)          (640×480, bank A → bank B, MRT)
  → Pass3: conv2 (N×N)          (640×480, bank B → bank A, MRT)
  → Pass4: conv3 (N×N)          (640×480, bank A → bank B, MRT)
  → Pass5: conv4 (N×N)          (640×480, bank B → bank A, MRT)
  → Pass6: out-shuffle (N×12)   (1280×960, INPUT + bank A/B → enhanced_tex)
  → Present (1280×960 → window, mode-dependent)
```

| 变体 | N_chan | MRT 数 | 中间纹理 | 中间格式 |
|------|--------|--------|---------|---------|
| CuNNy-4x16 | 16 | 4 | 8 个 (bank A: tex[0..3], bank B: tex[4..7]) | RGBA8_UNORM |
| CuNNy-4x24 | 24 | 6 | 12 个 (bank A: tex[0..5], bank B: tex[6..11]) | RGBA8_UNORM |

## 关键文件

| 文件 | 变更 |
|------|------|
| `renderer/pipeline/builtin_hlsl_cunny_4x16.cc` | 6 个 HLSL 字符串（独立编译单元，避免单文件过大） |
| `renderer/pipeline/builtin_hlsl_cunny_4x24.cc` | 6 个 HLSL 字符串 |
| `renderer/pipeline/builtin_hlsl.h` | 12 个 extern 声明 |
| `renderer/pipeline/render_pipeline.cc/h` | 12 个 pipeline 定义（两个变体各 6 个 pass） |
| `renderer/pipeline/render_binding.cc/h` | `Binding_CuNNy_Conv4` (4 纹理), `Binding_CuNNy_Conv6` (6 纹理), `Binding_CuNNy_Out` (5/7 纹理) |
| `renderer/CMakeLists.txt` | 添加两个新 .cc 文件 |
| `content/render/pipeline_collection.cc/h` | 12 个 PSO，中间格式 RGBA8_UNORM |
| `content/screen/renderscreen_impl.h` | `cunny_tex` (动态数组), `cunny_pass1_binding`, `cunny_conv_binding`, `cunny_out_binding` |
| `content/screen/renderscreen_impl.cc` | `GPURecreateCuNNyTargetsInternal`, `GPURunCuNNyPassesInternal`, UI 下拉框 (mode 8/9) |
| `content/profile/content_profile.h` | mode 8 = "CuNNy-4x16-NVL", mode 9 = "CuNNy-4x24-NVL" |

## Shader 转换要点

### Magpie compute → URGE pixel shader

使用 Python 脚本批量转换。核心差异：

| Magpie (compute) | URGE (pixel) |
|---|---|
| `void PassN(uint2 blockStart, uint3 tid)` | `void PSMain(in PSInput PSIn, out PSOutput PSOut)` |
| `uint2 gxy = Rmp8x8(tid.x) + blockStart` | `uint2 gxy = uint2(PSIn.UV * u_InputSize)` |
| `INPUT.SampleLevel` | `u_Texture.SampleLevel` |
| `T0[gxy] = r0` | `PSOut.Color0 = r0` |
| `OUTPUT[gxy + int2(0,0)] = ...` | `sub_idx` 选择 + `PSOut.Color` |
| `O(T, x, y)` 宏 | `T.SampleLevel(T_sampler, pos + float2(x, y) * pt, 0)` |
| `SP` (point sampler) | 各自纹理的 immutable point sampler |
| `SL` (linear sampler) | `u_Texture_sampler` (final pass residual) |

### Final pass (out-shuffle)

和 UDL Pass3 相同的 depth-to-space 模式：
```hlsl
uint2 pixel = uint2(PSIn.UV * u_OutputSize);
uint sub_idx = (pixel.x & 1) + ((pixel.y & 1) << 1);
// ... 计算 r0/r1/r2 ...
float3 cnn = sub_idx 选择 r0/r1/r2 对应通道;
float3 residual = u_Texture.SampleLevel(u_Texture_sampler, PSIn.UV, 0).rgb;
PSOut.Color = float4(saturate(residual + cnn), 1.0);
```

### 宏定义保留

每 pass 有自己的 `#define L0/L1/...` 采样宏和 `#define V3/V4/M3x4/M4` 类型别名，转换脚本保留这些 pass 特定定义。

### MulAdd 宏

与 UDL 相同：`MulAdd(a, M, target)` = `mul(a, M) + target`。所有矩阵使用 Magpie 原始值。

## Runtime 结构

```
GPURunCuNNyPassesInternal(ctx, variant) {
  // 1. 检查 PSO + binding + screen buffer
  // 2. 创建/复用中间纹理 (8 或 12 个 RGBA8_UNORM)
  // 3. Pass1: screen → bank A (单纹理 binding)
  // 4. Pass2-5: ping-pong bank A ↔ bank B (多纹理 binding)
  // 5. Pass6: screen + 最终 ping-pong 结果 → enhanced_tex (out-shuffle)
  // 6. Return: enhanced_tex 被 GPUScalingPassInternal 复用走通用缩放
}
```

### Binding 动态绑定

Pass2-5 共享同一个 `Binding_CuNNy_Conv4/Conv6`，每 pass 通过 `GetVariableByName` 动态绑定对应 `u_Texture0..N` 到 bank 中的纹理。

Pass6 的 `Binding_CuNNy_Out` 中 `u_Texture` 用 linear sampler（对应 Magpie 的 `SL`），`u_Texture0..N` 用 point sampler（对应 Magpie 的 `SP`）。

## 技术难点

### 1. 文件过大导致链接失败

CuNNy HLSL 约 500KB（4x16: 170KB, 4x24: 343KB）。直接嵌入 `builtin_hlsl.cc` 使文件膨胀到 630KB，导致 GCC 重定位溢出。

**解决**：拆分为两个独立 .cc 文件 (`builtin_hlsl_cunny_4x16.cc`, `builtin_hlsl_cunny_4x24.cc`)，每个文件约 170KB/344KB，编译器可正常处理。

### 2. 自动转换脚本

手动搬运数千行权重代码不可行。编写 Python 转换脚本统一处理：
- 解析 Magpie pass 结构 (//!PASS, //!IN, //!OUT)
- 提取 per-pass `#define` 宏
- 替换纹理引用、采样调用、输出写入
- 生成带 PSMain 包装的 HLSL 字符串

### 3. 中间纹理格式

Magpie 声明中间纹理为 `R8G8B8A8_UNORM`（对应 CuNNy 的训练格式）。URGE 使用 `TEX_FORMAT_RGBA8_UNORM`。Pass6 最终输出也是 RGBA8 + `saturate` 限幅。

## Binding 定义

```cpp
// 4-texture 中间 pass
class Binding_CuNNy_Conv4 {
    u_texture0, u_texture1, u_texture2, u_texture3, u_params    // point samplers
};

// 6-texture 中间 pass (4x24)
class Binding_CuNNy_Conv6 {
    u_texture0~5, u_params                                       // point samplers
};

// Final out-shuffle pass
class Binding_CuNNy_Out {
    u_texture,  u_texture0~3, u_params                            // u_Texture = linear, 其余 point
};
```

## 与 Magpie 的一致性

| 项目 | Magpie | URGE |
|------|--------|------|
| 权重 | 原始 MulAdd 矩阵 | 完全相同 |
| 采样器 | SP=point, SL=linear | immutable samplers 等效 |
| CReLU | `max(v,0)` + `max(-v,0)` | 完全相同 |
| 残差连接 | `INPUT.SampleLevel(SL, ...)` | `u_Texture.SampleLevel(PSIn.UV, 0)` |
| 输出限幅 | `saturate(cnn + residual)` | 完全相同 |
