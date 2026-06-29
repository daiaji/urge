# URGE 兼容脚本集成指南

## 文件说明

| 文件 | 用途 | 适用版本 |
|------|------|---------|
| `rgss3_patch.rb` | RGSS3 (VX Ace) RPG 数据结构 + Ruby 兼容补丁 | RGSS3 |
| `rgss2_patch.rb` | RGSS2 (VX) RPG 数据结构 + Ruby 兼容补丁 | RGSS2 |
| `rgss1_patch.rb` | RGSS1 (XP) RPG 数据结构 + Ruby 兼容补丁 | RGSS1 |
| `urge_compat.rb` | URGE 通用兼容层（Kernel 重定向、caller 重写等，最先加载） | RGSS1/2/3 |
| `rgss3_compat.rb` | RGSS3 行为兼容层（与 rgss3_patch.rb 配合，单独注入） | RGSS3 |
| `ruby19_compat.rb` | Ruby 1.9.2 → 3.x 通用兼容（需单独注入，非 RGSS3 必需） | RGSS2/3 |
| `ruby18_compat.rb` | Ruby 1.8 → 3.x 通用兼容（需单独注入，非 RGSS3 必需） | RGSS1 |

补丁文件通过 `rm-toolkit --inject-script` 按顺序注入到脚本数组，
**不是**通过 `require_relative` 链式加载（RGSS 的 `eval` 上下文无文件系统路径）。

### 加载顺序

```
urge_compat.rb → ruby19_compat.rb → rgss3_patch.rb → rgss3_compat.rb
```

## 方式一：RM-Toolkit（推荐）

### 前置条件

- Ruby 3.0+
- RM-Toolkit（`~/repo/RM-Toolkit`）
- 已编译的 C 扩展（用于解密游戏存档）

### 快速开始

```bash
# 1. 解包游戏存档（如 Game.rgss3a）
cd ~/repo/RM-Toolkit
bundle exec exe/rm-toolkit -b /path/to/game --extract-archive Game.rgss3a

# 2. 解包项目数据
bundle exec exe/rm-toolkit -b /path/to/game --unpack --rgss3

# 3. 查看脚本列表（输出取 stdout，格式: 序号|名称）
bundle exec exe/rm-toolkit -b /path/to/game --rgss3 --list-scripts

# 4. 注入 URGE 兼容补丁（--inject-script 会自动重新封包）
bundle exec exe/rm-toolkit -b /path/to/game --rgss3 \
  --inject-script "0:/absolute/path/to/urge_compat.rb" \
  --inject-script "1:/absolute/path/to/ruby19_compat.rb" \
  --inject-script "2:/absolute/path/to/rgss3_patch.rb" \
  --inject-script "3:/absolute/path/to/rgss3_compat.rb"
```

### 完整脚本管理命令

所有命令统一格式 `索引:参数`：

#### 查询

```bash
# 列出所有脚本序号和名称（stdout，格式: 序号|名称）
rm-toolkit -b /path/to/game --rgss3 --list-scripts

# 导出单个脚本到文件（stdout，格式: 序号|名称|路径|字节数）
rm-toolkit -b /path/to/game --rgss3 --export-script 3
rm-toolkit -b /path/to/game --rgss3 --export-script 3:custom_name.rb
```

#### 增删改

```bash
# ⚠️ 首次注入用 --inject-script（插入到指定序号，原序号及后续后移）
#    切勿在同一份 Scripts.rvdata2 上多次 --inject-script，否则会重复
rm-toolkit -b /path/to/game --rgss3 --inject-script 0:patch.rb

# 注入多个脚本（按顺序依次插入）
rm-toolkit -b /path/to/game --rgss3 \
  --inject-script 0:compat_a.rb \
  --inject-script 1:compat_b.rb

# ✅ 更新已有脚本用 --replace-script（保留序号，不会新增条目）
#    适用于仓库中的补丁文件变更后，更新到游戏
rm-toolkit -b /path/to/game --rgss3 --replace-script 5:fixed_script.rb

# 创建空脚本
rm-toolkit -b /path/to/game --rgss3 --create-script 2

# 清空脚本内容（保留位置和名称）
rm-toolkit -b /path/to/game --rgss3 --clear-script 7

# 删除脚本
rm-toolkit -b /path/to/game --rgss3 --remove-script 42

# 批量删除空脚本
rm-toolkit -b /path/to/game --rgss3 --prune-empty-scripts

# 重命名脚本
rm-toolkit -b /path/to/game --rgss3 --rename-script "3:新名称"

# 移动脚本
rm-toolkit -b /path/to/game --rgss3 --move-script 10:3
```

#### 封包

```bash
# 全量封包（脚本 + 所有数据文件）
rm-toolkit -b /path/to/game --pack --rgss3

# 仅封包脚本（手动修改 Source/scripts/ 后使用）
rm-toolkit -b /path/to/game --repack-scripts
```

### 典型工作流

```bash
GAME=/path/to/game
URGE=/path/to/urge/repo

cd ~/repo/RM-Toolkit

# 首次配置
bundle exec exe/rm-toolkit -b "$GAME" --extract-archive Game.rgss3a
bundle exec exe/rm-toolkit -b "$GAME" --unpack --rgss3

# 查看脚本列表确认结构
bundle exec exe/rm-toolkit -b "$GAME" --rgss3 --list-scripts

# 注入补丁（加载顺序：URGE 通用 → Ruby 兼容 → 数据结构 → RGSS3 行为）
bundle exec exe/rm-toolkit -b "$GAME" --rgss3 \
  --inject-script "0:${URGE}/binding/mri/third_party/rgss/urge_compat.rb" \
  --inject-script "1:${URGE}/binding/mri/third_party/rgss/ruby19_compat.rb" \
  --inject-script "2:${URGE}/binding/mri/third_party/rgss/rgss3_patch.rb" \
  --inject-script "3:${URGE}/binding/mri/third_party/rgss/rgss3_compat.rb"

# 更新 URGE 补丁（当仓库中的脚本文件变更时，替换对应索引位置）
bundle exec exe/rm-toolkit -b "$GAME" --rgss3 \
  --replace-script "3:${URGE}/binding/mri/third_party/rgss/rgss3_compat.rb"
# （只替换变更的脚本，不会新增条目；切勿重复 --inject-script，会导致重复）
```

## 方式二：手动集成

如果不想使用 RM-Toolkit，可直接在 RPG Maker 脚本编辑器中操作：

1. 打开 RPG Maker 的脚本编辑器
2. 在最顶部创建新脚本，命名为 `URGE Base`，粘贴 `urge_compat.rb`
3. 创建第二个脚本，命名为 `Ruby Compat`，粘贴 `ruby19_compat.rb`
4. 创建第三个脚本，命名为 `RGSS Patch`，粘贴 `rgss3_patch.rb`
5. 创建第四个脚本，命名为 `RGSS Compat`，粘贴 `rgss3_compat.rb`
6. 保存并导出游戏

## 游戏配置

URGE 读取 `Game.ini`，以下是完整配置示例：

```ini
[Game]
Scripts=Data\Scripts.rvdata2
Title=游戏标题

[Engine]
; 必填：字体搜索目录和默认字体文件
DefaultFontPath=Fonts/LGBaseFont.ttf
; 可选：字体缩放系数（默认 0.9）
FontScale=0.9
; 可选：字距调整（默认 true）
FontKerning=true
; 可选：字体抗锯齿模式 0-3（默认 3 = TTF_HINTING_NONE）
FontHinting=3
; 可选：描边裁切（默认 true）
FontOutlineCrop=true
; 可选：字体替换表，逗号分隔，格式 "原名>替换名"
; 游戏脚本中使用 "SimHei"、"VL Gothic" 等字族名时，
; URGE 需要文件名（含扩展名）才能找到字体。
FontSubs=lgbasefont>LGBaseFont.ttf, simhei>LGBaseFont.ttf, microsoft yahei>LGBaseFont.ttf, ms pgothic>LGBaseFont.ttf, vl gothic>VL-Gothic-Regular.ttf
```

### 字体说明

- `DefaultFontPath` 指定字体目录和默认字体文件。
- URGE 会加载该目录下的**所有字体文件**（`.ttf`/`.otf`），放入内存缓存。
- `Font.default_name` 使用**文件名（含扩展名）**，如 `"LGBaseFont.ttf"`。
- 游戏脚本中使用字族名（如 `"SimHei"`、`"VL Gothic"`）时，通过 `FontSubs` 映射到文件名。

### 字体替换配置

常见中文字体映射（根据游戏实际使用的字体名调整）：

```ini
[Engine]
FontSubs=simhei>LGBaseFont.ttf, microsoft yahei>LGBaseFont.ttf, ms pgothic>LGBaseFont.ttf, vl gothic>VL-Gothic-Regular.ttf, lgbasefont>LGBaseFont.ttf
```

## 注意事项

- RGSS 脚本按顺序依次执行，补丁必须在引用 RPG 数据结构的脚本**之前**加载
- 加载顺序：`urge_compat.rb` → `ruby19_compat.rb` → `rgss3_patch.rb` → `rgss3_compat.rb`
- `Win32API` 调用在 URGE 上不可用，需用 `rescue` 包裹或替换为纯 Ruby 实现
- `BasicObject#initialize` 覆写会导致 Ruby 3.3 segfault，`ruby19_compat.rb` 中已移除此补丁
- `require_relative` 在 RGSS 上下文中不可用（无文件系统路径），补丁文件之间不相互依赖
