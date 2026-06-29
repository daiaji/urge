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

## 非目标

- 本计划不实现 Diligent pipeline archive / shader archive / 磁盘缓存。
- 本计划不改变 Magpie 生成 HLSL、权重、dispatch 拓扑或输出结果。
- 本计划不处理 OpenGL fallback 的 compute shader 兼容问题。

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
