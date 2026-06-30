# URGE 兼容脚本集成指南

## 文件说明

| 文件 | 用途 | 适用版本 |
|------|------|---------|
| `rgss3_patch.rb` | RGSS3 (VX Ace) RPG 数据结构 + Ruby 兼容补丁 | RGSS3 |
| `rgss2_patch.rb` | RGSS2 (VX) RPG 数据结构 + Ruby 兼容补丁 | RGSS2 |
| `rgss1_patch.rb` | RGSS1 (XP) RPG 数据结构 + Ruby 兼容补丁 | RGSS1 |
| `ruby19_compat.rb` | Ruby 1.9.2 → 3.x 通用兼容（被以上文件内联，无需单独引用） | 全部 |
| `ruby18_compat.rb` | Ruby 1.8 → 3.x 通用兼容（被 rgss1_patch 内联） | RGSS1 |

所有 `*_patch.rb` 已是自包含文件，可直接放入游戏脚本编辑器使用。

## 方式一：RM-Toolkit（推荐）

### 前置条件

- Ruby 3.0+
- RM-Toolkit（`~/repo/RM-Toolkit`）
- 已编译的 C 扩展（用于解密游戏存档）

### 快速开始

```bash
# 1. 解包游戏存档（如 Game.rgss3a）
rm-toolkit -b /path/to/game --extract-archive Game.rgss3a

# 2. 解包项目数据
rm-toolkit -b /path/to/game --unpack --rgss3

# 3. 注入 URGE 兼容补丁（自动封包）
rm-toolkit -b /path/to/game --rgss3 --inject-script 0:rgss3_patch.rb
```

### 完整脚本管理命令

所有命令统一格式 `索引:参数`：

#### 查询

```bash
# 列出所有脚本序号和名称
rm-toolkit -b /path/to/game --rgss3 --list-scripts

# 导出单个脚本到文件
rm-toolkit -b /path/to/game --rgss3 --export-script 3
rm-toolkit -b /path/to/game --rgss3 --export-script 3:custom_name.rb
```

#### 增删改

```bash
# 注入脚本（插入到指定序号，原序号及后续后移）
rm-toolkit -b /path/to/game --rgss3 --inject-script 0:rgss3_patch.rb

# 注入多个脚本（按顺序依次插入）
rm-toolkit -b /path/to/game --rgss3 \
  --inject-script 0:compat_a.rb \
  --inject-script 1:compat_b.rb

# 替换脚本内容（保留序号）
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

# 全量封包 + 同时注入补丁
rm-toolkit -b /path/to/game --pack --rgss3 \
  --inject-script 0:rgss3_patch.rb \
  --rename-script "1:原空脚本"
```

### 典型工作流

```bash
# 首次配置
rm-toolkit -b /path/to/game --extract-archive Game.rgss3a
rm-toolkit -b /path/to/game --unpack --rgss3
rm-toolkit -b /path/to/game --rgss3 \
  --inject-script 0:binding/mri/third_party/rgss/rgss3_patch.rb \
  --rename-script "1:▼ 原空スクリプト"

# 日常修改（编辑 Source/scripts/xxx.rb 后）
rm-toolkit -b /path/to/game --repack-scripts
```

## 方式二：手动集成

如果不想使用 RM-Toolkit，可直接在 RPG Maker 脚本编辑器中操作：

1. 打开 RPG Maker 的脚本编辑器
2. 在最顶部创建新脚本，命名为 `URGE Patch`
3. 将对应版本的 `rgssN_patch.rb` 全部内容粘贴进去
4. 保存并导出游戏

### 注意事项

- RGSS 脚本按顺序依次执行，补丁必须在引用 RPG 数据结构的脚本**之前**加载
- `Font.default_name` 需使用**文件名**（含扩展名），如 `"LGBaseFont.ttf"`，而非字族名
- `Win32API` 调用在 URGE 上不可用，需用 `rescue` 包裹或替换为纯 Ruby 实现
- `BasicObject#initialize` 覆写会导致 Ruby 3.3 segfault，`ruby19_compat.rb` 中已移除此补丁
- `require_relative` 在 RGSS 上下文中不可用（无文件系统路径），补丁文件已内联所有依赖

## 游戏配置

URGE 读取 `Game.ini`，至少需要以下配置：

```ini
[Game]
Scripts=Data\Scripts.rvdata2
Title=游戏标题

[Engine]
DefaultFontPath=Fonts/LGBaseFont.ttf
```

`DefaultFontPath` 指定字体搜索目录和默认字体。URGE 会加载该目录下的所有字体文件。
