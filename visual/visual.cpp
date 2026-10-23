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
#include <unordered_map>
#include <string>

namespace visual {
    constexpr float kHeadHeight = 72.0f; // 脚底到头顶的高度

    // ESP 开关
    bool g_esp_enabled = false; // 透视总开关
    bool g_esp_show_health = false; // 显示血量
    bool g_esp_show_ray = true; // 显示射线
    bool g_esp_show_name = false; // 显示名称
    bool g_esp_show_skeleton = false; // 显示骨骼
    bool g_esp_show_weapon = false; // 显示武器
    bool g_esp_show_distance = false; // 显示距离
    bool g_esp_show_armor = false; // 显示护甲
    bool g_esp_show_helmet = false; // 显示头盔
    bool g_esp_show_c4 = false; // 显示 C4
    bool g_radar_enabled = false; // 雷达总开关

    // 名字 / 武器缓存
    static std::unordered_map<std::uintptr_t, std::string> g_weapon_cache; // 武器名缓存
    static std::unordered_map<std::uintptr_t, std::string> g_name_cache; // 玩家名缓存
    static size_t g_refresh_idx = 0; // 分帧刷新游标
    static constexpr size_t kRefreshPerFrame = 3; // 每帧刷新 3 个

    void draw_aim_circle(float radius) {
        ImDrawList *dl = ImGui::GetBackgroundDrawList(); // 背景绘制列表
        if (!dl) return;

        const float sw = game::screen_width();
        const float sh = game::screen_height();
        const ImVec2 center(sw * 0.5f, sh * 0.5f); // 屏幕正中心

        dl->AddCircle(center, radius, IM_COL32(255, 0, 0, 255), 64, 2.0f); // 自瞄范围圆

        // 画一个小准星，用来自检圆圈中心到底偏没偏
        dl->AddLine(ImVec2(center.x - 5, center.y), ImVec2(center.x + 5, center.y), IM_COL32(255, 0, 0, 255), 1.0f);
        dl->AddLine(ImVec2(center.x, center.y - 5), ImVec2(center.x, center.y + 5), IM_COL32(255, 0, 0, 255), 1.0f);
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

    void draw_c4_indicator() {
        if (!g_esp_enabled || !g_esp_show_c4) return; // 加上总开关

        const std::vector<std::uintptr_t> &c4_list = game::get_c4_cached();
        if (c4_list.empty()) return;

        const float sw = game::screen_width();
        const float sh = game::screen_height();
        ImDrawList *dl = ImGui::GetBackgroundDrawList();
        if (!dl) return;


        // 顶部提示条（只画一条）
        for (std::uintptr_t c4: c4_list) {
            const game::Vec3 c4_pos = game::get_c4_origin(c4);
            const bool has_pos = !(c4_pos.x == 0.0f && c4_pos.y == 0.0f && c4_pos.z == 0.0f);

            const int bomb_site = tools::mem::read_raw<int>(c4 + schema::m_nBombSite);
            const bool being_defused = tools::mem::read_raw<bool>(c4 + schema::m_bBeingDefused);
            const bool bomb_defused = tools::mem::read_raw<bool>(c4 + schema::m_bBombDefused);

            char name[64] = "C4";

            if (!has_pos) std::snprintf(name, sizeof(name), "C4 在玩家身上/未安放");
            else if (bomb_defused) std::snprintf(name, sizeof(name), "C4 已拆解");
            else if (being_defused) std::snprintf(name, sizeof(name), "C4 正在拆弹");
            else if (bomb_site == 0) std::snprintf(name, sizeof(name), "C4 - A 点");
            else if (bomb_site == 1) std::snprintf(name, sizeof(name), "C4 - B 点");

            const float tw = ImGui::CalcTextSize(name).x;
            const float bar_x = (sw - tw) * 0.5f;
            const float bar_y = 60.0f;

            dl->AddRectFilled(
                ImVec2(bar_x - 10.0f, bar_y - 5.0f),
                ImVec2(bar_x + tw + 10.0f, bar_y + 20.0f),
                IM_COL32(255, 0, 0, 180), 4.0f);

            dl->AddText(ImVec2(bar_x, bar_y), IM_COL32(255, 255, 255, 255), name);
            break; // 只画一条
        }

        // 视野内画圈
        for (std::uintptr_t c4: c4_list) {
            const game::Vec3 c4_pos = game::get_c4_origin(c4);
            if (c4_pos.x == 0.0f && c4_pos.y == 0.0f && c4_pos.z == 0.0f) continue; // 身上没坐标跳过

            const int bomb_site = tools::mem::read_raw<int>(c4 + schema::m_nBombSite); // 炸弹点
            const bool being_defused = tools::mem::read_raw<bool>(c4 + schema::m_bBeingDefused); // 是否在拆弹
            const bool bomb_defused = tools::mem::read_raw<bool>(c4 + schema::m_bBombDefused); // 是否已拆解

            char name[64] = "C4"; // 默认名
            if (bomb_defused) std::snprintf(name, sizeof(name), "C4 已拆解");
            else if (being_defused) std::snprintf(name, sizeof(name), "C4 正在拆弹");
            else if (bomb_site == 0) std::snprintf(name, sizeof(name), "C4 - A 点");
            else if (bomb_site == 1) std::snprintf(name, sizeof(name), "C4 - B 点");

            float sx = 0, sy = 0; // 屏幕坐标
            if (!game::world_to_screen(c4_pos, sx, sy)) continue;
            if (sx < 0 || sx > sw || sy < 0 || sy > sh) continue; // 屏幕外不画

            dl->AddCircle(ImVec2(sx, sy), 20.0f, IM_COL32(255, 0, 0, 255), 32, 2.0f); // C4 圆圈

            const float name_w = ImGui::CalcTextSize(name).x; // 名字宽度
            dl->AddText(ImVec2(sx - name_w * 0.5f, sy + 24.0f),
                        IM_COL32(255, 0, 0, 255), name); // C4 名字
        }
    }

    static void draw_enemy_esp(const game::PawnSnapshot &s) {
        const game::Vec3 &origin = s.origin; // 脚底世界坐标

        game::Vec3 head = origin; // 头部世界坐标
        head.z += kHeadHeight;

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

        const ImU32 box_color = IM_COL32(0, 255, 0, 255); // 固定绿色

        if (g_esp_show_ray) {
            const ImVec2 top_center(game::screen_width() * 0.5f, 0.0f); // 屏幕顶部中心
            const ImVec2 box_top_center(head_sx, box_top); // 方框顶部中心
            dl->AddLine(top_center, box_top_center, box_color, 1.5f); // 射线
        }

        dl->AddRect(
            ImVec2(box_left, box_top),
            ImVec2(box_right, box_bottom),
            box_color,
            0.0f, 0, 1.5f); // 敌人方框

        if (g_esp_show_health) {
            char buf[16]; // 血量文本
            std::snprintf(buf, sizeof(buf), "%d HP", s.health);
            dl->AddText(ImVec2(box_left, box_bottom + 2.0f),
                        IM_COL32(255, 255, 0, 255), buf); // 血量
        }

        if (g_esp_show_name) {
            auto it = g_name_cache.find(s.ptr); // 查名字缓存
            if (it != g_name_cache.end() && !it->second.empty()) {
                const char *name = it->second.c_str(); // 名字
                const float tw = ImGui::CalcTextSize(name).x; // 名字宽度
                dl->AddText(ImVec2(head_sx - tw * 0.5f, box_top - 16.0f),
                            IM_COL32(0, 255, 255, 255), name); // 名字
            }
        }

        if (g_esp_show_weapon) {
            auto it = g_weapon_cache.find(s.ptr); // 查武器缓存
            if (it != g_weapon_cache.end() && !it->second.empty()) {
                dl->AddText(ImVec2(box_left, box_bottom + 18.0f),
                            IM_COL32(255, 200, 100, 255), it->second.c_str()); // 武器名
            }
        }

        if (g_esp_show_distance) {
            const std::uintptr_t local2 = game::local_player(); // 本地玩家
            if (local2) {
                const game::Vec3 local_pos = game::get_origin(local2); // 本地世界坐标
                const float dist = game::get_distance(origin, local_pos); // 距离

                char buf[32]; // 距离文本
                std::snprintf(buf, sizeof(buf), "%.0fm", dist);

                dl->AddText(ImVec2(box_right + 4.0f, box_top),
                            IM_COL32(255, 255, 255, 255), buf); // 距离
            }
        }

        if (g_esp_show_armor) {
            if (s.armor > 0) {
                char buf[16]; // 护甲文本
                std::snprintf(buf, sizeof(buf), "%d", s.armor);
                dl->AddText(ImVec2(box_left - 20.0f, box_bottom - 14.0f),
                            IM_COL32(150, 150, 255, 255), buf); // 护甲
            }
        }

        if (g_esp_show_helmet) {
            if (s.helmet) {
                dl->AddText(ImVec2(box_left - 20.0f, box_top),
                            IM_COL32(150, 200, 255, 255), "H"); // 头盔
            }
        }
    }

    static const int kBonePairs[][2] = {
        {1, 3}, // pelvis → spine_1
        {3, 4}, // spine_1 → spine_2
        {4, 23}, // spine_2 → chest
        {23, 6}, // chest → neck
        {6, 7}, // neck → head
        {6, 9}, // neck → shoulder_L
        {9, 10}, // shoulder_L → elbow_L
        {10, 11}, // elbow_L → hand_L
        {6, 13}, // neck → shoulder_R
        {13, 14}, // shoulder_R → elbow_R
        {14, 15}, // elbow_R → hand_R
        {1, 17}, // pelvis → hip_L
        {17, 18}, // hip_L → knee_L
        {18, 19}, // knee_L → foot_heel_L
        {1, 20}, // pelvis → hip_R
        {20, 21}, // hip_R → knee_R
        {21, 22}, // knee_R → foot_heel_R
    };

    static void draw_skeleton(std::uintptr_t pawn) {
        ImDrawList *dl = ImGui::GetBackgroundDrawList(); // 背景绘制列表
        const ImU32 color = IM_COL32(255, 0, 0, 255); // 骨骼颜色

        game::Vec3 head; // 头部世界坐标
        if (game::get_bone_pos(pawn, 7, head)) {
            float hx, hy; // 头部屏幕坐标
            if (game::world_to_screen(head, hx, hy)) {
                dl->AddCircle(ImVec2(hx, hy), 6.0f, color, 16, 1.5f); // 头部圆
            }
        }

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

        const std::vector<game::PawnSnapshot> &snap = game::build_snapshot(local); // 当前帧快照

        // 分帧刷新名字 / 武器缓存
        const size_t total = snap.size();
        for (size_t i = 0; i < kRefreshPerFrame && total > 0; ++i) {
            std::uintptr_t e = snap[g_refresh_idx % total].ptr; // 当前刷新的实体
            g_weapon_cache[e] = game::get_weapon_name(e); // 刷新武器名
            g_name_cache[e] = game::get_player_name(game::get_controller_from_pawn(e)); // 刷新玩家名
            ++g_refresh_idx;
        }

        for (const game::PawnSnapshot &s: snap) {
            draw_enemy_esp(s);

            if (g_esp_show_skeleton) {
                draw_skeleton(s.ptr);
            }
        }

        // C4 绘制放在 ESP 末尾，跟随透视总开关
        draw_c4_indicator();
    }

    void draw_radar() {
        if (!g_radar_enabled) return;

        const std::uintptr_t local = game::local_player();
        if (!local) return;

        const game::Vec3 local_pos = game::get_origin(local);
        const int local_team = game::get_team(local);

        // 雷达参数
        const float radar_radius = 120.0f; // 雷达半径（圆形的）
        const float radar_range = 3000.0f; // 雷达能显示的物理范围（单位）
        const float radar_scale = radar_radius / radar_range; // 缩放比例

        // 雷达放在屏幕左上角，给左上角留出 40 像素边距
        const float sw = game::screen_width();
        const float sh = game::screen_height();
        const ImVec2 radar_center(40.0f + radar_radius, 400.0f + radar_radius);

        ImDrawList *dl = ImGui::GetBackgroundDrawList();
        if (!dl) return;

        // 1. 画雷达背景（暗色半透明圆）
        dl->AddCircleFilled(radar_center, radar_radius, IM_COL32(20, 20, 20, 160), 64);

        // 2. 画雷达外圈（紫色发光边框）
        dl->AddCircle(radar_center, radar_radius, IM_COL32(150, 50, 255, 200), 64, 2.0f);

        // 3. 画雷达内圈刻度（装饰）
        dl->AddCircle(radar_center, radar_radius * 0.66f, IM_COL32(150, 50, 255, 80), 64, 1.0f);
        dl->AddCircle(radar_center, radar_radius * 0.33f, IM_COL32(150, 50, 255, 80), 64, 1.0f);

        // 4. 画十字线
        dl->AddLine(ImVec2(radar_center.x - radar_radius, radar_center.y),
                    ImVec2(radar_center.x + radar_radius, radar_center.y), IM_COL32(150, 50, 255, 80), 1.0f);
        dl->AddLine(ImVec2(radar_center.x, radar_center.y - radar_radius),
                    ImVec2(radar_center.x, radar_center.y + radar_radius), IM_COL32(150, 50, 255, 80), 1.0f);

        // 5. 画中心点（自己）
        dl->AddCircleFilled(radar_center, 4.0f, IM_COL32(255, 255, 255, 255));

        // 获取所有玩家的快照
        const std::vector<game::PawnSnapshot> &snap = game::build_snapshot(local);

        for (const auto &s: snap) {
            // 计算相对坐标 (物理单位)
            float dx = s.origin.x - local_pos.x;
            float dy = s.origin.y - local_pos.y;

            // 如果敌人超出雷达范围，不画
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radar_range) continue;

            // 转换到雷达屏幕坐标
            // 注意：CS2 的 Y 轴和屏幕 Y 轴是反的，如果发现上下反了，把 dy 前面加个负号
            float radar_x = radar_center.x + (dx * radar_scale);
            float radar_y = radar_center.y + (dy * radar_scale);

            const ImVec2 enemy_pos(radar_x, radar_y);

            // 判断颜色：队友绿色，敌人红色
            ImU32 color = (s.team == local_team) ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255);

            // 画点和文字
            dl->AddCircleFilled(enemy_pos, 3.5f, color);

            // 如果敌人死了，画个叉
            if (s.health <= 0) {
                dl->AddLine(ImVec2(enemy_pos.x - 3, enemy_pos.y - 3), ImVec2(enemy_pos.x + 3, enemy_pos.y + 3),
                            IM_COL32(128, 128, 128, 255), 1.0f);
                dl->AddLine(ImVec2(enemy_pos.x - 3, enemy_pos.y + 3), ImVec2(enemy_pos.x + 3, enemy_pos.y - 3),
                            IM_COL32(128, 128, 128, 255), 1.0f);
            }
        }
    }

    void draw_welcome_text() {
        ImDrawList *dl = ImGui::GetBackgroundDrawList(); // 背景绘制列表
        if (!dl) return;

        const float font_size = 24.0f; // 字体大小
        const char *text = "欢迎使用 fēi fēi 辅助菜单";

        // 获取屏幕宽度
        const float sw = game::screen_width();

        // 获取文字的实际渲染宽度
        const ImVec2 text_size = ImGui::GetFont()->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text);

        // 计算右上角坐标：屏幕宽度 - 文字宽度 - 右边距(40)
        const ImVec2 pos(sw - text_size.x - 42.0f, 150.0f);

        // 画阴影（让文字在亮色背景下也能看清）
        dl->AddText(ImGui::GetFont(), font_size, ImVec2(pos.x + 1, pos.y + 1), IM_COL32(0, 0, 0, 255), text);

        // 画主文字：紫色发光颜色
        dl->AddText(ImGui::GetFont(), font_size, pos, IM_COL32(217, 128, 255, 255), text);
    }
}
