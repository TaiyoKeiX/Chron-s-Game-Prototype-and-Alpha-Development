#include "GameState.h"

static const float StatPointValues[static_cast<size_t>(EBioStat::Count)] =
{
    10.0f,  // HP per point
    3.0f,   // ATK per point
    3.0f,   // DEF per point
    3.0f    // SPD per point
};

FStatBlock FPlayerProfile::ComputeStatAllocationBonus() const
{
    FStatBlock Bonus;
    for (size_t I = 0; I < static_cast<size_t>(EBioStat::Count); ++I)
    {
        Bonus.Set(static_cast<EBioStat>(I), static_cast<float>(StatAllocation[I]) * StatPointValues[I]);
    }
    return Bonus;
}

void FPlayerProfile::AllocateStatPoint(EBioStat Stat)
{
    if (StatPoints <= 0)
    {
        return;
    }
    const size_t Index = static_cast<size_t>(Stat);
    if (Index >= static_cast<size_t>(EBioStat::Count))
    {
        return;
    }
    StatAllocation[Index]++;
    StatPoints--;
}

void FPlayerProfile::DeallocateStatPoint(EBioStat Stat)
{
    const size_t Index = static_cast<size_t>(Stat);
    if (Index >= static_cast<size_t>(EBioStat::Count))
    {
        return;
    }
    if (StatAllocation[Index] <= 0)
    {
        return;
    }
    StatAllocation[Index]--;
    StatPoints++;
}

void FPlayerProfile::ResetStatAllocation()
{
    for (size_t I = 0; I < static_cast<size_t>(EBioStat::Count); ++I)
    {
        StatPoints += StatAllocation[I];
        StatAllocation[I] = 0;
    }
}
