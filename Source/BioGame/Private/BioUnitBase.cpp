#include "BioUnitBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Math/UnrealMathUtility.h"

ABioUnitBase::ABioUnitBase()
{
    PrimaryActorTick.bCanEverTick = false;

    UnitMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UnitMesh"));
    SetRootComponent(UnitMesh);

    UnitWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("UnitWidget"));
    UnitWidget->SetupAttachment(UnitMesh);
}

FBiologicalStats ABioUnitBase::GetEffectiveStats_Implementation() const
{
    FBiologicalStats Effective = BaseStats;

    for (const UBioMutation* Mutation : AppliedMutations)
    {
        if (!Mutation)
        {
            continue;
        }

        Effective.MaxHealth += Mutation->StatModifiers.MaxHealth;
        Effective.CurrentHealth += Mutation->StatModifiers.CurrentHealth;
        Effective.AttackPower += Mutation->StatModifiers.AttackPower;
        Effective.Defense += Mutation->StatModifiers.Defense;
        Effective.MoveRange += Mutation->StatModifiers.MoveRange;
        Effective.InflammationCost += Mutation->StatModifiers.InflammationCost;
    }

    return Effective;
}

void ABioUnitBase::MoveToGrid_Implementation(FIntPoint NewCoords)
{
    GridCoordinates = NewCoords;
}

void ABioUnitBase::TakeDamage_Implementation(int32 Amount)
{
    if (Amount <= 0)
    {
        return;
    }

    BaseStats.CurrentHealth = FMath::Clamp(BaseStats.CurrentHealth - Amount, 0, BaseStats.MaxHealth);
}
