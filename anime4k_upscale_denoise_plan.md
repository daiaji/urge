# Anime4K_Upscale_Denoise_L 移植记录

## 状态：已完成 ✓

## 目标

将 Magpie 中的 `Anime4K_Upscale_Denoise_L` (CNN 2x 超分 + 降噪) 移植到 URGE 引擎。

源文件：`/home/daiaji/repo/Magpie/src/Effects/Anime4K/Anime4K_Upscale_Denoise_L.hlsl`

## 管线

```
Game (640×480)
  → Pass0: Conv-4x3x3x3      (640×480, INPUT → tex1, tex2, MRT)
  → Pass1: Conv-4x3x3x16     (640×480, tex1+tex2 → tex3, tex4, MRT)
  → Pass2: Conv-4x3x3x16     (640×480, tex3+tex4 → tex1, tex2, MRT)
  → Pass3: Conv-4x3x3x16+D2S (1280×960, INPUT+tex1+tex2 → enhanced_tex)
  → Present (1280×960 → window, mode-dependent)
```

## 关键文件

| 文件 | 变更 |
|------|------|
| `renderer/pipeline/builtin_hlsl.cc` | 4 个 UDL HLSL 字符串 (`kHLSL_Anime4K_UDL_Pass0~3_Pixel`) |
| `renderer/pipeline/builtin_hlsl.h` | 4 个 extern 声明 |
| `renderer/pipeline/render_pipeline.cc/h` | 4 个 pipeline 定义 (`Pipeline_Anime4K_UDL_Pass0~3`) + 匿名 namespace `MakeClampSampler` 辅助函数 |
| `renderer/pipeline/render_binding.cc/h` | `Binding_UDL_D2S`（Pass3: 3 纹理 + params） |
| `content/render/pipeline_collection.cc/h` | 4 个 PSO (`anime4k_udl_pass0~3`)，Pass0-2 使用 RGBA16F 中间格式 |
| `content/screen/renderscreen_impl.h` | `udl_tex1~4` (RGBA16F), `udl_pass0~3_binding` |
| `content/screen/renderscreen_impl.cc` | `GPURecreateAnime4KTargetsInternal`, `GPURunUDLPassesInternal`, UI 下拉框 (mode 6) |
| `content/profile/content_profile.h` | mode 6 = "Anime4K Denoise L", `udl_auto_fit` 自动适配窗口 |

## Shader 转换要点

1. **Magpie compute → URGE pixel shader**：每个 pass 是一个 fullscreen quad draw。Magpie 的 `blockStart + threadId` → pixel shader 的 `PSIn.UV` 映射像素坐标。

2. **Pass0**：使用 `SampleLevel` 采样 3×3 邻域（原始 Magpie 使用 `Gather` 指令），等效转换。

3. **Pass1-2**：3×3 卷积 + CReLU。两个输入纹理各产生 4+4 个正负部通道（`max(v,0)` + `max(-v,0)`），共 32 个输入通道。

4. **Pass3 (D2S)**：`sub_idx = (pixel.x & 1) + ((pixel.y & 1) << 1)` 选择对应子像素输出。最终 `cnn_rgb + residual`，residual 使用 `u_Texture.SampleLevel(PSIn.UV)` 直接对应 Magpie 的 `INPUT.SampleLevel(SL, ...)`。

5. **权重**：所有 MulAdd 矩阵权重保持 Magpie 原值，使用 `MF3x4`/`MF4x4` 宏展开。

## 实施后修复

| 问题 | 修复 | 文件 |
|------|------|------|
| UDL Pass0-3 继承全局 sampler (point)，导致边缘锯齿 | Pass0-2 feature 采样强制 point，Pass3 residual 强制 linear | `render_pipeline.cc` |
| UDL 2x 输出走 Lanczos3 重采样到同尺寸窗口，引入额外振铃 | 窗口尺寸 = 2x 输出时走 nearest identity (mode 1) | `renderscreen_impl.cc` |
| Mode A 移除不彻底 | 删除函数体/管线/绑定/PSO，HLSL 保留编译不引用 | 多文件 |

## 中间纹理格式

- 中间纹理 (tex1~4)：`TEX_FORMAT_RGBA16_FLOAT`（CNN 特征值范围超出 [0,1]）
- 输出纹理 (enhanced_tex)：`TEX_FORMAT_RGBA8_UNORM`

## Binding 结构

```
Pass0: Binding_Upscale     → u_Texture(point) + u_params
Pass1: Binding_Upscale     → u_Texture(point) + u_Texture1(point, 动态绑定) + u_params
Pass2: Binding_Upscale     → 同 Pass1
Pass3: Binding_UDL_D2S     → u_Texture(linear) + u_Texture1(point) + u_Texture2(point) + u_params
```
