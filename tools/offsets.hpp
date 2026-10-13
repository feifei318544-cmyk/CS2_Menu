#pragma once

#include <cstddef>

/**
 * 全局偏移，来自 cs2-dumper 的 output/offsets.hpp
 * 每次游戏更新后重新跑 dumper 同步。
 * 最后同步时间：2026-10-13 03:29:51 UTC
 */
namespace off {
    namespace client {
        constexpr std::ptrdiff_t dwCSGOInput = 0x2572460;
        constexpr std::ptrdiff_t dwEntityList = 0x2711598;
        constexpr std::ptrdiff_t dwGameEntitySystem = 0x2711598;
        constexpr std::ptrdiff_t dwGameEntitySystem_highestEntityIndex = 0x2120;
        constexpr std::ptrdiff_t dwGameRules = 0x255BE80;
        constexpr std::ptrdiff_t dwGlobalVars = 0x2228090;
        constexpr std::ptrdiff_t dwGlowManager = 0x2558BA0;
        constexpr std::ptrdiff_t dwLocalPlayerController = 0x25338A8;
        constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x255C3C8;
        constexpr std::ptrdiff_t dwPlantedC4 = 0x24C2248;
        constexpr std::ptrdiff_t dwPrediction = 0x255C2D0;
        constexpr std::ptrdiff_t dwViewAngles = 0x2572AE8;
        constexpr std::ptrdiff_t dwViewMatrix = 0x2561CD0;
        constexpr std::ptrdiff_t dwViewRender = 0x2562698;
        constexpr std::ptrdiff_t dwWeaponC4 = 0x24C0800;
    }

    namespace engine2 {
        constexpr std::ptrdiff_t dwBuildNumber = 0x61CFE8;
        constexpr std::ptrdiff_t dwNetworkGameClient = 0x91AFC0;
        constexpr std::ptrdiff_t dwNetworkGameClient_clientTickCount = 0x398;
        constexpr std::ptrdiff_t dwNetworkGameClient_deltaTick = 0x24C;
        constexpr std::ptrdiff_t dwNetworkGameClient_isBackgroundMap = 0x2C143F;
        constexpr std::ptrdiff_t dwNetworkGameClient_localPlayer = 0xF8;
        constexpr std::ptrdiff_t dwNetworkGameClient_maxClients = 0x240;
        constexpr std::ptrdiff_t dwNetworkGameClient_serverTickCount = 0x24C;
        constexpr std::ptrdiff_t dwNetworkGameClient_signOnState = 0x230;
        constexpr std::ptrdiff_t dwWindowHeight = 0x91F334;
        constexpr std::ptrdiff_t dwWindowWidth = 0x91F330;
    }

    namespace inputsystem {
        constexpr std::ptrdiff_t dwInputSystem = 0x46BC0;
    }
    namespace matchmaking {
        constexpr std::ptrdiff_t dwGameTypes = 0x1B0FD0;
    }
    namespace soundsystem {
        constexpr std::ptrdiff_t dwSoundSystem = 0x535350;
    }
} // offsets.hpp namespace off