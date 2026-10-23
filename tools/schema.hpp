#pragma once

#include <cstdint>
#include <cstddef>


// schema 偏移，取自 cs2-dumper 的 client_dll.hpp
namespace schema {
    // C_BaseEntity
    constexpr std::ptrdiff_t m_iMaxHealth = 0x348;
    constexpr std::ptrdiff_t m_iHealth = 0x34C;
    constexpr std::ptrdiff_t m_lifeState = 0x354;
    constexpr std::ptrdiff_t m_iTeamNum = 0x3E7;
    constexpr std::ptrdiff_t m_pGameSceneNode = 0x330;
    constexpr std::ptrdiff_t m_fFlags = 0x3F4;
    constexpr std::ptrdiff_t m_hOwnerEntity = 0x520;
    constexpr std::ptrdiff_t m_flSimulationTime = 0x3B8;
    constexpr std::ptrdiff_t m_flAnimTime = 0x3B4;
    constexpr std::ptrdiff_t m_nSubclassID = 0x380;
    constexpr std::ptrdiff_t m_bClientSideRagdoll = 0x3E6;

    // CGameSceneNode
    constexpr std::ptrdiff_t m_vecAbsOrigin = 0xC8;
    constexpr std::ptrdiff_t m_vecOrigin = 0x80;
    constexpr std::ptrdiff_t m_angAbsRotation = 0xD4;
    constexpr std::ptrdiff_t m_flAbsScale = 0xE0;
    constexpr std::ptrdiff_t m_bDormant = 0x103;

    // CSkeletonInstance
    constexpr std::ptrdiff_t m_modelState = 0x140;
    constexpr std::ptrdiff_t m_nHitboxSet = 0x3FC;
    constexpr std::ptrdiff_t m_BoneArray = 0x80;

    // CModelState
    constexpr std::ptrdiff_t m_hModel = 0xA0;
    constexpr std::ptrdiff_t m_ModelName = 0xA8;
    constexpr std::ptrdiff_t m_nBodyGroupChoices = 0x258;

    // CBasePlayerController
    constexpr std::ptrdiff_t m_iszPlayerName = 0x6FC;
    constexpr std::ptrdiff_t m_steamID = 0x788;

    // CCSPlayerController
    constexpr std::ptrdiff_t m_hPlayerPawn = 0x92C;
    constexpr std::ptrdiff_t m_iPawnHealth = 0x938;
    constexpr std::ptrdiff_t m_iPawnArmor = 0x93C;
    constexpr std::ptrdiff_t m_bPawnIsAlive = 0x934;
    constexpr std::ptrdiff_t m_bPawnHasDefuser = 0x940;
    constexpr std::ptrdiff_t m_bPawnHasHelmet = 0x941;

    // C_BasePlayerPawn
    constexpr std::ptrdiff_t v_angle = 0x13A8;
    constexpr std::ptrdiff_t m_hController = 0x14BC;
    constexpr std::ptrdiff_t m_pMovementServices = 0x1330;
    constexpr std::ptrdiff_t m_pItemServices = 0x12F8;
    constexpr std::ptrdiff_t m_pWeaponServices = 0x12F0;
    constexpr std::ptrdiff_t m_pObserverServices = 0x1308;

    // CPlayer_ObserverServices
    constexpr std::ptrdiff_t m_iObserverMode = 0x48;
    constexpr std::ptrdiff_t m_hObserverTarget = 0x4C;

    // C_CSPlayerPawn
    constexpr std::ptrdiff_t m_pAimPunchServices = 0x1598;
    constexpr std::ptrdiff_t m_angEyeAngles = 0x3600;
    constexpr std::ptrdiff_t m_entitySpottedState = 0x1E88;
    constexpr std::ptrdiff_t m_iShotsFired = 0x1EB8;
    constexpr std::ptrdiff_t m_ArmorValue = 0x1ED0;
    constexpr std::ptrdiff_t m_bIsScoped = 0x1EA0;
    constexpr std::ptrdiff_t m_bIsDefusing = 0x1EA2;
    constexpr std::ptrdiff_t m_bIsGrabbingHostage = 0x1EA3;
    constexpr std::ptrdiff_t m_bInBuyZone = 0x15E0;
    constexpr std::ptrdiff_t m_bInBombZone = 0x15E9;
    constexpr std::ptrdiff_t m_bIsWalking = 0x1E80;
    constexpr std::ptrdiff_t m_flVelocityModifier = 0x1EC0;
    constexpr std::ptrdiff_t m_iIDEntIndex = 0x36DC;
    constexpr std::ptrdiff_t m_pCameraServices = 0x1328;

    // CCSPlayerBase_CameraServices
    constexpr std::ptrdiff_t m_iFOV = 0x298;

    // C_CSPlayerPawn
    constexpr std::ptrdiff_t m_flViewmodelFOV = 0x1DCC;

    // CCSPlayer_AimPunchServices
    constexpr std::ptrdiff_t m_predictableBaseTick = 0x48;
    constexpr std::ptrdiff_t m_predictableBaseTickInterpAmount = 0x4C;
    constexpr std::ptrdiff_t m_predictableBaseAngle = 0x50;
    constexpr std::ptrdiff_t m_predictableBaseAngleVel = 0x5C;
    constexpr std::ptrdiff_t m_unpredictableBaseTick = 0xA0;
    constexpr std::ptrdiff_t m_unpredictableBaseAngle = 0xA4;

    // EntitySpottedState_t
    constexpr std::ptrdiff_t m_bSpotted = 0x8;
    constexpr std::ptrdiff_t m_bSpottedByMask = 0xC;

    // CPlayer_MovementServices
    constexpr std::ptrdiff_t m_bDucked = 0x408;
    constexpr std::ptrdiff_t m_flDuckAmount = 0x40C;
    constexpr std::ptrdiff_t m_bDucking = 0x416;

    // CCSPlayer_ItemServices
    constexpr std::ptrdiff_t m_bHasDefuser = 0x48;
    constexpr std::ptrdiff_t m_bHasHelmet = 0x49;

    // CPlayer_WeaponServices
    constexpr std::ptrdiff_t m_hActiveWeapon = 0x60;
    constexpr std::ptrdiff_t m_hLastWeapon = 0x64;
    constexpr std::ptrdiff_t m_iAmmo = 0x68;

    // C_AttributeContainer
    constexpr std::ptrdiff_t m_Item = 0x50;

    // C_EconItemView
    constexpr std::ptrdiff_t m_iItemDefinitionIndex = 0x1BA;
    constexpr std::ptrdiff_t m_iEntityQuality = 0x1BC;
    constexpr std::ptrdiff_t m_iEntityLevel = 0x1C0;
    constexpr std::ptrdiff_t m_iItemID = 0x1C8;
    constexpr std::ptrdiff_t m_iAccountID = 0x1D8;
    constexpr std::ptrdiff_t m_bInitialized = 0x1E8;

    // C_EconEntity
    constexpr std::ptrdiff_t m_AttributeManager = 0x1290;

    // entity list chunk 步长，固定 0x70
    constexpr std::uintptr_t kEntityStep = 0x70;

    // C_PlantedC4
    constexpr std::ptrdiff_t m_nBombSite = 0x128C;
    constexpr std::ptrdiff_t m_bBeingDefused = 0x12C4;
    constexpr std::ptrdiff_t m_bBombDefused = 0x12DC;
    constexpr std::ptrdiff_t m_flC4Blow = 0x12B8;

    // 可见性检查函数 RVA（client.dll 内偏移）
    // constexpr std::uintptr_t kIsVisibleRva = 0xB00160;

    // CCSGOInput (输入结构体内部偏移)
    constexpr std::ptrdiff_t m_vecViewAngles = 0x688; // 视角角度在 CSGOInput 里的偏移
}
