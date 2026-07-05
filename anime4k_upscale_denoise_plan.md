# Anime4K_Upscale_Denoise_L 移植记录

## 状态：生成式 compute 路线完成；启动期 Auto-fit 已延迟到渲染分辨率稳定后应用（2026-07-05）

## 目标

将 Magpie 中的 `Anime4K_Upscale_Denoise_L` (CNN 2x 超分 + 降噪) 移植到 URGE 引擎。

源文件：`/home/daiaji/repo/Magpie/src/Effects/Anime4K/Anime4K_Upscale_Denoise_L.hlsl`

## 管线

```
Game (640×480)
  → Pass1: Conv-4x3x3x3      (640×480, INPUT SRV → tex1, tex2 UAV)
  → Pass2: Conv-4x3x3x16     (640×480, tex1+tex2 SRV → tex3, tex4 UAV)
  → Pass3: Conv-4x3x3x16     (640×480, tex3+tex4 SRV → tex1, tex2 UAV)
  → Pass4: Conv-4x3x3x16+D2S (1280×960, INPUT+tex1+tex2 SRV → enhanced_tex UAV)
  → Present (1280×960 → window, mode-dependent)
```

## 关键文件

| 文件 | 变更 |
|------|------|
| `tools/generate_cunny_hlsl.py` | 从 Magpie 原始 `Anime4K_Upscale_Denoise_L.hlsl` 生成 UDL compute HLSL |
| `renderer/pipeline/builtin_hlsl_anime4k_udl.cc` | 4 个生成式 compute HLSL 字符串，文件头记录 Magpie 源 SHA256 |
| `renderer/pipeline/builtin_hlsl.cc` | 旧手写 UDL pixel shader 暂保留，用于差异对照，不再作为 mode 6 路径 |
| `renderer/pipeline/builtin_hlsl.h` | 旧 `*_Pixel` 声明 + 新 `kHLSL_Anime4K_UDL_Pass0~3_Compute` 声明 |
| `renderer/pipeline/render_pipeline.cc/h` | UDL Pass0~3 改为 compute pipeline，复用 `Binding_CuNNy_Compute` |
| `content/render/pipeline_collection.cc/h` | UDL Pass0~3 改用 `BuildComputePipeline()` |
| `content/screen/renderscreen_impl.h` | `udl_tex1~4` (RGBA16F), `udl_pass0~3_binding` 为 compute binding，新增 `udl_pass2_binding` |
| `content/screen/renderscreen_impl.cc` | `GPURunUDLPassesInternal()` 改为 compute dispatch；2x 直出；Auto-fit 延迟应用 |
| `content/profile/content_profile.h` | mode 6 = `Anime4K Denoise L`，`UDLAutoFit` 自动适配 2x 窗口 |
| `renderer/CMakeLists.txt` | 添加 `pipeline/builtin_hlsl_anime4k_udl.cc` |

## Shader 转换要点

1. **Magpie compute → URGE compute shader**：生成器保留 Magpie 的 `void PassN(uint2 blockStart, uint3 threadId)` 结构，外层只生成 `[numthreads(64,1,1)] CSMain` 并按 `//!BLOCK_SIZE` 传入 `blockStart`。

2. **Pass1 / Pass4 block 拓扑**：Magpie `//!BLOCK_SIZE 16`，`uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;`，一个 thread 负责 2x2 像素块。URGE dispatch 使用 `ceil(native/16)`，final Pass4 使用 `ceil(upscaled/16)` 覆盖完整 2x 输出。

3. **Pass2 / Pass3 block 拓扑**：Magpie `//!BLOCK_SIZE 8`，一个 thread 处理一个 native 像素。URGE dispatch 使用 `ceil(native/8)`。

4. **采样器**：Magpie `sam` 为 point，`sam1` 为 linear。URGE compute pipeline 对特征纹理使用 point immutable sampler，final pass 的 `INPUT` residual 使用 linear sampler。

5. **权重与 MulAdd**：所有 MulAdd 矩阵权重来自 Magpie 原文件。生成 HLSL 保留 `MulAdd` 调用，C++ 运行时复用 `ApplyCuNNyMadMulAdd()` 替换为 Magpie 同类 `mad` 展开；FP16 仅在设备支持时启用。

重放生成命令：

```bash
python3 -B tools/generate_cunny_hlsl.py
```

## 实施后修复

| 问题 | 修复 | 文件 |
|------|------|------|
| 旧手写 UDL pixel shader 与 Magpie compute 拓扑不一致 | 改为从 Magpie 原始 HLSL 生成 URGE compute shader | `tools/generate_cunny_hlsl.py`, `builtin_hlsl_anime4k_udl.cc` |
| UDL Pass0-3 继承全局 sampler (point)，导致 residual 边缘采样不匹配 Magpie | feature 采样强制 point，final residual `INPUT` 强制 linear | `render_pipeline.cc` |
| UDL final 输出 dispatch 只覆盖左上四分之一 | final Pass4 按 2x 输出尺寸 dispatch | `renderscreen_impl.cc` |
| UDL 2x 输出仍走 1:1 blit 到 `upscale_buffer` | 窗口尺寸等于 `native * 2` 且 CAS 关闭时直接 present `enhanced_tex` | `renderscreen_impl.cc` |
| `UDLAutoFit=true` 启动时太早锁窗口，被脚本初始化覆盖后窗口偏移/黑边 | 启动只记录 pending，`RenderScreenImpl` 首帧和 `ResizeScreen`/`MoveWindow` 后再应用 2x auto-fit | `renderscreen_impl.cc` |
| Mode A 移除不彻底 | 删除函数体/管线/绑定/PSO，HLSL 保留编译不引用 | 多文件 |

## 中间纹理格式

- 中间纹理 `tex1~4`：`TEX_FORMAT_RGBA16_FLOAT`，带 `BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS`。
- 输出纹理 `enhanced_tex`：`TEX_FORMAT_RGBA8_UNORM`，作为 final UAV 输出和 present 输入。

## Binding 结构

```
Pass0: Binding_CuNNy_Compute → u_Texture + u_Output0/u_Output1 + u_params
Pass1: Binding_CuNNy_Compute → u_Texture1/u_Texture2 + u_Output0/u_Output1 + u_params
Pass2: Binding_CuNNy_Compute → u_Texture3/u_Texture4 + u_Output0/u_Output1 + u_params
Pass3: Binding_CuNNy_Compute → u_Texture(linear) + u_Texture1/u_Texture2 + u_Output + u_params
```

## 与 Magpie 的一致性

- 源文件对应 Magpie `src/Effects/Anime4K/Anime4K_Upscale_Denoise_L.hlsl`。
- 生成文件记录源 SHA256：`9f1e62a8d1fac368a530fe329ca80a6e65a61b29ee9f2205c4ee15eed1a54325`。
- 4 个 pass、`//!BLOCK_SIZE`、`//!NUM_THREADS 64`、权重矩阵、CReLU、D2S 写出和 residual 采样均按 Magpie 原始 compute 结构生成。
- URGE 仍会改写资源名、UAV 输出名和常量 buffer 名，因此不是逐字编译 Magpie 原文件。
- 逐像素一致仍需要 Magpie/URGE 同输入截图或离屏输出对比；静态检查和构建通过只能确认结构对齐。

## 运行验证记录

- `python3 -B tools/generate_cunny_hlsl.py`：已生成 UDL compute HLSL。
- `cmake --build build -j$(nproc)`：通过。
- mode 6 短启动验证：未见 UDL shader/PSO/binding 创建错误。
- `UDLAutoFit=true` 的启动应用点已从早期窗口创建阶段移动到渲染层 pending 应用；仍建议在真实游戏窗口中确认居中、尺寸和黑边表现。

## 当前剩余工作

- 做 Magpie/URGE 同输入截图或离屏输出 diff，确认逐像素或视觉一致性。
- 在真实 Vulkan 环境下验证 mode 6 长时间运行与窗口 resize/auto-fit 交互。
- 后续按 `cunny_udl_pso_lazy_loading_plan.md` 实施 UDL/CuNNy 重型 shader/PSO 懒加载，降低启动黑屏时间。
