#pragma once

#include "BioGameMode.h"
#include "BioGridManager.h"
#include "BioTypes.h"
#include "BioUnitBase.h"
#include "SkillTree.h"

#include <random>
#include <string>
#include <vector>

#include "imgui.h"

enum class EGameScreen : uint8_t
{
    MainMenu,
    LevelSelect,
    CharacterUpgrade,
    Battle,
    BattleResult
};

enum class EAutoTurnState : uint8_t
{
    Idle,
    EnemyAI_Running,
    EnemyAI_Pause,
    Environment_Running,
    Environment_Pause,
    Transitioning
};

struct FPlayerProfile
{
    FSkillTreeState SkillTree;
    int32_t StatPoints = 0;
    std::array<int32_t, static_cast<size_t>(EBioStat::Count)> StatAllocation{};
    int32_t HighestLevelCompleted = 0;

    FStatBlock ComputeStatAllocationBonus() const;
    void AllocateStatPoint(EBioStat Stat);
    void DeallocateStatPoint(EBioStat Stat);
    void ResetStatAllocation();
};

struct FUnitInstance
{
    BioUnitBase Unit;
    std::string Name;
};

struct FGameState
{
    EGameScreen CurrentScreen = EGameScreen::MainMenu;
    FPlayerProfile Profile;

    // Battle state
    BioGameMode Mode;
    BioGridManager Grid;
    std::vector<FUnitInstance> Units;
    std::vector<FIntPoint> Obstacles;
    std::vector<FIntPoint> HealingPods;
    std::vector<FIntPoint> HazardTiles;
    int SelectedUnitIndex = -1;
    int NextUnitId = 1;
    ETurnPhase LastPhase = ETurnPhase::PlayerInput;
    bool EnemyAiExecuted = false;
    std::mt19937 Rng;
    std::uniform_real_distribution<float> Dist01{0.0f, 1.0f};

    // Camera
    float CameraYaw = 0.785398f;
    float CameraPitch = 0.61548f;
    float CameraZoom = 1.0f;
    ImVec2 CameraPan = ImVec2(0.0f, 0.0f);

    // Battle result
    bool VictoryPointAwarded = false;
    bool BattleWon = false;
    int CurrentLevel = 0;

    // Auto-turn
    EAutoTurnState AutoTurnState = EAutoTurnState::Idle;
    float AutoTurnTimer = 0.0f;
};
