#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace game {
    // 世界坐标三元组
    struct Vec3 {
        float x, y, z; // x y z 三个分量
    };


    // 每帧调用一次，刷新 ViewMatrix 和屏幕尺寸
    void update_frame_cache();

    // 用缓存的 ViewMatrix 把世界坐标转屏幕坐标
    bool world_to_screen(const Vec3 &world, float &sx, float &sy);

    // 屏幕宽高
    float screen_width(); // 取屏幕宽度
    float screen_height(); // 取屏幕高度

    // 敌人列表缓存，每 N 帧刷新一次
    const std::vector<std::uintptr_t> &get_pawns_cached();

    // 通过槽位 index 取 Controller 和 Pawn
    std::uintptr_t get_controller_by_index(int index); // 通过槽位 index 取 Controller
    std::uintptr_t get_pawn_from_controller(std::uintptr_t controller); // 通过 Controller 取 Pawn
    std::uintptr_t get_pawn_by_index(int index); // 通过槽位 index 取 Pawn
    std::uintptr_t get_entity_from_index(int index); // 通过实体 index 取实体

    // 本地玩家
    std::uintptr_t local_controller(); // 取本地玩家 Controller
    std::uintptr_t local_player(); // 取本地玩家 Pawn

    // 状态读取
    int get_health(std::uintptr_t entity); // 读血量
    int get_team(std::uintptr_t entity); // 读阵营
    int get_life_state(std::uintptr_t entity); // 读 lifeState
    int get_armor(std::uintptr_t pawn); // 读护甲值
    bool is_alive(std::uintptr_t entity); // 是否存活
    bool is_enemy(std::uintptr_t local, std::uintptr_t target); // 是否敌人
    bool is_ducking(std::uintptr_t pawn); // 是否蹲下
    bool has_helmet(std::uintptr_t pawn); // 是否有头盔

    // 世界坐标
    Vec3 get_origin(std::uintptr_t entity); // 读世界坐标

    // 骨骼
    bool get_bone_pos(std::uintptr_t pawn, int bone_index, Vec3 &out); // 读指定骨骼世界坐标
    bool get_head_pos(std::uintptr_t pawn, Vec3 &out); // 读头骨骼世界坐标

    // 武器和距离
    const char *get_weapon_name(std::uintptr_t pawn); // 读当前武器名称
    float get_distance(const Vec3 &a, const Vec3 &b); // 计算两点世界距离（米）

    // 玩家名字
    const char *get_player_name(std::uintptr_t controller); // 读 Controller 玩家名字

    // 敌人列表
    std::vector<std::uintptr_t> get_all_pawns(); // 扫描 entity list 取所有敌人 Pawn

    // 调试用，获取当前探测到的 entity list 步长
    std::uintptr_t get_current_step(); // 取当前步长

    // 从 Pawn 拿 Controller
    std::uintptr_t get_controller_from_pawn(std::uintptr_t pawn); // 从 Pawn 取 Controller
}