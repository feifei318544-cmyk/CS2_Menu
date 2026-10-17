#include <Windows.h>
#include "memory.hpp"

namespace tools::mem {
    std::uintptr_t get_module_base(const char *module_name) { // 取模块基址
        HMODULE handle = GetModuleHandleA(module_name); // 模块句柄
        return reinterpret_cast<std::uintptr_t>(handle);
    }

    std::uintptr_t client_base() { // 取 client.dll 基址
        static std::uintptr_t base = get_module_base("client.dll"); // 缓存基址
        if (!base) base = get_module_base("client.dll");
        return base;
    }

    std::uintptr_t engine_base() { // 取 engine2.dll 基址
        static std::uintptr_t base = get_module_base("engine2.dll"); // 缓存基址
        if (!base) base = get_module_base("engine2.dll");
        return base;
    }
}