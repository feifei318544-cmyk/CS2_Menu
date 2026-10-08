# CS2 内部辅助 DLL · 项目说明

> 本文档为 CS2 内部辅助项目的交接与使用说明。
> 项目包含 **静态自瞄**、**ESP 透视**、**ImGui 菜单** 三大模块，基于 DX11 Present Hook 实现。

---

## 一、项目概述

- **类型**：CS2 内部辅助 DLL
- **功能**：
   静态自瞄（准星可见转动）
   ESP 透视（方框 + 顶部射线 + 血量 + 名称）
   ImGui 菜单（DX11 Present hook）

### 当前状态

| 模块 | 状态 |
|---|---|
| 静态自瞄 |  已跑通 |
| 透视 |  已跑通（方框、射线、血量、名称） |
| 菜单 |  已跑通 |

---

## 二、技术栈

| 项目 | 说明 |
|---|---|
| 语言 | C++17 |
| 构建 | CMake + Visual Studio 2022（MSVC 14.51） |
| 注入方式 | DLL 注入 |
| Hook 框架 | MinHook（Tsuda Kageyu 原版 2009-2017，x64d 调试版） |
| 菜单 UI | ImGui（DX11 backend） |
| 目标进程 | `cs2.exe` |
| 目标模块 | `client.dll` |

---

## 三、目录结构

```
CS_menu/
├── CMakeLists.txt
├── dll_main.cpp              # DllMain + Present hook + ImGui 初始化
├── aimbot/
│   ├── aimbot.hpp
│   └── aimbot.cpp            # 静态自瞄核心逻辑
├── visual/
│   ├── visual.hpp
│   └── visual.cpp            # ESP 绘制
├── menu/
│   ├── menu.hpp
│   └── menu.cpp              # ImGui 菜单
├── game/
│   ├── entity.hpp            # Vec3 + 实体相关接口声明
│   └── entity.cpp            # 实体扫描/读取实现
├── tools/
│   ├── memory.hpp            # 读写内存封装
│   ├── memory.cpp
│   ├── schema.hpp            # 类字段偏移（本地手写）
│   ├── console.hpp           # 控制台开关
│   ├── console.cpp
│   └── scanner.hpp           # pattern scan（暂未使用）
├── imgui_d11/                # ImGui 源码
└── minhook_debug_x64/        # MinHook 库
```

---

## 四、编译与注入

### 1. 编译

使用 CMake + Visual Studio 2022：

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

生成 `d3d11_hook.dll`。

> **注意**：编译时可能出现 `LINK : warning LNK4098`（MSVCRTD 冲突），可忽略，不影响运行。

### 2. 注入

使用任意 DLL 注入器（如 Xenos、Extreme Injector）注入到 `cs2.exe`。

> **建议**：在游戏进入主界面后注入，避免在启动阶段注入导致崩溃。

---

## 五、使用说明

### 1. 快捷键

| 按键 | 功能 |
|---|---|
| **F11** | 开启 / 关闭菜单 |

**修改快捷键**：在 `dll_main.cpp` 中找到 GetAsyncKeyState(VK_F11)`，把 `VK_F11` 改成 `VK_INSERT`。

```cpp
// 原代码
bool insert_now = (GetAsyncKeyState(VK_F11) & 0x8000) != 0;

// 改成 INSERT
bool insert_now = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
```

### 2. 菜单功能

**自瞄**

- 勾选「暴力自瞄 (慎用)」开启
- 滑条「范围」：自瞄圆圈半径（像素）
- 滑条「最大距离」：最大世界距离（游戏单位）

**透视**

- 勾选「开启透视」
- 子项：显示血量 / 显示射线 / 显示名称

### 3. 参数建议

| 参数 | 默认值 | 建议值 | 说明 |
|---|---|---|---|
| `g_circle_radius` | 320 | 100 ~ 200 | 圆圈越小越接近「看得见才锁」 |
| `g_max_world_dist` | 5000 | 1500 | 距离越小越不容易锁到墙后目标 |

---

## 六、核心机制简述

### 1. 静态自瞄

- 每帧扫描所有敌人
- 找出屏幕中心圆内最近的敌人
- 计算角度 → 直接写 `dwViewAngles`（QAngle，2 个 float）
- 游戏自身旋转视角（准星可见转动）

### 2. 世界坐标转屏幕坐标

```cpp
w = m[12]*x + m[13]*y + m[14]*z + m[15]
if (w < 0.65) return false;

sx = m[0]*x + m[1]*y + m[2]*z + m[3]
sy = m[4]*x + m[5]*y + m[6]*z + m[7]

screen_x = (sw * 0.5) + (sx / w) * (sw * 0.5)
screen_y = (sh * 0.5) - (sy / w) * (sh * 0.5)
```

### 3. 角度计算

```cpp
dx = target.x - local.x
dy = target.y - local.y
dz = target.z - local.z
hyp = sqrt(dx*dx + dy*dy)

yaw   = atan2(dy, dx) * 180 / PI
pitch = -atan2(dz, hyp) * 180 / PI
```

### 4. ESP 绘制

- 敌人方框（绿色）：头顶 `origin + (0,0,72)`，脚底 `origin`
- 顶部射线（绿色）：屏幕顶边中心 → 方框顶部中心
- 血量（黄色）：方框底部下方
- 名称（青色）：方框顶部上方

### 5. Present Hook

1. `DllMain` 中 `MH_Initialize`
2. `CreateThread` 启动 `create()` 线程
3. `create()` 中：
   - 创建临时窗口
   - `D3D11CreateDeviceAndSwapChain` 拿临时设备
   - 从 `vtable[8]` 拿 `Present` 地址
   - `MH_CreateHook` + `MH_EnableHook`
   - 销毁临时窗口
4. `my_present` 中：
   - 首次调用初始化 ImGui
   - 每帧：`NewFrame` → 绘制 → `Render` → 调原始 `Present`

---

## 七、偏移更新说明

- `offsets.hpp` 来自 `cs2-dumper`，最后同步时间：**2026-10-06 17:44:59 UTC**
- **每次游戏更新后必须重新跑 dumper 同步**
- `schema.hpp` 中的字段偏移本次更新未变，可继续使用
- 如果游戏更新后自瞄/透视失效，首先检查 `offsets.hpp` 是否过期

> **提示**：`dwEntityList`、`dwLocalPlayerPawn`、`dwViewAngles`、`dwViewMatrix` 等关键偏移每次大更新都会变。

---

## 八、已知限制与坑

### 1. 静默自瞄不可行（已放弃）

- 试过 hook `CPlayerMove::RunCommand`、`CPlayerMove::PostMove`、`CCSGOInput::CreateMove`、`CPlayer_MovementServices::DoMovement::Subtick`
- 所有尝试要么不生效，要么跑动生效静止不生效
- 根本原因：`cmd` 里的 `viewangles` 是「输入记录」，游戏会用 `CCSGOInput` 的 frame history 覆盖，服务器读的是最终提交的 `CUserCmd`

### 2. `m_bSpottedByMask` 不可用

- 偏移 `0x1E94`（= `0x1E88 + 0xC`），bit 索引是 player slot 不是 entity index
- 服务器同步有 1 tick 延迟
- 结果和客户端渲染不一致
- **结论**：不要用它做可见性

### 3. trace 函数调用不可行

- 找到 `CGamePhysicsQueryInterface::TraceLineIntervals`（RVA `0xA1A530`）
- 调用需要构造 `CTraceFilter` + `CGameTrace`，布局复杂
- 容易崩
- **结论**：不做 trace

### 4. 切换分辨率/窗口模式后菜单消失

- 原因：`ResizeBuffers` 重建后备缓冲区后，旧的 `g_mainRTV` 变成野指针
- 尝试过 Hook `ResizeBuffers`（MinHook 和虚表替换都试过），**会崩溃**
- 最终方案：**不 Hook `ResizeBuffers`**，接受「切换后菜单消失」的限制
- **解决办法**：切换到目标分辨率/窗口模式后，**重新注入 DLL**

### 5. MinHook 原版的坑

- 对 `call rel32` 的 trampoline 不会重定位 rel32
- 对 `mov [rsp+...]` 序言处理有 bug
- Hook 函数入口尽量选普通序言（`mov rax, rsp` / `push` / `sub rsp`）

### 6. DX11 hook 前的临时窗口必须销毁

- 否则游戏关闭时残留

### 7. 编译时 MSVCRTD 冲突

- `LINK : warning LNK4098`
- 可以忽略，不影响运行

### 8. F11 不要用作菜单快捷键

- F11 在 CS2 中是「切换全屏/窗口化」
- 按 F11 会触发游戏全屏切换 → `ResizeBuffers` → 菜单消失
- **推荐用 Insert 或 F8/F9**

---

## 九、下一步可做的优化

1. **平滑自瞄**
   - 加 `g_smooth` 参数，每帧只插值一部分角度
   - 注意 yaw 的环绕问题（-180° ~ 180°）

2. **锁头近似**
   - 在 `get_origin` 基础上加 `origin + Vec3(0, 0, 64)` 作为头部位置
   - 比锁胸/脚体验更好

3. **ESP 距离显示**
   - 在方框底部血量旁显示 `XX m`

4. **默认参数调小**
   - `g_circle_radius = 100`
   - `g_max_world_dist = 1500`

5. **配置系统**
   - 保存/加载设置到文件，不用每次重启游戏都重新调

6. **武器名 / 被闪白显示**
   - 需要额外偏移，优先级较低

---

## 十、注意事项汇总

1. **偏移必须保持最新**：每次游戏更新后重新跑 dumper，同步 `offsets.hpp`
2. **不要用 F11 当快捷键**：会和游戏全屏切换冲突
3. **切换分辨率/窗口后菜单消失**：这是已知限制，重新注入即可
4. **不要尝试 Hook `ResizeBuffers`**：会崩，已多次验证
5. **静默自瞄、trace、`m_bSpottedByMask` 都已验证不可行**，不要浪费时间
6. **MinHook 原版对某些序言有 bug**：Hook 函数入口尽量选普通序言
7. **注入时机**：建议在游戏主界面注入，避免启动阶段注入
8. **编译警告 `LNK4098` 可忽略**

---

> **文档结束**
> 最后更新：2026-10-08
