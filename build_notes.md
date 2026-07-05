# URGE 编译经验记录

## 项目概况

URGE 是一个基于 CMake + C++20 的游戏引擎项目，使用 Visual Studio 2022 构建。

## 编译步骤

```powershell
# 1. 拉取更新
git pull

# 2. 编译（使用 CMake 调用 MSBuild）
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release
```

## 说明

- 项目已预先配置好 CMake 构建目录（`build/`），生成器为 Visual Studio 2022。
- CMake 路径：`C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`
- MSBuild 路径：`C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe`
- 如果 cmake 不在 PATH 中，需要用完整路径调用。

## 构建输出

- 主可执行文件：`build/app/Release/Game.exe`
- 运行时 DLL（SDL3.dll）：`build/third_party/SDL/Release/SDL3.dll`

## 部署

编译后将产物复制到游戏目录即可：

```powershell
Copy-Item -Force build/app/Release/Game.exe 'D:\レトリアの大冒険_URGE\Game.exe'
Copy-Item -Force build/third_party/SDL/Release/SDL3.dll 'D:\レトリアの大冒険_URGE\SDL3.dll'
```

## 注意事项

- 每次 `git pull` 后如果源文件有变更，需要重新编译。
- 编译 Release 配置时会有一些类型转换警告（C4244、C4267、C4996），均为已知警告，不影响运行。
- 生成 PDB 调试符号在 Release 构建中也会生成（Game.pdb）。
