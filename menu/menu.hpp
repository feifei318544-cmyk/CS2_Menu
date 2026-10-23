#pragma once

namespace menu {
    // 卸载请求标志，菜单点"卸载辅助"后置 true
    extern bool g_unload_requested; // 卸载请求标志

    // 后坐力控制模式 (0=关闭, 1=视觉压枪, 2=指令完美无后座)
    extern int g_rcs_mode;

    void draw(); // 绘制菜单
}
