#include "BioGridManager.h"

#include <cmath>

int32_t BioGridManager::GetManhattanDistance(FIntPoint A, FIntPoint B) const
{
    return std::abs(A.X - B.X) + std::abs(A.Y - B.Y);
}

FIntPoint BioGridManager::WorldToGrid(FVector WorldLoc) const
{
    if (TileSize <= 0.0f)
    {
        return FIntPoint::ZeroValue;
    }

    const FVector Local = WorldLoc - Origin;
    const int32_t X = static_cast<int32_t>(std::lround(Local.X / TileSize));
    const int32_t Y = static_cast<int32_t>(std::lround(Local.Y / TileSize));
    return FIntPoint(X, Y);
}

FVector BioGridManager::GridToWorld(FIntPoint GridLoc) const
{
    return Origin + FVector(GridLoc.X * TileSize, GridLoc.Y * TileSize, 0.0f);
}
