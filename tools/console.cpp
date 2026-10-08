#include "console.hpp"

#include <Windows.h>
#include <cstdio>
#include <iostream>

namespace tools::console {
    void open() {
        // 如果已经有控制台，就不重复分配
        if (GetConsoleWindow() != nullptr) return;

        // 分配一个新的控制台
        if (!AllocConsole()) return;

        // 重定向 stdout / stderr 到控制台
        FILE *f = nullptr;
        freopen_s(&f, "CONOUT$", "w", stdout);
        freopen_s(&f, "CONOUT$", "w", stderr);

        // 同步 C++ 流
        std::ios::sync_with_stdio();

        // 设置标题
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleTitleA("CS2 Debug Console");
    }

    void close() {
        if (GetConsoleWindow()) {
            FreeConsole();
        }
    }
} // namespace tools::console
