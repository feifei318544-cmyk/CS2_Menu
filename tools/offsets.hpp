#pragma once

#include <cstddef>

namespace off {
    namespace client {
        constexpr std::ptrdiff_t dwCSGOInput = 0x257B640; // CSGOInput 偏移
        constexpr std::ptrdiff_t dwEntityList = 0x271A908; // entity list 偏移
        constexpr std::ptrdiff_t dwGameEntitySystem = 0x271A908; // 实体系统偏移
        constexpr std::ptrdiff_t dwGameEntitySystem_highestEntityIndex = 0x2120; // 最高实体索引偏移
        constexpr std::ptrdiff_t dwGameRules = 0x25651D0; // 游戏规则偏移
        constexpr std::ptrdiff_t dwGlobalVars = 0x2230090; // 全局变量偏移
        constexpr std::ptrdiff_t dwGlowManager = 0x2561EF0; // 发光管理器偏移
        constexpr std::ptrdiff_t dwLocalPlayerController = 0x253CB78; // 本地 Controller 偏移
        constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x2565718; // 本地 Pawn 偏移
        constexpr std::ptrdiff_t dwPlantedC4 = 0x24CB3C0; // 已安放 C4 偏移
        constexpr std::ptrdiff_t dwPrediction = 0x2565620; // 预测偏移
        constexpr std::ptrdiff_t dwViewAngles = 0x257BCC8; // 视角角度偏移
        constexpr std::ptrdiff_t dwViewMatrix = 0x256B020; // 视图矩阵偏移
        constexpr std::ptrdiff_t dwViewRender = 0x256B9E8; // 视图渲染偏移
        constexpr std::ptrdiff_t dwWeaponC4 = 0x24C9B30; // C4 武器偏移
    }

    namespace engine2 {
        constexpr std::ptrdiff_t dwBuildNumber = 0x61CFEC; // 构建号偏移
        constexpr std::ptrdiff_t dwNetworkGameClient = 0x91AFC0; // 网络游戏客户端偏移
        constexpr std::ptrdiff_t dwNetworkGameClient_clientTickCount = 0x398; // 客户端 tick 偏移
        constexpr std::ptrdiff_t dwNetworkGameClient_deltaTick = 0x24C; // delta tick 偏移
        constexpr std::ptrdiff_t dwNetworkGameClient_isBackgroundMap = 0x2C143F; // 是否背景地图偏移
        constexpr std::ptrdiff_t dwNetworkGameClient_localPlayer = 0xF8; // 本地玩家偏移
        constexpr std::ptrdiff_t dwNetworkGameClient_maxClients = 0x240; // 最大客户端数偏移
        constexpr std::ptrdiff_t dwNetworkGameClient_serverTickCount = 0x24C; // 服务器 tick 偏移
        constexpr std::ptrdiff_t dwNetworkGameClient_signOnState = 0x230; // 登录状态偏移
        constexpr std::ptrdiff_t dwWindowHeight = 0x91F33C; // 窗口高度偏移
        constexpr std::ptrdiff_t dwWindowWidth = 0x91F338; // 窗口宽度偏移
    }

    namespace inputsystem {
        constexpr std::ptrdiff_t dwInputSystem = 0x46BC0; // 输入系统偏移
    }

    namespace matchmaking {
        constexpr std::ptrdiff_t dwGameTypes = 0x1B0FD0; // 游戏类型偏移
    }

    namespace soundsystem {
        constexpr std::ptrdiff_t dwSoundSystem = 0x535350; // 声音系统偏移
    }
}