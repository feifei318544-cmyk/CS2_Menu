#include "visual.hpp"
#include "imgui.h"

#include <Windows.h>

#include "../game/entity.hpp"
#include "../tools/memory.hpp"
#include "offsets.hpp"
#include "../tools/schema.hpp"

#include <cmath>
#include <vector>
#include <cstdio>

namespace visual {

    bool g_esp_enabled     = false;
    bool g_esp_show_health = false;
    bool g_esp_show_ray    = true;
    bool g_esp_show_name   = false;

    void draw_aim_circle(float radius) {
        ImDrawList *dl = ImGui::GetBackgroundDrawList();
        if (!dl) return;

        const ImVec2 center{
            game::screen_width()  * 0.5f,
            game::screen_height() * 0.5f
        };

        dl->AddCircle(center, radius, IM_COL32(255, 0, 0, 255), 64, 2.0f);

        dl->AddLine({center.x - 5, center.y},
                    {center.x + 5, center.y},
                    IM_COL32(255, 0, 0, 255), 1.0f);
        dl->AddLine({center.x, center.y - 5},
                    {center.x, center.y + 5},
                    IM_COL32(255, 0, 0, 255), 1.0f);
    }

    void draw_target_marker(std::uintptr_t target) {
        if (!target) return;

        ImDrawList *dl = ImGui::GetBackgroundDrawList();
        if (!dl) return;

        const ImVec2 center{
            game::screen_width()  * 0.5f,
            game::screen_height() * 0.5f
        };

        dl->AddText({center.x - 30, center.y + 120},
                    IM_COL32(0, 255, 0, 255),
                    "已锁定");
    }

    static void draw_enemy_esp(std::uintptr_t enemy) {
        game::Vec3 origin = game::get_origin(enemy);
        if (origin.x == 0.0f && origin.y == 0.0f && origin.z == 0.0f) return;

        game::Vec3 head = origin;
        head.z += 72.0f;

        float head_sx = 0, head_sy = 0;
        float foot_sx = 0, foot_sy = 0;
        if (!game::world_to_screen(head, head_sx, head_sy)) return;
        if (!game::world_to_screen(origin, foot_sx, foot_sy)) return;

        const float height = foot_sy - head_sy;
        if (height <= 1.0f) return;

        const float width = height * 0.45f;

        const float box_left   = head_sx - width * 0.5f;
        const float box_right  = head_sx + width * 0.5f;
        const float box_top    = head_sy;
        const float box_bottom = foot_sy;

        ImDrawList *dl = ImGui::GetBackgroundDrawList();
        if (!dl) return;

        // 1. 顶部射线
        if (g_esp_show_ray) {
            const ImVec2 top_center(game::screen_width() * 0.5f, 0.0f);
            const ImVec2 box_top_center(head_sx, box_top);
            dl->AddLine(top_center, box_top_center, IM_COL32(0, 255, 0, 200), 1.5f);
        }

        // 2. 方框
        dl->AddRect(
            ImVec2(box_left, box_top),
            ImVec2(box_right, box_bottom),
            IM_COL32(0, 255, 0, 255),
            0.0f, 0, 1.5f);

        // 3. 血量
        if (g_esp_show_health) {
            const int hp = game::get_health(enemy);
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%d HP", hp);
            dl->AddText(ImVec2(box_left, box_bottom + 2.0f),
                        IM_COL32(255, 255, 0, 255), buf);
        }

        // 4. 显示名称
        if (g_esp_show_name) {
            const std::uintptr_t controller = game::get_controller_from_pawn(enemy);
            const char* name = game::get_player_name(controller);
            if (name && name[0]) {
                const float tw = ImGui::CalcTextSize(name).x;
                dl->AddText(ImVec2(head_sx - tw * 0.5f, box_top - 16.0f),
                            IM_COL32(0, 255, 255, 255), name);
            }
        }
    }

    void draw_esp() {
        if (!g_esp_enabled) return;

        const std::uintptr_t local = game::local_player();
        if (!local) return;

        const int local_team = game::get_team(local);

        const std::vector<std::uintptr_t>& pawns = game::get_pawns_cached();
        for (std::uintptr_t e : pawns) {
            if (e == local) continue;

            const int team = game::get_team(e);
            if (team != 2 && team != 3) continue;
            if (team == local_team) continue;

            draw_enemy_esp(e);
        }
    }

}