#include "LevelData.h"

static std::vector<FLevelData> BuildLevelTable()
{
    std::vector<FLevelData> Levels;

    auto MakeStats = [](float Hp, float Atk, float Def, float Spd)
    {
        FStatBlock S;
        S.Set(EBioStat::HitPoints, Hp);
        S.Set(EBioStat::Attack, Atk);
        S.Set(EBioStat::Defense, Def);
        S.Set(EBioStat::Speed, Spd);
        return S;
    };

    // Level 1 — Infection Site
    {
        FLevelData L;
        L.LevelNumber = 1;
        L.Name = "Infection Site";
        L.Description = "A minor bacterial incursion. Clear the pathogens.";
        L.EnemyCount = 3;
        L.EnemyBaseStats = MakeStats(70.0f, 7.0f, 2.0f, 8.0f);
        L.GridWidth = 8;
        L.GridHeight = 8;
        L.ObstacleCount = 8;
        L.HealPodCount = 3;
        L.HazardCount = 3;
        L.PlayerUnitCount = 2;
        Levels.push_back(L);
    }

    // Level 2 — Mucous Membrane
    {
        FLevelData L;
        L.LevelNumber = 2;
        L.Name = "Mucous Membrane";
        L.Description = "Pathogens have breached the outer barrier.";
        L.EnemyCount = 4;
        L.EnemyBaseStats = MakeStats(80.0f, 8.0f, 3.0f, 9.0f);
        L.GridWidth = 8;
        L.GridHeight = 8;
        L.ObstacleCount = 10;
        L.HealPodCount = 3;
        L.HazardCount = 4;
        L.PlayerUnitCount = 2;
        Levels.push_back(L);
    }

    // Level 3 — Lymph Node
    {
        FLevelData L;
        L.LevelNumber = 3;
        L.Name = "Lymph Node";
        L.Description = "The infection has reached a lymph node. Expect heavier resistance.";
        L.EnemyCount = 5;
        L.EnemyBaseStats = MakeStats(95.0f, 10.0f, 5.0f, 10.0f);
        L.GridWidth = 8;
        L.GridHeight = 8;
        L.ObstacleCount = 10;
        L.HealPodCount = 2;
        L.HazardCount = 6;
        L.PlayerUnitCount = 2;
        Levels.push_back(L);
    }

    // Level 4 — Blood Stream
    {
        FLevelData L;
        L.LevelNumber = 4;
        L.Name = "Blood Stream";
        L.Description = "Wide open terrain. The pathogens are fast here.";
        L.EnemyCount = 5;
        L.EnemyBaseStats = MakeStats(110.0f, 12.0f, 6.0f, 13.0f);
        L.GridWidth = 10;
        L.GridHeight = 10;
        L.ObstacleCount = 12;
        L.HealPodCount = 4;
        L.HazardCount = 6;
        L.PlayerUnitCount = 3;
        Levels.push_back(L);
    }

    // Level 5 — Bone Marrow
    {
        FLevelData L;
        L.LevelNumber = 5;
        L.Name = "Bone Marrow";
        L.Description = "Deep in the body's defenses. Healing resources are plentiful.";
        L.EnemyCount = 6;
        L.EnemyBaseStats = MakeStats(130.0f, 14.0f, 8.0f, 13.0f);
        L.GridWidth = 10;
        L.GridHeight = 10;
        L.ObstacleCount = 14;
        L.HealPodCount = 6;
        L.HazardCount = 5;
        L.PlayerUnitCount = 3;
        Levels.push_back(L);
    }

    // Level 6 — Viral Stronghold
    {
        FLevelData L;
        L.LevelNumber = 6;
        L.Name = "Viral Stronghold";
        L.Description = "The heart of the infection. Hazards everywhere.";
        L.EnemyCount = 7;
        L.EnemyBaseStats = MakeStats(150.0f, 16.0f, 10.0f, 14.0f);
        L.GridWidth = 10;
        L.GridHeight = 10;
        L.ObstacleCount = 14;
        L.HealPodCount = 3;
        L.HazardCount = 10;
        L.PlayerUnitCount = 3;
        Levels.push_back(L);
    }

    return Levels;
}

const std::vector<FLevelData>& GetAllLevels()
{
    static std::vector<FLevelData> Levels = BuildLevelTable();
    return Levels;
}

const FLevelData* GetLevel(int LevelNumber)
{
    const auto& Levels = GetAllLevels();
    for (const FLevelData& L : Levels)
    {
        if (L.LevelNumber == LevelNumber)
        {
            return &L;
        }
    }
    return nullptr;
}
