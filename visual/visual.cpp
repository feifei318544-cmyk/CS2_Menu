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

    bool g_esp_enabled = false; // 透视总开关
    bool g_esp_show_health = false; // 显示血量
    bool g_esp_show_ray = true; // 显示射线
    bool g_esp_show_name = false; // 显示名称
    bool g_esp_show_skeleton = false; // 显示骨骼
    bool g_esp_show_weapon   = false; // 显示武器
    bool g_esp_show_distance = false; // 显示距离
    bool g_esp_show_armor    = false; // 显示护甲
    bool g_esp_show_helmet   = false; // 显示头盔

    void draw_aim_circle(float radius) {
        ImDrawList *dl = ImGui::GetBackgroundDrawList(); // 背景绘制列表
        if (!dl) return;

        const ImVec2 center{
            game::screen_width() * 0.5f,
            game::screen_height() * 0.5f
        }; // 屏幕中心

        dl->AddCircle(center, radius, IM_COL32(255, 0, 0, 255), 64, 2.0f); // 自瞄范围圆

        dl->AddLine({center.x - 5, center.y},
                    {center.x + 5, center.y},
                    IM_COL32(255, 0, 0, 255), 1.0f); // 中心水平线
        dl->AddLine({center.x, center.y - 5},
                    {center.x, center.y + 5},
                    IM_COL32(255, 0, 0, 255), 1.0f); // 中心垂直线
    }

    void draw_target_marker(std::uintptr_t target) {
        if (!target) return;

        ImDrawList *dl = ImGui::GetBackgroundDrawList(); // 背景绘制列表
        if (!dl) return;

        const ImVec2 center{
            game::screen_width() * 0.5f,
            game::screen_height() * 0.5f
        }; // 屏幕中心

        dl->AddText({center.x - 30, center.y + 120},
                    IM_COL32(0, 255, 0, 255),
                    "已锁定"); // 锁定标记
    }

    static void draw_enemy_esp(std::uintptr_t enemy) {
        game::Vec3 origin = game::get_origin(enemy); // 敌人世界坐标
        if (origin.x == 0.0f && origin.y == 0.0f && origin.z == 0.0f) return;

        game::Vec3 head = origin; // 头部世界坐标
        head.z += 72.0f;

        float head_sx = 0, head_sy = 0; // 头部屏幕坐标
        float foot_sx = 0, foot_sy = 0; // 脚部屏幕坐标
        if (!game::world_to_screen(head, head_sx, head_sy)) return;
        if (!game::world_to_screen(origin, foot_sx, foot_sy)) return;

        const float height = foot_sy - head_sy; // 方框高度
        if (height <= 1.0f) return;

        const float width = height * 0.45f; // 方框宽度

        const float box_left = head_sx - width * 0.5f; // 方框左边界
        const float box_right = head_sx + width * 0.5f; // 方框右边界
        const float box_top = head_sy; // 方框上边界
        const float box_bottom = foot_sy; // 方框下边界

        ImDrawList *dl = ImGui::GetBackgroundDrawList(); // 背景绘制列表
        if (!dl) return;

        // 1. 顶部射线
        if (g_esp_show_ray) {
            const ImVec2 top_center(game::screen_width() * 0.5f, 0.0f); // 屏幕顶部中心
            const ImVec2 box_top_center(head_sx, box_top); // 方框顶部中心
            dl->AddLine(top_center, box_top_center, IM_COL32(0, 255, 0, 200), 1.5f); // 射线
        }

        // 2. 方框
        dl->AddRect(
            ImVec2(box_left, box_top),
            ImVec2(box_right, box_bottom),
            IM_COL32(0, 255, 0, 255),
            0.0f, 0, 1.5f); // 敌人方框

        // 3. 血量
        if (g_esp_show_health) {
            const int hp = game::get_health(enemy); // 敌人血量
            char buf[16]; // 血量文本
            std::snprintf(buf, sizeof(buf), "%d HP", hp);
            dl->AddText(ImVec2(box_left, box_bottom + 2.0f),
                        IM_COL32(255, 255, 0, 255), buf);
        }

        // 4. 显示名称
        if (g_esp_show_name) {
            const std::uintptr_t controller = game::get_controller_from_pawn(enemy); // 敌人 Controller
            const char *name = game::get_player_name(controller); // 敌人名字
            if (name && name[0]) {
                const float tw = ImGui::CalcTextSize(name).x; // 名字宽度
                dl->AddText(ImVec2(head_sx - tw * 0.5f, box_top - 16.0f),
                            IM_COL32(0, 255, 255, 255), name);
            }
        }

        // 武器名称（方框下方）
        if (g_esp_show_weapon) {
            const char* wname = game::get_weapon_name(enemy); // 敌人武器名
            if (wname && wname[0]) {
                dl->AddText(ImVec2(box_left, box_bottom + 18.0f),
                            IM_COL32(255, 200, 100, 255), wname);
            }
        }

        // 距离（方框右上角）
        if (g_esp_show_distance) {
            const std::uintptr_t local = game::local_player(); // 本地玩家
            if (local) {
                const game::Vec3 local_pos = game::get_origin(local); // 本地世界坐标
                const float dist = game::get_distance(origin, local_pos); // 距离

                char buf[32]; // 距离文本
                std::snprintf(buf, sizeof(buf), "%.0fm", dist);

                dl->AddText(ImVec2(box_right + 4.0f, box_top),
                            IM_COL32(255, 255, 255, 255), buf);
            }
        }

        // 护甲（方框左下角显示数字）
        if (g_esp_show_armor) {
            const int armor = game::get_armor(enemy); // 敌人护甲
            if (armor > 0) {
                char buf[16]; // 护甲文本
                std::snprintf(buf, sizeof(buf), "%d", armor);
                dl->AddText(ImVec2(box_left - 20.0f, box_bottom - 14.0f),
                            IM_COL32(150, 150, 255, 255), buf);
            }
        }

        // 头盔（方框左上角显示 H）
        if (g_esp_show_helmet) {
            if (game::has_helmet(enemy)) {
                dl->AddText(ImVec2(box_left - 20.0f, box_top),
                            IM_COL32(150, 200, 255, 255), "H");
            }
        }
    }

    static const int kBonePairs[][2] = {
        // 中轴
        {1, 3}, // pelvis → spine_1
        {3, 4}, // spine_1 → spine_2
        {4, 23}, // spine_2 → chest
        {23, 6}, // chest → neck
        {6, 7}, // neck → head

        // 左臂
        {6, 9}, // neck → shoulder_L
        {9, 10}, // shoulder_L → elbow_L
        {10, 11}, // elbow_L → hand_L

        // 右臂
        {6, 13}, // neck → shoulder_R
        {13, 14}, // shoulder_R → elbow_R
        {14, 15}, // elbow_R → hand_R

        // 左腿
        {1, 17}, // pelvis → hip_L
        {17, 18}, // hip_L → knee_L
        {18, 19}, // knee_L → foot_heel_L

        // 右腿
        {1, 20}, // pelvis → hip_R
        {20, 21}, // hip_R → knee_R
        {21, 22}, // knee_R → foot_heel_R
    };

    static void draw_skeleton(std::uintptr_t pawn) {
        ImDrawList *dl = ImGui::GetBackgroundDrawList(); // 背景绘制列表
        const ImU32 color = IM_COL32(255, 0, 0, 255); // 骨骼颜色

        // 1. 头画圆（索引 7）
        game::Vec3 head; // 头部世界坐标
        if (game::get_bone_pos(pawn, 7, head)) {
            float hx, hy; // 头部屏幕坐标
            if (game::world_to_screen(head, hx, hy)) {
                dl->AddCircle(ImVec2(hx, hy), 6.0f, color, 16, 1.5f); // 头部圆
            }
        }

        // 2. 骨骼连线
        for (auto &pair: kBonePairs) {
            game::Vec3 p1, p2; // 两点世界坐标
            if (!game::get_bone_pos(pawn, pair[0], p1)) continue;
            if (!game::get_bone_pos(pawn, pair[1], p2)) continue;

            float x1, y1, x2, y2; // 两点屏幕坐标
            if (!game::world_to_screen(p1, x1, y1)) continue;
            if (!game::world_to_screen(p2, x2, y2)) continue;

            dl->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color, 1.5f); // 骨骼线
        }
    }

    void draw_esp() {
        if (!g_esp_enabled) return;

        const std::uintptr_t local = game::local_player(); // 本地玩家
        if (!local) return;

        const int local_team = game::get_team(local); // 本地阵营

        const std::vector<std::uintptr_t> &pawns = game::get_pawns_cached(); // 敌人列表缓存
        for (std::uintptr_t e: pawns) {
            if (e == local) continue;

            const int team = game::get_team(e); // 实体阵营
            if (team != 2 && team != 3) continue;
            if (team == local_team) continue;

            draw_enemy_esp(e);

            if (g_esp_show_skeleton) {
                draw_skeleton(e);
            }
        }
    }
}