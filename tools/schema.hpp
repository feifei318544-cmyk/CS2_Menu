#pragma once

#include <cstddef>

// schema 偏移，取自 cs2-dumper 的 client_dll.hpp
namespace schema {
    // C_BaseEntity
    constexpr std::ptrdiff_t m_iMaxHealth = 0x348; // 最大血量
    constexpr std::ptrdiff_t m_iHealth = 0x34C; // 当前血量
    constexpr std::ptrdiff_t m_lifeState = 0x354; // 生命状态
    constexpr std::ptrdiff_t m_iTeamNum = 0x3E7; // 阵营
    constexpr std::ptrdiff_t m_pGameSceneNode = 0x330; // 场景节点指针
    constexpr std::ptrdiff_t m_fFlags = 0x3F4; // 实体标志
    constexpr std::ptrdiff_t m_hOwnerEntity = 0x520; // 拥有者实体
    constexpr std::ptrdiff_t m_flSimulationTime = 0x3B8; // 模拟时间
    constexpr std::ptrdiff_t m_flAnimTime = 0x3B4; // 动画时间
    constexpr std::ptrdiff_t m_nSubclassID = 0x380; // 子类 ID
    constexpr std::ptrdiff_t m_bClientSideRagdoll = 0x3E6; // 客户端布娃娃

    // CGameSceneNode
    constexpr std::ptrdiff_t m_vecAbsOrigin = 0xC8; // 绝对世界坐标
    constexpr std::ptrdiff_t m_vecOrigin = 0x80; // 量化坐标
    constexpr std::ptrdiff_t m_angAbsRotation = 0xD4; // 绝对角度
    constexpr std::ptrdiff_t m_flAbsScale = 0xE0; // 绝对缩放
    constexpr std::ptrdiff_t m_bDormant = 0x103; // 是否休眠

    // CSkeletonInstance
    constexpr std::ptrdiff_t m_modelState = 0x140; // 模型状态
    constexpr std::ptrdiff_t m_nHitboxSet = 0x3FC; // 命中框集合

    // 骨骼数组，相对于 m_modelState 的偏移
    constexpr std::ptrdiff_t m_BoneArray = 0x80; // 骨骼数组

    // CModelState
    constexpr std::ptrdiff_t m_hModel = 0xA0; // 模型句柄
    constexpr std::ptrdiff_t m_ModelName = 0xA8; // 模型名字
    constexpr std::ptrdiff_t m_nBodyGroupChoices = 0x258; // 身体组选择

    // CBasePlayerController
    constexpr std::ptrdiff_t m_iszPlayerName = 0x6FC; // 玩家名字
    constexpr std::ptrdiff_t m_steamID = 0x788; // SteamID

    // CCSPlayerController
    constexpr std::ptrdiff_t m_hPlayerPawn = 0x92C; // 玩家 Pawn 句柄
    constexpr std::ptrdiff_t m_iPawnHealth = 0x938; // Pawn 血量
    constexpr std::ptrdiff_t m_iPawnArmor = 0x93C; // Pawn 护甲
    constexpr std::ptrdiff_t m_bPawnIsAlive = 0x934; // Pawn 是否存活
    constexpr std::ptrdiff_t m_bPawnHasDefuser = 0x940; // Pawn 是否有拆弹器
    constexpr std::ptrdiff_t m_bPawnHasHelmet = 0x941; // Pawn 是否有头盔

    // C_BasePlayerPawn
    constexpr std::ptrdiff_t v_angle = 0x13A8; // 视角角度
    constexpr std::ptrdiff_t m_hController = 0x14BC; // Controller 句柄
    constexpr std::ptrdiff_t m_pMovementServices = 0x1330; // 移动服务
    constexpr std::ptrdiff_t m_pItemServices = 0x12F8; // 物品服务
    constexpr std::ptrdiff_t m_pWeaponServices = 0x12F0; // 武器服务
    constexpr std::ptrdiff_t m_pObserverServices = 0x1308; // 观察者服务

    // C_CSPlayerPawn
    constexpr std::ptrdiff_t m_pAimPunchServices = 0x1598; // 后坐力服务
    constexpr std::ptrdiff_t m_angEyeAngles = 0x3600; // 眼睛角度
    constexpr std::ptrdiff_t m_entitySpottedState = 0x1E80; // 被标记状态
    constexpr std::ptrdiff_t m_iShotsFired = 0x1EB0; // 开枪数
    constexpr std::ptrdiff_t m_ArmorValue = 0x1EC8; // 护甲值
    constexpr std::ptrdiff_t m_bIsScoped = 0x1E98; // 是否开镜
    constexpr std::ptrdiff_t m_bIsDefusing = 0x1E9A; // 是否在拆弹
    constexpr std::ptrdiff_t m_bIsGrabbingHostage = 0x1E9B; // 是否在抓人质
    constexpr std::ptrdiff_t m_bInBuyZone = 0x15E0; // 是否在购买区
    constexpr std::ptrdiff_t m_bInBombZone = 0x15E9; // 是否在炸弹区
    constexpr std::ptrdiff_t m_bIsWalking = 0x1E78; // 是否在走路
    constexpr std::ptrdiff_t m_flVelocityModifier = 0x1EB8; // 速度倍率
    constexpr std::ptrdiff_t m_iIDEntIndex = 0x36DC; // 目标实体索引

    // CCSPlayer_AimPunchServices
    constexpr std::ptrdiff_t m_predictableBaseTick = 0x48; // 预测基准 tick
    constexpr std::ptrdiff_t m_predictableBaseTickInterpAmount = 0x4C; // 预测基准插值量
    constexpr std::ptrdiff_t m_predictableBaseAngle = 0x50; // 预测基准角度
    constexpr std::ptrdiff_t m_predictableBaseAngleVel = 0x5C; // 预测基准角速度
    constexpr std::ptrdiff_t m_unpredictableBaseTick = 0xA0; // 不可预测基准 tick
    constexpr std::ptrdiff_t m_unpredictableBaseAngle = 0xA4; // 不可预测基准角度

    // EntitySpottedState_t
    constexpr std::ptrdiff_t m_bSpotted = 0x8; // 是否被标记
    constexpr std::ptrdiff_t m_bSpottedByMask = 0xC; // 被谁标记

    // CPlayer_MovementServices
    constexpr std::ptrdiff_t m_bDucked = 0x408; // 是否蹲下
    constexpr std::ptrdiff_t m_flDuckAmount = 0x40C; // 蹲下程度
    constexpr std::ptrdiff_t m_bDucking = 0x416; // 蹲下标志

    // CCSPlayer_ItemServices
    constexpr std::ptrdiff_t m_bHasDefuser = 0x48; // 是否有拆弹器
    constexpr std::ptrdiff_t m_bHasHelmet = 0x49; // 是否有头盔

    // CPlayer_WeaponServices
    constexpr std::ptrdiff_t m_hActiveWeapon = 0x60; // 当前武器句柄
    constexpr std::ptrdiff_t m_hLastWeapon = 0x64; // 上一把武器句柄
    constexpr std::ptrdiff_t m_iAmmo = 0x68; // 弹药

    // C_AttributeContainer
    constexpr std::ptrdiff_t m_Item = 0x50; // 物品视图

    // C_EconItemView
    constexpr std::ptrdiff_t m_iItemDefinitionIndex = 0x1BA; // 物品定义索引
    constexpr std::ptrdiff_t m_iEntityQuality = 0x1BC; // 物品品质
    constexpr std::ptrdiff_t m_iEntityLevel = 0x1C0; // 物品等级
    constexpr std::ptrdiff_t m_iItemID = 0x1C8; // 物品 ID
    constexpr std::ptrdiff_t m_iAccountID = 0x1D8; // 账号 ID
    constexpr std::ptrdiff_t m_bInitialized = 0x1E8; // 是否初始化

    // C_EconEntity
    constexpr std::ptrdiff_t m_AttributeManager = 0x1290; // 属性管理器

    // entity list chunk 步长，固定 0x70
    constexpr std::uintptr_t kEntityStep = 0x70; // chunk 步长
}