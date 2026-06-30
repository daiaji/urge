# URGE Gamepad Support

已在源码中集成完整的手柄支持。以下是在 URGE (`urge_repo`) 中的修改。

## 修改的文件

```
app/urge_main.cc                     |   2 +-
content/input/keyboard_controller.cc | 168 ++++++++++++++++++++++++++-
content/input/keyboard_controller.h  |  18 +++
3 files changed, 186 insertions(+), 2 deletions(-)
```

## 实现的功能

### 1. 手柄连接/轮询 (`UpdateGamepad()`)

每帧检测手柄连接状态，自动打开第一个可用手柄。手柄断开后自动清理并重连。

```cpp
void KeyboardControllerImpl::UpdateGamepad() {
  // 检测断开
  if (gamepad_ && !SDL_GamepadConnected(gamepad_)) { ... }
  // 尝试打开第一个手柄
  if (!gamepad_) {
    SDL_JoystickID* sticks = SDL_GetJoysticks(&num_joysticks);
    for (int i = 0; i < num_joysticks; ++i)
      if (SDL_IsGamepad(sticks[i]))
        gamepad_ = SDL_OpenGamepad(sticks[i]);
  }
  // 轮询轴和按钮
  for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; ++a)
    gp_axes_[a] = SDL_GetGamepadAxis(gamepad_, axis);
  for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b)
    gp_buttons_[b] = SDL_GetGamepadButton(gamepad_, button);
}
```

### 2. 按钮映射表

| SDL_GamepadButton | RGSS 按键 |
|---|---|
| A (底键) | C (确认) |
| B (右键) | B (取消) |
| X (左键) | A |
| Y (上键) | X |
| LSB / RSB | Y / Z |
| LB / RB | L / R |
| Back | SHIFT |
| DPad 上下左右 | UP / DOWN / LEFT / RIGHT |

### 3. 摇杆

左摇杆 X/Y 轴通过死区 (`0x4000` = 16384, ~50%) 转换为方向输入，合并到
`Dir4()` / `Dir8()` / `IsPressed("UP")` 等接口。

### 4. 按键重复 (Repeat)

手柄独立维护重复计数：
- 首次按下 = trigger
- 第 0 帧和第 23 帧后每 6 帧 = repeat
- 与键盘逻辑一致 (`keyboard_controller.cc:219-222`)

### 5. `gamecontrollerdb.txt` 支持

启动时自动从游戏目录加载 `gamecontrollerdb.txt`（社区维护的映射数据库）。
可从此处下载：https://raw.githubusercontent.com/gabomdq/SDL_GameControllerDB/master/gamecontrollerdb.txt

把下载的文件放到游戏目录下即可，缺失不影响运行。

### 6. 按钮/轴名称列表

```cpp
// 21 个按钮
"B", "X", "Y", "Back", "Guide", "Start",
"LS", "RS", "LB", "RB",
"DPad_Up", "DPad_Down", "DPad_Left", "DPad_Right",
"Misc", "Paddle1", "Paddle2", "Paddle3", "Paddle4", "Touchpad"

// 6 个轴
"LStick_X", "LStick_Y", "RStick_X", "RStick_Y", "LTrigger", "RTrigger"
```

## 参考实现

mkxp-z (`/home/daiaji/repo/mkxp-z/`) 提供了完整参考：

| mkxp-z | 对应功能 |
|---|---|
| `src/eventthread.cpp` | 手柄事件处理、gamecontrollerdb 加载 |
| `src/input/keybindings.cpp` | 默认绑定表、持久化 |
| `src/input/input.cpp` | `CtrlAxisBinding`、`CtrlButtonBinding` |
| `src/settingsmenu.cpp` | 设置菜单手柄按键捕获 |
| `assets/gamecontrollerdb.txt` | 社区手柄映射库 |

SDL2 → SDL3 API 变化已在代码中适配。

## 后续可做

- **设置 UI 绑定页**：在 ImGui 设置菜单中增加手柄按键捕获页面
- **可配置映射持久化**：扩展 `.cfg` 文件格式，支持手柄按钮/轴映射保存
- **多手柄**：支持 2P 同时连接
- **力反馈**：`SDL_RumbleGamepad()` 支持手柄振动
