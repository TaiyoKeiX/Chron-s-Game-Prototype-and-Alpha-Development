#include "BioTypes.h"

#include <cmath>

const FIntPoint FIntPoint::ZeroValue{0, 0};

float FStatBlock::Get(EBioStat Stat) const
{
    return Values[static_cast<size_t>(Stat)];
}

void FStatBlock::Set(EBioStat Stat, float Value)
{
    Values[static_cast<size_t>(Stat)] = Value;
}

void FStatBlock::Add(EBioStat Stat, float Value)
{
    Values[static_cast<size_t>(Stat)] += Value;
}

void FStatBlock::AddBlock(const FStatBlock& Other)
{
    for (size_t Index = 0; Index < Values.size(); ++Index)
    {
        Values[Index] += Other.Values[Index];
    }
}

void FStatBlock::ClampNonNegative()
{
    for (float& Value : Values)
    {
        Value = std::max(0.0f, Value);
    }
}

namespace BioStatMath
{
    float ComputeMoveRange(float Speed)
    {
        constexpr float MoveBase = 2.0f;
        constexpr float MoveScale = 2.0f;
        constexpr float SpeedScale = 10.0f;

        const float ClampedSpeed = std::max(0.0f, Speed);
        const float Scaled = std::log1p(ClampedSpeed / SpeedScale);
        return MoveBase + MoveScale * Scaled;
    }

    float ComputeDodgeChance(float Speed)
    {
        constexpr float DodgeScale = 0.25f;
        constexpr float SpeedScale = 12.0f;
        constexpr float MaxDodge = 0.6f;

        const float ClampedSpeed = std::max(0.0f, Speed);
        const float Scaled = std::log1p(ClampedSpeed / SpeedScale);
        const float Chance = DodgeScale * Scaled;
        return std::clamp(Chance, 0.0f, MaxDodge);
    }

    float ComputeDefenseMitigation(float Defense)
    {
        constexpr float MitigationScale = 0.35f;
        constexpr float DefenseScale = 12.0f;
        constexpr float MaxMitigation = 0.75f;

        const float ClampedDefense = std::max(0.0f, Defense);
        const float Scaled = std::log1p(ClampedDefense / DefenseScale);
        const float Mitigation = MitigationScale * Scaled;
        return std::clamp(Mitigation, 0.0f, MaxMitigation);
    }
}
