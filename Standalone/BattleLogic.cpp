#include "BattleLogic.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace Battle
{
    const char* GetPhaseName(ETurnPhase Phase)
    {
        switch (Phase)
        {
        case ETurnPhase::PlayerInput: return "Player Input";
        case ETurnPhase::EnemyAI:    return "Enemy AI";
        case ETurnPhase::Environment: return "Environment";
        default:                      return "Unknown";
        }
    }

    int FindUnitAt(const FGameState& State, FIntPoint Cell)
    {
        for (size_t Index = 0; Index < State.Units.size(); ++Index)
        {
            const BioUnitBase& Unit = State.Units[Index].Unit;
            if (Unit.GridCoordinates.X == Cell.X && Unit.GridCoordinates.Y == Cell.Y)
            {
                return static_cast<int>(Index);
            }
        }
        return -1;
    }

    bool IsObstacleAt(const FGameState& State, FIntPoint Cell)
    {
        for (const FIntPoint& Obstacle : State.Obstacles)
        {
            if (Obstacle.X == Cell.X && Obstacle.Y == Cell.Y)
            {
                return true;
            }
        }
        return false;
    }

    bool IsHealingPodAt(const FGameState& State, FIntPoint Cell)
    {
        for (const FIntPoint& Pod : State.HealingPods)
        {
            if (Pod.X == Cell.X && Pod.Y == Cell.Y)
            {
                return true;
            }
        }
        return false;
    }

    bool IsHazardAt(const FGameState& State, FIntPoint Cell)
    {
        for (const FIntPoint& Hazard : State.HazardTiles)
        {
            if (Hazard.X == Cell.X && Hazard.Y == Cell.Y)
            {
                return true;
            }
        }
        return false;
    }

    bool IsEnvironmentAt(const FGameState& State, FIntPoint Cell)
    {
        return IsHealingPodAt(State, Cell) || IsHazardAt(State, Cell);
    }

    bool IsCellInBounds(const FGameState& State, FIntPoint Cell)
    {
        return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < State.Grid.GridSize.X && Cell.Y < State.Grid.GridSize.Y;
    }

    int CountAliveTeam(const FGameState& State, int TeamId)
    {
        int Count = 0;
        for (const FUnitInstance& UnitInstance : State.Units)
        {
            if (UnitInstance.Unit.TeamID == TeamId && UnitInstance.Unit.IsAlive())
            {
                ++Count;
            }
        }
        return Count;
    }

    FIntPoint FindFirstEmptyCell(const FGameState& State)
    {
        for (int32_t Y = 0; Y < State.Grid.GridSize.Y; ++Y)
        {
            for (int32_t X = 0; X < State.Grid.GridSize.X; ++X)
            {
                const FIntPoint Cell(X, Y);
                if (FindUnitAt(State, Cell) < 0 && !IsObstacleAt(State, Cell) && !IsEnvironmentAt(State, Cell))
                {
                    return Cell;
                }
            }
        }
        return FIntPoint(-1, -1);
    }

    FIntPoint FindEmptyCellInRange(const FGameState& State, int32_t MinX, int32_t MaxX)
    {
        MinX = std::max(0, MinX);
        MaxX = std::min(State.Grid.GridSize.X - 1, MaxX);

        for (int32_t Y = 0; Y < State.Grid.GridSize.Y; ++Y)
        {
            for (int32_t X = MinX; X <= MaxX; ++X)
            {
                const FIntPoint Cell(X, Y);
                if (FindUnitAt(State, Cell) < 0 && !IsObstacleAt(State, Cell) && !IsEnvironmentAt(State, Cell))
                {
                    return Cell;
                }
            }
        }
        return FIntPoint(-1, -1);
    }

    FIntPoint FindEmptyCellInRect(const FGameState& State, int32_t MinX, int32_t MaxX, int32_t MinY, int32_t MaxY)
    {
        MinX = std::max(0, MinX);
        MinY = std::max(0, MinY);
        MaxX = std::min(State.Grid.GridSize.X - 1, MaxX);
        MaxY = std::min(State.Grid.GridSize.Y - 1, MaxY);

        for (int32_t Y = MinY; Y <= MaxY; ++Y)
        {
            for (int32_t X = MinX; X <= MaxX; ++X)
            {
                const FIntPoint Cell(X, Y);
                if (FindUnitAt(State, Cell) < 0 && !IsObstacleAt(State, Cell) && !IsEnvironmentAt(State, Cell))
                {
                    return Cell;
                }
            }
        }
        return FIntPoint(-1, -1);
    }

    float GetStatValue(const BioUnitBase& Unit, EBioStat Stat)
    {
        return Unit.GetEffectiveStats().Get(Stat);
    }

    int32_t GetMoveRange(const BioUnitBase& Unit)
    {
        const float Speed = GetStatValue(Unit, EBioStat::Speed);
        const float RangeFloat = BioStatMath::ComputeMoveRange(Speed);
        const int32_t Range = static_cast<int32_t>(std::floor(RangeFloat));
        return std::max(1, Range);
    }

    void ResetMovementForTeam(FGameState& State, int TeamId)
    {
        for (FUnitInstance& UnitInstance : State.Units)
        {
            if (UnitInstance.Unit.TeamID != TeamId)
            {
                continue;
            }
            const int32_t Range = GetMoveRange(UnitInstance.Unit);
            UnitInstance.Unit.ResetMovePoints(Range);
        }
    }

    void HandlePhaseStart(FGameState& State)
    {
        if (State.Mode.CurrentPhase == ETurnPhase::PlayerInput)
        {
            ResetMovementForTeam(State, 1);
        }
        else if (State.Mode.CurrentPhase == ETurnPhase::EnemyAI)
        {
            ResetMovementForTeam(State, 2);
        }
        else if (State.Mode.CurrentPhase == ETurnPhase::Environment)
        {
            ApplyEnvironmentEffects(State);
        }
    }

    void AddUnit(FGameState& State, FIntPoint Cell, int TeamId, const FStatBlock* EnemyStats)
    {
        FUnitInstance UnitInstance;
        UnitInstance.Name = "Unit " + std::to_string(State.NextUnitId++);
        UnitInstance.Unit.TeamID = TeamId;
        UnitInstance.Unit.GridCoordinates = Cell;
        if (TeamId == 1)
        {
            UnitInstance.Unit.BaseStats.Set(EBioStat::HitPoints, 140.0f);
            UnitInstance.Unit.BaseStats.Set(EBioStat::Attack, 16.0f);
            UnitInstance.Unit.BaseStats.Set(EBioStat::Defense, 8.0f);
            UnitInstance.Unit.BaseStats.Set(EBioStat::Speed, 12.0f);
            UnitInstance.Unit.BaseStats.AddBlock(State.Profile.SkillTree.ComputeTotalBonus());
            UnitInstance.Unit.BaseStats.AddBlock(State.Profile.ComputeStatAllocationBonus());
        }
        else
        {
            if (EnemyStats)
            {
                UnitInstance.Unit.BaseStats = *EnemyStats;
            }
            else
            {
                UnitInstance.Unit.BaseStats.Set(EBioStat::HitPoints, 80.0f);
                UnitInstance.Unit.BaseStats.Set(EBioStat::Attack, 8.0f);
                UnitInstance.Unit.BaseStats.Set(EBioStat::Defense, 3.0f);
                UnitInstance.Unit.BaseStats.Set(EBioStat::Speed, 9.0f);
            }
        }
        UnitInstance.Unit.InitializeHitPoints();
        if ((TeamId == 1 && State.Mode.CurrentPhase == ETurnPhase::PlayerInput) ||
            (TeamId == 2 && State.Mode.CurrentPhase == ETurnPhase::EnemyAI))
        {
            UnitInstance.Unit.ResetMovePoints(GetMoveRange(UnitInstance.Unit));
        }
        State.Units.push_back(UnitInstance);
    }

    void AddObstacle(FGameState& State, FIntPoint Cell)
    {
        if (Cell.X < 0 || Cell.Y < 0) return;
        if (FindUnitAt(State, Cell) >= 0) return;
        if (IsObstacleAt(State, Cell)) return;
        if (IsEnvironmentAt(State, Cell)) return;
        State.Obstacles.push_back(Cell);
    }

    void AddHealingPod(FGameState& State, FIntPoint Cell)
    {
        if (Cell.X < 0 || Cell.Y < 0) return;
        if (FindUnitAt(State, Cell) >= 0) return;
        if (IsObstacleAt(State, Cell) || IsEnvironmentAt(State, Cell)) return;
        State.HealingPods.push_back(Cell);
    }

    void AddHazard(FGameState& State, FIntPoint Cell)
    {
        if (Cell.X < 0 || Cell.Y < 0) return;
        if (FindUnitAt(State, Cell) >= 0) return;
        if (IsObstacleAt(State, Cell) || IsEnvironmentAt(State, Cell)) return;
        State.HazardTiles.push_back(Cell);
    }

    void GenerateRandomObstacles(FGameState& State, int32_t Count)
    {
        if (Count <= 0) return;

        std::uniform_int_distribution<int32_t> DistX(0, State.Grid.GridSize.X - 1);
        std::uniform_int_distribution<int32_t> DistY(0, State.Grid.GridSize.Y - 1);

        int32_t Attempts = 0;
        const int32_t MaxAttempts = Count * 20;
        while (static_cast<int32_t>(State.Obstacles.size()) < Count && Attempts < MaxAttempts)
        {
            const FIntPoint Cell(DistX(State.Rng), DistY(State.Rng));
            AddObstacle(State, Cell);
            ++Attempts;
        }
    }

    void GenerateRandomEnvironment(FGameState& State, int32_t HealCount, int32_t HazardCount)
    {
        State.HealingPods.clear();
        State.HazardTiles.clear();

        std::uniform_int_distribution<int32_t> DistX(0, State.Grid.GridSize.X - 1);
        std::uniform_int_distribution<int32_t> DistY(0, State.Grid.GridSize.Y - 1);

        auto PlaceTiles = [&](int32_t Count, bool IsHeal)
        {
            int32_t Attempts = 0;
            const int32_t MaxAttempts = Count * 25;
            while (Attempts < MaxAttempts)
            {
                if (IsHeal && static_cast<int32_t>(State.HealingPods.size()) >= Count) break;
                if (!IsHeal && static_cast<int32_t>(State.HazardTiles.size()) >= Count) break;

                const FIntPoint Cell(DistX(State.Rng), DistY(State.Rng));
                if (IsHeal)
                {
                    AddHealingPod(State, Cell);
                }
                else
                {
                    AddHazard(State, Cell);
                }
                ++Attempts;
            }
        };

        if (HealCount > 0) PlaceTiles(HealCount, true);
        if (HazardCount > 0) PlaceTiles(HazardCount, false);
    }

    static bool IsAdjacentToAny(const FGameState& State, FIntPoint Position, const std::vector<FIntPoint>& Tiles)
    {
        const FIntPoint Directions[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
        for (const FIntPoint& Dir : Directions)
        {
            const FIntPoint Neighbor(Position.X + Dir.X, Position.Y + Dir.Y);
            for (const FIntPoint& Tile : Tiles)
            {
                if (Tile.X == Neighbor.X && Tile.Y == Neighbor.Y)
                {
                    return true;
                }
            }
        }
        return false;
    }

    void ApplyEnvironmentEffects(FGameState& State)
    {
        if (State.Units.empty()) return;

        for (FUnitInstance& UnitInstance : State.Units)
        {
            BioUnitBase& Unit = UnitInstance.Unit;
            if (!Unit.IsAlive()) continue;

            const FStatBlock Stats = Unit.GetEffectiveStats();
            const float MaxHp = Stats.Get(EBioStat::HitPoints);

            const bool AdjacentToHeal = (Unit.TeamID == 1) && IsAdjacentToAny(State, Unit.GridCoordinates, State.HealingPods);
            const bool AdjacentToHazard = IsAdjacentToAny(State, Unit.GridCoordinates, State.HazardTiles);

            if (AdjacentToHazard)
            {
                const float Damage = MaxHp * 0.10f;
                ApplyDamage(Unit, Damage);
            }

            if (AdjacentToHeal)
            {
                const float Heal = MaxHp * 0.25f;
                Unit.CurrentHitPoints = std::clamp(Unit.CurrentHitPoints + Heal, 0.0f, MaxHp);
            }
        }

        RemoveDeadUnits(State);
    }

    float ComputeDamageAfterDefense(float Attack, float Defense)
    {
        const float Mitigation = BioStatMath::ComputeDefenseMitigation(Defense);
        float Damage = std::max(0.0f, Attack) * (1.0f - Mitigation);
        if (Damage < 1.0f && Attack > 0.0f)
        {
            Damage = 1.0f;
        }
        return Damage;
    }

    void ApplyDamage(BioUnitBase& Unit, float Damage)
    {
        if (Damage <= 0.0f) return;
        const float MaxHitPoints = Unit.GetEffectiveStats().Get(EBioStat::HitPoints);
        Unit.CurrentHitPoints = std::clamp(Unit.CurrentHitPoints - Damage, 0.0f, MaxHitPoints);
    }

    void ResolveEncounterPair(FGameState& State, int IndexA, int IndexB)
    {
        if (IndexA < 0 || IndexB < 0 || IndexA >= static_cast<int>(State.Units.size()) || IndexB >= static_cast<int>(State.Units.size()))
        {
            return;
        }

        BioUnitBase& UnitA = State.Units[IndexA].Unit;
        BioUnitBase& UnitB = State.Units[IndexB].Unit;

        if (!UnitA.IsAlive() || !UnitB.IsAlive()) return;

        const FStatBlock StatsA = UnitA.GetEffectiveStats();
        const FStatBlock StatsB = UnitB.GetEffectiveStats();

        const float SpeedA = StatsA.Get(EBioStat::Speed);
        const float SpeedB = StatsB.Get(EBioStat::Speed);
        const float AttackA = StatsA.Get(EBioStat::Attack);
        const float AttackB = StatsB.Get(EBioStat::Attack);
        const float DefenseA = StatsA.Get(EBioStat::Defense);
        const float DefenseB = StatsB.Get(EBioStat::Defense);

        const float DodgeChanceA = BioStatMath::ComputeDodgeChance(SpeedA);
        const float DodgeChanceB = BioStatMath::ComputeDodgeChance(SpeedB);

        const bool ADodged = State.Dist01(State.Rng) < DodgeChanceA;
        const bool BDodged = State.Dist01(State.Rng) < DodgeChanceB;

        const float DamageToA = ADodged ? 0.0f : ComputeDamageAfterDefense(AttackB, DefenseA);
        const float DamageToB = BDodged ? 0.0f : ComputeDamageAfterDefense(AttackA, DefenseB);

        ApplyDamage(UnitA, DamageToA);
        ApplyDamage(UnitB, DamageToB);
    }

    void RemoveDeadUnits(FGameState& State)
    {
        std::vector<FUnitInstance> Survivors;
        Survivors.reserve(State.Units.size());

        int NewSelectedIndex = -1;
        for (int Index = 0; Index < static_cast<int>(State.Units.size()); ++Index)
        {
            FUnitInstance& UnitInstance = State.Units[Index];
            if (!UnitInstance.Unit.IsAlive()) continue;

            if (Index == State.SelectedUnitIndex)
            {
                NewSelectedIndex = static_cast<int>(Survivors.size());
            }
            Survivors.push_back(UnitInstance);
        }

        State.Units.swap(Survivors);
        State.SelectedUnitIndex = NewSelectedIndex;
    }

    void ResolveEncounters(FGameState& State)
    {
        if (State.Units.empty()) return;

        const FIntPoint Directions[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };

        for (int Index = 0; Index < static_cast<int>(State.Units.size()); ++Index)
        {
            const BioUnitBase& Unit = State.Units[Index].Unit;
            if (!Unit.IsAlive()) continue;

            for (const FIntPoint& Dir : Directions)
            {
                const FIntPoint Neighbor(Unit.GridCoordinates.X + Dir.X, Unit.GridCoordinates.Y + Dir.Y);
                const int OtherIndex = FindUnitAt(State, Neighbor);
                if (OtherIndex <= Index) continue;

                if (OtherIndex >= 0 && OtherIndex < static_cast<int>(State.Units.size()))
                {
                    const BioUnitBase& OtherUnit = State.Units[OtherIndex].Unit;
                    if (!OtherUnit.IsAlive()) continue;
                    if (OtherUnit.TeamID != Unit.TeamID)
                    {
                        ResolveEncounterPair(State, Index, OtherIndex);
                    }
                }
            }
        }

        RemoveDeadUnits(State);
    }

    bool MoveEnemyUnitTowards(FGameState& State, int UnitIndex, FIntPoint Target)
    {
        if (UnitIndex < 0 || UnitIndex >= static_cast<int>(State.Units.size()))
        {
            return false;
        }

        BioUnitBase& Enemy = State.Units[UnitIndex].Unit;
        const FIntPoint Current = Enemy.GridCoordinates;

        const int32_t Dx = Target.X - Current.X;
        const int32_t Dy = Target.Y - Current.Y;

        FIntPoint StepPrimary = Current;
        FIntPoint StepSecondary = Current;

        if (std::abs(Dx) >= std::abs(Dy))
        {
            StepPrimary.X += (Dx > 0) ? 1 : (Dx < 0 ? -1 : 0);
            StepSecondary.Y += (Dy > 0) ? 1 : (Dy < 0 ? -1 : 0);
        }
        else
        {
            StepPrimary.Y += (Dy > 0) ? 1 : (Dy < 0 ? -1 : 0);
            StepSecondary.X += (Dx > 0) ? 1 : (Dx < 0 ? -1 : 0);
        }

        auto CanMoveTo = [&State](FIntPoint Cell)
        {
            if (!IsCellInBounds(State, Cell)) return false;
            if (IsObstacleAt(State, Cell)) return false;
            if (FindUnitAt(State, Cell) >= 0) return false;
            return true;
        };

        if (CanMoveTo(StepPrimary) && (StepPrimary.X != Current.X || StepPrimary.Y != Current.Y))
        {
            Enemy.MoveToGrid(StepPrimary);
            return true;
        }

        if (CanMoveTo(StepSecondary) && (StepSecondary.X != Current.X || StepSecondary.Y != Current.Y))
        {
            Enemy.MoveToGrid(StepSecondary);
            return true;
        }

        return false;
    }

    void RunEnemyAiStep(FGameState& State)
    {
        if (State.Units.empty()) return;

        for (int EnemyIndex = 0; EnemyIndex < static_cast<int>(State.Units.size()); ++EnemyIndex)
        {
            if (State.Units[EnemyIndex].Unit.TeamID != 2) continue;

            int BestTargetIndex = -1;
            int32_t BestDistance = INT32_MAX;
            const FIntPoint EnemyPos = State.Units[EnemyIndex].Unit.GridCoordinates;

            for (int PlayerIndex = 0; PlayerIndex < static_cast<int>(State.Units.size()); ++PlayerIndex)
            {
                if (State.Units[PlayerIndex].Unit.TeamID != 1) continue;

                const FIntPoint PlayerPos = State.Units[PlayerIndex].Unit.GridCoordinates;
                const int32_t Dist = std::abs(PlayerPos.X - EnemyPos.X) + std::abs(PlayerPos.Y - EnemyPos.Y);
                if (Dist < BestDistance)
                {
                    BestDistance = Dist;
                    BestTargetIndex = PlayerIndex;
                }
            }

            if (BestTargetIndex >= 0)
            {
                const int32_t Steps = State.Units[EnemyIndex].Unit.RemainingMovePoints;
                for (int32_t Step = 0; Step < Steps; ++Step)
                {
                    if (!MoveEnemyUnitTowards(State, EnemyIndex, State.Units[BestTargetIndex].Unit.GridCoordinates))
                    {
                        break;
                    }
                    State.Units[EnemyIndex].Unit.ConsumeMovePoints(1);
                }
            }
        }

        ResolveEncounters(State);
    }

    void InitializeBattle(FGameState& State, const FLevelData& Level)
    {
        State.Grid.GridSize = FIntPoint(Level.GridWidth, Level.GridHeight);
        State.Units.clear();
        State.Obstacles.clear();
        State.HealingPods.clear();
        State.HazardTiles.clear();
        State.SelectedUnitIndex = -1;
        State.NextUnitId = 1;
        State.Mode.CurrentPhase = ETurnPhase::PlayerInput;
        State.Mode.TurnCount = 1;
        State.LastPhase = State.Mode.CurrentPhase;
        State.EnemyAiExecuted = false;
        State.VictoryPointAwarded = false;
        State.BattleWon = false;
        State.AutoTurnState = EAutoTurnState::Idle;
        State.AutoTurnTimer = 0.0f;
        State.CameraYaw = 0.785398f;
        State.CameraPitch = 0.61548f;
        State.CameraZoom = 1.0f;
        State.CameraPan = ImVec2(0.0f, 0.0f);
        State.Rng.seed(static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

        GenerateRandomObstacles(State, Level.ObstacleCount);
        GenerateRandomEnvironment(State, Level.HealPodCount, Level.HazardCount);

        const int32_t CornerSize = std::min(3, std::min(Level.GridWidth, Level.GridHeight));
        const int32_t PlayerMinX = 0;
        const int32_t PlayerMaxX = CornerSize - 1;
        const int32_t PlayerMinY = 0;
        const int32_t PlayerMaxY = CornerSize - 1;
        const int32_t EnemyMinX = Level.GridWidth - CornerSize;
        const int32_t EnemyMaxX = Level.GridWidth - 1;
        const int32_t EnemyMinY = Level.GridHeight - CornerSize;
        const int32_t EnemyMaxY = Level.GridHeight - 1;

        for (int I = 0; I < Level.PlayerUnitCount; ++I)
        {
            FIntPoint Spawn = FindEmptyCellInRect(State, PlayerMinX, PlayerMaxX, PlayerMinY, PlayerMaxY);
            if (Spawn.X < 0) Spawn = FindFirstEmptyCell(State);
            AddUnit(State, Spawn, 1);
        }

        for (int I = 0; I < Level.EnemyCount; ++I)
        {
            FIntPoint Spawn = FindEmptyCellInRect(State, EnemyMinX, EnemyMaxX, EnemyMinY, EnemyMaxY);
            if (Spawn.X < 0) Spawn = FindFirstEmptyCell(State);
            AddUnit(State, Spawn, 2, &Level.EnemyBaseStats);
        }

        HandlePhaseStart(State);
    }
}
