# Anime4K_Upscale_Denoise_L 移植计划

## 目标

将 Magpie 中的 `Anime4K_Upscale_Denoise_L` (CNN 2x 超分 + 降噪) 移植到 URGE 引擎，
**替换现有 Mode A**（20-pass Clamp + Restore_CNN + Upscale_CNN_x2）。

## 背景

- Mode A 的 Restore 步骤对 RPG Maker 像素风素材过度处理，导致树木/角色液化
- `Anime4K_Upscale_Denoise_L` 仅做 2x 放大 + 降噪，对像素艺术素材效果更好
- 源文件：`/home/daiaji/repo/Magpie/src/Effects/Anime4K/Anime4K_Upscale_Denoise_L.hlsl`

## 管线

```
Game (640×480)
  → Pass 1: Conv-4x3x3x3      (640×480, INPUT → tex1, tex2)
  → Pass 2: Conv-4x3x3x16     (640×480, tex1, tex2 → tex3, tex4)
  → Pass 3: Conv-4x3x3x16     (640×480, tex3, tex4 → tex1, tex2)
  → Pass 4: Conv-4x3x3x16 + D2S (1280×960, INPUT + tex1 + tex2 → OUTPUT)
  → Present (1280×960)
```

## 现有 Mode A 对比

| | Mode A (移除) | Upscale_Denoise_L (新增) |
|---|---|---|
| Pass 数 | 20 (3 Clamp + 8 Restore + 9 Upscale) | 4 |
| 中间纹理 | 11 个 | 4 个 (tex1..tex4) |
| 多纹理 binding | 3 个 (Merge×2 + D2S) | 1 个 (Pass4: INPUT + tex1 + tex2) |
| CNN 参数量 | ~10k | ~20k |
| 效果 | 线条增强过度 | 2x 放大 + 温和降噪 |

---

## 实施步骤

### Step 1: Shader 转换

**来源**：Magpie compute shader (4 passes)  
**目标**：URGE pixel shader (每 pass 一个 fullscreen quad draw)

**转换要点**：

1. **Pass 1** — 使用 `GatherRed/GatherGreen/GatherBlue` 进行 4×4 像素块采样。
   这些是 DirectX 硬件 gather 指令，HLSL pixel shader 同样支持。
   需注意 `MF3`/`MF4` 等 Magpie 宏需展开为标准 HLSL 类型。

2. **Pass 2-3** — 标准 3×3 卷积 + CReLU（Concatenated ReLU）。
   每个输入纹理采样 9 个位置 (a,b,c,d,e,f,g,h,i)，
   分别计算正部 `max(v, 0)` 和负部 `max(-v, 0)`。
   两个输入纹理 (tex1, tex2 或 tex3, tex4) 各自产生 4+4 个正负部通道，
   共 16 个正部 + 16 个负部 = 32 个输入通道。

3. **Pass 4** — 3×3 卷积 + Depth-to-Space (2x upscale)。
   读取 INPUT (残差) + tex1 + tex2。
   输出 4 个子像素 (target1/target2/target3 的 xyzw 对应 2×2 网格)。
   最终 `OUTPUT = CNN_output + INPUT_upSampled` (残差连接)。

4. **权重矩阵转换**：
   GLSL `mul(v, M)` 对应 HLSL `mul(M, v)` 时矩阵需按列主序存储。
   `adapt_hlsl.py` 当前已正确处理此转换（参考 Mode A 修复记录）。

5. **MulAdd 宏**：
   Magpie 的 `MulAdd(a, M, target)` = `target + mul(a, M)`。
   需转换为 URGE HLSL 中对应的 matrix-vector 乘法。

**Shader 文件**：
```
renderer/pipeline/builtin_hlsl.cc  ← 添加 4 个 HLSL 字符串常量
  kHLSL_Anime4K_UDL_Pass0_Pixel   // Conv-4x3x3x3
  kHLSL_Anime4K_UDL_Pass1_Pixel   // Conv-4x3x3x16
  kHLSL_Anime4K_UDL_Pass2_Pixel   // Conv-4x3x3x16
  kHLSL_Anime4K_UDL_Pass3_Pixel   // D2S merge
```

### Step 2: Pipeline 注册

**文件**：`renderer/pipeline/render_pipeline.cc`

Pass 1-3 为单纹理 (ST) pipeline，Pass 4 为多纹理 (MT) pipeline。

```cpp
// 复用现有 MAKEPIPELINE_ST 宏（单纹理 + params buffer）
MAKE_UDL_ST_PIPELINE(kHLSL_Anime4K_UDL_Pass0_Pixel, UDL_Pass0)
MAKE_UDL_ST_PIPELINE(kHLSL_Anime4K_UDL_Pass1_Pixel, UDL_Pass1)
MAKE_UDL_ST_PIPELINE(kHLSL_Anime4K_UDL_Pass2_Pixel, UDL_Pass2)

// Pass 3 (D2S): 3 纹理 binding (INPUT + tex1 + tex2)
PIPELINE_HEADER(Anime4K_UDL_Pass3) {
  // 使用类似 A4A_Merge 的多纹理 signature
  ...
};
```

**文件**：`renderer/pipeline/render_pipeline.h`
- 移除 Mode A 的 20 个 pipeline field
- 添加 4 个新的 pipeline field

### Step 3: Binding 定义

**文件**：`renderer/pipeline/render_binding.cc` / `.h`

- Pass 1-3：复用现有 `Binding_Upscale`（单纹理 + params）
- Pass 4 (D2S)：
  ```cpp
  // 3 纹理 binding: u_Texture(INPUT) + u_Texture1(tex1) + u_Texture2(tex2)
  struct Binding_UDL_D2S {
    ShaderResourceBinding u_texture;   // INPUT (bilinear sampler)
    ShaderResourceBinding u_texture1;  // tex1 (point sampler)
    ShaderResourceBinding u_texture2;  // tex2 (point sampler)
    ShaderResourceBinding u_params;    // params buffer
  };
  ```

### Step 4: GPU 资源管理

**文件**：`content/screen/renderscreen_impl.h`

修改 `GPUData` struct：
```cpp
// 移除 Mode A 资源:
//   mode_a_tex0, mode_a_tex1, mode_a_tex2, mode_a_tex3
//   mode_a_restore_tex (7 个)
//   mode_a_binding, mode_a_restore_merge_binding, mode_a_upscale_merge_binding
//   mode_a_upscale_d2s_binding

// 新增 Upscale_Denoise_L 资源:
RRefPtr<Diligent::ITexture> udl_tex1;  // 640×480, RGBA16F
RRefPtr<Diligent::ITexture> udl_tex2;  // 640×480, RGBA16F
RRefPtr<Diligent::ITexture> udl_tex3;  // 640×480, RGBA16F
RRefPtr<Diligent::ITexture> udl_tex4;  // 640×480, RGBA16F
renderer::Binding_Upscale udl_binding;      // Pass 0-2 单纹理
renderer::Binding_UDL_D2S udl_d2s_binding;  // Pass 3 多纹理

// enhanced_tex 复用为 1280×960 输出
```

**文件**：`content/screen/renderscreen_impl.cc`

- 移除 `GPURecreateAnime4KTargetsInternal` 和相关 Mode A 代码
- 添加 `GPURecreateUDLTargetsInternal`（创建 4 个 640×480 RGBA16F 中间纹理）

### Step 5: 渲染链集成

**文件**：`content/screen/renderscreen_impl.cc`

新增函数 `GPURunUDLPassesInternal`：

```cpp
void RenderScreenImpl::GPURunUDLPassesInternal(
    Diligent::IDeviceContext* ctx) {
  auto native = context()->resolution;  // 640×480
  auto upscaled = native * 2;           // 1280×960

  // Pass 0: Conv-4x3x3x3 (INPUT → tex1, tex2)
  //   需要一次 draw 输出 2 个 RT (MRT)

  // Pass 1: Conv-4x3x3x16 (tex1, tex2 → tex3, tex4)
  //   绑定 tex1, tex2 作为 SRV，输出 tex3, tex4 (MRT)

  // Pass 2: Conv-4x3x3x16 (tex3, tex4 → tex1, tex2)
  //   绑定 tex3, tex4 作为 SRV，输出 tex1, tex2 (MRT)

  // Pass 3: D2S (INPUT + tex1 + tex2 → enhanced_tex, 1280×960)
  //   多纹理 binding，2x upscale viewport
}
```

**关键差异 vs Mode A**：
- Mode A 用 ping-pong (tex0 ↔ tex1) 每次单输出
- Upscale_Denoise_L 的 Pass 1-3 每次双输出 (tex1+tex2 或 tex3+tex4)，需用 MRT
- Pass 4 输出 1280×960 的 enhanced_tex，复用现有 upscale 路径

### Step 6: ImGui UI 更新

**文件**：`content/screen/renderscreen_impl.cc`

```cpp
// 修改 scaling_items 数组，移除 "Anime4K Mode A"
const char* scaling_items[] = {
    "Bilinear", "Nearest",
    "Lanczos3", "Bicubic",
    "Anime4K", "Anime4K+Sobel",
    "Anime4K Denoise L"  // 新增，mode == 6
};

// mode == 6 触发 upscale_denoise 路径
if (mode == 6) {
    GPURunUDLPassesInternal(render_context);
    input_size = input_size * 2;  // 2x output
    mode = 2;  // Lanczos for final fit
}
```

### Step 7: 清理

- 从 `render_pipeline.cc` / `.h` 移除所有 Mode A 的 20 个 pipeline 定义
- 从 `builtin_hlsl.cc` / `.h` 移除 Mode A 的 16 个 shader 字符串
- 从 `render_binding.cc` / `.h` 移除 `Binding_A4A_Merge`
- 删除 `anime4k_mode_a.md`（进度存档）

---

## 难点

### 1. Pass 1 的 Gather 指令

Magpie compute shader 使用 `GatherRed/GatherGreen/GatherBlue` 从 4 个像素一次采样。
URGE 是 pixel shader，每个像素独立执行。需要改写为：
- 每个 output pixel 对应 1 个 conv 输出
- 手动采样 3×3 邻域或使用 `SampleLevel` + `Gather`（pixel shader 也支持）

### 2. MRT（多渲染目标）

Pass 1-3 各输出 2 个纹理 (tex1+tex2 或 tex3+tex4)。
需要在 pipeline 签名中使用 2 个 render target 的 MRT 设置。

### 3. CReLU 分离

Pass 2-4 需要正部 (`max(v,0)`) 和负部 (`max(-v,0)`) 作为独立的 16+16=32 个卷积输入通道。
Magpie 在 compute shader 中手动分离，URGE pixel shader 需同样处理。

### 4. Pass 4 的 D2S + 残差

输出 4 个子像素（2×2 网格），每个是 CNN 3 通道输出 + bilinear 上采样残差。
与现有 Mode A 的 d2s pass 结构类似，但：
- 读取 3 个纹理 (INPUT + tex1 + tex2) vs Mode A 的 2 个
- 3 个输出通道 (target1/2/3) vs Mode A 的 4 个

### 5. 权重对齐验证

此模型有大量权重（~20k 参数），任何矩阵转置错误都会导致输出无效。
参考 Mode A 的 `float4x4` 转置修复流程：
1. 先把 Magpie HLSL 逐字转换（不修改矩阵顺序）
2. 在 `adapt_hlsl.py` 中处理格式转换
3. 用已知测试图像对比 GPU 输出 vs 参考实现

---

## 文件变更清单

```
修改：
  renderer/pipeline/builtin_hlsl.cc       ← +4 shader, -16 Mode A shader
  renderer/pipeline/builtin_hlsl.h        ← +4 声明, -16 声明
  renderer/pipeline/render_pipeline.cc    ← +4 pipeline, -20 Mode A pipeline
  renderer/pipeline/render_pipeline.h     ← +4 field, -20 field
  renderer/pipeline/render_binding.cc     ← +1 UDL_D2S binding, -1 A4A_Merge
  renderer/pipeline/render_binding.h      ← +1 声明, -1 声明
  content/screen/renderscreen_impl.cc     ← +UDL 渲染链, -Mode A 渲染链, UI
  content/screen/renderscreen_impl.h      ← +UDL GPUData, -Mode A GPUData
  content/profile/content_profile.h       ← 不变 (scaling_mode 复用)
  content/profile/content_profile.cc      ← 可能需调整 auto_fit 逻辑

删除引用：
  anime4k_mode_a.md                       ← 进度存档 (已过时)
```

---

## 预估工作量

| 步骤 | 内容 | 预估时间 |
|------|------|----------|
| Step 1 | Shader 转换 (4 个 HLSL) | 2-3 小时 |
| Step 2 | Pipeline 注册 | 0.5 小时 |
| Step 3 | Binding 定义 | 0.5 小时 |
| Step 4 | GPU 资源管理 | 1 小时 |
| Step 5 | 渲染链集成 | 2 小时 |
| Step 6 | ImGui UI | 0.5 小时 |
| Step 7 | 清理 Mode A 代码 | 1 小时 |
| 调试 | 权重对齐 + 渲染验证 | 2-4 小时 |
| **合计** | | **1-1.5 天** |
