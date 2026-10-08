#include "menu.hpp"
#include "../aimbot/aimbot.hpp"
#include "../visual/visual.hpp"
#include <imgui.h>

namespace menu {
    void draw() {
        ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_FirstUseEver);
        ImGui::Begin("fēifēi菜单");

        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "自瞄");
        ImGui::Separator();

        ImGui::Checkbox("暴力自瞄 (慎用)", &aimbot::g_enabled);

        ImGui::BeginDisabled(!aimbot::g_enabled);
        ImGui::SliderFloat("范围", &aimbot::g_circle_radius, 320.0f, 800.0f, "%.0f px");
        ImGui::SliderFloat("最大距离", &aimbot::g_max_world_dist, 500.0f, 10000.0f, "%.0f 单位");
        ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "透视");
        ImGui::Separator();

        ImGui::Checkbox("开启透视", &visual::g_esp_enabled);

        ImGui::BeginDisabled(!visual::g_esp_enabled);
        ImGui::Checkbox("显示血量", &visual::g_esp_show_health);
        ImGui::SameLine(0, 20.0f);
        ImGui::Checkbox("显示射线", &visual::g_esp_show_ray);
        ImGui::SameLine(0, 20.0f);
        ImGui::Checkbox("显示名称", &visual::g_esp_show_name);
        ImGui::EndDisabled();

        ImGui::Spacing();

        ImGui::End();
    }
}