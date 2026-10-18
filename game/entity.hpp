#pragma once

#include <cstdint>
#include <Windows.h>
#include <cstddef>
#include <vector>
#include "../tools/memory.hpp"

namespace game {
    // 世界坐标三元组
    struct Vec3 {
        float x, y, z; // x y z 三个分量
    };

    // 每帧实体快照
    struct PawnSnapshot {
        std::uintptr_t ptr = 0; // 实体地址
        int team = 0; // 阵营编号
        int health = 0; // 当前血量
        int armor = 0; // 护甲值
        bool helmet = false; // 是否有头盔
        bool ducking = false; // 是否蹲下
        Vec3 origin{}; // 脚底世界坐标
        Vec3 head{}; // 头部世界坐标
        std::uintptr_t controller = 0; // 对应 Controller 地址

        Vec3 neck{}; // 脖子坐标
        Vec3 chest{}; // 胸部坐标
        Vec3 body{}; // 身体（脊柱）坐标
        Vec3 pelvis{}; // 骨盆坐标
        Vec3 armL{}; // 左大臂坐标
        Vec3 forearmL{}; // 左小臂坐标
        Vec3 handL{}; // 左手坐标
        Vec3 armR{}; // 右大臂坐标
        Vec3 forearmR{}; // 右小臂坐标
        Vec3 handR{}; // 右手坐标
        Vec3 thighL{}; // 左大腿坐标
        Vec3 calfL{}; // 左小腿坐标
        Vec3 footL{}; // 左脚坐标
        Vec3 thighR{}; // 右大腿坐标
        Vec3 calfR{}; // 右小腿坐标
        Vec3 footR{}; // 右脚坐标
    };

    // 帧缓存
    void update_frame_cache(); // 每帧刷新 ViewMatrix 和屏幕尺寸
    bool world_to_screen(const Vec3 &world, float &sx, float &sy); // 世界坐标转屏幕坐标
    float screen_width(); // 取屏幕宽度
    float screen_height(); // 取屏幕高度

    // 实体列表
    const std::vector<std::uintptr_t> &get_pawns_cached(); // 敌人列表缓存
    const std::vector<PawnSnapshot> &build_snapshot(std::uintptr_t local); // 构建一帧快照
    std::vector<std::uintptr_t> get_all_pawns(); // 扫描 entity list 取所有敌人 Pawn

    // 按 index 取实体
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

    // 从 Pawn 拿 Controller
    std::uintptr_t get_controller_from_pawn(std::uintptr_t pawn); // 从 Pawn 拿 Controller

    // C4
    std::uintptr_t get_planted_c4(); // 取已安放的 C4 实体
    std::vector<std::uintptr_t> get_all_c4(); // 扫描 entity list 取所有 C4 实体（含身上/地上/已安放）
    const std::vector<std::uintptr_t> &get_c4_cached(); // C4 列表缓存（每 20 帧刷新）
    Vec3 get_c4_origin(std::uintptr_t c4); // 读 C4 世界坐标（带兜底）

    // 调试用
    std::uintptr_t get_current_step(); // 获取当前 entity list 步长

    // 可见性检查
    bool is_visible(std::uintptr_t local_pawn, std::uintptr_t target_pawn); // 本地玩家眼睛到目标是否有遮挡
    bool is_visible_throttled(std::uintptr_t local_pawn, std::uintptr_t target_pawn); // 每敌人每 3 帧最多查一次

    // 无后座
    void update_rcs_visual(); // 纯视觉层无后座力控制
}