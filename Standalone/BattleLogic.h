#pragma once

#include "GameState.h"
#include "LevelData.h"

namespace Battle
{
    const char* GetPhaseName(ETurnPhase Phase);

    // Queries
    int FindUnitAt(const FGameState& State, FIntPoint Cell);
    bool IsObstacleAt(const FGameState& State, FIntPoint Cell);
    bool IsHealingPodAt(const FGameState& State, FIntPoint Cell);
    bool IsHazardAt(const FGameState& State, FIntPoint Cell);
    bool IsEnvironmentAt(const FGameState& State, FIntPoint Cell);
    bool IsCellInBounds(const FGameState& State, FIntPoint Cell);
    int CountAliveTeam(const FGameState& State, int TeamId);

    // Cell finding
    FIntPoint FindFirstEmptyCell(const FGameState& State);
    FIntPoint FindEmptyCellInRange(const FGameState& State, int32_t MinX, int32_t MaxX);
    FIntPoint FindEmptyCellInRect(const FGameState& State, int32_t MinX, int32_t MaxX, int32_t MinY, int32_t MaxY);

    // Stats
    float GetStatValue(const BioUnitBase& Unit, EBioStat Stat);
    int32_t GetMoveRange(const BioUnitBase& Unit);

    // Phase management
    void ResetMovementForTeam(FGameState& State, int TeamId);
    void HandlePhaseStart(FGameState& State);

    // Unit management
    void AddUnit(FGameState& State, FIntPoint Cell, int TeamId, const FStatBlock* EnemyStats = nullptr);

    // Environment
    void AddObstacle(FGameState& State, FIntPoint Cell);
    void AddHealingPod(FGameState& State, FIntPoint Cell);
    void AddHazard(FGameState& State, FIntPoint Cell);
    void GenerateRandomObstacles(FGameState& State, int32_t Count);
    void GenerateRandomEnvironment(FGameState& State, int32_t HealCount, int32_t HazardCount);
    void ApplyEnvironmentEffects(FGameState& State);

    // Combat
    float ComputeDamageAfterDefense(float Attack, float Defense);
    void ApplyDamage(BioUnitBase& Unit, float Damage);
    void ResolveEncounterPair(FGameState& State, int IndexA, int IndexB);
    void RemoveDeadUnits(FGameState& State);
    void ResolveEncounters(FGameState& State);

    // AI
    bool MoveEnemyUnitTowards(FGameState& State, int UnitIndex, FIntPoint Target);
    void RunEnemyAiStep(FGameState& State);

    // Initialization
    void InitializeBattle(FGameState& State, const FLevelData& Level);
}
