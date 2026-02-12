#include "BioGridManager.h"
#include "Math/UnrealMathUtility.h"

ABioGridManager::ABioGridManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

int32 ABioGridManager::GetManhattanDistance_Implementation(FIntPoint A, FIntPoint B) const
{
    return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
}

FIntPoint ABioGridManager::WorldToGrid_Implementation(FVector WorldLoc) const
{
    if (TileSize <= 0.0f)
    {
        return FIntPoint::ZeroValue;
    }

    const FVector Local = WorldLoc - GetActorLocation();
    const int32 X = FMath::RoundToInt(Local.X / TileSize);
    const int32 Y = FMath::RoundToInt(Local.Y / TileSize);
    return FIntPoint(X, Y);
}

FVector ABioGridManager::GridToWorld_Implementation(FIntPoint GridLoc) const
{
    const FVector Origin = GetActorLocation();
    return Origin + FVector(GridLoc.X * TileSize, GridLoc.Y * TileSize, 0.0f);
}
