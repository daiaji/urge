# CuNNy-4x16-NVL / CuNNy-4x24-NVL 移植记录

## 状态：路线 C 完成；生成链路已恢复，Pass6 拓扑、sampler signature、2x 直出与 Auto-fit 交互已修复，待真实截图对比验证（2026-07-05）

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
  → Present (1280×960 → window；窗口正好 2x 且 CAS 关闭时直接 present enhanced_tex)
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
| `tools/generate_cunny_hlsl.py` | 从 Magpie 原始 CuNNy HLSL 重新生成两个内嵌 HLSL `.cc` 文件 |
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
| `OUTPUT[gxy + int2(0,0)] = ...` | `u_Output[gxy + int2(...)] = ...`，Pass6 每个 thread 写 2x2 四个输出像素 |
| `//!USE MulAdd` | 生成阶段替换为 Magpie 同款 `mad` 版 `MulAdd` |
| `//!CAPABILITY FP16` → `min16float` | 仅在非 GL/GLES 且设备 `ShaderFloat16` enabled 时将 `MF*` 替换为 `min16float*`；否则保留 `float*` |

### Final pass (out-shuffle)

URGE 现在保留 Magpie 的 out-shuffle 拓扑：Pass6 按输入尺寸 dispatch，每个 thread 计算一个 2x2 输出块并写出四个子像素。边缘位置额外加了单像素越界保护，以兼容非偶数输出尺寸。

```hlsl
[numthreads(64, 1, 1)]
void CSMain(uint3 tid : SV_GroupThreadID, uint3 gid : SV_GroupID) {
  uint2 gxy = (Rmp8x8(tid.x) << 1u) + (gid.xy << 4u);
  if (gxy.x >= uint(u_OutputSize.x) || gxy.y >= uint(u_OutputSize.y)) return;
  // ... 计算 r0/r1/r2 ...
  u_Output[gxy + int2(0, 0)] = float4(saturate(residual00 + float3(r0.x, r1.x, r2.x)), 1.0);
  u_Output[gxy + int2(1, 0)] = float4(saturate(residual10 + float3(r0.y, r1.y, r2.y)), 1.0);
  u_Output[gxy + int2(0, 1)] = float4(saturate(residual01 + float3(r0.z, r1.z, r2.z)), 1.0);
  u_Output[gxy + int2(1, 1)] = float4(saturate(residual11 + float3(r0.w, r1.w, r2.w)), 1.0);
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
  // 6. Return: enhanced_tex 被 GPUScalingPassInternal 复用；2x 窗口且 CAS 关闭时跳过 1:1 blit
}
```

### Dispatch 方式

- Pass1-5: `DispatchCompute(ceil(w/8), ceil(h/8), 1)`，每个 thread 处理一个像素
- Pass6: `DispatchCompute(ceil(w/8), ceil(h/8), 1)`，每个 thread 写一个 2x2 输出块

Magpie 原始 Pass6 是按输入尺寸 dispatch，每个 thread 通过 `OUTPUT[gxy + int2(0/1, 0/1)]` 写出 4 个 2x 子像素。URGE 已改回同类拓扑，避免一个 2x2 输出块重复计算 4 次。

### Binding 统一

所有 CuNNy pass 共用 `Binding_CuNNy_Compute`：
- `u_texture` — 画面输入（Pass1/6）
- `u_textures[12]` — 特征纹理输入（Pass2-6）
- `u_output` — 单张 UAV 输出（Pass6）
- `u_outputs[6]` — 多张 UAV 输出（Pass1-5）
- `u_params` — `ScalingParamsBuffer`

### 2x present 优化与 Auto-fit

- `ScalingMode=7/8` 现在和 UDL mode 6 一样支持 `Auto-fit Window`。
- 勾选时立即把窗口设置为 `context()->resolution * 2`，并用 min/max size 锁定；取消时恢复 `engine_profile->window_size` 与 `win_resizable`。
- 启动时不再在早期 `main()` 窗口创建阶段应用 auto-fit；渲染层只记录 pending，并在 `RenderScreenImpl` 首帧、`ResizeScreen()` 或 `MoveWindow()` 后用稳定后的分辨率重新应用，避免脚本初始化覆盖窗口尺寸导致偏移和黑边。
- 当 `enhanced_tex` 尺寸等于 swapchain 输出尺寸且 CAS 关闭时，`GPUScalingPassInternal()` 直接返回 `enhanced_tex`，跳过 `enhanced_tex -> upscale_buffer` 的 1:1 blit。

## 技术难点

### 1. 文件过大导致链接失败

CuNNy HLSL 约 500KB（4x16: 170KB, 4x24: 343KB）。直接嵌入 `builtin_hlsl.cc` 使文件膨胀到 630KB，导致 GCC 重定位溢出。

**解决**：拆分为两个独立 .cc 文件 (`builtin_hlsl_cunny_4x16.cc`, `builtin_hlsl_cunny_4x24.cc`)，每个文件约 170KB/344KB，编译器可正常处理。

### 2. 自动转换脚本 / compute 生成

手动搬运数千行权重代码不可行。`tools/generate_cunny_hlsl.py` 从 Magpie 原始 CuNNy HLSL 直接生成 URGE 内嵌 compute HLSL：
- 解析 Magpie pass 结构 (//!PASS, //!IN, //!OUT)
- 提取 per-pass `#define` 宏
- 替换纹理引用、采样调用、输出写入
- 保留 `void PassN(uint2 blockStart, uint3 tid)` 结构
- 生成 `CSMain`、UAV 输出、`Rmp8x8` helper 与 Magpie 源 SHA256 注释
- C++ 运行时仅保留条件 FP16 `MF*` 宏替换和 `mad` 版 `MulAdd` 替换

重放生成命令：

```bash
python3 -B tools/generate_cunny_hlsl.py
```

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

当前实现结论：`CuNNy-4x16-NVL` / `CuNNy-4x24-NVL` 在算法层面、Pass 函数结构和 Pass6 执行拓扑上已对齐 Magpie，包含权重、6-pass 结构、纹理数量、采样器意图、中间格式、ping-pong 依赖和 out-shuffle 四像素写出。URGE 仍不是直接逐字编译 Magpie 原始文件，而是由生成器生成内嵌 compute HLSL。

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
| Dispatch | `Rmp8x8` 8x8 block；Pass6 每 thread 写 4 个子像素 | Pass1-5 相同；Pass6 每 thread 写 4 个子像素 |
| MulAdd | `mad` 逐分量展开（`//!USE MulAdd`） | 已按 Magpie `EffectCompiler.cpp` 逻辑移植 |

### 已确认对齐点

- 源文件对应 Magpie `src/Effects/CuNNy2/CuNNy-4x16-NVL.hlsl` 与 `src/Effects/CuNNy2/CuNNy-4x24-NVL.hlsl`。
- URGE 内嵌实现位于 `renderer/pipeline/builtin_hlsl_cunny_4x16.cc` 与 `renderer/pipeline/builtin_hlsl_cunny_4x24.cc`。
- 两个变体均保留 6 个 pass：Pass1 输入卷积、Pass2-5 中间卷积、Pass6 out-shuffle。
- 4x16 使用 8 张中间纹理：bank A 为 `tex[0..3]`，bank B 为 `tex[4..7]`。
- 4x24 使用 12 张中间纹理：bank A 为 `tex[0..5]`，bank B 为 `tex[6..11]`。
- Pass2-5 的 bank A/B ping-pong 后，Pass6 读取最终 bank A，依赖关系与 Magpie pass 输出一致。
- Magpie 中间纹理声明为 `R8G8B8A8_UNORM`，URGE 使用 `TEX_FORMAT_RGBA8_UNORM`。
- Magpie 中间特征采样使用 point sampler，最终残差采样使用 linear sampler；URGE 的 immutable sampler 设置与该意图一致。
- Magpie `//!USE MulAdd` 的 `mad` 展开逻辑已在 `ApplyCuNNyMadMulAdd()` 中移植。
- `tools/generate_cunny_hlsl.py` 可从 Magpie 原始文件重放生成 4x16/4x24 内嵌 compute HLSL，并在生成文件头部记录源 SHA256。
- 生成结果保留 Magpie 的 `void PassN(uint2 blockStart, uint3 tid)` 结构；`CSMain` 只负责按 pass block size 传入 `blockStart`。
- Pass6 已恢复为 Magpie 同类四像素 out-shuffle 拓扑，并在运行时按 native 尺寸 dispatch。

### 未完全对齐点 / 风险

- URGE 内嵌 HLSL 仍由生成器改写纹理名、输出 UAV 和边界保护，不是逐字保留 Magpie 原始 `.hlsl`。
- FP16 与 Magpie 默认 D3D 路径的实际启用状态需要运行时确认。URGE 在 GL/GLES 或设备不支持 `ShaderFloat16` 时会降级为 `float`，可能导致与 Magpie 输出有轻微差异。

### 对齐判定

- 若验收标准是算法、权重、pass 依赖、资源布局和 Pass6 out-shuffle 拓扑：当前实现已对齐 Magpie。
- 若验收标准是逐字编译 Magpie 原始 HLSL：当前实现不满足，URGE 仍使用生成后的内嵌 compute HLSL。
- 若验收标准是逐像素输出一致：当前静态分析无法确认，必须抓 Magpie/URGE 同输入截图或离屏输出对比；若出现差异，优先检查 FP16 启用状态、后端 shader 编译和残差采样。

## 运行验证记录

- `cmake --build build -j$(nproc)`：通过。
- 2026-07-05 路线 C 改动后 `cmake --build build -j$(nproc)`：通过。
- 2026-07-05 生成器改为直接输出 compute HLSL 后 `cmake --build build -j$(nproc)`：通过。
- 2026-07-05 Auto-fit 延迟应用与 2x 直出改动后 `cmake --build build -j$(nproc)`：通过。
- 2026-07-05 `git diff --check`：通过。
- `bash ./build_and_deploy.sh`：通过，已复制到 `/home/daiaji/下载/レトリアの大冒険_URGE/Game`。
- 部署目录 `Game.ini` 当前为 `ScalingMode=7`，覆盖 CuNNy-4x16-NVL 路径。
- 旧运行日志暴露过 `CuNNy_4x16_Pass3/Pass5`、`CuNNy_4x24_Pass3/Pass5` PSO 创建失败：`u_Texture4_sampler` / `u_Texture6_sampler` 未出现在 pipeline resource signature 中。
- 已修复 sampler signature：CuNNy compute signature 现在只按纹理资源名注册 immutable sampler，不再额外注册不存在对应纹理资源的 `u_TextureN_sampler` / `u_Texture_sampler`。
- 工具会话直接运行无 X11；`xvfb-run` 可启动但 Vulkan present 因 DRI3 缺失失败并 fallback 到 OpenGL/llvmpipe，所以不能代表真实 Vulkan 运行结果。
- OpenGL fallback 当前已知仍不能编译 CuNNy compute：`RWTexture2D` 缺 image format qualifier，且 `mad` 版矩阵分量访问无法被 HLSL2GLSL 正确转换。按当前任务先不处理 OpenGL，只记录为已知限制。

## 当前剩余工作

- 抓 Magpie/URGE 同输入截图或离屏输出对比；如果视觉仍不一致，优先检查 FP16 启用状态、残差 linear sampler、后端 shader 编译和常量布局。
- 在真实显示环境下分别验证 `ScalingMode=7` / `ScalingMode=8`、`UDLAutoFit=true` 启动、窗口居中和无黑边表现。
- 按 `cunny_udl_pso_lazy_loading_plan.md` 实施 UDL/CuNNy 重型 shader/PSO 懒加载，降低不使用 mode 6/7/8 时的启动黑屏时间。
- OpenGL fallback 暂不作为验收目标，只保留已知问题记录。
