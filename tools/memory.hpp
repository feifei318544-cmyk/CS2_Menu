#pragma once

#include <cstdint>
#include <cstring>
#include <Windows.h>

namespace tools::mem {
    // 获取模块基址
    std::uintptr_t get_module_base(const char *module_name);

    std::uintptr_t client_base();

    std::uintptr_t engine_base();

    /*
     * 安全的读取函数
     * 每次读取前先 VirtualQuery 检查地址是否可读
     * 避免扫内存时读到未映射地址直接崩溃
     */
    template<typename T>
    T read(std::uintptr_t addr) {
        if (!addr) return T{};

        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi)))
            return T{};

        if (mbi.State != MEM_COMMIT)
            return T{};

        // 必须可读，不能是 PAGE_NOACCESS 或 PAGE_GUARD
        if ((mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD))
            return T{};

        T val{};
        std::memcpy(&val, reinterpret_cast<const void *>(addr), sizeof(T));
        return val;
    }

    /*
     * 安全的写入函数
     * 同样先检查地址是否可写
     */
    template<typename T>
    bool write(std::uintptr_t addr, const T &val) {
        if (!addr) return false;

        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi)))
            return false;

        if (mbi.State != MEM_COMMIT)
            return false;

        // 必须是可写或可读写
        if (!(mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY
                             | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
            return false;

        std::memcpy(reinterpret_cast<void *>(addr), &val, sizeof(T));
        return true;
    }
}
