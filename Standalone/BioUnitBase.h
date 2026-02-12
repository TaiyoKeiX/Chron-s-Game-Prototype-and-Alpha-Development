#pragma once

#include <vector>
#include "BioTypes.h"

class BioUnitBase
{
public:
    FStatBlock BaseStats;
    std::vector<const UBioMutation*> AppliedMutations;
    FIntPoint GridCoordinates = FIntPoint::ZeroValue;
    int32_t TeamID = 0;
    float CurrentHitPoints = 0.0f;
    int32_t RemainingMovePoints = 0;

    FStatBlock GetEffectiveStats() const;
    void InitializeHitPoints();
    void ClampHitPointsToMax();
    bool IsAlive() const;
    void ResetMovePoints(int32_t NewMovePoints);
    void ConsumeMovePoints(int32_t Amount);
    void MoveToGrid(FIntPoint NewCoords);
    void TakeDamage(int32_t Amount);
};
