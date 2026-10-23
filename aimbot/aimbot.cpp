#include "aimbot.hpp"
#include "../game/entity.hpp"
#include "../tools/memory.hpp"
#include "offsets.hpp"
#include "schema.hpp"

#include <Windows.h>
#include <cmath>
#include <limits>
#include <vector>
#include <cstdio>

#include "menu.hpp"
#include "MinHook.h"

namespace aimbot {
    float g_circle_radius = 320.0f;
    float g_max_world_dist = 700.0f;

    bool g_prefer_visible = true;
    bool g_prefer_targeting_me = true;
    bool g_only_visible = true;

    bool g_enabled = false;
    bool g_silent_enabled = false;
    float g_silent_pitch = 0.0f;
    float g_silent_yaw = 0.0f;
    bool g_silent_has_target = false;

    SmoothType g_smooth_type = SmoothType::Exponential;
    float g_smooth_x = 5.0f;
    float g_smooth_y = 5.0f;

    uint32_t g_target_bones = Bone_Head;

    constexpr float kEyeStand = 64.0f;
    constexpr float kEyeDuck = 46.0f;

    /*
     * sub_180CFBD10 实际签名（IDA）:
     * __int64 __fastcall(_DWORD *a1, __int64 a2, char a3, double a4, int a5, __int64 a6)
     * a1 是输入历史条目，a2 是最终发包的 protobuf 结构。
     * 视角写入在 a2+0x18 指向的结构体的 +0x18 / +0x1C / +0x20 处，
     * 必须在原函数返回后覆盖，否则会被原函数覆盖。
     */
    using FillUserCmdFn = __int64(__fastcall*)(void *a1, void *a2, char a3,
                                               double a4, int a5, __int64 a6);
    FillUserCmdFn oFillUserCmd = nullptr;

    __int64 __fastcall hkFillUserCmd(void *a1, void *a2, char a3,
                                     double a4, int a5, __int64 a6) {
        if (!a1 || !a2) {
            return oFillUserCmd(a1, a2, a3, a4, a5, a6);
        }

        // 先让原函数完整跑完，它会初始化 a2+0x18 的指针
        __int64 result = oFillUserCmd(a1, a2, a3, a4, a5, a6);

        __try {
            // a2+0x18 存放的是真正的视角结构体指针
            uintptr_t v16 = *(uintptr_t *) ((char *) a2 + 0x18);
            if (v16) {
                // 1. 子弹追踪（静默自瞄）
                if (g_silent_enabled && g_silent_has_target) {
                    float safe_pitch = g_silent_pitch;
                    float safe_yaw = g_silent_yaw;
                    if (safe_pitch != safe_pitch) safe_pitch = 0.0f;
                    if (safe_yaw != safe_yaw) safe_yaw = 0.0f;

                    // 视角在 v16 + 0x18 / 0x1C / 0x20
                    *(float *) (v16 + 0x18) = safe_pitch; // Pitch
                    *(float *) (v16 + 0x1C) = safe_yaw; // Yaw
                    *(float *) (v16 + 0x20) = 0.0f; // Roll
                }

                // 2. RCS 指令修正 (模式 2)
                else if (menu::g_rcs_mode == 2) {
                    std::uintptr_t pawn = game::local_player();
                    if (pawn) {
                        int shots_fired = tools::mem::read_raw<int>(pawn + schema::m_iShotsFired);
                        if (shots_fired > 0) {
                            std::uintptr_t aimPunch = tools::mem::read_raw<std::uintptr_t>(
                                pawn + schema::m_pAimPunchServices);
                            if (aimPunch) {
                                float punch_x = tools::mem::read_raw<float>(aimPunch + schema::m_predictableBaseAngle);
                                float punch_y = tools::mem::read_raw<float>(
                                    aimPunch + schema::m_predictableBaseAngle + 4);
                                if (punch_x != 0.0f || punch_y != 0.0f) {
                                    *(float *) (v16 + 0x18) -= punch_x * 2.0f;
                                    *(float *) (v16 + 0x1C) -= punch_y * 2.0f;
                                }
                            }
                        }
                    }
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            // 吞掉异常，确保原函数结果顺利返回
        }

        return result;
    }

    void install_silent_hook() {
        uintptr_t base = tools::mem::client_base();
        if (!base) return;

        // dwFillUserCmd = 0xCFBD10，对应 sub_180CFBD10
        uintptr_t target = base + off::client::dwFillUserCmd;
        if (!target) return;

        MH_STATUS st = MH_CreateHook((LPVOID) target, (LPVOID) hkFillUserCmd,
                                     (LPVOID *) &oFillUserCmd);
        if (st == MH_OK) {
            MH_EnableHook((LPVOID) target);
        }
    }

    static void calc_angle(const game::Vec3 &local_eye, const game::Vec3 &target_aim,
                           float &pitch, float &yaw) {
        const float dx = target_aim.x - local_eye.x;
        const float dy = target_aim.y - local_eye.y;
        const float dz = target_aim.z - local_eye.z;
        const float hyp = std::sqrt(dx * dx + dy * dy);
        yaw = std::atan2(dy, dx) * 180.0f / 3.14159265f;
        pitch = -std::atan2(dz, hyp) * 180.0f / 3.14159265f;
    }

    static void set_view_angles(float target_pitch, float target_yaw) {
        const std::uintptr_t base = tools::mem::client_base();
        if (!base) return;
        const std::uintptr_t angles = base + off::client::dwViewAngles;

        __try {
            const float cur_pitch = tools::mem::read_raw<float>(angles + 0x0);
            const float cur_yaw = tools::mem::read_raw<float>(angles + 0x4);

            float dp = target_pitch - cur_pitch;
            float dy = target_yaw - cur_yaw;
            while (dy > 180.0f) dy -= 360.0f;
            while (dy < -180.0f) dy += 360.0f;

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

            if (std::fabs(dp) < 0.05f) new_pitch = target_pitch;
            if (std::fabs(dy) < 0.05f) new_yaw = target_yaw;

            tools::mem::write<float>(angles + 0x0, new_pitch);
            tools::mem::write<float>(angles + 0x4, new_yaw);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    static float local_eye_offset(std::uintptr_t local) {
        if (game::is_ducking(local)) return kEyeDuck;
        return kEyeStand;
    }

    static bool is_targeting_me(std::uintptr_t local, std::uintptr_t target,
                                const game::Vec3 &local_origin,
                                const game::Vec3 &target_eye) {
        if (!local || !target) return false;
        const float dx = local_origin.x - target_eye.x;
        const float dy = local_origin.y - target_eye.y;
        const float to_me_yaw = std::atan2(dy, dx) * 180.0f / 3.14159265f;
        const float target_yaw = tools::mem::read_raw<float>(
            target + schema::m_angEyeAngles + 0x4);
        float diff = to_me_yaw - target_yaw;
        while (diff > 180.0f) diff -= 360.0f;
        while (diff < -180.0f) diff += 360.0f;
        if (std::fabs(diff) < 25.0f) return true;

        const std::uint32_t id_ent = tools::mem::read_raw<std::uint32_t>(
            target + schema::m_iIDEntIndex);
        if (id_ent && id_ent != 0xFFFFFFFF) {
            const int id_index = static_cast<int>(id_ent & 0x7FFF);
            if (game::get_entity_from_index(id_index) == local) return true;
        }
        return false;
    }

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
        return s.head;
    }

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

        const std::vector<game::PawnSnapshot> &snap = game::build_snapshot(local);
        if (snap.empty()) {
            g_silent_has_target = false;
            return 0;
        }

        game::Vec3 local_eye{};
        if (!game::get_head_pos(local, local_eye)) {
            const game::Vec3 local_origin = game::get_origin(local);
            local_eye = local_origin;
            local_eye.z += local_eye_offset(local);
        }
        const game::Vec3 local_origin = game::get_origin(local);

        const float sw = game::screen_width();
        const float sh = game::screen_height();
        const float cx = sw * 0.5f;
        const float cy = sh * 0.5f;

        std::uintptr_t best_target = 0;
        game::Vec3 best_aim{};
        int best_key0 = 2;
        int best_key1 = 2;
        float best_world_dist = std::numeric_limits<float>::max();

        const float circle_r = g_circle_radius;
        const float max_dist = g_max_world_dist;

        for (const game::PawnSnapshot &s: snap) {
            __try {
                const game::Vec3 aim = get_multi_bone_center(s);

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

                // 可见性过滤
                bool visible = true;
                if (g_only_visible || g_prefer_visible) {
                    visible = game::is_visible(local, s.ptr);

                    // 如果是“只瞄可见敌人”，看不见直接跳过，绝不锁墙后面的人
                    if (g_only_visible && !visible) {
                        continue;
                    }
                }

                int key0 = (g_prefer_visible && visible) ? 1 : 2;
                if (!g_prefer_visible) key0 = 1;

                int key1 = 2;
                if (g_prefer_targeting_me) {
                    key1 = is_targeting_me(local, s.ptr, local_origin, s.head) ? 1 : 2;
                }

                bool better = false;
                if (key0 < best_key0) better = true;
                else if (key0 == best_key0) {
                    if (key1 < best_key1) better = true;
                    else if (key1 == best_key1 && world_dist < best_world_dist) better = true;
                }

                if (better) {
                    best_key0 = key0;
                    best_key1 = key1;
                    best_world_dist = world_dist;
                    best_target = s.ptr;
                    best_aim = aim;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                continue;
            }
        }

        if (best_target) {
            float pitch = 0, yaw = 0;
            calc_angle(local_eye, best_aim, pitch, yaw);

            if (pitch != pitch) pitch = 0.0f;
            if (yaw != yaw) yaw = 0.0f;

            if (g_enabled) set_view_angles(pitch, yaw);

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