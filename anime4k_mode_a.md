# Anime4K Mode A 集成 — 进度存档

## 目标

为レトリアの大冒険（RMXP, 640×480 原生）在 URGE 中实现 Anime4K Mode A 多 Pass 放大链，
将渲染分辨率提升至 **1280×960（2x 整数倍）**，并结合 Clamp_Highlights + Restore_CNN 后处理提升线条质量。

## 完整管线

```
Game (640×480)
  → Clamp_Highlights (3 pass, de-ring)
  → Restore_CNN (8 pass, 卷积恢复, 7 中间层独立 RT)
  → Upscale_CNN_x2 (5 pass, 2x → 1280×960)
  → Anime4K_Enhance(DoG) [复用现有]
  → Upscale (Lanczos3, 到窗口大小)
  → CAS (可选)
  → Present
```

## 已实现

### 转换工具链 ✅

| 工具 | 位置 | 说明 |
|------|------|------|
| `mpv2glsl.py` | `ShaderTranspiler/tools/` | mpv GLSL → 标准 GLSL 420 |
| `convert` | `ShaderTranspiler/build/convert` | GLSL → spirv-cross HLSL |
| `adapt_hlsl.py` | `urge/tools/adapt_hlsl.py` | spirv-cross HLSL → URGE HLSL |

**mpv2glsl.py 修复清单**：
- pass 分割：只在 `meta.hook` 存在时才分割（修复 license 块被当 pass）
- `go_N` 宏展开：纹理名从 group(4) 获取（非 group(2)）
- `g_N` 宏：内联展开 + word-boundary 替换
- `_texOff`：支持小写纹理名
- uniform block：满足 Vulkan GLSL 要求
- 入口点：`hook()` → `main()` + `fragColor` 赋值
- 大括号深度跟踪 + 自动补全 `}`

**adapt_hlsl.py 转换步骤**：
1. spirv-cross 纹理声明 → `Texture2D u_Texture`
2. spirv 静态变量（`static float2 _21`）→ 全局声明 + PSMain 内同步
3. spirv 输出变量（`_250`）→ `PSOut.Color`
4. cbuffer field（`_28_m0`）→ `u_InputPt` / `u_InputSize`（depth-to-space 检测）
5. 未使用纹理自动过滤
6. frag_main 提取：大括号深度跟踪，断在正确的 `}`

### 已注册 Pipeline (16 passes)

所有 pass 注册在 `render_pipeline.cc`，使用 `kHLSL_UpscalePass_Vertex`（全屏 Quad）：
- `Pipeline_Anime4K_Clamp_Highlights_Pass0~2` — Clamp de-ring
- `Pipeline_Anime4K_Restore_CNN_Pass0~7` — 卷积恢复
- `Pipeline_Anime4K_Upscale_CNN_x2_S_Pass0~4` — 2x 上采样

### 渲染链 (`GPURunModeAPassesInternal`)

- 阶段 1: Clamp_Highlights (3 passes, 640×480)
- 阶段 2: Restore_CNN (8 passes, 7 中间纹理 + merge)
- 阶段 3: Upscale_CNN_x2 (5 passes → 1280×960)
- 阶段 4: Anime4K_Enhance(DoG) 复用现有 pass
- 开关: `anime4k_mode_a_enabled` ImGui 复选框

## 已知问题

### P0 — 崩溃

- `GPURunModeAPassesInternal` 存在 SIGSEGV
- 疑似 Pipeline State 或 texture 为 null 时未跳过
- 已添加初步 null 检查，需验证

### P1 — 多纹理约束

当前所有 pass 强制使用单纹理（`u_Texture`），多纹理 pass（Restore pass7 merge, Upscale pass4 depth-to-space）
在语义上不正确——所有中间层读取同一纹理，导致 merge 退化。

**根本原因**：Diligent 的 `UseCombinedTextureSamplers` 与多纹理 resource signature 冲突，
`CreatePipelineResourceSignature` 报 `Multiple resources with name 'u_Texture1'`。

修复方向：
- 用 `SHADER_RESOURCE_TYPE_COMBINED_SAMPLER` 注册（Diligent 版本不支持该枚举）
- 改用 `SHADER_RESOURCE_TYPE_TEXTURE_SRV` + `SAMPLER` 分开注册
- 或为每组纹理配置创建独立的资源签名

### P2 — 未实现

- ImGui 开关彻底测试
- scaling_mode 联动（目前 mode_a 开启时设 mode=2 Lanczos3）
- 性能测量（~0.35ms 预算内）
- HLSL 变量名清理（spirv 残留 `_11`, `_12` 等）

## 文件清单

```
urge/
├── tools/
│   └── adapt_hlsl.py              ← spirv-cross HLSL → URGE HLSL
├── renderer/pipeline/
│   ├── builtin_hlsl.cc            ← 16 个 HLSL pass 字符串常量
│   ├── builtin_hlsl.h             ← 声明
│   ├── render_pipeline.cc         ← Pipeline_* 定义 (ST/MT macro)
│   └── render_pipeline.h          ← PipelineSet 成员
├── content/
│   ├── profile/content_profile.h  ← anime4k_mode_a_enabled 配置
│   ├── render/pipeline_collection.cc/h  ← PSO 构建
│   └── screen/renderscreen_impl.cc/h   ← 渲染链 + 中间 RT
ShaderTranspiler/
└── tools/
    ├── mpv2glsl.py                ← mpv GLSL → 标准 GLSL
    └── convert.cpp                ← ShaderTranspiler CLI
```
