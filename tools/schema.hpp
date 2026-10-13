#pragma once

#include <cstddef>

namespace schema {
    // C_BaseEntity
    constexpr std::ptrdiff_t m_iMaxHealth = 0x348;
    constexpr std::ptrdiff_t m_iHealth = 0x34C;
    constexpr std::ptrdiff_t m_lifeState = 0x354;
    constexpr std::ptrdiff_t m_iTeamNum = 0x3E7;
    constexpr std::ptrdiff_t m_pGameSceneNode = 0x330;

    // CGameSceneNode
    constexpr std::ptrdiff_t m_vecAbsOrigin = 0xC8;
    constexpr std::ptrdiff_t m_vecOrigin = 0x80;

    // CCSPlayerController（继承自 CBasePlayerController）
    constexpr std::ptrdiff_t m_hPlayerPawn = 0x92C;
    constexpr std::ptrdiff_t m_iszPlayerName = 0x6FC;

    // C_BasePlayerPawn
    constexpr std::ptrdiff_t v_angle = 0x13A8;
    constexpr std::ptrdiff_t m_hController = 0x14BC;

    // C_CSPlayerPawn
    constexpr std::ptrdiff_t m_angEyeAngles = 0x3600;
    constexpr std::ptrdiff_t m_entitySpottedState = 0x1E88;

    // EntitySpottedState_t 的子结构
    constexpr std::ptrdiff_t m_bSpotted = 0x8;
    constexpr std::ptrdiff_t m_bSpottedByMask = 0xC;
}