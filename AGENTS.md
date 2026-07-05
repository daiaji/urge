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
bundle exec exe/rm-toolkit -b "$GAME" --rgss3 --list-scripts

# 1. 首次注入用 --inject-script
URGE=/home/daiaji/repo/urge
bundle exec exe/rm-toolkit -b "$GAME" --rgss3 \
  --inject-script "0:${URGE}/binding/mri/third_party/rgss/urge_compat.rb" \
  --inject-script "1:${URGE}/binding/mri/third_party/rgss/ruby19_compat.rb" \
  --inject-script "2:${URGE}/binding/mri/third_party/rgss/rgss3_patch.rb" \
  --inject-script "3:${URGE}/binding/mri/third_party/rgss/rgss3_compat.rb"
```

⚠️ **不要重复 `--inject-script`**：每次会在脚本数组中插入新条目，多次注入会产生重复脚本。更新已有补丁用 `--replace-script` 替换对应索引位置。

```bash
# 更新前先查看脚本列表确认索引
bundle exec exe/rm-toolkit -b "$GAME" --rgss3 --list-scripts

# 更新脚本用 --replace-script（保留序号，不会新增条目）
bundle exec exe/rm-toolkit -b "$GAME" --rgss3 \
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

## 提交前检查清单

- [ ] `git diff --stat` — 确认没有无关文件（PNG、build 产物等）
- [ ] `git diff` — 检查悬空代码、死分支、未使用变量
- [ ] 如果修改了 `content/public/*.h`，确认 `autogen.py` 已重新运行
- [ ] 编译通过（`cmake --build build -j$(nproc)`）
- [ ] 测试游戏运行正常（启动、字体渲染、音频播放）
