# CS2 External Cheat

## 项目简介
基于 C++ 编写的 CS2 外部辅助，采用 DX11 内部注入实现菜单渲染与底层 Hook。项目注重运行稳定性与抗更新能力，通过特征码扫描与动态偏移双保险机制，提供丰富的视觉与战斗辅助。

## 核心功能
- 视觉 ESP：方框、血量、护甲、名字、武器、距离、骨骼透视、雷达、C4 提示。
- 自瞄系统：支持 11 个部位单选，内置 3 种平滑算法（指数 / 线性 / 混合）。
- 静默自瞄（子弹追踪）：通过 Hook `FillUserCmd` 覆盖指令视角，实现精准追踪。
- 双模式 RCS 无后座：可在菜单中互斥切换。
  - 视觉回正模式：基于 `m_iShotsFired` 状态，实现屏幕视角平滑下拉及松开左键自动回正。
  - 指令修正模式：在 `FillUserCmd` 底层 Hook 中直接扣除后坐力偏移，实现子弹绝对精准。
- 精准可见性判定：内部 `IsVisible` 特征码调用，防隔墙锁人。

## 架构优势
1. 高精度扫描器：仅遍历 `.text` 代码段，彻底解决高频通用特征码扫错位置的问题。
2. 三重安全校验：关键函数采用“特征码 + RVA 兜底 + 汇编字节校验”，避免小更新导致的崩溃。
3. 全链路 SEH 防护：调用游戏函数与 Hook 内部指针读写均包裹 `__try / __except` 防爆盾，杜绝按下左键等操作引发的访问冲突闪退。
4. 高频快照缓存：帧级实体快照，降低每帧 `VirtualQuery` 开销，保证游戏不卡顿。

## 维护指南 (偏移更新)
如果游戏更新导致辅助失效（如 ESP 不显示、自瞄不准、游戏崩溃），通常是因为内部偏移过期。请按以下步骤更新：

1. **获取最新偏移**：前往 [cs2-dumper](https://github.com/a2x/cs2-dumper) 项目，获取最新的 `client_dll.hpp` 和 `offsets.hpp`。
2. **更新 schema (结构体偏移)**：对比最新的 `client_dll.hpp`，将更新的 `m_iHealth`、`m_pGameSceneNode`、`m_entitySpottedState` 等字段替换到本项目 `schema.hpp` 中。
3. **更新 offsets (模块基址偏移)**：对比最新的 `offsets.hpp`，将 `dwEntityList`、`dwLocalPlayerPawn`、`dwViewMatrix`、`dwViewAngles` 等偏移替换到本项目 `offsets.hpp` 中。
4. **更新 RVA 兜底**：若 `FillUserCmd` 的特征码失效，请从 dump 工具中寻找最新的 `dwFillUserCmd` 偏移，更新 `offsets.hpp` 中的 `dwFillUserCmd` 备用值。
5. **重新编译注入**：更新完成后，重新编译 DLL 并注入测试。

## 免责声明
本项目仅供 C++ 逆向工程、内存安全与图形学渲染技术的学习与交流使用。严禁用于商业用途或破坏游戏公平性的行为。
