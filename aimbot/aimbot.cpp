#include "aimbot.hpp"
#include "../game/entity.hpp"
#include "../tools/memory.hpp"
#include "pattern_scan.hpp"
#include "offsets.hpp"
#include "schema.hpp"

#include <Windows.h>
#include <cmath>
#include <limits>
#include <vector>

#include "MinHook.h"

namespace aimbot {
    // 共用参数
    float g_circle_radius = 320.0f; // 屏幕中心圆半径（像素）
    float g_max_world_dist = 700.0f; // 最大世界距离

    // 选目标偏好
    bool g_prefer_visible = true; // 优先瞄准可见敌人
    bool g_prefer_targeting_me = true; // 优先瞄准正在瞄准我的敌人
    bool g_only_visible = true; // 只瞄准可见敌人（硬过滤）

    // 暴力瞄准
    bool g_enabled = false; // 暴力自瞄开关

    // 子弹追踪
    bool g_silent_enabled = false; // 子弹追踪开关
    float g_silent_pitch = 0.0f; // 静默瞄准 pitch（俯仰角）
    float g_silent_yaw = 0.0f; // 静默瞄准 yaw（偏航角）
    bool g_silent_has_target = false; // 本帧是否有静默目标

    // RCS
    bool g_rcs_enabled = false; // RCS 压枪开关
    float g_rcs_strength = 0.5f; // RCS 压枪强度（0~1）

    // 自动开火
    bool g_auto_fire_enabled = false; // 自动开火开关

    // 平滑配置
    SmoothType g_smooth_type = SmoothType::Exponential; // 当前平滑算法
    float g_smooth_x = 5.0f; // 水平平滑系数
    float g_smooth_y = 5.0f; // 垂直平滑系数

    uint32_t g_target_bones = Bone_Head; // 默认瞄准头部

    constexpr float kEyeStand = 64.0f; // 站立时眼睛高度
    constexpr float kEyeDuck = 46.0f; // 蹲下时眼睛高度

    // 根据 CE 和 IDA 验证过的特征码
    static const char* kFillUserCmdPatterns[] = {
        // 完整特征（包括 sub rsp 栈大小调整）
        "48 89 5C 24 18 55 57 41 56 48 8D 6C 24 C9 48 81 EC B0 00 00 00",
        // 容忍堆栈大小变化的模糊特征
        "48 89 5C 24 ? 55 57 41 56 48 8D 6C 24 ? 48 81 EC ? ? ? ?",
    };

    using FillUserCmdFn = __int64(*)(int *a1, __int64 a2);
    FillUserCmdFn oFillUserCmd = nullptr;

    __int64 __fastcall hkFillUserCmd(int *a1, __int64 a2) {
        // 防护：防止空指针导致崩溃
        if (!a1) return oFillUserCmd(a1, a2);

        if (g_silent_enabled && g_silent_has_target) {
            *(float *) ((char *) a1 + 0x10) = g_silent_pitch; // 覆盖源 pitch
            *(float *) ((char *) a1 + 0x14) = g_silent_yaw;   // 覆盖源 yaw
            *(float *) ((char *) a1 + 0x18) = 0.0f;           // 清零 roll
        }
        return oFillUserCmd(a1, a2);
    }

    void install_silent_hook() {
        uintptr_t base = tools::mem::client_base();
        if (!base) return;

        uintptr_t target = 0;

        // 1. 使用我们专属的特征码扫描函数
        for (const char* pattern : kFillUserCmdPatterns) {
            target = Scanner::scan_fill_user_cmd_pattern(pattern);
            if (target) break;
        }

        if (!target) return;

        // 3. 校验并 Hook
        uint8_t* func_start = reinterpret_cast<uint8_t*>(target);
        if (func_start[0] == 0x48 && func_start[1] == 0x89 && func_start[2] == 0x5C && func_start[3] == 0x24) {
            MH_STATUS st = MH_CreateHook((LPVOID) target, (LPVOID) hkFillUserCmd, (LPVOID *) &oFillUserCmd);
            if (st == MH_OK) {
                MH_EnableHook((LPVOID) target);
            }
        }
    }

    // 计算目标角度
    static void calc_angle(const game::Vec3 &local_eye, const game::Vec3 &target_aim, float &pitch, float &yaw) {
        const float dx = target_aim.x - local_eye.x; // x 差值
        const float dy = target_aim.y - local_eye.y; // y 差值
        const float dz = target_aim.z - local_eye.z; // z 差值
        const float hyp = std::sqrt(dx * dx + dy * dy); // 水平距离
        yaw = std::atan2(dy, dx) * 180.0f / 3.14159265f; // 偏航角
        pitch = -std::atan2(dz, hyp) * 180.0f / 3.14159265f; // 俯仰角
    }

    // 写视角（支持三种算法，分 X/Y 独立平滑）
    static void set_view_angles(float target_pitch, float target_yaw) {
        const std::uintptr_t base = tools::mem::client_base(); // client 模块基址
        if (!base) return;
        const std::uintptr_t angles = base + off::client::dwViewAngles; // 视角地址

        // 读取游戏引擎当前的真实视角
        const float cur_pitch = tools::mem::read_raw<float>(angles + 0x0); // 当前 pitch
        const float cur_yaw = tools::mem::read_raw<float>(angles + 0x4); // 当前 yaw

        float dp = target_pitch - cur_pitch; // pitch 差
        float dy = target_yaw - cur_yaw; // yaw 差
        while (dy > 180.0f) dy -= 360.0f; // 归一化
        while (dy < -180.0f) dy += 360.0f;

        // 默认不移动
        float new_pitch = cur_pitch;
        float new_yaw = cur_yaw;

        if (g_smooth_type == SmoothType::Exponential) {
            float factorX = g_smooth_x <= 1.0f ? 1.0f : (1.0f / g_smooth_x);
            float factorY = g_smooth_y <= 1.0f ? 1.0f : (1.0f / g_smooth_y);
            new_pitch = cur_pitch + dp * factorX;
            new_yaw = cur_yaw + dy * factorY;
        } else if (g_smooth_type == SmoothType::Linear) {
            float stepX = g_smooth_x * 0.1f;
            float stepY = g_smooth_y * 0.1f;
            new_pitch = cur_pitch + (dp > stepX ? stepX : (dp < -stepX ? -stepX : dp));
            new_yaw = cur_yaw + (dy > stepY ? stepY : (dy < -stepY ? -stepY : dy));
        } else {
            float factorX = g_smooth_x <= 1.0f ? 1.0f : (1.0f / g_smooth_x);
            float factorY = g_smooth_y <= 1.0f ? 1.0f : (1.0f / g_smooth_y);
            new_pitch = cur_pitch + dp * (factorX * 0.8f);
            new_yaw = cur_yaw + dy * (factorY * 0.8f);
        }

        // 核心修正：当准星已经无限接近目标时，直接吸附过去，消灭虚位
        if (std::fabs(dp) < 0.05f) new_pitch = target_pitch;
        if (std::fabs(dy) < 0.05f) new_yaw = target_yaw;

        tools::mem::write<float>(angles + 0x0, new_pitch); // 写 pitch
        tools::mem::write<float>(angles + 0x4, new_yaw); // 写 yaw
    }

    static float local_eye_offset(std::uintptr_t local) {
        if (game::is_ducking(local)) return kEyeDuck; // 蹲下
        return kEyeStand; // 站立
    }

    static bool is_targeting_me(std::uintptr_t local, std::uintptr_t target, const game::Vec3 &local_origin, const game::Vec3 &target_eye) {
        if (!local || !target) return false;
        const float dx = local_origin.x - target_eye.x;
        const float dy = local_origin.y - target_eye.y;
        const float to_me_yaw = std::atan2(dy, dx) * 180.0f / 3.14159265f; // 目标->我 的 yaw
        const float target_yaw = tools::mem::read_raw<float>(target + schema::m_angEyeAngles + 0x4); // 目标眼睛 yaw
        float diff = to_me_yaw - target_yaw;
        while (diff > 180.0f) diff -= 360.0f; // 归一化
        while (diff < -180.0f) diff += 360.0f;
        if (std::fabs(diff) < 25.0f) return true; // 视线夹角 < 25°

        const std::uint32_t id_ent = tools::mem::read_raw<std::uint32_t>(target + schema::m_iIDEntIndex); // 准星指向的实体
        if (id_ent && id_ent != 0xFFFFFFFF) {
            const int id_index = static_cast<int>(id_ent & 0x7FFF); // 实体索引
            if (game::get_entity_from_index(id_index) == local) return true; // 正对着我
        }
        return false;
    }

    // 获取多部位中心点
    static game::Vec3 get_multi_bone_center(const game::PawnSnapshot &s) {
        if (g_target_bones == Bone_Head) return s.head;
        if (g_target_bones == Bone_Neck) return s.neck;
        if (g_target_bones == Bone_Chest) return s.chest;
        if (g_target_bones == Bone_Body) return s.body;
        if (g_target_bones == Bone_Pelvis) return s.pelvis;
        if (g_target_bones == Bone_ArmL) return s.armL;
        if (g_target_bones == Bone_ForearmL) return s.forearmL;
        if (g_target_bones == Bone_HandL) return s.handL;
        if (g_target_bones == Bone_ArmR) return s.armR;
        if (g_target_bones == Bone_ForearmR) return s.forearmR;
        if (g_target_bones == Bone_HandR) return s.handR;
        if (g_target_bones == Bone_ThighL) return s.thighL;
        if (g_target_bones == Bone_CalfL) return s.calfL;
        if (g_target_bones == Bone_FootL) return s.footL;
        if (g_target_bones == Bone_ThighR) return s.thighR;
        if (g_target_bones == Bone_CalfR) return s.calfR;
        if (g_target_bones == Bone_FootR) return s.footR;
        return s.head; // 兜底
    }

    std::uintptr_t run_once() {
        if (!g_enabled && !g_silent_enabled) {
            g_silent_has_target = false;
            return 0;
        }

        const std::uintptr_t local = game::local_player(); // 本地玩家
        if (!local) { g_silent_has_target = false; return 0; }

        const std::vector<game::PawnSnapshot> &snap = game::build_snapshot(local); // 当前帧快照
        if (snap.empty()) { g_silent_has_target = false; return 0; }

        game::Vec3 local_eye{};
        if (!game::get_head_pos(local, local_eye)) {
            const game::Vec3 local_origin = game::get_origin(local); // 本地脚底坐标
            local_eye = local_origin;
            local_eye.z += local_eye_offset(local); // 估算眼睛高度
        }
        const game::Vec3 local_origin = game::get_origin(local); // 本地脚底坐标

        const float sw = game::screen_width(); // 屏幕宽
        const float sh = game::screen_height(); // 屏幕高
        const float cx = sw * 0.5f; // 屏幕中心 x
        const float cy = sh * 0.5f; // 屏幕中心 y

        std::uintptr_t best_target = 0; // 最佳目标
        game::Vec3 best_aim{}; // 最佳瞄准点
        int best_key0 = 2; // 可见优先级
        int best_key1 = 2; // 瞄准优先级
        float best_world_dist = std::numeric_limits<float>::max(); // 世界距离

        const float circle_r = g_circle_radius; // 屏幕圆半径
        const float max_dist = g_max_world_dist; // 最大世界距离

        for (const game::PawnSnapshot &s: snap) {
            const game::Vec3 aim = get_multi_bone_center(s); // 取部位中心

            const float dxw = aim.x - local_eye.x; // x 差值
            const float dyw = aim.y - local_eye.y; // y 差值
            const float dzw = aim.z - local_eye.z; // z 差值
            const float world_dist = std::sqrt(dxw * dxw + dyw * dyw + dzw * dzw); // 世界距离

            if (world_dist > max_dist) continue; // 超距

            float sx = 0, sy = 0; // 屏幕坐标
            if (!game::world_to_screen(aim, sx, sy)) continue;
            if (sx < 0 || sx > sw || sy < 0 || sy > sh) continue; // 屏幕外

            const float dx = sx - cx; // 屏幕 x 差
            const float dy = sy - cy; // 屏幕 y 差
            const float screen_dist = std::sqrt(dx * dx + dy * dy); // 屏幕距离
            if (screen_dist > circle_r) continue; // 圆外

            const bool visible = game::is_visible_throttled(local, s.ptr); // 可见性
            if (g_only_visible && !visible) continue; // 只瞄可见

            int key0 = (g_prefer_visible && visible) ? 1 : 2; // 可见排序键
            if (!g_prefer_visible) key0 = 1;

            int key1 = (g_prefer_targeting_me && is_targeting_me(local, s.ptr, local_origin, s.head)) ? 1 : 2; // 瞄准排序键
            if (!g_prefer_targeting_me) key1 = 1;

            bool better = false; // 是否更优
            if (key0 < best_key0) better = true;
            else if (key0 == best_key0) {
                if (key1 < best_key1) better = true;
                else if (key1 == best_key1 && world_dist < best_world_dist) better = true;
            }

            if (better) {
                best_key0 = key0; best_key1 = key1; best_world_dist = world_dist;
                best_target = s.ptr; best_aim = aim;
            }
        }

        if (best_target) {
            float pitch = 0, yaw = 0; // 目标角度
            calc_angle(local_eye, best_aim, pitch, yaw);

            if (g_enabled) set_view_angles(pitch, yaw); // 暴力自瞄写视角

            if (g_silent_enabled) {
                g_silent_pitch = pitch; // 静默 pitch
                g_silent_yaw = yaw; // 静默 yaw
                g_silent_has_target = true; // 标记有目标
            }
        } else {
            g_silent_has_target = false;
        }
        return best_target;
    }
}