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
     * 全局缓存正确步长
     * 第一次调用时自动探测，用 local_pawn 反推，之后复用
     * 0 = 未探测
     */
    static std::uintptr_t g_entity_step = 0; // entity list chunk 步长缓存，0 表示未探测

    /*
     * 探测 entity list 的 chunk 步长
     * 用 local_pawn（已知的正确 Pawn）反推
     * 返回 0x70 或 0x78，找不到返回 0x78 兜底
     */
    static std::uintptr_t detect_step() {
        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (!base) return 0x78;

        const std::uintptr_t local_pawn = tools::mem::read<std::uintptr_t>(
            base + off::client::dwLocalPlayerPawn); // 本地玩家 Pawn
        if (!local_pawn) return 0x78;

        const std::uintptr_t list = tools::mem::read<std::uintptr_t>(
            base + off::client::dwEntityList); // entity list 基址
        if (!list) return 0x78;

        // 试 0x70 和 0x78
        for (std::uintptr_t step: {0x70, 0x78}) {
            for (int group = 0; group < 16; ++group) {
                const std::uintptr_t chunk = tools::mem::read<std::uintptr_t>(
                    list + 0x10 + 8 * group); // 第 group 个 chunk
                if (!chunk) continue;

                MEMORY_BASIC_INFORMATION mbi{}; // 内存信息
                if (!VirtualQuery(reinterpret_cast<LPCVOID>(chunk), &mbi, sizeof(mbi)))
                    continue;
                if (mbi.State != MEM_COMMIT) continue;

                for (int slot = 0; slot < 512; ++slot) {
                    std::uintptr_t v = read_fast<std::uintptr_t>(
                        chunk + step * slot); // 当前槽位实体
                    if (v == local_pawn) {
                        return step;
                    }
                }
            }
        }

        return 0x78; // 兜底
    }

    /*
     * 获取步长，第一次探测，之后缓存
     */
    static std::uintptr_t get_step() {
        if (g_entity_step == 0) {
            g_entity_step = detect_step();
        }
        return g_entity_step;
    }

    // 全局缓存：ViewMatrix
    static float g_view_matrix[16] = {}; // 视图矩阵缓存

    // 全局缓存：屏幕尺寸
    static float g_screen_w = 0.0f; // 屏幕宽度
    static float g_screen_h = 0.0f; // 屏幕高度

    // 全局缓存：敌人列表
    static std::vector<std::uintptr_t> g_pawns; // 敌人 Pawn 列表缓存
    static int g_pawns_tick = 0; // 缓存剩余帧数
    static constexpr int PAWNS_CACHE_INTERVAL = 3; // 敌人列表刷新间隔（帧）

    /*
     * 每帧调用一次
     * 刷新 ViewMatrix 和屏幕尺寸
     */
    void update_frame_cache() {
        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (base) {
            const std::uintptr_t matrix = base + off::client::dwViewMatrix; // 视图矩阵地址
            for (int i = 0; i < 16; ++i) {
                __try {
                    g_view_matrix[i] = *reinterpret_cast<const float *>(
                        matrix + i * sizeof(float));
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    g_view_matrix[i] = 0.0f;
                }
            }
        }

        RECT rect{}; // 窗口客户区
        GetClientRect(GetForegroundWindow(), &rect);
        g_screen_w = static_cast<float>(rect.right - rect.left); // 屏幕宽度
        g_screen_h = static_cast<float>(rect.bottom - rect.top); // 屏幕高度
    }

    /*
     * 用缓存的 ViewMatrix 转屏幕坐标
     */
    bool world_to_screen(const Vec3 &world, float &sx, float &sy) {
        const float *m = g_view_matrix; // 视图矩阵

        const float w = m[12] * world.x + m[13] * world.y
                        + m[14] * world.z + m[15]; // 裁剪空间 w
        if (w < 0.65f) return false;

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

    /*
     * 敌人列表缓存
     * 每 PAWNS_CACHE_INTERVAL 帧刷新一次
     */
    const std::vector<std::uintptr_t> &get_pawns_cached() {
        if (g_pawns_tick <= 0) {
            g_pawns = get_all_pawns();
            g_pawns_tick = PAWNS_CACHE_INTERVAL;
        }
        --g_pawns_tick;
        return g_pawns;
    }

    /*
     * 从 entity list 按 index 取 Controller
     */
    std::uintptr_t get_controller_by_index(int index) {
        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (!base) return 0;

        const std::uintptr_t list = tools::mem::read<std::uintptr_t>(
            base + off::client::dwEntityList); // entity list 基址
        if (!list) return 0;

        const std::uintptr_t chunk = tools::mem::read<std::uintptr_t>(
            list + 8 * (index >> 9) + 0x10); // 对应 chunk
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
            controller + schema::m_hPlayerPawn); // 玩家 Pawn 句柄
        if (!handle || handle == 0xFFFFFFFF) return 0;

        const int pawn_index = handle & 0x7FFF; // Pawn 实体索引

        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (!base) return 0;

        const std::uintptr_t list = tools::mem::read<std::uintptr_t>(
            base + off::client::dwEntityList); // entity list 基址
        if (!list) return 0;

        const std::uintptr_t chunk = tools::mem::read<std::uintptr_t>(
            list + 8 * (pawn_index >> 9) + 0x10); // 对应 chunk
        if (!chunk) return 0;

        return tools::mem::read<std::uintptr_t>(
            chunk + get_step() * (pawn_index & 0x1FF));
    }

    /*
     * 按玩家槽位 index（1~64）取 Pawn
     */
    std::uintptr_t get_pawn_by_index(int index) {
        const std::uintptr_t controller = get_controller_by_index(index); // 对应 Controller
        if (!controller) return 0;
        return get_pawn_from_controller(controller);
    }

    /*
     * 取本地玩家 Controller
     */
    std::uintptr_t local_controller() {
        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (!base) return 0;
        return tools::mem::read<std::uintptr_t>(
            base + off::client::dwLocalPlayerController);
    }

    /*
     * 取本地玩家 Pawn
     */
    std::uintptr_t local_player() {
        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (!base) return 0;
        return tools::mem::read<std::uintptr_t>(
            base + off::client::dwLocalPlayerPawn);
    }

    /*
     * 读血量
     */
    int get_health(std::uintptr_t entity) {
        if (!entity) return 0;
        return read_fast<int>(entity + schema::m_iHealth); // 当前血量
    }

    /*
     * 读阵营
     */
    int get_team(std::uintptr_t entity) {
        if (!entity) return 0;
        return read_fast<std::uint8_t>(entity + schema::m_iTeamNum); // 阵营编号
    }

    /*
     * 读 lifeState
     */
    int get_life_state(std::uintptr_t entity) {
        if (!entity) return 1;
        return read_fast<std::uint8_t>(entity + schema::m_lifeState); // 生命状态
    }

    /*
     * 是否存活
     */
    bool is_alive(std::uintptr_t entity) {
        if (!entity) return false;
        const int hp = get_health(entity); // 当前血量
        const int max_hp = read_fast<int>(entity + schema::m_iMaxHealth); // 最大血量
        return hp > 0 || max_hp > 0;
    }

    /*
     * 是否敌人
     */
    bool is_enemy(std::uintptr_t local, std::uintptr_t target) {
        if (!local || !target) return false;
        if (target == local) return false;
        if (!is_alive(target)) return false;

        const int local_team = get_team(local); // 本地阵营
        const int target_team = get_team(target); // 目标阵营

        if (target_team != 2 && target_team != 3) return false;
        return target_team != local_team;
    }

    /*
     * 读世界坐标
     */
    Vec3 get_origin(std::uintptr_t entity) {
        if (!entity) return {};

        const std::uintptr_t node = read_fast<std::uintptr_t>(
            entity + schema::m_pGameSceneNode); // 场景节点
        if (!node) return {};

        return read_fast<Vec3>(node + schema::m_vecAbsOrigin); // 绝对世界坐标
    }

    /*
     * 扫 entity list 的所有 chunk + slot，返回所有 team 2/3 且 pos 有效的实体
     * 用自动探测的步长
     *
     * 优化：chunk 只查一次 VirtualQuery，后续所有读用 read_fast，不带 VirtualQuery
     * 之前每帧 40960 次 VirtualQuery，现在每帧 16 次
     */
    std::vector<std::uintptr_t> get_all_pawns() {
        std::vector<std::uintptr_t> result; // 结果列表

        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (!base) return result;

        const std::uintptr_t list = tools::mem::read<std::uintptr_t>(
            base + off::client::dwEntityList); // entity list 基址
        if (!list) return result;

        const std::uintptr_t step = get_step(); // chunk 步长

        for (int group = 0; group < 16; ++group) {
            const std::uintptr_t chunk = tools::mem::read<std::uintptr_t>(
                list + 0x10 + 8 * group); // 第 group 个 chunk
            if (!chunk) continue;

            // 只对 chunk 检查一次 VirtualQuery
            MEMORY_BASIC_INFORMATION mbi{}; // 内存信息
            if (!VirtualQuery(reinterpret_cast<LPCVOID>(chunk), &mbi, sizeof(mbi)))
                continue;
            if (mbi.State != MEM_COMMIT) continue;
            if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) continue;

            // 边界保护：chunk + step * 512 不能溢出
            const std::uintptr_t chunk_end = chunk + step * 512; // chunk 结束地址
            if (chunk_end < chunk) continue;

            for (int slot = 0; slot < 512; ++slot) {
                // read_fast：不带 VirtualQuery
                std::uintptr_t e = read_fast<std::uintptr_t>(chunk + step * slot); // 当前实体
                if (!e || e < 0x10000 || e >= 0x7FFFFFFFFFFF
                    || e >= 0x7FF000000000ULL)
                    continue;

                const int team = read_fast<std::uint8_t>(
                    e + schema::m_iTeamNum); // 实体阵营
                if (team != 2 && team != 3) continue;

                const int hp = read_fast<int>(e + schema::m_iHealth); // 实体血量
                if (hp <= 0) continue; // 死了，立即跳过

                const int life = read_fast<std::uint8_t>(e + schema::m_lifeState); // 生命状态
                if (life != 0) continue; // lifeState != 0 也跳过

                const std::uintptr_t node = read_fast<std::uintptr_t>(
                    e + schema::m_pGameSceneNode); // 场景节点
                if (!node) continue;

                const Vec3 pos = read_fast<Vec3>(
                    node + schema::m_vecAbsOrigin); // 世界坐标
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

        const std::uint32_t handle = read_fast<std::uint32_t>(
            pawn + 0x14BC); // m_hController
        if (!handle || handle == 0xFFFFFFFF) return 0;

        const int index = handle & 0x7FFF; // Controller 实体索引
        return get_controller_by_index(index);
    }

    // 读 Controller 的玩家名字
    // 返回内部静态缓冲区，调用完立刻用，不要保存指针
    const char *get_player_name(std::uintptr_t controller) {
        static char buf[128]; // 名字缓冲区
        buf[0] = '\0';

        if (!controller) return buf;

        // m_iszPlayerName 是 char[128]
        for (int i = 0; i < 127; ++i) {
            char c = read_fast<char>(controller + 0x6FC + i); // 当前字符
            buf[i] = c;
            if (c == '\0') break;
        }
        buf[127] = '\0';
        return buf;
    }

    /*
     * 是否蹲下
     * 优先读 m_bDucking（bool），失败则退到 m_flDuckAmount > 0.5
     * 两个偏移都需要在 schema.hpp 里补上，见下方说明
    */
    bool is_ducking(std::uintptr_t pawn) {
        if (!pawn) return false;

        const std::uintptr_t movement = read_fast<std::uintptr_t>(
            pawn + schema::m_pMovementServices); // 移动服务
        if (!movement) return false;

        const std::uint8_t ducking = read_fast<std::uint8_t>(
            movement + schema::m_bDucking); // 蹲下标志
        if (!(ducking != 0)) {
            const float amount = read_fast<float>(
                movement + schema::m_flDuckAmount); // 蹲下程度
            return amount > 0.5f;
        }
        return true;
    }

    /*
     * 读取单个骨骼的世界坐标
     * Pawn → m_pGameSceneNode → CSkeletonInstance + m_modelState → m_BoneArray
     * 每个 bone 是 matrix3x4_t（48 字节），位置在矩阵第 4 列
    */
    bool get_bone_pos(std::uintptr_t pawn, int bone_index, Vec3 &out) {
        if (!pawn) return false;

        const std::uintptr_t node = read_fast<std::uintptr_t>(
            pawn + schema::m_pGameSceneNode); // 场景节点
        if (!node) return false;

        const std::uintptr_t bone_array = read_fast<std::uintptr_t>(
            node + schema::m_modelState + schema::m_BoneArray); // 骨骼数组
        if (!bone_array) return false;

        if (bone_index < 0 || bone_index > 128) return false;

        // 步长 32 字节，位置在前 3 个 float
        const std::uintptr_t bone_addr = bone_array + bone_index * 32; // 骨骼地址

        out.x = read_fast<float>(bone_addr + 0x00); // 骨骼 x
        out.y = read_fast<float>(bone_addr + 0x04); // 骨骼 y
        out.z = read_fast<float>(bone_addr + 0x08); // 骨骼 z

        return true;
    }

    // 读头骨骼的世界坐标（索引 7）
    bool get_head_pos(std::uintptr_t pawn, Vec3 &out) {
        return get_bone_pos(pawn, 7, out);
    }

    // 从 entity index 拿实体（复用你已有的逻辑）
    std::uintptr_t get_entity_from_index(int index) {
        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (!base) return 0;

        const std::uintptr_t list = tools::mem::read<std::uintptr_t>(
            base + off::client::dwEntityList); // entity list 基址
        if (!list) return 0;

        const std::uintptr_t chunk = tools::mem::read<std::uintptr_t>(
            list + 8 * (index >> 9) + 0x10); // 对应 chunk
        if (!chunk) return 0;

        return tools::mem::read<std::uintptr_t>(
            chunk + get_step() * (index & 0x1FF));
    }

    // 读护甲值
    int get_armor(std::uintptr_t pawn) {
        if (!pawn) return 0;
        return read_fast<int>(pawn + 0x1EC8); // m_ArmorValue
    }

    // 读是否有头盔
    bool has_helmet(std::uintptr_t pawn) {
        if (!pawn) return false;

        const std::uintptr_t item_services = read_fast<std::uintptr_t>(
            pawn + schema::m_pItemServices); // 物品服务
        if (!item_services) return false;

        return read_fast<std::uint8_t>(item_services + 0x49) != 0; // m_bHasHelmet
    }

    // 计算两点世界距离（米）
    float get_distance(const Vec3 &a, const Vec3 &b) {
        const float dx = a.x - b.x; // x 差值
        const float dy = a.y - b.y; // y 差值
        const float dz = a.z - b.z; // z 差值
        return std::sqrt(dx * dx + dy * dy + dz * dz) * 0.01905f;
    }

    // 读武器名称（用 item definition index 查表）
    const char *get_weapon_name(std::uintptr_t pawn) {
        static char buf[32] = "unknown"; // 武器名缓冲区

        if (!pawn) return buf;

        const std::uintptr_t weapon_services = read_fast<std::uintptr_t>(
            pawn + schema::m_pWeaponServices); // 武器服务
        if (!weapon_services) return buf;

        const std::uint32_t handle = read_fast<std::uint32_t>(
            weapon_services + schema::m_hActiveWeapon); // 当前武器句柄
        if (!handle || handle == 0xFFFFFFFF) return buf;

        const int weapon_index = handle & 0x7FFF; // 武器实体索引
        const std::uintptr_t weapon = get_entity_from_index(weapon_index); // 武器实体
        if (!weapon) return buf;

        const std::uintptr_t attr = read_fast<std::uintptr_t>(
            weapon + schema::m_AttributeManager); // 属性管理器
        if (!attr) return buf;

        const std::uintptr_t item = attr + schema::m_Item; // 物品
        const std::uint16_t def_index = read_fast<std::uint16_t>(
            item + schema::m_iItemDefinitionIndex); // 物品定义索引

        // 常见武器 ID 表
        switch (def_index) {
            case 1: return "deagle";
            case 2: return "elite";
            case 3: return "fiveseven";
            case 4: return "glock";
            case 7: return "ak47";
            case 8: return "aug";
            case 9: return "awp";
            case 10: return "famas";
            case 11: return "g3sg1";
            case 13: return "galil";
            case 14: return "m249";
            case 16: return "m4a4";
            case 17: return "mac10";
            case 19: return "p90";
            case 23: return "mp5sd";
            case 24: return "ump45";
            case 25: return "xm1014";
            case 26: return "bizon";
            case 27: return "mag7";
            case 28: return "negev";
            case 29: return "sawedoff";
            case 30: return "tec9";
            case 31: return "taser";
            case 32: return "hkp2000";
            case 33: return "mp7";
            case 34: return "mp9";
            case 35: return "nova";
            case 36: return "p250";
            case 38: return "scar20";
            case 39: return "sg556";
            case 40: return "ssg08";
            case 60: return "m4a1_silencer";
            case 61: return "usp_silencer";
            case 63: return "cz75a";
            case 64: return "revolver";
            default:
                std::snprintf(buf, sizeof(buf), "id_%d", def_index);
                return buf;
        }
    }
}