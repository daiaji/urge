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
# 使用 RM-Toolkit 注入补丁
GAME=/path/to/game
URGE=/home/daiaji/repo/urge
cd ~/repo/RM-Toolkit
bundle exec exe/rm-toolkit -b "$GAME" --rgss3 \
  --inject-script "0:${URGE}/binding/mri/third_party/rgss/urge_compat.rb" \
  --inject-script "1:${URGE}/binding/mri/third_party/rgss/ruby19_compat.rb" \
  --inject-script "2:${URGE}/binding/mri/third_party/rgss/rgss3_patch.rb" \
  --inject-script "3:${URGE}/binding/mri/third_party/rgss/rgss3_compat.rb"
```

脚本加载顺序必须为：`urge_compat.rb` → `ruby19_compat.rb` → `rgss*_patch.rb` → `rgss3_compat.rb`。

## 提交前检查清单

- [ ] `git diff --stat` — 确认没有无关文件（PNG、build 产物等）
- [ ] `git diff` — 检查悬空代码、死分支、未使用变量
- [ ] 如果修改了 `content/public/*.h`，确认 `autogen.py` 已重新运行
- [ ] 编译通过（`cmake --build build -j$(nproc)`）
- [ ] 测试游戏运行正常（启动、字体渲染、音频播放）
