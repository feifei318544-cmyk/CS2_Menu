#include "aimbot.hpp"
#include "../game/entity.hpp"
#include "../tools/memory.hpp"
#include "offsets.hpp"

#include <Windows.h>
#include <cmath>
#include <limits>
#include <vector>

namespace aimbot {
    float g_circle_radius = 320.0f;
    bool g_enabled = false;
    float g_smooth = 1.0f;

    // 最大世界距离，单位游戏单位，1 米 ≈ 52 单位
    float g_max_world_dist = 700.0f;

    // 计算目标角度
    static void calc_angle(const game::Vec3 &local, const game::Vec3 &target,
                           float &pitch, float &yaw) {
        const float dx = target.x - local.x;
        const float dy = target.y - local.y;
        const float dz = target.z - local.z;

        const float hyp = std::sqrt(dx * dx + dy * dy);

        yaw   = std::atan2(dy, dx) * 180.0f / 3.14159265f;
        pitch = -std::atan2(dz, hyp) * 180.0f / 3.14159265f;
    }

    // 写视角，静态自瞄，准星可见转动
    static void set_view_angles(float pitch, float yaw) {
        const std::uintptr_t base = tools::mem::client_base();
        if (!base) return;

        const std::uintptr_t angles = base + off::client::dwViewAngles;

        tools::mem::write<float>(angles + 0x0, pitch);
        tools::mem::write<float>(angles + 0x4, yaw);
    }

    // 主循环
    std::uintptr_t run_once() {
        if (!g_enabled) return 0;

        const std::uintptr_t local = game::local_player();
        if (!local) return 0;

        const game::Vec3 local_pos = game::get_origin(local);
        const int local_team = game::get_team(local);

        const float sw = game::screen_width();
        const float sh = game::screen_height();
        const float cx = sw * 0.5f;
        const float cy = sh * 0.5f;

        std::uintptr_t best_target = 0;
        float best_world_dist = std::numeric_limits<float>::max();
        game::Vec3 best_pos{};

        const std::vector<std::uintptr_t>& pawns = game::get_pawns_cached();

        for (std::uintptr_t e : pawns) {
            if (e == local) continue;

            const int team = game::get_team(e);
            if (team == local_team) continue;

            const game::Vec3 pos = game::get_origin(e);

            const float dxw = pos.x - local_pos.x;
            const float dyw = pos.y - local_pos.y;
            const float dzw = pos.z - local_pos.z;
            const float world_dist = std::sqrt(dxw*dxw + dyw*dyw + dzw*dzw);

            if (world_dist > g_max_world_dist) continue;

            float sx = 0, sy = 0;
            if (!game::world_to_screen(pos, sx, sy)) continue;

            // 屏幕外跳过
            if (sx < 0 || sx > sw || sy < 0 || sy > sh) continue;

            const float dx = sx - cx;
            const float dy = sy - cy;
            const float screen_dist = std::sqrt(dx*dx + dy*dy);

            // 圈外跳过
            if (screen_dist > g_circle_radius) continue;

            // 挑世界距离最近的
            if (world_dist < best_world_dist) {
                best_world_dist = world_dist;
                best_target = e;
                best_pos = pos;
            }
        }

        if (best_target) {
            float pitch = 0, yaw = 0;
            calc_angle(local_pos, best_pos, pitch, yaw);
            set_view_angles(pitch, yaw);
        }

        return best_target;
    }
}