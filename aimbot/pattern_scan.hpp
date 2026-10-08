#pragma once

#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <vector>

namespace Scanner {

    inline void* PatternScan(const char* module_name, const char* signature) {
        const HMODULE module_handle = GetModuleHandleA(module_name);
        if (!module_handle) return nullptr;

        // 把 "4C 89 4C 24 ??" 这种字符串解析成字节数组
        // -1 表示通配
        static auto pattern_to_byte = [](const char* pattern) {
            std::vector<int> bytes;

            char* start = const_cast<char*>(pattern);
            char* end = start + std::strlen(pattern);
            char* current = start;

            while (current < end) {
                // 跳过空格 / 制表符
                while (current < end && (*current == ' ' || *current == '\t')) {
                    ++current;
                }
                if (current >= end) break;

                if (*current == '?') {
                    // 通配符
                    ++current;
                    if (current < end && *current == '?') {
                        ++current;
                    }
                    bytes.push_back(-1);
                } else {
                    // 十六进制字节
                    char* next = nullptr;
                    unsigned long value = std::strtoul(current, &next, 16);
                    bytes.push_back(static_cast<int>(value));
                    current = next;
                }
            }

            return bytes;
        };

        const auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);
        const auto nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(
            reinterpret_cast<std::uint8_t*>(module_handle) + dos_header->e_lfanew);

        const std::size_t size_of_image = nt_headers->OptionalHeader.SizeOfImage;
        const auto pattern_bytes = pattern_to_byte(signature);
        const auto scan_bytes = reinterpret_cast<std::uint8_t*>(module_handle);

        const std::size_t s = pattern_bytes.size();
        const int* d = pattern_bytes.data();

        if (s == 0) {
            return nullptr;
        }

        for (std::size_t i = 0; i + s <= size_of_image; ++i) {
            bool found = true;
            for (std::size_t j = 0; j < s; ++j) {
                if (d[j] != -1 && scan_bytes[i + j] != static_cast<std::uint8_t>(d[j])) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return &scan_bytes[i];
            }
        }

        return nullptr;
    }

}