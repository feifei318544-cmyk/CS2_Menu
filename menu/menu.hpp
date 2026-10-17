#pragma once

namespace menu {
    // 卸载请求标志，菜单点"卸载辅助"后置 true
    extern bool g_unload_requested; // 卸载请求标志

    void draw(); // 绘制菜单
}