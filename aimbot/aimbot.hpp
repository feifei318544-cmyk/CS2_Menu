#pragma once
#include <cstdint>

/**
 * 自瞄主逻辑
 * 屏幕中心画一个圆，敌人在圆内就自动把视角对准他
 */
namespace aimbot {
    // 屏幕中心的圆半径（像素），UI 里可以改
    extern float g_circle_radius;

    // 是否启用自瞄
    extern bool g_enabled;

    // 平滑
    extern float g_smooth;

    extern float g_max_world_dist;
    extern bool g_require_visible;

    /**
     * 每帧调用一次
     * 找到屏幕距离准星最近、且在圆内的敌人，把视角对准他
     * 返回被锁定的敌人实体地址，0 表示没锁
     */
    std::uintptr_t run_once();
}
