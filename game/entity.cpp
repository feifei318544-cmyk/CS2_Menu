#include "entity.hpp"
#include <Windows.h>
#include "../tools/memory.hpp"
#include "offsets.hpp"
#include "../tools/schema.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "pattern_scan.hpp"

namespace game {
    // 帧缓存
    static float g_view_matrix[16] = {}; // ViewMatrix 缓存
    static float g_screen_w = 1920.0f; // 屏幕宽度
    static float g_screen_h = 1080.0f; // 屏幕高度

    // 敌人列表缓存
    static std::vector<std::uintptr_t> g_pawns; // 敌人 Pawn 列表缓存
    static int g_pawns_tick = 0; // 缓存剩余帧数
    static constexpr int PAWNS_CACHE_INTERVAL = 20; // 敌人列表刷新间隔（帧）

    // 每帧快照缓存
    static std::vector<PawnSnapshot> g_snapshot; // 当前帧快照

    // 可见性节流缓存
    static std::unordered_map<std::uintptr_t, std::pair<int, bool> > g_vis_cache; // 敌人 -> {tick, 是否可见}
    static int g_vis_tick = 0; // 可见性节流计数

    // C4 列表缓存
    static std::vector<std::uintptr_t> g_c4_cache; // C4 列表缓存
    static int g_c4_tick = 0; // 缓存剩余帧数
    static constexpr int C4_CACHE_INTERVAL = 20; // C4 列表刷新间隔（帧）

    void update_frame_cache() {
        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (base) {
            const std::uintptr_t matrix = base + off::client::dwViewMatrix; // 视图矩阵地址
            if (!tools::mem::safe_memcpy(
                g_view_matrix,
                reinterpret_cast<const void *>(matrix),
                sizeof(g_view_matrix))) {
                std::memset(g_view_matrix, 0, sizeof(g_view_matrix)); // 读失败清零
            }
        }

        RECT rect{}; // 窗口客户区
        HWND hwnd = GetForegroundWindow();
        if (hwnd && GetClientRect(hwnd, &rect)) {
            float w = static_cast<float>(rect.right - rect.left);
            float h = static_cast<float>(rect.bottom - rect.top);
            if (w <= 0 || h <= 0) {
                w = (float) GetSystemMetrics(SM_CXSCREEN); // 兜底宽
                h = (float) GetSystemMetrics(SM_CYSCREEN); // 兜底高
            }
            g_screen_w = w;
            g_screen_h = h;
        }
    }

    bool world_to_screen(const Vec3 &world, float &sx, float &sy) {
        const float *m = g_view_matrix; // 视图矩阵

        const float w = m[12] * world.x + m[13] * world.y
                        + m[14] * world.z + m[15]; // 裁剪空间 w
        if (w < 0.65f) return false; // 太近，剔除

        const float x = m[0] * world.x + m[1] * world.y
                        + m[2] * world.z + m[3]; // 裁剪空间 x
        const float y = m[4] * world.x + m[5] * world.y
                        + m[6] * world.z + m[7]; // 裁剪空间 y

        sx = (g_screen_w * 0.5f) + (x / w) * (g_screen_w * 0.5f); // 屏幕 x
        sy = (g_screen_h * 0.5f) - (y / w) * (g_screen_h * 0.5f); // 屏幕 y
        return true;
    }

    float screen_width() { return g_screen_w; } // 取屏幕宽度
    float screen_height() { return g_screen_h; } // 取屏幕高度

    const std::vector<std::uintptr_t> &get_pawns_cached() {
        if (g_pawns_tick <= 0) {
            g_pawns = get_all_pawns(); // 重新扫描
            g_pawns_tick = PAWNS_CACHE_INTERVAL; // 重置计数
        }
        --g_pawns_tick;
        return g_pawns;
    }

    std::uintptr_t get_controller_by_index(int index) {
        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (!base) return 0;

        const std::uintptr_t list = tools::mem::read<std::uintptr_t>(
            base + off::client::dwEntityList); // entity list 基址
        if (!list) return 0;

        const std::uintptr_t chunk = tools::mem::read<std::uintptr_t>(
            list + 0x10 + 0x8 * (index >> 9)); // 对应 chunk
        if (!chunk) return 0;

        return tools::mem::read_raw<std::uintptr_t>(
            chunk + schema::kEntityStep * (index & 0x1FF)); // 对应 slot
    }

    std::uintptr_t get_pawn_from_controller(std::uintptr_t controller) {
        if (!controller) return 0;

        const std::uint32_t handle = tools::mem::read_raw<std::uint32_t>(
            controller + schema::m_hPlayerPawn); // 玩家 Pawn 句柄
        if (!handle || handle == 0xFFFFFFFF) return 0;

        const int pawn_index = handle & 0x7FFF; // Pawn 实体索引

        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (!base) return 0;

        const std::uintptr_t list = tools::mem::read<std::uintptr_t>(
            base + off::client::dwEntityList); // entity list 基址
        if (!list) return 0;

        const std::uintptr_t chunk = tools::mem::read<std::uintptr_t>(
            list + 0x10 + 0x8 * (pawn_index >> 9)); // 对应 chunk
        if (!chunk) return 0;

        return tools::mem::read_raw<std::uintptr_t>(
            chunk + schema::kEntityStep * (pawn_index & 0x1FF)); // 对应 slot
    }

    std::uintptr_t get_pawn_by_index(int index) {
        const std::uintptr_t controller = get_controller_by_index(index); // 对应 Controller
        if (!controller) return 0;
        return get_pawn_from_controller(controller); // 通过 Controller 拿 Pawn
    }

    std::uintptr_t local_controller() {
        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (!base) return 0;
        return tools::mem::read<std::uintptr_t>(
            base + off::client::dwLocalPlayerController); // 本地 Controller
    }

    std::uintptr_t local_player() {
        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (!base) return 0;
        return tools::mem::read<std::uintptr_t>(
            base + off::client::dwLocalPlayerPawn); // 本地 Pawn
    }

    int get_health(std::uintptr_t entity) {
        if (!entity) return 0;
        return tools::mem::read_raw<int>(entity + schema::m_iHealth); // 当前血量
    }

    int get_team(std::uintptr_t entity) {
        if (!entity) return 0;
        return tools::mem::read_raw<std::uint8_t>(entity + schema::m_iTeamNum); // 阵营编号
    }

    int get_life_state(std::uintptr_t entity) {
        if (!entity) return 1;
        return tools::mem::read_raw<std::uint8_t>(entity + schema::m_lifeState); // 生命状态
    }

    bool is_alive(std::uintptr_t entity) {
        if (!entity) return false;
        const int hp = get_health(entity); // 当前血量
        const int max_hp = tools::mem::read_raw<int>(entity + schema::m_iMaxHealth); // 最大血量
        return hp > 0 || max_hp > 0;
    }

    bool is_enemy(std::uintptr_t local, std::uintptr_t target) {
        if (!local || !target) return false;
        if (target == local) return false;
        if (!is_alive(target)) return false;

        const int local_team = get_team(local); // 本地阵营
        const int target_team = get_team(target); // 目标阵营

        if (target_team != 2 && target_team != 3) return false;
        return target_team != local_team;
    }

    Vec3 get_origin(std::uintptr_t entity) {
        if (!entity) return {};

        const std::uintptr_t node = tools::mem::read_raw<std::uintptr_t>(
            entity + schema::m_pGameSceneNode); // 场景节点
        if (!node) return {};

        return tools::mem::read_raw<Vec3>(node + schema::m_vecAbsOrigin); // 绝对世界坐标
    }

    std::vector<std::uintptr_t> get_all_pawns() {
        std::vector<std::uintptr_t> result; // 结果列表

        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (!base) return result;

        const std::uintptr_t list = tools::mem::read<std::uintptr_t>(
            base + off::client::dwEntityList); // entity list 基址
        if (!list) return result;

        for (int group = 0; group < 16; ++group) {
            const std::uintptr_t chunk = tools::mem::read<std::uintptr_t>(
                list + 0x10 + 0x8 * group); // 第 group 个 chunk
            if (!chunk) continue;

            MEMORY_BASIC_INFORMATION mbi{}; // 内存信息
            if (!VirtualQuery(reinterpret_cast<LPCVOID>(chunk), &mbi, sizeof(mbi)))
                continue; // 查询失败
            if (mbi.State != MEM_COMMIT) continue; // 未提交
            if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) continue; // 不可读或保护页

            for (int slot = 0; slot < 512; ++slot) {
                std::uintptr_t e = 0; // 当前实体
                if (!tools::mem::safe_memcpy(
                    &e,
                    reinterpret_cast<const void *>(
                        chunk + schema::kEntityStep * slot),
                    sizeof(e)))
                    continue; // 读失败

                if (!e || e < 0x10000 || e >= 0x7FF000000000ULL)
                    continue; // 地址非法

                const int team = tools::mem::read_raw<std::uint8_t>(
                    e + schema::m_iTeamNum); // 实体阵营
                if (team != 2 && team != 3) continue;

                const int hp = tools::mem::read_raw<int>(e + schema::m_iHealth); // 实体血量
                if (hp <= 0) continue; // 死了，立即跳过

                const int life = tools::mem::read_raw<std::uint8_t>(
                    e + schema::m_lifeState); // 生命状态
                if (life != 0) continue; // lifeState != 0 也跳过

                const std::uintptr_t node = tools::mem::read_raw<std::uintptr_t>(
                    e + schema::m_pGameSceneNode); // 场景节点
                if (!node) continue;

                const Vec3 pos = tools::mem::read_raw<Vec3>(
                    node + schema::m_vecAbsOrigin); // 世界坐标
                if (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f) continue;
                if (std::fabs(pos.x) > 20000.0f) continue;
                if (std::fabs(pos.y) > 20000.0f) continue;
                if (std::fabs(pos.z) > 20000.0f) continue;

                result.push_back(e); // 收进结果
            }
        }

        return result;
    }

    std::uintptr_t get_current_step() {
        return schema::kEntityStep; // 当前步长
    }

    std::uintptr_t get_controller_from_pawn(std::uintptr_t pawn) {
        if (!pawn) return 0;

        const std::uint32_t handle = tools::mem::read_raw<std::uint32_t>(
            pawn + schema::m_hController); // m_hController
        if (!handle || handle == 0xFFFFFFFF) return 0;

        const int index = handle & 0x7FFF; // Controller 实体索引
        return get_controller_by_index(index);
    }

    const char *get_player_name(std::uintptr_t controller) {
        static char buf[128]; // 名字缓冲区
        buf[0] = '\0';
        if (!controller) return buf;

        if (!tools::mem::safe_memcpy(
            buf,
            reinterpret_cast<const void *>(controller + schema::m_iszPlayerName),
            128)) {
            buf[0] = '\0';
            return buf;
        }
        buf[127] = '\0';
        return buf;
    }

    bool is_ducking(std::uintptr_t pawn) {
        if (!pawn) return false;

        const std::uintptr_t movement = tools::mem::read_raw<std::uintptr_t>(
            pawn + schema::m_pMovementServices); // 移动服务
        if (!movement) return false;

        const std::uint8_t ducking = tools::mem::read_raw<std::uint8_t>(
            movement + schema::m_bDucking); // 蹲下标志
        if (ducking != 0) return true;

        const float amount = tools::mem::read_raw<float>(
            movement + schema::m_flDuckAmount); // 蹲下程度
        return amount > 0.5f;
    }

    bool get_bone_pos(std::uintptr_t pawn, int bone_index, Vec3 &out) {
        if (!pawn) return false;
        if (bone_index < 0 || bone_index > 128) return false;

        const std::uintptr_t node = tools::mem::read_raw<std::uintptr_t>(
            pawn + schema::m_pGameSceneNode); // 场景节点
        if (!node) return false;

        const std::uintptr_t bone_array = tools::mem::read_raw<std::uintptr_t>(
            node + schema::m_modelState + schema::m_BoneArray); // 骨骼数组
        if (!bone_array) return false;

        const std::uintptr_t bone_addr = bone_array + bone_index * 32; // 骨骼地址

        if (!tools::mem::safe_memcpy(
            &out,
            reinterpret_cast<const void *>(bone_addr),
            sizeof(Vec3)))
            return false;

        return true;
    }

    bool get_head_pos(std::uintptr_t pawn, Vec3 &out) {
        return get_bone_pos(pawn, 7, out); // 头骨骼索引 7
    }

    std::uintptr_t get_entity_from_index(int index) {
        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (!base) return 0;

        const std::uintptr_t list = tools::mem::read<std::uintptr_t>(
            base + off::client::dwEntityList); // entity list 基址
        if (!list) return 0;

        const std::uintptr_t chunk = tools::mem::read<std::uintptr_t>(
            list + 0x10 + 0x8 * (index >> 9)); // 对应 chunk
        if (!chunk) return 0;

        return tools::mem::read_raw<std::uintptr_t>(
            chunk + schema::kEntityStep * (index & 0x1FF)); // 对应 slot
    }

    int get_armor(std::uintptr_t pawn) {
        if (!pawn) return 0;
        return tools::mem::read_raw<int>(pawn + schema::m_ArmorValue); // m_ArmorValue
    }

    bool has_helmet(std::uintptr_t pawn) {
        if (!pawn) return false;

        const std::uintptr_t item_services = tools::mem::read_raw<std::uintptr_t>(
            pawn + schema::m_pItemServices); // 物品服务
        if (!item_services) return false;

        return tools::mem::read_raw<std::uint8_t>(
                   item_services + schema::m_bHasHelmet) != 0; // m_bHasHelmet
    }

    float get_distance(const Vec3 &a, const Vec3 &b) {
        const float dx = a.x - b.x; // x 差值
        const float dy = a.y - b.y; // y 差值
        const float dz = a.z - b.z; // z 差值
        return std::sqrt(dx * dx + dy * dy + dz * dz) * 0.01905f; // 换算成米
    }

    const char *get_weapon_name(std::uintptr_t pawn) {
        static char buf[32] = "unknown"; // 武器名缓冲区

        if (!pawn) return buf;

        const std::uintptr_t weapon_services = tools::mem::read_raw<std::uintptr_t>(
            pawn + schema::m_pWeaponServices); // 武器服务
        if (!weapon_services) return buf;

        const std::uint32_t handle = tools::mem::read_raw<std::uint32_t>(
            weapon_services + schema::m_hActiveWeapon); // 当前武器句柄
        if (!handle || handle == 0xFFFFFFFF) return buf;

        const int weapon_index = handle & 0x7FFF; // 武器实体索引
        const std::uintptr_t weapon = get_entity_from_index(weapon_index); // 武器实体
        if (!weapon) return buf;

        const std::uint16_t def_index = tools::mem::read_raw<std::uint16_t>(
            weapon + schema::m_AttributeManager
            + schema::m_Item
            + schema::m_iItemDefinitionIndex); // 物品定义索引

        switch (def_index) {
            case 1: return "沙鹰";
            case 2: return "双持贝瑞塔";
            case 3: return "FN57";
            case 4: return "格洛克";
            case 7: return "AK47";
            case 8: return "AUG";
            case 9: return "AWP";
            case 10: return "法玛斯";
            case 11: return "G3SG1";
            case 13: return "加利尔";
            case 14: return "M249";
            case 16: return "M4A4";
            case 17: return "MAC10";
            case 19: return "P90";
            case 23: return "MP5SD";
            case 24: return "UMP45";
            case 25: return "XM1014";
            case 26: return "PP野牛";
            case 27: return "MAG7";
            case 28: return "内格夫";
            case 29: return "截短霰弹枪";
            case 30: return "Tec9";
            case 31: return "电击枪";
            case 32: return "P2000";
            case 33: return "MP7";
            case 34: return "MP9";
            case 35: return "新星";
            case 36: return "P250";
            case 38: return "SCAR20";
            case 39: return "SG556";
            case 40: return "SSG08";
            case 41: return "T刀";
            case 42: return "CT刀";
            case 43: return "闪光弹";
            case 44: return "高爆手雷";
            case 45: return "烟雾弹";
            case 46: return "燃烧瓶";
            case 47: return "诱饵弹";
            case 48: return "燃烧弹";
            case 49: return "C4";
            case 59: return "幽灵刀";
            case 60: return "M4A1消音";
            case 61: return "USP消音";
            case 63: return "CZ75";
            case 64: return "R8左轮";
            case 500: return "刺刀";
            case 503: return "经典刀";
            case 505: return "折叠刀";
            case 506: return "穿肠刀";
            case 507: return "爪子刀";
            case 508: return "M9刺刀";
            case 509: return "猎杀者匕首";
            case 512: return "弯刀";
            case 514: return "鲍伊猎刀";
            case 515: return "蝴蝶刀";
            case 516: return "暗影双匕";
            case 517: return "系绳匕首";
            case 518: return "求生匕首";
            case 519: return "熊刀";
            case 520: return "折刀";
            case 521: return "流浪者匕首";
            case 522: return "短剑";
            case 523: return "锯齿爪刀";
            case 525: return "骷髅匕首";
            case 526: return "库克里刀";
            default:
                std::snprintf(buf, sizeof(buf), "id_%d", def_index); // 未知武器
                return buf;
        }
    }

    std::uintptr_t get_planted_c4() {
        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (!base) return 0;
        return tools::mem::read<std::uintptr_t>(
            base + off::client::dwPlantedC4); // 已安放 C4
    }

    // 纯内存读取个人视野掩码
    bool is_visible(std::uintptr_t local_pawn, std::uintptr_t target_pawn) {
        if (!local_pawn || !target_pawn) return false;
        if (local_pawn == target_pawn) return false;

        __try {
            // 1. 获取本地玩家的 Controller
            std::uintptr_t local_controller = get_controller_from_pawn(local_pawn);
            if (!local_controller) return false;

            // 2. 通过 EntityList 反查 Controller 的实体索引 (Entity Index)
            const std::uintptr_t base = tools::mem::client_base();
            if (!base) return false;
            const std::uintptr_t list = tools::mem::read<std::uintptr_t>(base + off::client::dwEntityList);
            if (!list) return false;

            int local_entity_index = -1;
            // 遍历 64 个 Chunk（group），每个 Chunk 有 512 个槽位（slot）
            for (int group = 0; group < 64; ++group) {
                const std::uintptr_t chunk = tools::mem::read<std::uintptr_t>(list + 0x10 + 0x8 * group);
                if (!chunk) continue;
                for (int slot = 0; slot < 512; ++slot) {
                    if (tools::mem::read_raw<std::uintptr_t>(chunk + schema::kEntityStep * slot) == local_controller) {
                        // 还原全局实体索引： group * 512 + slot，即 group << 9 加 slot
                        local_entity_index = (group << 9) + slot;
                        break;
                    }
                }
                if (local_entity_index != -1) break;
            }
            if (local_entity_index <= 0) return false;

            // 3. 计算 PlayerSlot (CS2 规则: Slot = EntityIndex - 1)
            int local_slot = local_entity_index - 1;
            if (local_slot < 0 || local_slot > 63) return false;

            // 4. 读取目标玩家的 m_bSpottedByMask (你 schema 里的 0x1E80 + 0xC)
            std::uint64_t spotted_mask = tools::mem::read_raw<std::uint64_t>(
                target_pawn + schema::m_entitySpottedState + schema::m_bSpottedByMask
            );

            // 5. 检查掩码中对应本地 PlayerSlot 的位是否为 1
            return (spotted_mask & (1ULL << local_slot)) != 0;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    bool is_visible_throttled(std::uintptr_t local_pawn, std::uintptr_t target_pawn) {
        if (!local_pawn || !target_pawn) return false;

        ++g_vis_tick;
        auto it = g_vis_cache.find(target_pawn); // 查缓存
        if (it != g_vis_cache.end() && g_vis_tick - it->second.first < 3)
            return it->second.second; // 3 帧内复用

        const bool v = is_visible(local_pawn, target_pawn); // 实际查询
        g_vis_cache[target_pawn] = {g_vis_tick, v}; // 写入缓存
        return v;
    }

    const std::vector<PawnSnapshot> &build_snapshot(std::uintptr_t local) {
        g_snapshot.clear(); // 清空上一帧
        if (!local) return g_snapshot;

        const int local_team = get_team(local); // 本地阵营
        const std::vector<std::uintptr_t> &pawns = get_pawns_cached(); // 敌人列表缓存

        for (std::uintptr_t e: pawns) {
            if (e == local) continue; // 跳过自己

            const int team = get_team(e); // 实体阵营
            if (team != 2 && team != 3) continue;
            if (team == local_team) continue; // 跳过队友

            PawnSnapshot s{}; // 快照
            s.ptr = e;
            s.team = team;
            s.health = get_health(e); // 血量
            if (s.health <= 0) continue; // 死了跳过

            s.armor = get_armor(e); // 护甲
            s.helmet = has_helmet(e); // 头盔
            s.ducking = is_ducking(e); // 蹲下

            s.origin = get_origin(e); // 脚底坐标
            if (s.origin.x == 0.0f && s.origin.y == 0.0f && s.origin.z == 0.0f) continue;

            if (!get_bone_pos(e, 7, s.head)) {
                s.head = s.origin;
                s.head.z += s.ducking ? 46.0f : 64.0f; // 估算眼睛高度
            }

            get_bone_pos(e, 6, s.neck); // 脖子
            get_bone_pos(e, 23, s.chest); // 胸部
            get_bone_pos(e, 4, s.body); // 身体（脊柱）
            get_bone_pos(e, 1, s.pelvis); // 骨盆

            get_bone_pos(e, 9, s.armL); // 左大臂
            get_bone_pos(e, 10, s.forearmL); // 左小臂
            get_bone_pos(e, 11, s.handL); // 左手

            get_bone_pos(e, 13, s.armR); // 右大臂
            get_bone_pos(e, 14, s.forearmR); // 右小臂
            get_bone_pos(e, 15, s.handR); // 右手

            get_bone_pos(e, 17, s.thighL); // 左大腿
            get_bone_pos(e, 18, s.calfL); // 左小腿
            get_bone_pos(e, 19, s.footL); // 左脚

            get_bone_pos(e, 20, s.thighR); // 右大腿
            get_bone_pos(e, 21, s.calfR); // 右小腿
            get_bone_pos(e, 22, s.footR); // 右脚

            s.controller = get_controller_from_pawn(e); // 对应 Controller
            g_snapshot.push_back(s); // 收进快照
        }
        return g_snapshot;
    }

        std::vector<std::uintptr_t> get_all_c4() {
        std::vector<std::uintptr_t> result; // 结果列表

        // 1. 首先获取已安放的 C4（使用 dwPlantedC4 偏移）
        std::uintptr_t planted_c4 = get_planted_c4();
        if (planted_c4) {
            result.push_back(planted_c4);
        }

        // 2. 扫描 EntityList 找 掉落 / 背包里的 C4
        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (!base) return result;

        const std::uintptr_t list = tools::mem::read<std::uintptr_t>(
            base + off::client::dwEntityList); // entity list 基址
        if (!list) return result;

        for (int group = 0; group < 16; ++group) {
            const std::uintptr_t chunk = tools::mem::read<std::uintptr_t>(
                list + 0x10 + 0x8 * group); // 第 group 个 chunk
            if (!chunk) continue;

            MEMORY_BASIC_INFORMATION mbi{}; // 内存信息
            if (!VirtualQuery(reinterpret_cast<LPCVOID>(chunk), &mbi, sizeof(mbi))) continue;
            if (mbi.State != MEM_COMMIT) continue;
            if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) continue;

            for (int slot = 0; slot < 512; ++slot) {
                std::uintptr_t e = 0; // 当前实体
                if (!tools::mem::safe_memcpy(
                    &e,
                    reinterpret_cast<const void *>(chunk + schema::kEntityStep * slot),
                    sizeof(e)))
                    continue;

                if (!e || e < 0x10000 || e >= 0x7FF000000000ULL) continue; // 地址非法

                // 避免重复添加（如果已安放C4在EntityList里也有一份）
                if (e == planted_c4) continue;

                // 读物品定义索引，49 = C4
                const std::uint16_t def_index = tools::mem::read_raw<std::uint16_t>(
                    e + schema::m_AttributeManager
                    + schema::m_Item
                    + schema::m_iItemDefinitionIndex);
                if (def_index != 49) continue; // 不是掉落/背包里的 C4

                result.push_back(e); // 收进结果
            }
        }

        return result;
    }

    const std::vector<std::uintptr_t> &get_c4_cached() {
        if (g_c4_tick <= 0) {
            g_c4_cache = get_all_c4(); // 重新扫描
            g_c4_tick = C4_CACHE_INTERVAL; // 重置计数
        }
        --g_c4_tick;

        bool need_rescan = false; // 是否需要立即重扫
        for (std::uintptr_t c4: g_c4_cache) {
            if (!c4) {
                need_rescan = true;
                break;
            }

            // 如果是已安放的 C4，它没有 def_index，只要指针不为空就认为有效
            // 你可以简单加一个判断：如果 c4 指向的是已安放的 C4
            if (c4 == get_planted_c4()) {
                continue;
            }

            // 否则（背包里或地上的），才校验 def_index
            const std::uint16_t def_index = tools::mem::read_raw<std::uint16_t>(
                c4 + schema::m_AttributeManager
                + schema::m_Item
                + schema::m_iItemDefinitionIndex);
            if (def_index != 49) {
                need_rescan = true;
                break;
            }
        }

        if (need_rescan) {
            g_c4_cache = get_all_c4(); // 立即重扫
            g_c4_tick = C4_CACHE_INTERVAL; // 重置计数
        }

        return g_c4_cache;
    }

    Vec3 get_c4_origin(std::uintptr_t c4) {
        if (!c4) return {};

        // 1. 尝试通过节点读绝对坐标（对已安放和掉地上的都有效）
        Vec3 pos = get_origin(c4);
        if (pos.x != 0.0f || pos.y != 0.0f || pos.z != 0.0f) return pos;

        // 2. 有些情况 C4 节点读不到，尝试直接读实体自身的坐标（老版本做法）
        pos = tools::mem::read_raw<Vec3>(c4 + schema::m_vecOrigin);
        if (pos.x != 0.0f || pos.y != 0.0f || pos.z != 0.0f) return pos;

        return {}; // 返回空坐标
    }

    void NormalizeAngles(float &angle) {
        while (angle > 180.0f) angle -= 360.0f;
        while (angle < -180.0f) angle += 360.0f;
    }


    void update_rcs_visual() {
        static float old_aimpunch_x = 0.0f;
        static float old_aimpunch_y = 0.0f;

        std::uintptr_t pawn = local_player();
        if (!pawn) return;

        int shots_fired = tools::mem::read_raw<int>(pawn + schema::m_iShotsFired);

        if (shots_fired <= 0) {
            old_aimpunch_x = 0.0f;
            old_aimpunch_y = 0.0f;
            return;
        }

        std::uintptr_t aimPunchServices = tools::mem::read_raw<std::uintptr_t>(pawn + schema::m_pAimPunchServices);
        if (!aimPunchServices) return;

        float aimpunch_x = tools::mem::read_raw<float>(aimPunchServices + schema::m_predictableBaseAngle);
        float aimpunch_y = tools::mem::read_raw<float>(aimPunchServices + schema::m_predictableBaseAngle + 4);

        const std::uintptr_t base = tools::mem::client_base();
        auto local_view_angles = reinterpret_cast<Vec3 *>(base + off::client::dwViewAngles);
        if (!local_view_angles) return;

        Vec3 new_view_angles{};
        new_view_angles.x = local_view_angles->x + old_aimpunch_x - (aimpunch_x * 2.0f);
        new_view_angles.y = local_view_angles->y + old_aimpunch_y - (aimpunch_y * 2.0f);

        NormalizeAngles(new_view_angles.x);
        NormalizeAngles(new_view_angles.y);

        local_view_angles->x = new_view_angles.x;
        local_view_angles->y = new_view_angles.y;

        old_aimpunch_x = aimpunch_x * 2.0f;
        old_aimpunch_y = aimpunch_y * 2.0f;
    }
}
