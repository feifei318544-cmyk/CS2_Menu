#include <Windows.h>
#include "memory.hpp"

namespace tools::mem {
    std::uintptr_t get_module_base(const char *module_name) {
        HMODULE handle = GetModuleHandleA(module_name);
        return reinterpret_cast<std::uintptr_t>(handle);
    }

    std::uintptr_t client_base() {
        static std::uintptr_t base = get_module_base("client.dll");
        if (!base) base = get_module_base("client.dll");
        return base;
    }

    std::uintptr_t engine_base() {
        static std::uintptr_t base = get_module_base("engine2.dll");
        if (!base) base = get_module_base("engine2.dll");
        return base;
    }
}
