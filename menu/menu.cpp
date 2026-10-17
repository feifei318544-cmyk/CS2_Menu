#include "menu.hpp"
#include "../aimbot/aimbot.hpp"
#include "../visual/visual.hpp"
#include <imgui.h>

namespace menu {
    bool g_unload_requested = false; // 卸载请求标志

    void draw() {
        // 1. 先设置炫酷样式
        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding    = 12.0f; // 窗口圆角
        style.FrameRounding     = 6.0f; // 控件圆角
        style.GrabRounding      = 6.0f; // 滑块圆角
        style.TabRounding       = 6.0f; // Tab 圆角
        style.ScrollbarRounding = 6.0f; // 滚动条圆角
        style.WindowPadding     = ImVec2(12, 12); // 窗口内边距
        style.FramePadding      = ImVec2(8, 5); // 控件内边距
        style.ItemSpacing       = ImVec2(10, 8); // 控件间距

        // 深色 + 紫色主题
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg]       = ImVec4(0.06f, 0.06f, 0.09f, 0.96f); // 窗口背景
        colors[ImGuiCol_ChildBg]        = ImVec4(0.08f, 0.08f, 0.12f, 0.90f); // 子窗口背景
        colors[ImGuiCol_Border]         = ImVec4(0.30f, 0.15f, 0.60f, 0.80f); // 边框
        colors[ImGuiCol_FrameBg]        = ImVec4(0.12f, 0.10f, 0.20f, 1.00f); // 控件背景
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.15f, 0.35f, 1.00f); // 控件悬停
        colors[ImGuiCol_FrameBgActive]  = ImVec4(0.30f, 0.20f, 0.55f, 1.00f); // 控件激活
        colors[ImGuiCol_TitleBg]        = ImVec4(0.08f, 0.06f, 0.15f, 1.00f); // 标题栏背景
        colors[ImGuiCol_TitleBgActive]  = ImVec4(0.15f, 0.08f, 0.30f, 1.00f); // 标题栏激活
        colors[ImGuiCol_CheckMark]      = ImVec4(0.75f, 0.40f, 1.00f, 1.00f); // 复选框勾
        colors[ImGuiCol_SliderGrab]     = ImVec4(0.70f, 0.35f, 1.00f, 1.00f); // 滑块
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.85f, 0.50f, 1.00f, 1.00f); // 滑块激活
        colors[ImGuiCol_Button]         = ImVec4(0.20f, 0.10f, 0.40f, 1.00f); // 按钮
        colors[ImGuiCol_ButtonHovered]  = ImVec4(0.35f, 0.18f, 0.65f, 1.00f); // 按钮悬停
        colors[ImGuiCol_ButtonActive]   = ImVec4(0.50f, 0.25f, 0.90f, 1.00f); // 按钮激活
        colors[ImGuiCol_Header]         = ImVec4(0.20f, 0.10f, 0.40f, 1.00f); // 标题
        colors[ImGuiCol_HeaderHovered]  = ImVec4(0.35f, 0.18f, 0.65f, 1.00f); // 标题悬停
        colors[ImGuiCol_HeaderActive]   = ImVec4(0.50f, 0.25f, 0.90f, 1.00f); // 标题激活
        colors[ImGuiCol_Tab]            = ImVec4(0.10f, 0.08f, 0.18f, 1.00f); // Tab
        colors[ImGuiCol_TabHovered]     = ImVec4(0.35f, 0.18f, 0.65f, 1.00f); // Tab 悬停
        colors[ImGuiCol_TabActive]      = ImVec4(0.30f, 0.15f, 0.55f, 1.00f); // Tab 激活
        colors[ImGuiCol_Separator]      = ImVec4(0.35f, 0.18f, 0.65f, 0.60f); // 分隔线
        colors[ImGuiCol_Text]           = ImVec4(0.95f, 0.95f, 1.00f, 1.00f); // 文本
        colors[ImGuiCol_TextDisabled]   = ImVec4(0.50f, 0.50f, 0.55f, 1.00f); // 禁用文本

        // 2. 窗口
        ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_FirstUseEver);
        ImGui::Begin("", nullptr,
            ImGuiWindowFlags_NoCollapse);

        // 顶部标题栏
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.50f, 1.00f, 1.00f));
        ImGui::Text("F E I F E I   C H E A T");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("v1.0");

        ImGui::Separator();
        ImGui::Spacing();

        // 3. Tab 栏
        if (ImGui::BeginTabBar("##MainTabs", ImGuiTabBarFlags_None)) {

            // 游戏 Tab
            if (ImGui::BeginTabItem("  游戏  ")) {
                ImGui::Spacing();

                // 瞄准选项
                if (ImGui::CollapsingHeader("瞄准选项", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "瞄准方式");
                    ImGui::Separator();

                    bool aim_changed = ImGui::Checkbox("暴力自瞄", &aimbot::g_enabled); // 暴力自瞄开关
                    if (aim_changed && aimbot::g_enabled) {
                        aimbot::g_silent_enabled = false;
                        aimbot::g_silent_has_target = false;
                    }

                    bool silent_changed = ImGui::Checkbox("子弹追踪", &aimbot::g_silent_enabled); // 子弹追踪开关
                    if (silent_changed && aimbot::g_silent_enabled) {
                        aimbot::g_enabled = false;
                    }

                    ImGui::Checkbox("RCS 压枪", &aimbot::g_rcs_enabled); // RCS 压枪开关
                    ImGui::BeginDisabled(!aimbot::g_rcs_enabled);
                    ImGui::SliderFloat("RCS 强度", &aimbot::g_rcs_strength, 0.0f, 1.0f, "%.2f"); // RCS 强度
                    ImGui::EndDisabled();

                    ImGui::Checkbox("自动开火", &aimbot::g_auto_fire_enabled); // 自动开火开关

                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "瞄准参数");
                    ImGui::Separator();

                    const bool any_aim = aimbot::g_enabled || aimbot::g_silent_enabled; // 是否启用任意瞄准
                    ImGui::BeginDisabled(!any_aim);
                    ImGui::SliderFloat("范围", &aimbot::g_circle_radius, 320.0f, 800.0f, "%.0f px"); // 瞄准范围
                    ImGui::SliderFloat("最大距离", &aimbot::g_max_world_dist, 500.0f, 10000.0f, "%.0f 单位"); // 最大世界距离
                    ImGui::EndDisabled();

                    ImGui::Spacing();
                }

                // 透视
                if (ImGui::CollapsingHeader("透视选项", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "透视");
                    ImGui::Separator();

                    ImGui::Checkbox("开启绘制", &visual::g_esp_enabled); // 透视总开关

                    ImGui::BeginDisabled(!visual::g_esp_enabled);
                    ImGui::Checkbox("显示血量", &visual::g_esp_show_health); // 显示血量
                    ImGui::SameLine(0, 20.0f);
                    ImGui::Checkbox("显示射线", &visual::g_esp_show_ray); // 显示射线
                    ImGui::SameLine(0, 20.0f);
                    ImGui::Checkbox("显示名称", &visual::g_esp_show_name); // 显示名称
                    ImGui::SameLine(0, 20.0f);
                    ImGui::Checkbox("显示骨骼", &visual::g_esp_show_skeleton); // 显示骨骼

                    ImGui::Checkbox("显示武器", &visual::g_esp_show_weapon); // 显示武器
                    ImGui::SameLine(0, 20.0f);
                    ImGui::Checkbox("显示距离", &visual::g_esp_show_distance); // 显示距离
                    ImGui::SameLine(0, 20.0f);
                    ImGui::Checkbox("显示护甲", &visual::g_esp_show_armor); // 显示护甲
                    ImGui::SameLine(0, 20.0f);
                    ImGui::Checkbox("显示头盔", &visual::g_esp_show_helmet); // 显示头盔
                    ImGui::EndDisabled();

                    ImGui::Spacing();
                }

                ImGui::EndTabItem();
            }

            // 设置 Tab
            if (ImGui::BeginTabItem("  设置  ")) {
                ImGui::Spacing();

                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.5f, 1.0f), "辅助");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.50f, 0.10f, 0.10f, 1.00f)); // 卸载按钮颜色
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.15f, 0.15f, 1.00f)); // 卸载按钮悬停颜色
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.90f, 0.20f, 0.20f, 1.00f)); // 卸载按钮激活颜色

                if (ImGui::Button("卸载辅助菜单", ImVec2(-1, 36))) {
                    g_unload_requested = true;
                }

                ImGui::PopStyleColor(3);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextDisabled("提示: 按 F11 开关菜单");
                ImGui::TextDisabled("卸载后请用注入器卸载 DLL");

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }
}