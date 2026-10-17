#pragma once

namespace menu {
    // 卸载请求标志，菜单点"卸载辅助"后置 true
    extern bool g_unload_requested; // 卸载请求标志

    // 无后座相关
    extern bool g_rcs_enabled;     // 无后座开关
    extern float g_rcs_smooth;     // 压枪力度 0.0 - 1.0

    void draw(); // 绘制菜单
}