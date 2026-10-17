#pragma once
#include <cstdint>
#include <excpt.h>


/**
* 自瞄主逻辑
* 屏幕中心画一个圆，敌人在圆内就自动把视角对准他
*/
namespace aimbot {
    // 共用参数，暴力自瞄和子弹追踪都用这一套
    extern float g_circle_radius; // 屏幕中心的圆半径（像素）
    extern float g_max_world_dist; // 最大世界距离

    // 暴力瞄准
    extern bool g_enabled; // 暴力自瞄开关
    extern float g_smooth; // 平滑

    // 子弹追踪
    extern bool g_silent_enabled; // 子弹追踪开关

    // 静默瞄准角度，run_once 里写，hook 里读
    extern float g_silent_pitch; // 静默瞄准 pitch（俯仰角）
    extern float g_silent_yaw; // 静默瞄准 yaw（偏航角）
    extern bool g_silent_has_target; // 本帧是否有静默目标

    // RCS 压枪
    extern bool g_rcs_enabled; // RCS 压枪开关
    extern float g_rcs_strength; // RCS 压枪强度（0~1）

    // 自动开火
    extern bool g_auto_fire_enabled; // 自动开火开关

    // 每帧调用一次，找到屏幕距离准星最近且在圆内的敌人，把视角对准他
    // 返回被锁定的敌人实体地址，0 表示没锁
    std::uintptr_t run_once(); // 主循环

    // 安装子弹追踪 hook
    void install_silent_hook(); // 安装 hook
}