#include "aimbot.hpp"
#include "../game/entity.hpp"
#include "../tools/memory.hpp"
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
    float g_max_world_dist = 700.0f; // 最大世界距离（单位）

    // 暴力瞄准
    bool g_enabled = false; // 暴力自瞄开关
    float g_smooth = 1.0f; // 平滑

    // 子弹追踪（静默自瞄）
    bool g_silent_enabled = false; // 子弹追踪开关

    float g_silent_pitch = 0.0f; // 静默瞄准 pitch
    float g_silent_yaw = 0.0f; // 静默瞄准 yaw
    bool g_silent_has_target = false; // 本帧是否有静默目标

    // RCS 压枪
    bool g_rcs_enabled = false; // RCS 压枪开关
    float g_rcs_strength = 0.5f; // RCS 压枪强度（0~1）

    // 自动开火
    bool g_auto_fire_enabled = false; // 自动开火开关

    // 眼睛高度常量
    constexpr float kEyeStand = 64.0f; // 站立时眼睛高度
    constexpr float kEyeDuck = 46.0f; // 蹲下时眼睛高度

    // 子弹追踪 hook 目标 RVA
    constexpr uintptr_t kFillUserCmdRva = 0xCFA6B0; // hook 目标 RVA

    using FillUserCmdFn = __int64(*)(int *a1, __int64 a2, char a3, double a4, int a5, __int64 a6); // 原函数类型
    FillUserCmdFn oFillUserCmd = nullptr; // 原函数指针

    // hook 函数，只做子弹追踪
    __int64 __fastcall hkFillUserCmd(int *a1, __int64 a2, char a3, double a4, int a5, __int64 a6) {
        if (g_silent_enabled && g_silent_has_target) {
            *(float *) ((char *) a1 + 0x10) = g_silent_pitch;
            *(float *) ((char *) a1 + 0x14) = g_silent_yaw;
            *(float *) ((char *) a1 + 0x18) = 0.0f;
        }

        return oFillUserCmd(a1, a2, a3, a4, a5, a6);
    }

    // 安装子弹追踪 hook
    void install_silent_hook() {
        uintptr_t base = tools::mem::client_base();
        if (!base) return;

        uintptr_t target = base + kFillUserCmdRva;

        MH_STATUS st = MH_CreateHook((LPVOID) target, (LPVOID) hkFillUserCmd, (LPVOID *) &oFillUserCmd);
        if (!(st != MH_OK)) {
            MH_EnableHook((LPVOID) target);
        }
    }

    // 计算目标角度
    static void calc_angle(const game::Vec3 &local_eye, const game::Vec3 &target_aim,
                           float &pitch, float &yaw) {
        const float dx = target_aim.x - local_eye.x;
        const float dy = target_aim.y - local_eye.y;
        const float dz = target_aim.z - local_eye.z;

        const float hyp = std::sqrt(dx * dx + dy * dy);

        yaw = std::atan2(dy, dx) * 180.0f / 3.14159265f; // 偏航角
        pitch = -std::atan2(dz, hyp) * 180.0f / 3.14159265f; // 俯仰角
    }

    // 写视角，暴力自瞄用
    static void set_view_angles(float pitch, float yaw) {
        const std::uintptr_t base = tools::mem::client_base();
        if (!base) return;

        const std::uintptr_t angles = base + off::client::dwViewAngles;

        tools::mem::write<float>(angles + 0x0, pitch); // 写 pitch
        tools::mem::write<float>(angles + 0x4, yaw); // 写 yaw
    }

    // 取本地眼睛高度
    static float local_eye_offset(std::uintptr_t local) {
        if (game::is_ducking(local)) return kEyeDuck;
        return kEyeStand;
    }

    // 取敌人瞄准点高度
    static float target_aim_offset(std::uintptr_t e) {
        if (game::is_ducking(e)) return kEyeDuck;
        return kEyeStand;
    }

    // 主循环
    std::uintptr_t run_once() {
        if (!g_enabled && !g_silent_enabled) {
            g_silent_has_target = false;
            return 0;
        }

        const std::uintptr_t local = game::local_player();
        if (!local) {
            g_silent_has_target = false;
            return 0;
        }

        const game::Vec3 local_origin = game::get_origin(local);
        const int local_team = game::get_team(local);

        game::Vec3 local_eye{};
        if (!game::get_head_pos(local, local_eye)) {
            local_eye = local_origin;
            local_eye.z += local_eye_offset(local);
        }

        const float sw = game::screen_width();
        const float sh = game::screen_height();
        const float cx = sw * 0.5f;
        const float cy = sh * 0.5f;

        std::uintptr_t best_target = 0;
        float best_world_dist = std::numeric_limits<float>::max();
        game::Vec3 best_aim{};

        const std::vector<std::uintptr_t> &pawns = game::get_pawns_cached();

        const float circle_r = g_circle_radius; // 屏幕圆半径
        const float max_dist = g_max_world_dist; // 最大世界距离

        for (std::uintptr_t e: pawns) {
            if (!(e != local)) continue; // 跳过本地玩家
            if (!e) continue; // 空指针保护

            const int team = game::get_team(e);
            if (!(team != local_team)) continue; // 跳过队友

            game::Vec3 aim{};
            if (!game::get_head_pos(e, aim)) {
                // 骨骼读不到就退到原点加估算高度
                const game::Vec3 origin = game::get_origin(e);
                aim = origin;
                aim.z += target_aim_offset(e);
            }

            const float dxw = aim.x - local_eye.x;
            const float dyw = aim.y - local_eye.y;
            const float dzw = aim.z - local_eye.z;
            const float world_dist = std::sqrt(dxw * dxw + dyw * dyw + dzw * dzw);

            if (world_dist > max_dist) continue;

            float sx = 0, sy = 0;
            if (!game::world_to_screen(aim, sx, sy)) continue;

            if (sx < 0 || sx > sw || sy < 0 || sy > sh) continue;

            const float dx = sx - cx;
            const float dy = sy - cy;
            const float screen_dist = std::sqrt(dx * dx + dy * dy);

            if (screen_dist > circle_r) continue;

            if (world_dist < best_world_dist) {
                best_world_dist = world_dist;
                best_target = e;
                best_aim = aim;
            }
        }

        if (best_target) {
            float pitch = 0, yaw = 0;
            calc_angle(local_eye, best_aim, pitch, yaw);

            // 暴力自瞄，写本地视角
            if (g_enabled) {
                set_view_angles(pitch, yaw);
            }

            // 子弹追踪，写静默角度
            if (g_silent_enabled) {
                g_silent_pitch = pitch;
                g_silent_yaw = yaw;
                g_silent_has_target = true;
            }
        } else {
            g_silent_has_target = false;
        }

        return best_target;
    }
}