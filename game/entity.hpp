#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

/**
 * 类字段偏移，来自 cs2-dumper 的 output/client_dll.hpp。
 * 每次游戏更新后需要重新跑 dumper 同步。
 */
namespace game {
    /** 世界坐标三元组 */
    struct Vec3 {
        float x, y, z;
    };

    std::uintptr_t get_controller_by_index(int index);

    std::uintptr_t get_pawn_from_controller(std::uintptr_t controller);

    std::uintptr_t get_pawn_by_index(int index);

    std::uintptr_t local_controller();

    std::uintptr_t local_player();

    int get_health(std::uintptr_t entity);

    int get_team(std::uintptr_t entity);

    int get_life_state(std::uintptr_t entity);

    bool is_alive(std::uintptr_t entity);

    bool is_enemy(std::uintptr_t local, std::uintptr_t target);

    Vec3 get_origin(std::uintptr_t entity);

    std::vector<std::uintptr_t> get_all_pawns();

    // 调试用：获取当前探测到的步长
    std::uintptr_t get_current_step();

    std::uintptr_t get_controller_from_pawn(std::uintptr_t pawn);
    const char* get_player_name(std::uintptr_t controller);

} // entity.hpp namespace schema
