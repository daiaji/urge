# URGE 引擎开发指南

## 项目哲学

URGE 定位与 **RGD** 类似，面向**游戏作者**而非终端玩家。核心原则：

| 原则 | 说明 |
|------|------|
| **引擎只提供通用能力** | C++ 层内置基础渲染/输入/音频/配置，不做 RGSS 特化 |
| **RGSS 兼容在 Ruby 脚本层** | 类似 `rgss_patch`，由游戏作者在脚本编辑器中主动插入 |
| **不内置 RGSS 数据结构** | 规避法律风险，不直接嵌入 Enterbrain 的加密/数据格式 |
| **不追求免修改运行** | 不考虑"玩家下载原版游戏直接用 URGE 打开"的场景 |

据此，功能应分类到对应层次：

- **引擎 C++ 层** — 通用能力：渲染管线、字体排版、音频输出、输入管理、网络、文件 I/O。Ruby 做不到的才进引擎。
- **脚本层** (`binding/mri/third_party/rgss/`) — RGSS 特化行为：`rgss3_compat.rb` 覆盖 `draw_text` 对齐、squeeze、`Font.default_shadow_color` 等。所有 RPG 数据结构（`RPG::Map`、`RPG::Actor` 等）放在 `rgss*_patch.rb`。

## 构建与部署

```bash
# 1. 重新生成 MRI 绑定（修改 content/public/*.h 后需要）
python3 -B binding/mri/tools/autogen.py content/public/ build/gen/binding/mri/

# 2. 多线程编译
cmake --build build -j$(nproc)

# 3. 杀死正在运行的 Game 进程
pkill -9 -x Game 2>/dev/null; sleep 1

# 4. 复制产物
cp build/app/Game /home/daiaji/下载/レトリアの大冒険_URGE/Game
```

或直接运行：
```bash
bash /home/daiaji/repo/urge/build_and_deploy.sh
```

## 脚本集成

如果需要更新 Ruby 兼容脚本并集成到游戏中，阅读 `INTEGRATION.md`。核心流程：

```bash
# 0. 先查看当前脚本列表，确认索引
GAME=/path/to/game
cd ~/repo/RM-Toolkit
bundle exec exe/rm-toolkit -b "$GAME" --rgss3 --allow-unsafe-marshal --list-scripts

# 1. 首次注入用 --inject-script
URGE=/home/daiaji/repo/urge
bundle exec exe/rm-toolkit -b "$GAME" --rgss3 \
  --allow-unsafe-marshal \
  --inject-script "0:${URGE}/binding/mri/third_party/rgss/urge_compat.rb" \
  --inject-script "1:${URGE}/binding/mri/third_party/rgss/ruby19_compat.rb" \
  --inject-script "2:${URGE}/binding/mri/third_party/rgss/rgss3_patch.rb" \
  --inject-script "3:${URGE}/binding/mri/third_party/rgss/rgss3_compat.rb"
```

RM-Toolkit 读取 RGSS `.rxdata` / `.rvdata` / `.rvdata2` 时会使用 Ruby `Marshal.load`。只对可信来源的游戏项目追加 `--allow-unsafe-marshal`；纯 archive 提取不需要该参数。

⚠️ **不要重复 `--inject-script`**：每次会在脚本数组中插入新条目，多次注入会产生重复脚本。更新已有补丁用 `--replace-script` 替换对应索引位置。

```bash
# 更新前先查看脚本列表确认索引
bundle exec exe/rm-toolkit -b "$GAME" --rgss3 --allow-unsafe-marshal --list-scripts

# 更新脚本用 --replace-script（保留序号，不会新增条目）
bundle exec exe/rm-toolkit -b "$GAME" --rgss3 \
  --allow-unsafe-marshal \
  --replace-script "3:${URGE}/binding/mri/third_party/rgss/rgss3_compat.rb"
```

脚本加载顺序必须为：`urge_compat.rb` → `ruby19_compat.rb` → `rgss*_patch.rb` → `rgss3_compat.rb`。

## 构建类型

| 类型 | 调试符号 | 优化 | ImGui assertion | 用途 |
|------|---------|------|:---------------:|------|
| `Debug` | ✅ | `-O0` | ✅ 生效 | 开发调试 |
| `RelWithDebInfo` | ✅ | `-O2` | ❌ 禁用 | 带符号的发布 |
| `Release` | ❌ | `-O3` | ❌ 禁用 | 最终发布 |

ImGui 控制台使用 `CallbackHistory + InputTextMultiline` 组合。`IM_ASSERT` 只在 `Debug` 构建下生效，`RelWithDebInfo` 和 `Release` 下 `assert()` 为空操作。如果控制台打开崩溃，检查构建类型。

```bash
# 切换构建类型
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j$(nproc)
```

## 常见陷阱

- **ABGR8888 像素格式**：手动构造像素时使用 `SDL_GetRGBA` / `SDL_MapRGBA`，不要手动 `<< 16 / << 8` 移位，R/B 通道顺序容易搞反。
- **git stash drop 前先看内容**：`git stash show -p` 确认 stash 里是什么再决定是否丢弃。
- **Source/scripts/ 目录**：注入脚本前先清除旧的 `Source/scripts/`，避免残留文件干扰。`rm -f Source/scripts/*.rb`
- **Diligent shader 编译缓存**：优化运行时 shader creation 卡顿时，优先使用 `IBytecodeCache` 缓存 `IShader::GetBytecode()` 结果，并通过 `ShaderCreateInfo::ByteCode/ByteCodeSize` 回放。缓存文件按 backend 分离（如 `ShaderCache/shader_bytecode_cache_<backend>.bin`），命中失败必须移除该条缓存并 fallback 到 HLSL source compile。当前不要优先接 `IRenderStateCache`/Archiver；该路径在 URGE 现有接入下曾出现 `WriteToBlob()` 序列化失败。
- **shader cache 验证闭环**：首次运行应看到 cache miss 并在退出时写出缓存；第二次运行应看到 cache loaded/hit，且重型 shader loader 耗时显著下降。不要只看是否生成文件，也要检查 `Create compute shader ... finished in ...ms` 和 lazy loading 总耗时。

## 日志排查

- 普通日志是 append 模式，文件开头可能是旧运行记录。先找最新的 session 分隔线，再只看它后面的内容：
  - `========== URGE session start run=YYYYMMDD-HHMMSS-PID pid=PID cwd=... config=... ==========`
  - `run=` 是一次启动的关联 ID，`Engine.log` / `Script.log` / `Console.log` 同一次运行会写入相同 `run`。
  - 普通日志每行带时间戳：`[YYYY-MM-DD HH:MM:SS.mmm] [level] ...`，可用来判断事件先后顺序。
- `Engine.log`：引擎内部日志，优先看渲染、音频、输入、配置、资源加载等 C++ 侧问题。
- `Script.log`：Ruby/RGSS 脚本诊断，优先看脚本 warning、`Console.puts` 输出、未捕获 Ruby 异常和完整 backtrace。
- `Console.log`：游戏内控制台 overlay 的持久化副本，适合看用户可见脚本输出和 Ruby 异常摘要。
- `crash.log`：native 崩溃日志，只用于 SIGSEGV、SIGABRT、Windows SEH 等 C/C++ 层 fatal crash；普通 Ruby `ZeroDivisionError`/`NoMethodError` 不应写到这里。
- 遇到 Ruby 脚本报错时先看 `Script.log`，再看 `Console.log` 摘要；遇到引擎直接崩溃或无 Ruby 弹窗退出时先看 `crash.log`，其中会包含最近日志 breadcrumbs。
- `crash.log` 仍按 native crash 路径写出，不要求和普通 spdlog 格式完全一致；其中的 recent breadcrumbs 会包含最近普通日志内容，用于定位崩溃前上下文。

## 提交前检查清单

- [ ] `git diff --stat` — 确认没有无关文件（PNG、build 产物等）
- [ ] `git diff` — 检查悬空代码、死分支、未使用变量
- [ ] 如果修改了 `content/public/*.h`，确认 `autogen.py` 已重新运行
- [ ] 编译通过（`cmake --build build -j$(nproc)`）
- [ ] 测试游戏运行正常（启动、字体渲染、音频播放）

<!-- gitnexus:start -->
# GitNexus — Code Intelligence

This project is indexed by GitNexus as **urge** (8124 symbols, 13035 relationships, 283 execution flows). Use the GitNexus MCP tools to understand code, assess impact, and navigate safely.

> Index stale? Run `node .gitnexus/run.cjs analyze` from the project root — it auto-selects an available runner. No `.gitnexus/run.cjs` yet? `npx gitnexus analyze` (npm 11 crash → `npm i -g gitnexus`; #1939).

## Always Do

- **MUST run impact analysis before editing any symbol.** Before modifying a function, class, or method, run `impact({target: "symbolName", direction: "upstream"})` and report the blast radius (direct callers, affected processes, risk level) to the user.
- **MUST run `detect_changes()` before committing** to verify your changes only affect expected symbols and execution flows. For regression review, compare against the default branch: `detect_changes({scope: "compare", base_ref: "main"})`.
- **MUST warn the user** if impact analysis returns HIGH or CRITICAL risk before proceeding with edits.
- When exploring unfamiliar code, use `query({query: "concept"})` to find execution flows instead of grepping. It returns process-grouped results ranked by relevance.
- When you need full context on a specific symbol — callers, callees, which execution flows it participates in — use `context({name: "symbolName"})`.

## Never Do

- NEVER edit a function, class, or method without first running `impact` on it.
- NEVER ignore HIGH or CRITICAL risk warnings from impact analysis.
- NEVER rename symbols with find-and-replace — use `rename` which understands the call graph.
- NEVER commit changes without running `detect_changes()` to check affected scope.

## Resources

| Resource | Use for |
|----------|---------|
| `gitnexus://repo/urge/context` | Codebase overview, check index freshness |
| `gitnexus://repo/urge/clusters` | All functional areas |
| `gitnexus://repo/urge/processes` | All execution flows |
| `gitnexus://repo/urge/process/{name}` | Step-by-step execution trace |

## CLI

| Task | Read this skill file |
|------|---------------------|
| Understand architecture / "How does X work?" | `.claude/skills/gitnexus/gitnexus-exploring/SKILL.md` |
| Blast radius / "What breaks if I change X?" | `.claude/skills/gitnexus/gitnexus-impact-analysis/SKILL.md` |
| Trace bugs / "Why is X failing?" | `.claude/skills/gitnexus/gitnexus-debugging/SKILL.md` |
| Rename / extract / split / refactor | `.claude/skills/gitnexus/gitnexus-refactoring/SKILL.md` |
| Tools, resources, schema reference | `.claude/skills/gitnexus/gitnexus-guide/SKILL.md` |
| Index, status, clean, wiki CLI commands | `.claude/skills/gitnexus/gitnexus-cli/SKILL.md` |

<!-- gitnexus:end -->
