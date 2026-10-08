#pragma once

#include <cstdint>

/**
 * 屏幕覆盖层绘制
 * 只画一个红色圆圈，表示自瞄判定范围
 */
namespace visual {
    /**
     * 在屏幕中心画红色圆圈
     * 需要在 ImGui::NewFrame() 之后、ImGui::Render() 之前调用
     */
    void draw_aim_circle(float radius);

    /** 顺便把当前锁定的目标画个标记 */
    void draw_target_marker(std::uintptr_t target);

    // ESP 总开关
    extern bool g_esp_enabled;
    extern bool g_esp_show_health;
    extern bool g_esp_show_ray;
    extern bool g_esp_show_name;

    // 绘制框
    void draw_esp();
} // namespace visual
