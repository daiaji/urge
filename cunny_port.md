# CuNNy-4x16-NVL / CuNNy-4x24-NVL 移植记录

## 状态：Compute 路径完成；sampler signature 与 FP16 条件化已补，待真实 Vulkan 环境重测（2026-07-05）

## 目标

将 Magpie 中的 `CuNNy-4x16-NVL` 和 `CuNNy-4x24-NVL` (CNN 2x 超分) 移植到 URGE 引擎。

源文件：
- `/home/daiaji/repo/Magpie/src/Effects/CuNNy2/CuNNy-4x16-NVL.hlsl` (1062 行)
- `/home/daiaji/repo/Magpie/src/Effects/CuNNy2/CuNNy-4x24-NVL.hlsl` (1940 行)

## 管线

两个变体结构完全相同，仅中间通道数不同：

```
Game (640×480)
  → Pass1: in (3×N_chan)       (640×480, INPUT SRV → bank A UAVs)
  → Pass2: conv1 (N×N)          (640×480, bank A SRVs → bank B UAVs)
  → Pass3: conv2 (N×N)          (640×480, bank B SRVs → bank A UAVs)
  → Pass4: conv3 (N×N)          (640×480, bank A SRVs → bank B UAVs)
  → Pass5: conv4 (N×N)          (640×480, bank B SRVs → bank A UAVs)
  → Pass6: out-shuffle (N×12)   (1280×960, INPUT + bank A SRVs → enhanced_tex UAV)
  → Present (1280×960 → window, mode-dependent)
```

| 变体 | N_chan | UAV 输出数 | 中间纹理 | 中间格式 |
|------|--------|------------|---------|---------|
| CuNNy-4x16 | 16 | 4 | 8 个 (bank A: tex[0..3], bank B: tex[4..7]) | RGBA8_UNORM |
| CuNNy-4x24 | 24 | 6 | 12 个 (bank A: tex[0..5], bank B: tex[6..11]) | RGBA8_UNORM |

## 关键文件

| 文件 | 变更 |
|------|------|
| `renderer/pipeline/builtin_hlsl_cunny_4x16.cc` | 6 个 HLSL 字符串（独立编译单元） |
| `renderer/pipeline/builtin_hlsl_cunny_4x24.cc` | 6 个 HLSL 字符串 |
| `renderer/pipeline/builtin_hlsl.h` | 12 个 extern 声明 |
| `renderer/pipeline/render_pipeline.cc/h` | `BuildComputePipeline()` / `SetupComputePipelineBasis()`；`MAKE_CUNNY_COMPUTE` 宏生成 12 个 compute PSO；CuNNy sampler signature、FP16/mad shader 生成 |
| `renderer/pipeline/render_binding.cc/h` | `Binding_CuNNy_Compute`（统一 compute binding: u_texture, u_textures[12], u_output, u_outputs[6], u_params） |
| `renderer/device/render_device.cc` | 请求可选 `ShaderFloat16` 设备特性，供 CuNNy FP16 条件启用 |
| `renderer/CMakeLists.txt` | 添加两个新 .cc 文件 |
| `content/render/pipeline_collection.cc/h` | CuNNy PSO 改用 `BuildComputePipeline()` |
| `content/screen/renderscreen_impl.h` | `cunny_4x16_bindings[6]`, `cunny_4x24_bindings[6]` |
| `content/screen/renderscreen_impl.cc` | `GPURunCuNNyPassesInternal` 改用 `DispatchCompute` + SRV/UAV 绑定 |
| `content/profile/content_profile.h` | mode 7 = "CuNNy-4x16-NVL", mode 8 = "CuNNy-4x24-NVL" |

## Shader 转换要点

### Magpie compute → URGE compute shader

不再走 pixel-shader MRT，直接在 C++ 层做字符串转换生成 `[numthreads(64,1,1)]` compute shader：

| Magpie (compute) | URGE (compute) |
|---|---|
| `void PassN(uint2 blockStart, uint3 tid)` | `void CSMain(uint3 tid : SV_GroupThreadID, uint3 gid : SV_GroupID)` |
| `uint2 gxy = Rmp8x8(tid.x) + blockStart` | `Rmp8x8` 内联实现，pass 1-5 用 8x8；pass 6 按 2x 像素输出 |
| `T0[gxy] = r0` | `u_Output0[gxy] = r0` |
| `OUTPUT[gxy + int2(0,0)] = ...` | 当前实现为 `u_Output[pixel] = ...`，每个输出像素一个 thread |
| `//!USE MulAdd` | 生成阶段替换为 Magpie 同款 `mad` 版 `MulAdd` |
| `//!CAPABILITY FP16` → `min16float` | 仅在非 GL/GLES 且设备 `ShaderFloat16` enabled 时将 `MF*` 替换为 `min16float*`；否则保留 `float*` |

### Final pass (out-shuffle)

当前 URGE 实现按最终 2x 输出尺寸 dispatch，每个 thread 写一个输出像素：

```hlsl
[numthreads(64, 1, 1)]
void CSMain(uint3 tid : SV_GroupThreadID, uint3 gid : SV_GroupID) {
  uint2 pixel = Rmp8x8(tid.x) + (gid.xy << 3u);
  if (pixel.x >= uint(u_OutputSize.x) || pixel.y >= uint(u_OutputSize.y)) return;
  uint sub_idx = (pixel.x & 1) + ((pixel.y & 1) << 1);
  // ... 计算 r0/r1/r2 ...
  float3 cnn = sub_idx 选择 r0/r1/r2 对应通道;
  float3 residual = u_Texture.SampleLevel(u_Texture_sampler, (float2(pixel) + 0.5) * u_OutputPt, 0).rgb;
  u_Output[pixel] = float4(saturate(residual + cnn), 1.0);
}
```

### 宏定义保留

每 pass 保留 `#define L0/L1/...` 采样宏和 `#define V3/V4/M3x4/M4` 类型别名。

### MulAdd 宏

所有矩阵使用 Magpie 原始值；生成 compute shader 时把 `MulAdd` 函数替换成 Magpie 的 `mad()` 分量展开版本。

## Runtime 结构

```
GPURunCuNNyPassesInternal(ctx, variant) {
  // 1. 检查 6 个 compute PSO + binding + screen buffer
  // 2. 创建/复用中间纹理 (8 或 12 个 RGBA8_UNORM, 带 BIND_UNORDERED_ACCESS)
  // 3. Pass1: screen → bank A (SRV + UAV compute dispatch)
  // 4. Pass2-5: ping-pong bank A ↔ bank B (SRV input bank → UAV output bank)
  // 5. Pass6: screen + bank → enhanced_tex (UAV 输出 2x)
  // 6. Return: enhanced_tex 被 GPUScalingPassInternal 复用走通用缩放
}
```

### Dispatch 方式

- Pass1-5: `DispatchCompute(ceil(w/8), ceil(h/8), 1)`，每个 thread 处理一个像素
- Pass6: `DispatchCompute(ceil(w*2/8), ceil(h*2/8), 1)`，当前每个 thread 写一个最终输出像素

Magpie 原始 Pass6 是按输入尺寸 dispatch，每个 thread 通过 `OUTPUT[gxy + int2(0/1, 0/1)]` 写出 4 个 2x 子像素。URGE 当前实现把这 4 个子像素拆成 4 个独立 thread，通过 `pixel & 1` 选择通道。数学结果应可等价，但执行拓扑不完全一致；如果视觉仍不一致，优先回到 Magpie 的四像素写出拓扑。

### Binding 统一

所有 CuNNy pass 共用 `Binding_CuNNy_Compute`：
- `u_texture` — 画面输入（Pass1/6）
- `u_textures[12]` — 特征纹理输入（Pass2-6）
- `u_output` — 单张 UAV 输出（Pass6）
- `u_outputs[6]` — 多张 UAV 输出（Pass1-5）
- `u_params` — `ScalingParamsBuffer`

## 技术难点

### 1. 文件过大导致链接失败

CuNNy HLSL 约 500KB（4x16: 170KB, 4x24: 343KB）。直接嵌入 `builtin_hlsl.cc` 使文件膨胀到 630KB，导致 GCC 重定位溢出。

**解决**：拆分为两个独立 .cc 文件 (`builtin_hlsl_cunny_4x16.cc`, `builtin_hlsl_cunny_4x24.cc`)，每个文件约 170KB/344KB，编译器可正常处理。

### 2. 自动转换脚本 / 运行时生成

手动搬运数千行权重代码不可行。先用 Python 转换脚本生成 URGE 内嵌 HLSL，再在 `MakeCuNNyComputeShader()` 运行时做最终 compute 包装：
- 解析 Magpie pass 结构 (//!PASS, //!IN, //!OUT)
- 提取 per-pass `#define` 宏
- 替换纹理引用、采样调用、输出写入
- 生成带 PSMain 包装的中间 HLSL 字符串
- C++ 生成器再替换成 `CSMain`、UAV 输出、条件 FP16 `MF*` 宏和 `mad` 版 `MulAdd`

### 3. 中间纹理格式

Magpie 声明中间纹理为 `R8G8B8A8_UNORM`（对应 CuNNy 的训练格式）。URGE 使用 `TEX_FORMAT_RGBA8_UNORM`。Pass6 最终输出也是 RGBA8 + `saturate` 限幅。

## Binding 定义

```cpp
// 统一 compute binding（Pass1-6 通用）
class Binding_CuNNy_Compute {
    u_texture;                           // 画面 SRV（Pass1/6）
    u_textures[12];                      // 特征纹理 SRVs
    u_output;                            // 单 UAV 输出（Pass6）
    u_outputs[6];                        // 多 UAV 输出（Pass1-5）
    u_params;                            // ScalingParamsBuffer
};
```

## 与 Magpie 的一致性

| 项目 | Magpie | URGE |
|------|--------|------|
| 权重 | 原始 MulAdd 矩阵 | 完全相同 |
| 精度 | `//!CAPABILITY FP16` → `min16float` + `mad` | 非 GL/GLES 且 `ShaderFloat16` enabled 时启用 `min16float`；否则保留 `float` |
| 执行模型 | `cs_5_0` compute shader | `Vulkan/HLSL` compute shader |
| 采样器 | 全局 `SP`(point) / `SL`(linear) | immutable sampler 等效 |
| CReLU | `max(v,0)` + `max(-v,0)` | 完全相同 |
| 残差连接 | `INPUT.SampleLevel(SL, ...)` | `u_Texture.SampleLevel(..., 0)` |
| 输出限幅 | `saturate(cnn + residual)` | 完全相同 |
| 中间格式 | `R8G8B8A8_UNORM` | `TEX_FORMAT_RGBA8_UNORM`（等效） |
| 常量布局 | `cbuffer __CB1` 10 字段 | `ScalingParamsBuffer` 布局不同 |
| Dispatch | `Rmp8x8` 8x8 block；Pass6 每 thread 写 4 个子像素 | Pass1-5 相同；Pass6 当前每 thread 写 1 个最终像素 |
| MulAdd | `mad` 逐分量展开（`//!USE MulAdd`） | 已按 Magpie `EffectCompiler.cpp` 逻辑移植 |

## 运行验证记录

- `cmake --build build -j$(nproc)`：通过。
- `bash ./build_and_deploy.sh`：通过，已复制到 `/home/daiaji/下载/レトリアの大冒険_URGE/Game`。
- 部署目录 `Game.ini` 当前为 `ScalingMode=7`，覆盖 CuNNy-4x16-NVL 路径。
- 旧运行日志暴露过 `CuNNy_4x16_Pass3/Pass5`、`CuNNy_4x24_Pass3/Pass5` PSO 创建失败：`u_Texture4_sampler` / `u_Texture6_sampler` 未出现在 pipeline resource signature 中。
- 已修复 sampler signature：CuNNy compute signature 现在同时注册纹理名和显式 `u_TextureN_sampler` / `u_Texture_sampler` immutable sampler。
- 工具会话直接运行无 X11；`xvfb-run` 可启动但 Vulkan present 因 DRI3 缺失失败并 fallback 到 OpenGL/llvmpipe，所以不能代表真实 Vulkan 运行结果。
- OpenGL fallback 当前已知仍不能编译 CuNNy compute：`RWTexture2D` 缺 image format qualifier，且 `mad` 版矩阵分量访问无法被 HLSL2GLSL 正确转换。按当前任务先不处理 OpenGL，只记录为已知限制。

## 当前剩余工作

- 在真实显示环境下重跑 Vulkan，确认 sampler signature 修复后 12 个 CuNNy compute PSO 都能创建。
- 分别验证 `ScalingMode=7` 和 `ScalingMode=8` 的运行时 shader 编译和画面输出。
- 抓 Magpie/URGE 截图对比；如果视觉仍不一致，优先检查 Pass6 dispatch 拓扑，再看常量布局。
- OpenGL fallback 暂不作为验收目标，只保留已知问题记录。
