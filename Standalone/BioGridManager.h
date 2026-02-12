#pragma once

#include "BioTypes.h"

class BioGridManager
{
public:
    FIntPoint GridSize = FIntPoint(8, 8);
    float TileSize = 100.0f;
    FVector Origin = FVector(0.0f, 0.0f, 0.0f);

    int32_t GetManhattanDistance(FIntPoint A, FIntPoint B) const;
    FIntPoint WorldToGrid(FVector WorldLoc) const;
    FVector GridToWorld(FIntPoint GridLoc) const;
};
