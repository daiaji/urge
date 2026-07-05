# CuNNy / Anime4K UDL PSO 懒加载实施计划

## 背景

当前 `PipelineSet` 在构造时会构造所有 `Pipeline_*` loader。UDL 与 CuNNy loader 构造阶段会同步创建 compute shader，`PipelineCollection` 构造阶段又会同步创建对应 compute PSO。

这意味着即使游戏当前不使用 `ScalingMode=6/7/8`，启动阶段仍可能编译这些重型 shader 和 PSO，造成黑屏时间增加。仅把 `BuildComputePipeline()` 延后还不够，因为 loader 构造时的 `SetupComputePipelineBasis()` / `CreateShader()` 已经发生。

## 目标

- 启动阶段只创建基础渲染、present、普通 upscale、Anime4K Enhance 和 CAS 等常用 pipeline。
- `ScalingMode=6` 首次使用时才创建 Anime4K UDL 4 个 compute loader、4 个 compute PSO 和对应 bindings。
- `ScalingMode=7` 首次使用时才创建 CuNNy-4x16 6 个 compute loader、6 个 compute PSO 和对应 bindings。
- `ScalingMode=8` 首次使用时才创建 CuNNy-4x24 6 个 compute loader、6 个 compute PSO 和对应 bindings。
- 懒加载失败时只禁用对应重型路径，不影响基础渲染和其它缩放模式。

## 当前实施状态

已完成基础懒加载方案：

- `renderer/pipeline/render_pipeline.h/.cc`
  - UDL / CuNNy loader 已从直接成员改为 `std::unique_ptr` lazy-owned 成员。
  - 已新增 `EnsureAnime4KUDLLoaders()`、`EnsureCuNNy4x16Loaders()`、`EnsureCuNNy4x24Loaders()`。
- `content/render/pipeline_collection.h/.cc`
  - 构造函数中已移除 UDL / CuNNy compute PSO 的启动期创建。
  - 已新增 `EnsureAnime4KUDLPipelines()`、`EnsureCuNNy4x16Pipelines()`、`EnsureCuNNy4x24Pipelines()`。
  - 分组 PSO 创建失败时会记录具体 pass，并释放该组已部分创建的 PSO。
- `content/screen/renderscreen_impl.h/.cc`
  - 启动期已不再创建 UDL / CuNNy shader resource bindings。
  - 已新增 lazy ready state：`LazyPipelineState::{kUninitialized,kReady,kFailed}`。
  - `GPUScalingPassInternal()` 在 `ScalingMode=6/7/8` 首次实际使用时才串联创建 loader、PSO 和 binding。
  - 懒加载失败后当前运行期间不会每帧重复尝试，并 fallback 到 Lanczos3。
  - 已加入 lazy load 总耗时、loader/shader creation 耗时、PSO creation 耗时日志。

已完成 auto-fit 相关配套修复：

- `app/urge_main.cc`
  - 当 `UDLAutoFit=true` 且 `ScalingMode=6/7/8` 时，初始窗口尺寸直接使用 `resolution * 2`，避免先创建小窗口再延迟放大。
- `content/screen/renderscreen_impl.cc`
  - auto-fit 从“首帧 pending 应用”改为在构造、`ResizeScreen`、`MoveWindow`、`ScalingMode` 改变、GUI checkbox 改变时同步应用。
  - 关闭 auto-fit 时恢复尺寸使用 `max(profile->window_size, context->resolution)`，避免游戏脚本把分辨率改大后仍恢复到过小的 `WindowSize`。
  - 移除 `auto_fit_window_pending_` 和每帧 pending 检查。

已完成 shader bytecode cache 加速：

- `content/worker/content_runner.h/.cc`
  - 新增 Diligent `IBytecodeCache` 生命周期管理。
  - 启动时从 `ShaderCache/shader_bytecode_cache_<backend>.bin` 加载缓存，退出时保存缓存。
  - 缓存目录不存在时自动创建；缓存不存在、无效或写入失败时只记录日志，不影响渲染。
- `renderer/pipeline/render_pipeline.h/.cc`
  - `PipelineInitParams` 新增 `IBytecodeCache* bytecode_cache`。
  - `RenderPipelineBase::CreateShaderWithCache()` 会先查 bytecode cache；命中时用 bytecode 创建 shader。
  - miss 时保持原有 HLSL runtime compile 路径，并把 `IShader::GetBytecode()` 结果写回 cache。
  - cache hit 但 bytecode 创建失败时会移除该条缓存并 fallback 到原始 source 编译。
  - vertex/pixel shader 名称加 `.vertex` / `.pixel` 后缀，避免同一 pipeline 的 VS/PS 共用同名。

验证结果：

- `cmake --build build -j$(nproc)` 通过。
- `git diff --check --ignore-cr-at-eol` 通过。
- 日志中 `ScalingMode=8` 首次触发 CuNNy-4x24 lazy loading，样本显示总耗时约 2.1 到 2.6 秒，其中 loader/shader creation 约 2.0 到 2.5 秒，PSO creation 约 0.1 秒。
- shader bytecode cache 冷启动首次运行会生成 `ShaderCache/shader_bytecode_cache_5.bin`，样本大小约 988KB。
- 第二次启动加载该缓存后，CuNNy-4x24 六个 compute shader 全部 cache hit：loader/shader creation 从约 3.0 秒下降到 13ms，lazy loading 总耗时从约 3.1 秒下降到 154ms；剩余主要耗时是 compute PSO creation，约 0.14 秒。

当前未解决/待确认问题：

- auto-fit 启动位移已确认与 Ruby 脚本有关：`Scripts.rvdata2` 解包后，`155.rb`（`ウィンドウサイズ変更`）在加载时执行 `Graphics.resize_screen(640, 480)`；注入的 `rgss3_compat.rb` 又在 `resize_screen` 后调用 `move_window(rect.x, rect.y, width, height)`。
- C++ 已在 `MoveWindow()` 中对 auto-fit 模式做兜底：脚本请求 `640x480` 时会重新应用 `resolution * 2`，并输出 `2x auto-fit overriding move_window request`。
- 为避免用户看到从 `1088x832` 二次居中到 `1280x960` 的位移，auto-fit 启动时窗口会先隐藏，在第一帧 present 后显示，并输出 `2x auto-fit window shown after first present`。
- 如果隐藏窗口导致用户感知黑屏变长，下一步可把显示窗口的时机提前到 Ruby 脚本加载完成且最终 auto-fit 尺寸应用后、CuNNy lazy shader 编译前。

## 非目标

- 本计划不实现 Diligent pipeline archive；当前采用 shader bytecode cache 加速 shader creation，PSO 仍按需 runtime 创建。
- 当前 profiling 显示主要耗时在 loader/shader creation，不在 PSO creation；进一步减少首次卡顿应优先评估 Diligent shader archive/cache。
- 本计划不改变 Magpie 生成 HLSL、权重、dispatch 拓扑或输出结果。
- 本计划不处理 OpenGL fallback 的 compute shader 兼容问题。

## 已完成：着色器加载速度优化

### 当前性能结论

现有 lazy loading 已经把不使用 UDL/CuNNy 时的启动成本移除，但如果游戏默认 `ScalingMode=6/7/8`，首次实际渲染仍会同步创建对应 compute shader 和 PSO。

已观察到的 `ScalingMode=8` / CuNNy-4x24 日志样本：

```text
[Graphics] Lazy loading CuNNy-4x24 compute pipelines
[Graphics] CuNNy-4x24 compute loader creation finished in 2409ms
[Graphics] CuNNy-4x24 compute PSO creation finished in 141ms
[Graphics] CuNNy-4x24 compute pipelines ready
[Graphics] Lazy loading CuNNy-4x24 compute pipelines finished in 2551ms
```

结论：

- 主要瓶颈是 loader/shader creation，约 2.0 到 2.5 秒。
- PSO creation 约 0.1 秒，不是第一优先优化对象。
- 继续只优化 `BuildComputePipeline()` 或 PSO 延迟不会明显降低首次卡顿。

### 方案评估结论

1. 已采用：Diligent `IBytecodeCache`。
2. 已尝试但放弃：Diligent `IRenderStateCache` + Archiver。该方案能编译并创建 shader，但运行退出时 `WriteToBlob()` 序列化失败，无法稳定持久化。
3. 暂不采用：预编译 HLSL 到目标后端中间产物。当前 bytecode cache 已把第二次启动的 CuNNy-4x24 shader creation 降到 13ms，收益足够。
4. 暂不采用：后台预热。当前热启动剩余主要耗时是约 0.14 秒 PSO 创建，不值得引入多线程状态机风险。

### 方案 A：Diligent shader bytecode 缓存（已落地）

目标：让第二次启动或第二次使用同一 renderer/backend 时，UDL/CuNNy 的 shader creation 不再走完整 HLSL 编译路径。

实际落地方案：

- 使用 `Diligent::IBytecodeCache`，而不是 `IRenderStateCache`。
- cache key 由 Diligent 按 `ShaderCreateInfo` 和 `DeviceType` 计算，覆盖 shader source、宏、entry point、compile flags、backend 等因素。
- cache 文件按 backend 分离：`ShaderCache/shader_bytecode_cache_<backend>.bin`。
- source compile 成功后调用 `IShader::GetBytecode()`，复制 bytecode 到 `IDataBlob` 并 `AddBytecode()`。
- 第二次启动先 `Load()`，命中时设置 `ShaderCreateInfo::ByteCode/ByteCodeSize` 并清空 `Source`，直接从 bytecode 创建 shader。
- 任何 cache hit 失败都会 fallback 到原始 HLSL compile。

验证样本：

```text
[Graphics] Shader bytecode cache loaded: ShaderCache/shader_bytecode_cache_5.bin bytes=987708
[Graphics] Shader bytecode cache hit: CuNNy_4x24.Pass1.compute
[Graphics] Create compute shader CuNNy_4x24.Pass1.compute finished in 0ms
[Graphics] Shader bytecode cache hit: CuNNy_4x24.Pass2.compute
[Graphics] Create compute shader CuNNy_4x24.Pass2.compute finished in 4ms
[Graphics] Shader bytecode cache hit: CuNNy_4x24.Pass3.compute
[Graphics] Create compute shader CuNNy_4x24.Pass3.compute finished in 3ms
[Graphics] Shader bytecode cache hit: CuNNy_4x24.Pass4.compute
[Graphics] Create compute shader CuNNy_4x24.Pass4.compute finished in 1ms
[Graphics] Shader bytecode cache hit: CuNNy_4x24.Pass5.compute
[Graphics] Create compute shader CuNNy_4x24.Pass5.compute finished in 1ms
[Graphics] Shader bytecode cache hit: CuNNy_4x24.Pass6.compute
[Graphics] Create compute shader CuNNy_4x24.Pass6.compute finished in 0ms
[Graphics] CuNNy-4x24 compute loader creation finished in 13ms
[Graphics] CuNNy-4x24 compute PSO creation finished in 141ms
[Graphics] Lazy loading CuNNy-4x24 compute pipelines finished in 154ms
```

相关代码位置：

- `renderer/pipeline/render_pipeline.cc`
  - `Pipeline_*` loader 构造函数。
  - `SetupComputePipelineBasis()`。
  - `CreateShader()` 调用点。
- `renderer/pipeline/render_pipeline.h`
  - `PipelineInitParams` 是否需要加入 archive/cache 相关对象或路径。
- `content/worker/content_runner.cc`
  - `CreateRenderComponents()` 中创建 `PipelineSet` / `PipelineCollection` 的位置。

已满足的设计要求：

- 缓存 key 由 Diligent `IBytecodeCache` 按 `ShaderCreateInfo` 和 device type 计算，覆盖 shader source、宏定义、entry point、shader model、编译 flags 和 backend。
- 缓存文件不能和用户游戏数据混在一起，建议放在引擎可写缓存目录或 `ShaderCache/`，并允许删除后自动重建。
- 缓存命中失败必须 fallback 到现有 runtime compile，不能让基础渲染失败。
- 缓存格式要考虑跨 driver/backend 不兼容；Vulkan、OpenGL、D3D 的产物不能混用。
- 失败时输出明确日志：cache miss、cache invalid、compile fallback、cache write failed。

实际日志：

```text
[Graphics] Shader bytecode cache miss: ShaderCache/shader_bytecode_cache_5.bin
[Graphics] Shader bytecode cache loaded: ShaderCache/shader_bytecode_cache_5.bin bytes=987708
[Graphics] Shader bytecode cache hit: CuNNy_4x24.Pass2.compute
[Graphics] Shader bytecode cache saved: ShaderCache/shader_bytecode_cache_5.bin bytes=987708
```

验证目标：

- 冷启动第一次：允许仍有 2 秒级编译，但必须写入缓存。已验证。
- 第二次启动：CuNNy-4x24 loader/shader creation 应显著下降，目标小于 300ms；理想小于 100ms。已验证为 13ms。
- 删除缓存目录后能自动回到冷编译并重新生成缓存。代码路径支持，后续可按需手动回归。
- 切换 renderer backend 后不能错误复用旧缓存。缓存文件名包含 Diligent backend type，例如当前 Vulkan 为 `shader_bytecode_cache_5.bin`。

### 方案 B：预编译 shader 中间产物

目标：构建时将 UDL/CuNNy HLSL 编译为 Diligent 可直接加载的中间产物，运行时避免 HLSL 文本编译。

适用条件：

- Diligent 当前 backend 能直接从 bytecode / SPIR-V / archive blob 创建 shader。
- 构建系统可稳定生成这些产物，并随游戏可执行文件部署。

风险：

- 不同 backend 需要不同产物，尤其 OpenGL fallback 可能需要不同路径。
- Driver/compiler 版本差异可能导致缓存兼容性问题。
- 如果产物生成流程复杂，会增加构建和部署维护成本。

建议只在方案 A 调查发现 Diligent runtime archive 不好接入时再考虑。

### 方案 C：后台预热

目标：当窗口已经显示、基础画面已 present 后，在后台或低优先级任务中预热用户可能使用的 UDL/CuNNy shader。

注意：当前不应直接做，除非先确认这些操作可安全离开 render thread：

- `IRenderDevice::CreateShader()`
- `IRenderDevice::CreateComputePipelineState()`
- `Pipeline_*::CreateBinding()`

如果 Diligent device 创建资源不是线程安全的，后台线程只能做 CPU 侧文件读取、hash、cache lookup，真正 `CreateShader()` / PSO 创建仍应切回 render thread。

可接受的折中实现：

- 后台线程准备缓存 blob 和校验信息。
- render thread 每帧只执行一个 pass 的 `CreateShader()` 或 PSO 创建，避免单帧 2.5 秒长卡顿。
- UI/日志提示 `Warming CuNNy shaders... pass 1/6`。

风险：

- 分帧预热实现复杂，会引入更多状态机。
- 如果游戏默认一启动就使用 CuNNy，仍然需要同步等待可用结果或临时 fallback。

### 推荐落地顺序

1. 阅读 Diligent Engine 文档和本仓库 Diligent 初始化代码，确认是否已有 `IShaderSourceInputStreamFactory`、archive、cache、serialized shader 支持。
2. 在 `CreateShader()` 调用点外围加更细粒度日志，区分 HLSL 读取、macro/setup、shader compile、Diligent object creation。
3. 先为一个 pass 做最小缓存原型，例如 CuNNy-4x24 Pass1。
4. 验证缓存命中后再推广到 UDL 4 pass、CuNNy 4x16 6 pass、CuNNy 4x24 6 pass。
5. 加 cache invalidation：shader source hash 或 engine build revision 变化时自动失效。
6. 保留现有 lazy loading 和 fallback 逻辑，缓存只是加速路径，不应改变画面结果。

### 新会话交接提示

继续工作时建议先执行：

```bash
cmake --build build -j$(nproc)
pkill -9 -x Game 2>/dev/null; sleep 1
cp build/app/Game /home/daiaji/下载/レトリアの大冒険_URGE/Game
```

然后启动游戏并查看：

```text
/home/daiaji/下载/レトリアの大冒険_URGE/Engine.log
```

重点看这些日志：

```text
[Graphics] Lazy loading CuNNy-4x24 compute pipelines
[Graphics] CuNNy-4x24 compute loader creation finished in ...ms
[Graphics] CuNNy-4x24 compute PSO creation finished in ...ms
[Graphics] Lazy loading CuNNy-4x24 compute pipelines finished in ...ms
```

如果新优化有效，首先应该看到 `compute loader creation` 明显下降，而不是只看总时间。

## 设计方案

### 1. 拆分 PipelineSet 中的重型 loader 生命周期

把 `PipelineSet` 中的 UDL/CuNNy loader 从直接成员改为可选成员，例如 `std::unique_ptr`：

```cpp
std::unique_ptr<Pipeline_Anime4K_UDL_Pass0> anime4k_udl_pass0;
std::unique_ptr<Pipeline_CuNNy_4x16_Pass1> cunny_4x16_p1;
```

保留基础 pipeline loader 作为直接成员，避免改动普通渲染路径。

新增确保函数：

```cpp
bool EnsureAnime4KUDLLoaders(const PipelineInitParams& init_params);
bool EnsureCuNNy4x16Loaders(const PipelineInitParams& init_params);
bool EnsureCuNNy4x24Loaders(const PipelineInitParams& init_params);
```

这些函数只在对应指针为空时构造 loader。构造失败时记录 `LOG(ERROR)` 并返回 false。

### 2. 拆分 PipelineCollection 中的重型 PSO 创建

`PipelineCollection` 构造函数中移除 UDL/CuNNy `BuildComputePipeline()`。新增确保函数：

```cpp
bool EnsureAnime4KUDLPipelines(renderer::PipelineSet* loader);
bool EnsureCuNNy4x16Pipelines(renderer::PipelineSet* loader);
bool EnsureCuNNy4x24Pipelines(renderer::PipelineSet* loader);
```

每个函数检查所有目标 PSO 是否已存在，已存在则直接返回 true；不存在则调用对应 loader 的 `BuildComputePipeline()`。

成功标准：
- UDL 必须 4 个 PSO 全部非空。
- CuNNy 4x16 必须 6 个 PSO 全部非空。
- CuNNy 4x24 必须 6 个 PSO 全部非空。

如果中途失败，记录具体 pass 名称，释放已部分创建的该组 PSO，避免半初始化状态。

### 3. 延迟创建 shader resource binding

当前 `GPUCreateGraphicsHostInternal()` 会启动时创建 UDL/CuNNy bindings。懒加载后要改为按需创建：

```cpp
bool EnsureAnime4KUDLBindingsInternal();
bool EnsureCuNNyBindingsInternal(int variant);
```

调用顺序：
- `Ensure*Loaders()`
- `Ensure*Pipelines()`
- `CreateBinding()`

binding 只在对应 PSO 成功后创建。`GPUData` 里可以继续保存现有 binding 字段，但默认保持空。

### 4. 在运行路径接入 ensure

在 `GPUScalingPassInternal()` 内进入 mode 6/7/8 前确保：

```cpp
if (mode == 6 && !EnsureAnime4KUDLReadyInternal())
  return nullptr;
if ((mode == 7 || mode == 8) && !EnsureCuNNyReadyInternal(mode))
  return nullptr;
```

`EnsureAnime4KUDLReadyInternal()` 与 `EnsureCuNNyReadyInternal()` 负责串联 loader、PSO、binding 创建，并只在首次使用时输出日志。

### 5. 失败状态缓存

为每组重型 pipeline 增加状态枚举：

```cpp
enum class LazyPipelineState { kUninitialized, kReady, kFailed };
```

失败后同一运行期间不再每帧重复尝试编译，避免卡顿和刷日志。用户切换 renderer 或重建 render device 时再重置状态。

### 6. 设备重建与资源释放

如果未来支持 render device 重建，需要同步释放懒加载资源：

- PSO：释放 `PipelineCollection` 中对应 `PipelineObject`。
- binding：释放 `GPUData` 中对应 binding。
- loader：可保留或释放；如果 loader 内持有 device shader 对象，必须释放并在新 device 上重建。

当前先按现有 device 生命周期实现，不额外引入热重建机制。

## 文件级改动清单

| 文件 | 计划改动 |
|------|----------|
| `renderer/pipeline/render_pipeline.h` | 将 UDL/CuNNy loader 改为 lazy-owned；新增 ensure loader 方法 |
| `renderer/pipeline/render_pipeline.cc` | 实现 ensure loader；保留现有 compute shader 构造逻辑 |
| `content/render/pipeline_collection.h` | 新增 `EnsureAnime4KUDLPipelines` / `EnsureCuNNy*Pipelines` |
| `content/render/pipeline_collection.cc` | 从构造函数移除重型 PSO 创建；实现分组 PSO 懒创建和失败回滚 |
| `content/screen/renderscreen_impl.h` | 新增 lazy state 字段和 `Ensure*ReadyInternal` 声明 |
| `content/screen/renderscreen_impl.cc` | 移除启动时 UDL/CuNNy binding 创建；在 mode 6/7/8 运行前按需 ensure |

## 日志要求

首次触发时打印：

```text
[Graphics] Lazy loading Anime4K UDL compute pipelines
[Graphics] Anime4K UDL compute pipelines ready
[Graphics] Lazy loading CuNNy-4x16 compute pipelines
[Graphics] CuNNy-4x16 compute pipelines ready
```

失败时打印具体 pass：

```text
[Graphics] Failed to create CuNNy-4x24 Pass5 compute pipeline
```

## 验证计划

1. 构建验证：`cmake --build build -j$(nproc)`。
2. 启动验证：`ScalingMode=0/1/2` 启动日志不应出现 UDL/CuNNy lazy loading 日志。
3. mode 6 首次切换：只加载 UDL 4 个 compute pipeline，画面正常。
4. mode 7 首次切换：只加载 CuNNy-4x16 6 个 compute pipeline，画面正常。
5. mode 8 首次切换：只加载 CuNNy-4x24 6 个 compute pipeline，画面正常。
6. 重复切换 mode 6/7/8：不重复创建已 ready 的 pipeline，不重复输出首次加载日志。
7. 失败注入：临时改错某个 shader 名称，确认失败后不会每帧重复尝试，不影响其它缩放模式。

## 风险点

- `PipelineSet` 直接成员改成指针后，所有访问点需要空指针守卫。
- `CreateBinding()` 必须在 loader 和 PSO 均成功后执行，不能在空 loader 上调用。
- 懒加载发生在首次使用时，会把启动黑屏转移为首次切换/首次进入对应模式的短暂停顿；后续可用后台预热或 Diligent archive 继续优化。
- 如果游戏默认 `ScalingMode=6/7/8`，启动仍会在第一帧创建重型 pipeline，但时机已从全局初始化推迟到实际需要路径。
