#pragma once

#include <cstdint>
#include <cstring>
#include <Windows.h>

namespace tools::mem {
    // 获取模块基址
    std::uintptr_t get_module_base(const char *module_name);

    std::uintptr_t client_base(); // 取 client.dll 基址

    std::uintptr_t engine_base(); // 取 engine2.dll 基址

    /*
     * 非模板 SEH 包装：单独一个普通函数，避开 MSVC "模板 + __try/__except" 的栈展开 bug
     * 成功返回 true，访问违例被吃掉返回 false
     */
    inline bool safe_memcpy(void *dst, const void *src, std::size_t n) {
        __try {
            std::memcpy(dst, src, n);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    /*
     * 裸读：不做 VirtualQuery，调用者必须保证地址来源可信
     * 内部只调 safe_memcpy，异常安全
     * 适用：client_base + 偏移、已校验实体的 schema 字段、骨骼、ViewMatrix
     */
    template<typename T>
    inline T read_raw(std::uintptr_t addr) {
        if (!addr) return T{};
        T val{};
        safe_memcpy(&val, reinterpret_cast<const void *>(addr), sizeof(T));
        return val;
    }

    /*
     * 安全的读取函数（带 VirtualQuery，慢）
     * 只用于"来源未知"的第一次跳转：模块基址、entity list 首地址、chunk 数组
     */
    template<typename T>
    T read(std::uintptr_t addr) {
        if (!addr) return T{};

        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi)))
            return T{};

        if (mbi.State != MEM_COMMIT)
            return T{};

        if ((mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD))
            return T{};

        T val{};
        std::memcpy(&val, reinterpret_cast<const void *>(addr), sizeof(T));
        return val;
    }

    /*
     * 安全的写入函数
     */
    template<typename T>
    bool write(std::uintptr_t addr, const T &val) {
        if (!addr) return false;

        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi)))
            return false;

        if (mbi.State != MEM_COMMIT)
            return false;

        if (!(mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY
                             | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
            return false;

        std::memcpy(reinterpret_cast<void *>(addr), &val, sizeof(T));
        return true;
    }
}