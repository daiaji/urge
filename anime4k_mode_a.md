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

### Pipeline 注册 ✅

16 个 pass 注册在 `render_pipeline.cc`：
- `Pipeline_Anime4K_Clamp_Highlights_Pass0~2` — Clamp de-ring (640×480)
- `Pipeline_Anime4K_Restore_CNN_Pass0~7` — 卷积恢复 (640×480)
- `Pipeline_Anime4K_Upscale_CNN_x2_S_Pass0~4` — 2x 上采样 (640×480 → 1280×960)

其中 Pass7 (merge) 和 Pass4 (d2s) 使用**显式多纹理 pipeline signature**。

### 渲染链 ✅

- **Phase 1**: Clamp_Highlights (3 passes, 640×480)
  - Pass0: 水平统计 → tex0
  - Pass1: 垂直统计 → tex1
  - Pass2: 钳位 → tex0

- **Phase 2**: Restore_CNN (8 passes, 640×480)
  - Pass0: Conv-4x3x3x3 → rt[0]
  - Pass1~6: Conv-4x3x3x8 → rt[1..6]，每 pass 写入独立中间纹理
  - Pass7: Merge — 读取 screen(残差) + rt[0..6] 共 8 纹理 → tex0

- **Phase 3**: Upscale_CNN_x2 (5 passes)
  - Pass0~2: Conv at native (640×480), ping-pong tex0/tex1
  - Pass3: Conv at native → tex1
  - Pass4: Depth-to-space — 读取 tex1(conv) + screen(残差), 输出 1280×960 → tex2

- **Phase 4**: Anime4K_Enhance(DoG) → enhanced_tex (1280×960)

- **Final**: input_size *= 2 → Lanczos3 读取 1280×960 → 窗口大小

### ImGui 集成 ✅

- `scaling_mode == 6` = Anime4K Mode A (Scaling Algorithm 下拉框)
- Auto-fit Window 复选框 + Apply 按钮（整数倍缩放至屏幕最大适配）
- 配置持久化到 INI

## 已知问题

### P0 — SIGSEGV ✅ 已修复

入口 null 检查原先只检查 4 个 PSO，遗漏 restore_pass1~6、upscale_pass0~4、enhance 共 13 个 PSO，
以及 binding 子变量和 restore 中间纹理。已全部补全。

### P1 — Pass4 d2s 自读错误 ✅ 已修复

原始 HLSL d2s 从自身纹理读取残差（等同于无残差）。
改为从 `u_Texture1` + `screen_buffer` 读取原始帧残差，匹配 GLSL `MAIN_tex(MAIN_pos)`。

### P1 — Pass7 merge 单纹理退化 ✅ 已修复

原所有 pass 强制使用单纹理 `u_Texture`，merge pass 无法读取中间层。
修复：创建 `Binding_A4A_Merge`（8 纹理 + 参数），HLSL 添加 `u_Texture1..u_Texture7` 声明，
pipeline 注册使用独立的 8 纹理 resource signature。

### P1 — Pass3 分辨率错误 ✅ 已修复

Pass3 原本渲染在 1280×960 viewport，应为 640×480（conv pass 全是 native 分辨率，
只有 depth-to-space pass 做 2× 上采样）。

### P1 — enhanced_tex 尺寸错误 ✅ 已修复

Mode A 下 enhanced_tex 大小应为 1280×960（2×），非原生 640×480。

### P1 — input_size 未加倍 ❓ 待确认

Mode A 后 `input_size *= 2` 使 Lanczos3 正确读取 1280×960 纹理。

### 颜色渲染 ✅

使用 screen_buffer 作为残差输入，颜色表现正确。

### 锯齿/混叠 (Aliasing) ⚠️ 需要排查

Mode A 输出存在明显锯齿（staircase artifacts），即使 640×480→1280×960（2×）是理想场景。

**对比 GLSL 参考** (Upscale_CNN_x2_S Pass4 d2s, `Anime4K/glsl/Upscale/Anime4K_Upscale_CNN_x2_S.glsl`):
- GLSL d2s 使用 `texelFetch`-like 采样（texel 坐标精确对齐）
- HLSL d2s 使用 `Sample` + UV 偏移到 texel 中心，可能存在采样不精确
- d2s 输出 `float4(_32, _32, _32, _32)` 将单个分量复制到所有 4 通道（与 GLSL 一致）

**可能原因**：
1. d2s 的 bilinear sampler 在 sub-pixel 边界采样不精确
2. 卷积 pass 在 d2s 前后的分辨率设置错误
3. d2s shader 索引逻辑与 GLSL 有差异

## 文件清单

```
urge/
├── tools/
│   └── adapt_hlsl.py              ← spirv-cross HLSL → URGE HLSL
├── renderer/pipeline/
│   ├── builtin_hlsl.cc            ← 16+ HLSL shader 字符串常量 + Pass7/Pass4 更新
│   ├── builtin_hlsl.h             ← 声明
│   ├── render_pipeline.cc         ← Pipeline_* 定义 + 多纹理 signature
│   ├── render_pipeline.h          ← PipelineSet 成员
│   ├── render_binding.cc          ← Binding_A4A_Merge 定义
│   └── render_binding.h           ← Binding_A4A_Merge 声明
├── content/
│   ├── profile/content_profile.h  ← scaling_mode, mode_a_auto_fit
│   ├── profile/content_profile.cc ← mode_a_auto_fit INI 序列化
│   ├── render/pipeline_collection.cc  ← PSO 构建
│   └── screen/renderscreen_impl.cc/h  ← GPUData + 渲染链 + ImGui UI
```

## 关键文件

| 文件 | 关键内容 |
|------|----------|
| `content/screen/renderscreen_impl.cc` | `GPURunModeAPassesInternal` 全管线, ImGui UI, auto-fit |
| `content/screen/renderscreen_impl.h` | GPUData: mode_a_tex0..3, merge/upscale4 binding |
| `renderer/pipeline/builtin_hlsl.cc` | Pass7 merge (8 tex) ~line 2245, Pass4 d2s ~line 2658 |
| `renderer/pipeline/render_pipeline.cc` | Pipeline_Anime4K_A_* 注册, 多纹理 signature |
| `renderer/pipeline/render_binding.h/cc` | `Binding_A4A_Merge` (8 tex + params) |
| `content/profile/content_profile.h/cc` | `mode_a_auto_fit`, `scaling_mode==6` |
