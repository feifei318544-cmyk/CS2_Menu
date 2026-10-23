#pragma once
#include <cstdint>
#include <excpt.h>

namespace aimbot {
    // 共用参数
    extern float g_circle_radius; // 屏幕中心圆半径（像素）
    extern float g_max_world_dist; // 最大世界距离

    // 选目标偏好
    extern bool g_prefer_visible; // 优先瞄准可见敌人
    extern bool g_prefer_targeting_me; // 优先瞄准正在瞄准我的敌人
    extern bool g_only_visible; // 只瞄准可见敌人（硬过滤）

    // 暴力瞄准
    extern bool g_enabled; // 暴力自瞄开关

    // 子弹追踪
    extern bool g_silent_enabled; // 子弹追踪开关
    extern float g_silent_pitch; // 静默瞄准 pitch（俯仰角）
    extern float g_silent_yaw; // 静默瞄准 yaw（偏航角）
    extern bool g_silent_has_target; // 本帧是否有静默目标

    // RCS 压枪
    extern bool g_rcs_enabled; // RCS 压枪开关
    extern float g_rcs_strength; // RCS 压枪强度（0~1）

    // 自动开火
    extern bool g_auto_fire_enabled; // 自动开火开关

    // 平滑算法配置
    enum class SmoothType { Exponential = 0, Linear = 1, Hybrid = 2 }; // 平滑算法枚举
    extern SmoothType g_smooth_type; // 当前平滑算法
    extern float g_smooth_x; // 水平平滑系数
    extern float g_smooth_y; // 垂直平滑系数

    // 11 组瞄准部位位掩码
    enum TargetBone : uint32_t {
        Bone_Head = 1 << 0, // 头部
        Bone_Neck = 1 << 1, // 脖子
        Bone_Chest = 1 << 2, // 胸部
        Bone_Body = 1 << 3, // 身体（脊柱）
        Bone_Pelvis = 1 << 4, // 骨盆
        Bone_ArmL = 1 << 5, // 左大臂
        Bone_ForearmL = 1 << 6, // 左小臂
        Bone_HandL = 1 << 7, // 左手
        Bone_ArmR = 1 << 8, // 右大臂
        Bone_ForearmR = 1 << 9, // 右小臂
        Bone_HandR = 1 << 10, // 右手
        Bone_ThighL = 1 << 11, // 左大腿
        Bone_CalfL = 1 << 12, // 左小腿
        Bone_FootL = 1 << 13, // 左脚
        Bone_ThighR = 1 << 14, // 右大腿
        Bone_CalfR = 1 << 15, // 右小腿
        Bone_FootR = 1 << 16, // 右脚
    };

    extern uint32_t g_target_bones; // 当前选中的部位组合

    std::uintptr_t run_once(); // 主循环
    void install_silent_hook(); // 安装 hook
}
