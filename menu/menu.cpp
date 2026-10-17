#include "menu.hpp"
#include "../aimbot/aimbot.hpp"
#include "../visual/visual.hpp"
#include <imgui.h>

namespace menu {
    bool g_unload_requested = false;
    bool g_rcs_enabled = false;
    float g_rcs_smooth = 0.7f;

    void draw() {
        static bool style_done = false;
        if (!style_done) {
            style_done = true;

            ImGuiStyle &style = ImGui::GetStyle();
            style.WindowRounding = 12.0f;
            style.FrameRounding = 6.0f;
            style.GrabRounding = 6.0f;
            style.TabRounding = 6.0f;
            style.ScrollbarRounding = 6.0f;
            style.WindowPadding = ImVec2(12, 12);
            style.FramePadding = ImVec2(8, 5);
            style.ItemSpacing = ImVec2(10, 8);

            ImVec4 *colors = style.Colors;
            colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.09f, 0.96f);
            colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.90f);
            colors[ImGuiCol_Border] = ImVec4(0.30f, 0.15f, 0.60f, 0.80f);
            colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.10f, 0.20f, 1.00f);
            colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.15f, 0.35f, 1.00f);
            colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.20f, 0.55f, 1.00f);
            colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.06f, 0.15f, 1.00f);
            colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.08f, 0.30f, 1.00f);
            colors[ImGuiCol_CheckMark] = ImVec4(0.75f, 0.40f, 1.00f, 1.00f);
            colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.35f, 1.00f, 1.00f);
            colors[ImGuiCol_SliderGrabActive] = ImVec4(0.85f, 0.50f, 1.00f, 1.00f);
            colors[ImGuiCol_Button] = ImVec4(0.20f, 0.10f, 0.40f, 1.00f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.18f, 0.65f, 1.00f);
            colors[ImGuiCol_ButtonActive] = ImVec4(0.50f, 0.25f, 0.90f, 1.00f);
            colors[ImGuiCol_Header] = ImVec4(0.20f, 0.10f, 0.40f, 1.00f);
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.18f, 0.65f, 1.00f);
            colors[ImGuiCol_HeaderActive] = ImVec4(0.50f, 0.25f, 0.90f, 1.00f);
            colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.08f, 0.18f, 1.00f);
            colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.18f, 0.65f, 1.00f);
            colors[ImGuiCol_TabActive] = ImVec4(0.30f, 0.15f, 0.55f, 1.00f);
            colors[ImGuiCol_Separator] = ImVec4(0.35f, 0.18f, 0.65f, 0.60f);
            colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 1.00f, 1.00f);
            colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);

            ImGuiIO &io = ImGui::GetIO();
            io.ConfigWindowsMoveFromTitleBarOnly = false;
        }

        ImGui::SetNextWindowSize(ImVec2(800, 900), ImGuiCond_FirstUseEver);
        ImGui::Begin("##CS2Menu", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

        {
            const float title_h = 34.0f;
            const ImVec2 title_pos = ImGui::GetCursorScreenPos();
            const float title_w = ImGui::GetContentRegionAvail().x;

            ImGui::InvisibleButton("##titlebar", ImVec2(title_w, title_h), ImGuiButtonFlags_MouseButtonLeft);

            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                const ImVec2 delta = ImGui::GetIO().MouseDelta;
                const ImVec2 wpos = ImGui::GetWindowPos();
                ImGui::SetWindowPos(ImVec2(wpos.x + delta.x, wpos.y + delta.y));
            }

            ImDrawList *dl = ImGui::GetWindowDrawList();
            const char *title = "f ē i f ē i   辅 助 菜 单";
            const ImVec2 tsize = ImGui::CalcTextSize(title);
            const ImVec2 tpos(title_pos.x + (title_w - tsize.x) * 0.5f, title_pos.y + (title_h - tsize.y) * 0.5f);
            dl->AddText(tpos, IM_COL32(217, 128, 255, 255), title);

            const float sep_y = title_pos.y + title_h;
            dl->AddLine(ImVec2(title_pos.x, sep_y), ImVec2(title_pos.x + title_w, sep_y), IM_COL32(89, 46, 166, 180), 1.0f);

            ImGui::SetCursorScreenPos(ImVec2(title_pos.x, sep_y + ImGui::GetStyle().ItemSpacing.y));
        }

        if (ImGui::BeginTabBar("##MainTabs", ImGuiTabBarFlags_None)) {
            // 游戏 Tab
            if (ImGui::BeginTabItem("  游戏  ")) {
                ImGui::Spacing();

                if (ImGui::CollapsingHeader("瞄准选项(慎用)", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "瞄准方式");
                    ImGui::Separator();

                    bool aim_changed = ImGui::Checkbox("暴力自瞄", &aimbot::g_enabled);
                    if (aim_changed && aimbot::g_enabled) {
                        aimbot::g_silent_enabled = false;
                        aimbot::g_silent_has_target = false;
                    }

                    bool silent_changed = ImGui::Checkbox("子弹追踪", &aimbot::g_silent_enabled);
                    if (silent_changed && aimbot::g_silent_enabled) {
                        aimbot::g_enabled = false;
                    }

                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "瞄准参数");
                    ImGui::Separator();

                    const bool any_aim = aimbot::g_enabled || aimbot::g_silent_enabled;
                    ImGui::BeginDisabled(!any_aim);
                    ImGui::SliderFloat("范围", &aimbot::g_circle_radius, 320.0f, 800.0f, "%.0f px");
                    ImGui::SliderFloat("最大距离", &aimbot::g_max_world_dist, 500.0f, 10000.0f, "%.0f 单位");

                    const char* smooth_types[] = { "指数收敛 (Exp)", "恒速步进 (Linear)", "混合 (Hybrid)" };
                    int current_type = (int)aimbot::g_smooth_type;
                    if (ImGui::Combo("平滑算法", &current_type, smooth_types, IM_ARRAYSIZE(smooth_types))) {
                        aimbot::g_smooth_type = (aimbot::SmoothType)current_type;
                    }

                    // 算法说明展示
                    if (aimbot::g_smooth_type == aimbot::SmoothType::Exponential) {
                        ImGui::TextDisabled("'指数收敛'越接近目标速度越慢，极其平滑，适合远距离锁头");
                    } else if (aimbot::g_smooth_type == aimbot::SmoothType::Linear) {
                        ImGui::TextDisabled("'恒速步进'固定速度拉向目标，中距离手感极佳，像人类操作");
                    } else {
                        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "'混合平滑'起步快，停止稳，防拉回（职业哥最爱）");
                    }

                    ImGui::SliderFloat("水平平滑(X)", &aimbot::g_smooth_x, 0.1f, 20.0f, "%.1f");
                    ImGui::SliderFloat("垂直平滑(Y)", &aimbot::g_smooth_y, 0.1f, 20.0f, "%.1f");

                    ImGui::Checkbox("优先可见敌人", &aimbot::g_prefer_visible);
                    ImGui::SameLine(0, 20.0f);
                    ImGui::Checkbox("优先瞄准我的人", &aimbot::g_prefer_targeting_me);
                    ImGui::SameLine(0, 20.0f);
                    ImGui::Checkbox("只瞄可见敌人", &aimbot::g_only_visible);
                    ImGui::EndDisabled();

                    // 瞄准部位 (单选)
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "瞄准部位 (单选)");
                    ImGui::Separator();

                    auto BoneRadio = [](const char* label, uint32_t bit) {
                        if (ImGui::RadioButton(label, aimbot::g_target_bones == bit)) {
                            aimbot::g_target_bones = bit;
                        }
                    };

                    // 第一排：躯干
                    BoneRadio("头部", aimbot::Bone_Head); ImGui::SameLine(0, 15.0f);
                    BoneRadio("脖子", aimbot::Bone_Neck); ImGui::SameLine(0, 15.0f);
                    BoneRadio("胸部", aimbot::Bone_Chest); ImGui::SameLine(0, 15.0f);
                    BoneRadio("身体", aimbot::Bone_Body); ImGui::SameLine(0, 15.0f);
                    BoneRadio("骨盆", aimbot::Bone_Pelvis);

                    // 第二排：左臂
                    BoneRadio("左大臂", aimbot::Bone_ArmL); ImGui::SameLine(0, 15.0f);
                    BoneRadio("左小臂", aimbot::Bone_ForearmL); ImGui::SameLine(0, 15.0f);
                    BoneRadio("左手", aimbot::Bone_HandL);

                    // 第三排：右臂
                    BoneRadio("右大臂", aimbot::Bone_ArmR); ImGui::SameLine(0, 15.0f);
                    BoneRadio("右小臂", aimbot::Bone_ForearmR); ImGui::SameLine(0, 15.0f);
                    BoneRadio("右手", aimbot::Bone_HandR);

                    // 第四排：腿部
                    BoneRadio("左大腿", aimbot::Bone_ThighL); ImGui::SameLine(0, 15.0f);
                    BoneRadio("左小腿", aimbot::Bone_CalfL); ImGui::SameLine(0, 15.0f);
                    BoneRadio("左脚", aimbot::Bone_FootL); ImGui::SameLine(0, 15.0f);
                    BoneRadio("右大腿", aimbot::Bone_ThighR); ImGui::SameLine(0, 15.0f);
                    BoneRadio("右小腿", aimbot::Bone_CalfR); ImGui::SameLine(0, 15.0f);
                    BoneRadio("右脚", aimbot::Bone_FootR);

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "后坐力控制 (RCS)");
                    ImGui::Separator();
                    ImGui::Checkbox("开启独立无后座(RCS)", &g_rcs_enabled);
                    ImGui::BeginDisabled(!g_rcs_enabled);
                    ImGui::SliderFloat("压枪平滑度", &g_rcs_smooth, 0.1f, 2.0f, "%.1f");
                    ImGui::TextDisabled("数值建议为0.7");
                    ImGui::TextDisabled("1.0 为标准值，过大准星会往地上栽");
                    ImGui::EndDisabled();
                    ImGui::Spacing();
                }

                if (ImGui::CollapsingHeader("绘制选项", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "透视");
                    ImGui::Separator();
                    ImGui::Checkbox("开启绘制", &visual::g_esp_enabled);

                    ImGui::BeginDisabled(!visual::g_esp_enabled);
                    ImGui::Checkbox("显示血量", &visual::g_esp_show_health); ImGui::SameLine(0, 20.0f);
                    ImGui::Checkbox("显示射线", &visual::g_esp_show_ray); ImGui::SameLine(0, 20.0f);
                    ImGui::Checkbox("显示名称", &visual::g_esp_show_name); ImGui::SameLine(0, 20.0f);
                    ImGui::Checkbox("显示骨骼", &visual::g_esp_show_skeleton);

                    ImGui::Checkbox("显示武器", &visual::g_esp_show_weapon); ImGui::SameLine(0, 20.0f);
                    ImGui::Checkbox("显示距离", &visual::g_esp_show_distance); ImGui::SameLine(0, 20.0f);
                    ImGui::Checkbox("显示护甲", &visual::g_esp_show_armor); ImGui::SameLine(0, 20.0f);
                    ImGui::Checkbox("显示头盔", &visual::g_esp_show_helmet);
                    ImGui::Checkbox("显示 C4", &visual::g_esp_show_c4);

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    ImGui::EndDisabled();


                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "雷达");
                    ImGui::Checkbox("开启雷达", &visual::g_radar_enabled);
                    ImGui::Spacing();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("  设置  ")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.5f, 1.0f), "辅助");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.50f, 0.10f, 0.10f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.15f, 0.15f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.90f, 0.20f, 0.20f, 1.00f));
                if (ImGui::Button("卸载辅助菜单", ImVec2(-1, 36))) g_unload_requested = true;
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