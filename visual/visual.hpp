#pragma once

#include <cstdint>

/*
 * 屏幕覆盖层绘制
 * 只画一个红色圆圈，表示自瞄判定范围
 */
namespace visual {
    /*
     * 在屏幕中心画红色圆圈
     * 需要在 ImGui::NewFrame() 之后、ImGui::Render() 之前调用
     */
    void draw_aim_circle(float radius); // 画自瞄范围圆

    /*
     * 顺便把当前锁定的目标画个标记
     */
    void draw_target_marker(std::uintptr_t target); // 画锁定标记

    // ESP 总开关
    extern bool g_esp_enabled; // 透视总开关
    extern bool g_esp_show_health; // 显示血量
    extern bool g_esp_show_ray; // 显示射线
    extern bool g_esp_show_name; // 显示名称
    extern bool g_esp_show_skeleton; // 显示骨骼

    extern bool g_esp_show_weapon; // 显示武器
    extern bool g_esp_show_distance; // 显示距离
    extern bool g_esp_show_armor; // 显示护甲
    extern bool g_esp_show_helmet; // 显示头盔

    // 绘制框
    void draw_esp(); // 绘制所有 ESP

    // 绘制 C4 位置提示
    void draw_c4_indicator(); // 画 C4 提示条 + 位置

    // 是否显示C4
    extern bool g_esp_show_c4; // 显示 C4 开关

    // 雷达
    extern bool g_radar_enabled; // 雷达总开关
    void draw_radar(); // 绘制雷达

    // 绘制欢迎文字
    void draw_welcome_text();

    // 第三人称开关
    extern bool g_thirdperson;
}
