#include "entity.hpp"
#include "../tools/memory.hpp"
#include "offsets.hpp"
#include "../tools/schema.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include <Windows.h>

namespace game {
    /*
     * 快速读取（不带 VirtualQuery，用 __try 保护）
     * 只能用于已经确认可读的内存块
     * 比 tools::mem::read 快 50 倍以上
     */
    template<typename T>
    static T read_fast(std::uintptr_t addr) {
        __try {
            return *reinterpret_cast<const T *>(addr);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return T{};
        }
    }

    /*
     * 全局缓存正确步长
     * 第一次调用时自动探测（用 local_pawn 反推），之后复用
     * 0 = 未探测
     */
    static std::uintptr_t g_entity_step = 0;

    /*
     * 探测 entity list 的 chunk 步长
     * 用 local_pawn（已知的正确 Pawn）反推
     * 返回 0x70 或 0x78，找不到返回 0x78 兜底
     */
    static std::uintptr_t detect_step() {
        const std::uintptr_t base = tools::mem::client_base();
        if (!base) return 0x78;

        const std::uintptr_t local_pawn = tools::mem::read<std::uintptr_t>(
            base + off::client::dwLocalPlayerPawn);
        if (!local_pawn) return 0x78;

        const std::uintptr_t list = tools::mem::read<std::uintptr_t>(
            base + off::client::dwEntityList);
        if (!list) return 0x78;

        // 试 0x70 和 0x78
        for (std::uintptr_t step: {0x70, 0x78}) {
            for (int group = 0; group < 16; ++group) {
                const std::uintptr_t chunk = tools::mem::read<std::uintptr_t>(
                    list + 0x10 + 8 * group);
                if (!chunk) continue;

                MEMORY_BASIC_INFORMATION mbi{};
                if (!VirtualQuery(reinterpret_cast<LPCVOID>(chunk), &mbi, sizeof(mbi)))
                    continue;
                if (mbi.State != MEM_COMMIT) continue;

                for (int slot = 0; slot < 512; ++slot) {
                    std::uintptr_t v = read_fast<std::uintptr_t>(
                        chunk + step * slot);
                    if (v == local_pawn) {
                        return step;
                    }
                }
            }
        }

        return 0x78; // 兜底
    }

    /*
     * 获取步长（第一次探测，之后缓存）
     */
    static std::uintptr_t get_step() {
        if (g_entity_step == 0) {
            g_entity_step = detect_step();
        }
        return g_entity_step;
    }

    /*
     * 从 entity list 按 index 取 Controller
     */
    std::uintptr_t get_controller_by_index(int index) {
        const std::uintptr_t base = tools::mem::client_base();
        if (!base) return 0;

        const std::uintptr_t list = tools::mem::read<std::uintptr_t>(
            base + off::client::dwEntityList);
        if (!list) return 0;

        const std::uintptr_t chunk = tools::mem::read<std::uintptr_t>(
            list + 8 * (index >> 9) + 0x10);
        if (!chunk) return 0;

        return tools::mem::read<std::uintptr_t>(
            chunk + get_step() * (index & 0x1FF));
    }

    /*
     * 通过 Controller 拿 Pawn
     */
    std::uintptr_t get_pawn_from_controller(std::uintptr_t controller) {
        if (!controller) return 0;

        const std::uint32_t handle = tools::mem::read<std::uint32_t>(
            controller + schema::m_hPlayerPawn);
        if (!handle || handle == 0xFFFFFFFF) return 0;

        const int pawn_index = handle & 0x7FFF;

        const std::uintptr_t base = tools::mem::client_base();
        if (!base) return 0;

        const std::uintptr_t list = tools::mem::read<std::uintptr_t>(
            base + off::client::dwEntityList);
        if (!list) return 0;

        const std::uintptr_t chunk = tools::mem::read<std::uintptr_t>(
            list + 8 * (pawn_index >> 9) + 0x10);
        if (!chunk) return 0;

        return tools::mem::read<std::uintptr_t>(
            chunk + get_step() * (pawn_index & 0x1FF));
    }

    /*
     * 按玩家槽位 index（1~64）取 Pawn
     */
    std::uintptr_t get_pawn_by_index(int index) {
        const std::uintptr_t controller = get_controller_by_index(index);
        if (!controller) return 0;
        return get_pawn_from_controller(controller);
    }

    /*
     * 取本地玩家 Controller
     */
    std::uintptr_t local_controller() {
        const std::uintptr_t base = tools::mem::client_base();
        if (!base) return 0;
        return tools::mem::read<std::uintptr_t>(
            base + off::client::dwLocalPlayerController);
    }

    /*
     * 取本地玩家 Pawn
     */
    std::uintptr_t local_player() {
        const std::uintptr_t base = tools::mem::client_base();
        if (!base) return 0;
        return tools::mem::read<std::uintptr_t>(
            base + off::client::dwLocalPlayerPawn);
    }

    /*
     * 读血量
     */
    int get_health(std::uintptr_t entity) {
        if (!entity) return 0;
        return tools::mem::read<int>(entity + schema::m_iHealth);
    }

    /*
     * 读阵营
     */
    int get_team(std::uintptr_t entity) {
        if (!entity) return 0;
        return tools::mem::read<std::uint8_t>(entity + schema::m_iTeamNum);
    }

    /*
     * 读 lifeState
     */
    int get_life_state(std::uintptr_t entity) {
        if (!entity) return 1;
        return tools::mem::read<std::uint8_t>(entity + schema::m_lifeState);
    }

    /*
     * 是否存活
     */
    bool is_alive(std::uintptr_t entity) {
        if (!entity) return false;
        const int hp = get_health(entity);
        const int max_hp = tools::mem::read<int>(
            entity + schema::m_iMaxHealth);
        return hp > 0 || max_hp > 0;
    }

    /*
     * 是否敌人
     */
    bool is_enemy(std::uintptr_t local, std::uintptr_t target) {
        if (!local || !target) return false;
        if (target == local) return false;
        if (!is_alive(target)) return false;

        const int local_team = get_team(local);
        const int target_team = get_team(target);

        if (target_team != 2 && target_team != 3) return false;
        return target_team != local_team;
    }

    /*
     * 读世界坐标
     */
    Vec3 get_origin(std::uintptr_t entity) {
        if (!entity) return {};

        const std::uintptr_t node = tools::mem::read<std::uintptr_t>(
            entity + schema::m_pGameSceneNode);
        if (!node) return {};

        return tools::mem::read<Vec3>(node + schema::m_vecAbsOrigin);
    }

    /*
     * 扫 entity list 的所有 chunk + slot，返回所有 team 2/3 且 pos 有效的实体
     * 用自动探测的步长
     *
     * 优化：chunk 只查一次 VirtualQuery，后续所有读用 read_fast（不带 VirtualQuery）
     * 之前每帧 40960 次 VirtualQuery，现在每帧 16 次
     */
    std::vector<std::uintptr_t> get_all_pawns() {
        std::vector<std::uintptr_t> result;

        const std::uintptr_t base = tools::mem::client_base();
        if (!base) return result;

        const std::uintptr_t list = tools::mem::read<std::uintptr_t>(
            base + off::client::dwEntityList);
        if (!list) return result;

        const std::uintptr_t step = get_step();

        for (int group = 0; group < 16; ++group) {
            const std::uintptr_t chunk = tools::mem::read<std::uintptr_t>(
                list + 0x10 + 8 * group);
            if (!chunk) continue;

            // 只对 chunk 检查一次 VirtualQuery
            MEMORY_BASIC_INFORMATION mbi{};
            if (!VirtualQuery(reinterpret_cast<LPCVOID>(chunk), &mbi, sizeof(mbi)))
                continue;
            if (mbi.State != MEM_COMMIT) continue;
            if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) continue;

            // 边界保护：chunk + step * 512 不能溢出
            const std::uintptr_t chunk_end = chunk + step * 512;
            if (chunk_end < chunk) continue;

            for (int slot = 0; slot < 512; ++slot) {
                // ★ read_fast：不带 VirtualQuery
                std::uintptr_t e = read_fast<std::uintptr_t>(chunk + step * slot);
                if (!e || e < 0x10000 || e >= 0x7FFFFFFFFFFF
                    || e >= 0x7FF000000000ULL)
                    continue;

                const int team = read_fast<std::uint8_t>(
                    e + schema::m_iTeamNum);
                if (team != 2 && team != 3) continue;

                const int hp = read_fast<int>(e + schema::m_iHealth);
                if (hp <= 0) continue; // 死了，立即跳过

                const int life = read_fast<std::uint8_t>(e + schema::m_lifeState);
                if (life != 0) continue; // lifeState != 0 也跳过

                const std::uintptr_t node = read_fast<std::uintptr_t>(
                    e + schema::m_pGameSceneNode);
                if (!node) continue;

                const Vec3 pos = read_fast<Vec3>(
                    node + schema::m_vecAbsOrigin);
                if (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f) continue;
                if (std::fabs(pos.x) > 20000.0f) continue;
                if (std::fabs(pos.y) > 20000.0f) continue;
                if (std::fabs(pos.z) > 20000.0f) continue;

                result.push_back(e);
            }
        }

        return result;
    }

    /*
     * 获取当前探测到的步长（调试用）
     */
    std::uintptr_t get_current_step() {
        return get_step();
    }

    // 从 Pawn 拿 Controller
    std::uintptr_t get_controller_from_pawn(std::uintptr_t pawn) {
        if (!pawn) return 0;

        const std::uint32_t handle = tools::mem::read<std::uint32_t>(
            pawn + 0x14BC);   // m_hController
        if (!handle || handle == 0xFFFFFFFF) return 0;

        const int index = handle & 0x7FFF;
        return get_controller_by_index(index);
    }

    // 读 Controller 的玩家名字
    // 返回内部静态缓冲区，调用完立刻用（不要保存指针）
    const char* get_player_name(std::uintptr_t controller) {
        static char buf[128];
        buf[0] = '\0';

        if (!controller) return buf;

        // m_iszPlayerName 是 char[128]
        for (int i = 0; i < 127; ++i) {
            char c = tools::mem::read<char>(controller + 0x6FC + i);
            buf[i] = c;
            if (c == '\0') break;
        }
        buf[127] = '\0';
        return buf;
    }
}
