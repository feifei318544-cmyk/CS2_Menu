#pragma once

#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>

namespace Scanner {
    // 内部函数：将字符串特征码解析为字节数组 (-1 表示通配符)
    inline std::vector<int> pattern_to_byte(const char* pattern) {
        std::vector<int> bytes;
        const char* current = pattern;
        const char* end = pattern + std::strlen(pattern);

        while (current < end) {
            while (current < end && (*current == ' ' || *current == '\t')) {
                ++current;
            }
            if (current >= end) break;

            if (*current == '?') {
                ++current;
                if (current < end && *current == '?') {
                    ++current;
                }
                bytes.push_back(-1);
            } else {
                char* next = nullptr;
                unsigned long value = std::strtoul(current, &next, 16);
                bytes.push_back(static_cast<int>(value));
                current = next;
            }
        }
        return bytes;
    }

    // 核心：在当前模块的 .text (可执行) 段中扫描特征码
    inline uintptr_t scan_text_section(const char* module_name, const char* signature) {
        const HMODULE module_handle = GetModuleHandleA(module_name);
        if (!module_handle) return 0;

        const auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);
        const auto nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(
            reinterpret_cast<std::uint8_t*>(module_handle) + dos_header->e_lfanew);

        auto section = IMAGE_FIRST_SECTION(nt_headers);
        const std::vector<int> pattern_bytes = pattern_to_byte(signature);
        if (pattern_bytes.empty()) return 0;

        const std::size_t pattern_size = pattern_bytes.size();
        const int* pattern_data = pattern_bytes.data();

        for (int i = 0; i < nt_headers->FileHeader.NumberOfSections; ++i, ++section) {
            // 只扫描具备可执行权限的段（.text）
            if (!(section->Characteristics & IMAGE_SCN_MEM_EXECUTE)) {
                continue;
            }

            uintptr_t section_start = reinterpret_cast<uintptr_t>(module_handle) + section->VirtualAddress;
            size_t section_size = section->Misc.VirtualSize;

            if (section_size < pattern_size) continue;

            uint8_t* scan_start = reinterpret_cast<uint8_t*>(section_start);

            for (size_t j = 0; j <= section_size - pattern_size; ++j) {
                bool found = true;
                for (size_t k = 0; k < pattern_size; ++k) {
                    if (pattern_data[k] != -1 && scan_start[j + k] != static_cast<uint8_t>(pattern_data[k])) {
                        found = false;
                        break;
                    }
                }
                if (found) {
                    return section_start + j;
                }
            }
        }
        return 0;
    }

    // 兼容你原来 aimbot.cpp 里的函数名，直接转发给 scan_text_section
    inline uintptr_t scan_fill_user_cmd_pattern(const char* pattern) {
        return scan_text_section("client.dll", pattern);
    }
}