#pragma once

#include "BioTypes.h"
#include <cstdint>
#include <string>
#include <vector>

struct FLevelData
{
    int LevelNumber = 0;
    std::string Name;
    std::string Description;
    int EnemyCount = 4;
    FStatBlock EnemyBaseStats;
    int GridWidth = 8;
    int GridHeight = 8;
    int ObstacleCount = 10;
    int HealPodCount = 3;
    int HazardCount = 4;
    int PlayerUnitCount = 2;
};

const std::vector<FLevelData>& GetAllLevels();
const FLevelData* GetLevel(int LevelNumber);
