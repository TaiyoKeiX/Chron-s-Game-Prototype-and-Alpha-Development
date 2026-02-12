#include "BioUnitBase.h"

#include <algorithm>

FStatBlock BioUnitBase::GetEffectiveStats() const
{
    FStatBlock Effective = BaseStats;

    for (const UBioMutation* Mutation : AppliedMutations)
    {
        if (!Mutation)
        {
            continue;
        }

        Effective.AddBlock(Mutation->StatModifiers);
    }

    Effective.ClampNonNegative();
    return Effective;
}

void BioUnitBase::InitializeHitPoints()
{
    CurrentHitPoints = GetEffectiveStats().Get(EBioStat::HitPoints);
}

void BioUnitBase::ClampHitPointsToMax()
{
    const float MaxHitPoints = GetEffectiveStats().Get(EBioStat::HitPoints);
    CurrentHitPoints = std::clamp(CurrentHitPoints, 0.0f, MaxHitPoints);
}

bool BioUnitBase::IsAlive() const
{
    return CurrentHitPoints > 0.0f;
}

void BioUnitBase::ResetMovePoints(int32_t NewMovePoints)
{
    RemainingMovePoints = std::max(0, NewMovePoints);
}

void BioUnitBase::ConsumeMovePoints(int32_t Amount)
{
    if (Amount <= 0)
    {
        return;
    }
    RemainingMovePoints = std::max(0, RemainingMovePoints - Amount);
}

void BioUnitBase::MoveToGrid(FIntPoint NewCoords)
{
    GridCoordinates = NewCoords;
}

void BioUnitBase::TakeDamage(int32_t Amount)
{
    if (Amount <= 0)
    {
        return;
    }

    CurrentHitPoints = std::clamp(CurrentHitPoints - static_cast<float>(Amount), 0.0f, GetEffectiveStats().Get(EBioStat::HitPoints));
}
