#include "BioGameMode.h"
#include "BioGridManager.h"
#include "BioTypes.h"
#include "BioUnitBase.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <vector>
#include <random>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d11.h>
#include <windows.h>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
    struct FUnitInstance
    {
        BioUnitBase Unit;
        std::string Name;
    };

    struct FGameState
    {
        BioGameMode Mode;
        BioGridManager Grid;
        std::vector<FUnitInstance> Units;
        std::vector<FIntPoint> Obstacles;
        int SelectedUnitIndex = -1;
        int NextUnitId = 1;
        ETurnPhase LastPhase = ETurnPhase::PlayerInput;
        bool EnemyAiExecuted = false;
        std::mt19937 Rng;
        std::uniform_real_distribution<float> Dist01{0.0f, 1.0f};
    };

    static ID3D11Device* GDevice = nullptr;
    static ID3D11DeviceContext* GDeviceContext = nullptr;
    static IDXGISwapChain* GSwapChain = nullptr;
    static ID3D11RenderTargetView* GMainRenderTargetView = nullptr;

    const char* GetPhaseName(ETurnPhase Phase)
    {
        switch (Phase)
        {
        case ETurnPhase::PlayerInput:
            return "Player Input";
        case ETurnPhase::EnemyAI:
            return "Enemy AI";
        case ETurnPhase::Environment:
            return "Environment";
        default:
            return "Unknown";
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

    FIntPoint FindFirstEmptyCell(const FGameState& State)
    {
        for (int32_t Y = 0; Y < State.Grid.GridSize.Y; ++Y)
        {
            for (int32_t X = 0; X < State.Grid.GridSize.X; ++X)
            {
                const FIntPoint Cell(X, Y);
                if (FindUnitAt(State, Cell) < 0 && !IsObstacleAt(State, Cell))
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
                if (FindUnitAt(State, Cell) < 0 && !IsObstacleAt(State, Cell))
                {
                    return Cell;
                }
            }
        }

        return FIntPoint(-1, -1);
    }

    bool IsCellInBounds(const FGameState& State, FIntPoint Cell)
    {
        return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < State.Grid.GridSize.X && Cell.Y < State.Grid.GridSize.Y;
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
    }

    void ResolveEncounters(FGameState& State);

    void AddUnit(FGameState& State, FIntPoint Cell, int TeamId)
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
        }
        else
        {
            UnitInstance.Unit.BaseStats.Set(EBioStat::HitPoints, 80.0f);
            UnitInstance.Unit.BaseStats.Set(EBioStat::Attack, 8.0f);
            UnitInstance.Unit.BaseStats.Set(EBioStat::Defense, 3.0f);
            UnitInstance.Unit.BaseStats.Set(EBioStat::Speed, 9.0f);
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
        if (Cell.X < 0 || Cell.Y < 0)
        {
            return;
        }
        if (FindUnitAt(State, Cell) >= 0)
        {
            return;
        }
        if (IsObstacleAt(State, Cell))
        {
            return;
        }
        State.Obstacles.push_back(Cell);
    }

    void GenerateRandomObstacles(FGameState& State, int32_t Count)
    {
        if (Count <= 0)
        {
            return;
        }

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
            if (!IsCellInBounds(State, Cell))
            {
                return false;
            }
            if (IsObstacleAt(State, Cell))
            {
                return false;
            }
            if (FindUnitAt(State, Cell) >= 0)
            {
                return false;
            }
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
        if (State.Units.empty())
        {
            return;
        }

        for (int EnemyIndex = 0; EnemyIndex < static_cast<int>(State.Units.size()); ++EnemyIndex)
        {
            if (State.Units[EnemyIndex].Unit.TeamID != 2)
            {
                continue;
            }

            int BestTargetIndex = -1;
            int32_t BestDistance = INT32_MAX;
            const FIntPoint EnemyPos = State.Units[EnemyIndex].Unit.GridCoordinates;

            for (int PlayerIndex = 0; PlayerIndex < static_cast<int>(State.Units.size()); ++PlayerIndex)
            {
                if (State.Units[PlayerIndex].Unit.TeamID != 1)
                {
                    continue;
                }

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
        if (Damage <= 0.0f)
        {
            return;
        }

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

        if (!UnitA.IsAlive() || !UnitB.IsAlive())
        {
            return;
        }

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
            if (!UnitInstance.Unit.IsAlive())
            {
                continue;
            }

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
        if (State.Units.empty())
        {
            return;
        }

        const FIntPoint Directions[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };

        for (int Index = 0; Index < static_cast<int>(State.Units.size()); ++Index)
        {
            const BioUnitBase& Unit = State.Units[Index].Unit;
            if (!Unit.IsAlive())
            {
                continue;
            }

            for (const FIntPoint& Dir : Directions)
            {
                const FIntPoint Neighbor(Unit.GridCoordinates.X + Dir.X, Unit.GridCoordinates.Y + Dir.Y);
                const int OtherIndex = FindUnitAt(State, Neighbor);
                if (OtherIndex <= Index)
                {
                    continue;
                }

                if (OtherIndex >= 0 && OtherIndex < static_cast<int>(State.Units.size()))
                {
                    const BioUnitBase& OtherUnit = State.Units[OtherIndex].Unit;
                    if (!OtherUnit.IsAlive())
                    {
                        continue;
                    }
                    if (OtherUnit.TeamID != Unit.TeamID)
                    {
                        ResolveEncounterPair(State, Index, OtherIndex);
                    }
                }
            }
        }

        RemoveDeadUnits(State);
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

    void InitializeGame(FGameState& State)
    {
        State.Grid.GridSize = FIntPoint(8, 8);
        State.Units.clear();
        State.Obstacles.clear();
        State.SelectedUnitIndex = -1;
        State.NextUnitId = 1;
        State.LastPhase = State.Mode.CurrentPhase;
        State.EnemyAiExecuted = false;
        State.Rng.seed(static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

        const int32_t ObstacleCount = (State.Grid.GridSize.X * State.Grid.GridSize.Y) / 6;
        GenerateRandomObstacles(State, ObstacleCount);

        const int32_t MidX = State.Grid.GridSize.X / 2;
        FIntPoint PlayerSpawnA = FindEmptyCellInRange(State, 0, MidX - 1);
        if (PlayerSpawnA.X < 0)
        {
            PlayerSpawnA = FindFirstEmptyCell(State);
        }
        AddUnit(State, PlayerSpawnA, 1);

        FIntPoint PlayerSpawnB = FindEmptyCellInRange(State, 0, MidX - 1);
        if (PlayerSpawnB.X < 0)
        {
            PlayerSpawnB = FindFirstEmptyCell(State);
        }
        AddUnit(State, PlayerSpawnB, 1);

        for (int EnemyIndex = 0; EnemyIndex < 4; ++EnemyIndex)
        {
            FIntPoint EnemySpawn = FindEmptyCellInRange(State, MidX, State.Grid.GridSize.X - 1);
            if (EnemySpawn.X < 0)
            {
                EnemySpawn = FindFirstEmptyCell(State);
            }
            AddUnit(State, EnemySpawn, 2);
        }

        HandlePhaseStart(State);
    }

    void CreateRenderTarget()
    {
        ID3D11Texture2D* BackBuffer = nullptr;
        GSwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer));
        GDevice->CreateRenderTargetView(BackBuffer, nullptr, &GMainRenderTargetView);
        BackBuffer->Release();
    }

    void CleanupRenderTarget()
    {
        if (GMainRenderTargetView)
        {
            GMainRenderTargetView->Release();
            GMainRenderTargetView = nullptr;
        }
    }

    bool CreateDeviceD3D(HWND Hwnd)
    {
        DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
        SwapChainDesc.BufferCount = 2;
        SwapChainDesc.BufferDesc.Width = 0;
        SwapChainDesc.BufferDesc.Height = 0;
        SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        SwapChainDesc.OutputWindow = Hwnd;
        SwapChainDesc.SampleDesc.Count = 1;
        SwapChainDesc.SampleDesc.Quality = 0;
        SwapChainDesc.Windowed = TRUE;
        SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT CreateDeviceFlags = 0;
#if defined(_DEBUG)
        CreateDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_FEATURE_LEVEL FeatureLevel;
        const D3D_FEATURE_LEVEL FeatureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0 };
        const HRESULT Result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            CreateDeviceFlags,
            FeatureLevelArray,
            1,
            D3D11_SDK_VERSION,
            &SwapChainDesc,
            &GSwapChain,
            &GDevice,
            &FeatureLevel,
            &GDeviceContext);

        if (Result != S_OK)
        {
            return false;
        }

        CreateRenderTarget();
        return true;
    }

    void CleanupDeviceD3D()
    {
        CleanupRenderTarget();
        if (GSwapChain)
        {
            GSwapChain->Release();
            GSwapChain = nullptr;
        }
        if (GDeviceContext)
        {
            GDeviceContext->Release();
            GDeviceContext = nullptr;
        }
        if (GDevice)
        {
            GDevice->Release();
            GDevice = nullptr;
        }
    }

    LRESULT WINAPI MainWindowProc(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(Hwnd, Msg, WParam, LParam))
        {
            return true;
        }

        switch (Msg)
        {
        case WM_SIZE:
            if (GDevice != nullptr && WParam != SIZE_MINIMIZED)
            {
                CleanupRenderTarget();
                GSwapChain->ResizeBuffers(0, static_cast<UINT>(LOWORD(LParam)), static_cast<UINT>(HIWORD(LParam)), DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((WParam & 0xfff0) == SC_KEYMENU)
            {
                return 0;
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
        }

        return DefWindowProcA(Hwnd, Msg, WParam, LParam);
    }

    void RenderGameUi(FGameState& State)
    {
        ImGui::Begin("Game");
        ImGui::Text("Phase: %s", GetPhaseName(State.Mode.CurrentPhase));
        ImGui::Text("Turn: %d", State.Mode.TurnCount);
        const int AlivePlayers = CountAliveTeam(State, 1);
        const int AliveEnemies = CountAliveTeam(State, 2);
        const bool GameOver = (AlivePlayers == 0 || AliveEnemies == 0);
        ImGui::Text("Players: %d  Enemies: %d", AlivePlayers, AliveEnemies);
        if (AlivePlayers == 0)
        {
            ImGui::TextColored(ImVec4(0.85f, 0.25f, 0.25f, 1.0f), "Defeat");
        }
        else if (AliveEnemies == 0)
        {
            ImGui::TextColored(ImVec4(0.25f, 0.85f, 0.35f, 1.0f), "Victory");
        }

        if (State.Mode.CurrentPhase != State.LastPhase)
        {
            State.LastPhase = State.Mode.CurrentPhase;
            State.EnemyAiExecuted = false;
            HandlePhaseStart(State);
        }

        if (!GameOver && ImGui::Button("Advance Phase"))
        {
            State.Mode.AdvanceTurn();
        }
        ImGui::SameLine();
        if (!GameOver && ImGui::Button("End Player Turn"))
        {
            State.Mode.EndPlayerTurn();
        }

        if (State.SelectedUnitIndex >= 0 && State.SelectedUnitIndex < static_cast<int>(State.Units.size()))
        {
            const FUnitInstance& Selected = State.Units[State.SelectedUnitIndex];
            ImGui::Text("Selected: %s", Selected.Name.c_str());
            const int32_t MoveRange = GetMoveRange(Selected.Unit);
            ImGui::Text("Move Range: %d", MoveRange);
            ImGui::Text("Move Remaining: %d", Selected.Unit.RemainingMovePoints);
            ImGui::Text("Move: click an empty cell within range.");
        }
        else
        {
            ImGui::Text("Select a unit to move it.");
        }

        if (!GameOver && ImGui::Button("Add Unit"))
        {
            const FIntPoint EmptyCell = FindFirstEmptyCell(State);
            if (EmptyCell.X >= 0)
            {
                AddUnit(State, EmptyCell, 1);
                ResolveEncounters(State);
            }
        }

        ImGui::SameLine();
        if (!GameOver && ImGui::Button("Add Enemy Unit"))
        {
            const FIntPoint EmptyCell = FindFirstEmptyCell(State);
            if (EmptyCell.X >= 0)
            {
                AddUnit(State, EmptyCell, 2);
                ResolveEncounters(State);
            }
        }

        ImGui::SameLine();
        if (!GameOver && ImGui::Button("Randomize Obstacles"))
        {
            State.Obstacles.clear();
            const int32_t ObstacleCount = (State.Grid.GridSize.X * State.Grid.GridSize.Y) / 6;
            GenerateRandomObstacles(State, ObstacleCount);
        }

        if (!GameOver && State.Mode.CurrentPhase == ETurnPhase::EnemyAI)
        {
            if (!State.EnemyAiExecuted)
            {
                RunEnemyAiStep(State);
                State.EnemyAiExecuted = true;
            }

            if (ImGui::Button("Run Enemy AI Step"))
            {
                RunEnemyAiStep(State);
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Reset Game"))
        {
            InitializeGame(State);
        }
        ImGui::End();

        ImGui::Begin("Grid");
        const float CellSize = 40.0f;
        for (int32_t Y = 0; Y < State.Grid.GridSize.Y; ++Y)
        {
            for (int32_t X = 0; X < State.Grid.GridSize.X; ++X)
            {
                const FIntPoint Cell(X, Y);
                const int UnitIndex = FindUnitAt(State, Cell);
                const bool HasUnit = UnitIndex >= 0;
                const bool IsSelected = (UnitIndex == State.SelectedUnitIndex);
                const bool IsObstacle = IsObstacleAt(State, Cell);

                ImVec4 BaseColor = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
                if (IsObstacle)
                {
                    BaseColor = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
                }
                else if (HasUnit)
                {
                    const int TeamId = State.Units[UnitIndex].Unit.TeamID;
                    BaseColor = (TeamId == 2) ? ImVec4(0.65f, 0.20f, 0.22f, 1.0f) : ImVec4(0.20f, 0.45f, 0.75f, 1.0f);
                }
                if (IsSelected)
                {
                    BaseColor = ImVec4(0.85f, 0.70f, 0.25f, 1.0f);
                }

                ImGui::PushStyleColor(ImGuiCol_Button, BaseColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(BaseColor.x + 0.1f, BaseColor.y + 0.1f, BaseColor.z + 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, BaseColor);

                ImGui::PushID(Y * State.Grid.GridSize.X + X);
                char Label[4] = " ";
                if (IsObstacle)
                {
                    Label[0] = '#';
                    Label[1] = '\0';
                }
                else if (HasUnit && !State.Units[UnitIndex].Name.empty())
                {
                    Label[0] = State.Units[UnitIndex].Name[0];
                    Label[1] = '\0';
                }

                if (ImGui::Button(Label, ImVec2(CellSize, CellSize)))
                {
                    if (HasUnit)
                    {
                        State.SelectedUnitIndex = UnitIndex;
                    }
                    else if (!GameOver && !IsObstacle && State.SelectedUnitIndex >= 0)
                    {
                        BioUnitBase& SelectedUnit = State.Units[State.SelectedUnitIndex].Unit;
                        if (SelectedUnit.TeamID == 1 && State.Mode.CurrentPhase == ETurnPhase::PlayerInput)
                        {
                            const int32_t Range = GetMoveRange(SelectedUnit);
                            const int32_t Dist = State.Grid.GetManhattanDistance(SelectedUnit.GridCoordinates, Cell);
                            if (Dist <= Range && Dist <= SelectedUnit.RemainingMovePoints)
                            {
                                SelectedUnit.MoveToGrid(Cell);
                                SelectedUnit.ConsumeMovePoints(Dist);
                                ResolveEncounters(State);
                            }
                        }
                    }
                }
                ImGui::PopID();

                ImGui::PopStyleColor(3);

                if (X < State.Grid.GridSize.X - 1)
                {
                    ImGui::SameLine();
                }
            }
        }
        ImGui::End();

        ImGui::Begin("Selected Unit");
        if (State.SelectedUnitIndex >= 0 && State.SelectedUnitIndex < static_cast<int>(State.Units.size()))
        {
            FUnitInstance& Selected = State.Units[State.SelectedUnitIndex];
            BioUnitBase& Unit = Selected.Unit;

            ImGui::Text("Name: %s", Selected.Name.c_str());
            ImGui::Text("Team: %d", Unit.TeamID);
            ImGui::Text("Grid: (%d, %d)", Unit.GridCoordinates.X, Unit.GridCoordinates.Y);

            float MaxHealth = Unit.BaseStats.Get(EBioStat::HitPoints);
            if (ImGui::SliderFloat("Max HP", &MaxHealth, 1.0f, 300.0f, "%.0f"))
            {
                Unit.BaseStats.Set(EBioStat::HitPoints, MaxHealth);
                Unit.ClampHitPointsToMax();
            }

            float CurrentHealth = Unit.CurrentHitPoints;
            const float MaxHealthForSlider = std::max(1.0f, Unit.BaseStats.Get(EBioStat::HitPoints));
            if (ImGui::SliderFloat("Current HP", &CurrentHealth, 0.0f, MaxHealthForSlider, "%.0f"))
            {
                Unit.CurrentHitPoints = std::clamp(CurrentHealth, 0.0f, MaxHealthForSlider);
            }

            float Attack = Unit.BaseStats.Get(EBioStat::Attack);
            if (ImGui::SliderFloat("Attack", &Attack, 0.0f, 100.0f, "%.0f"))
            {
                Unit.BaseStats.Set(EBioStat::Attack, Attack);
            }

            float Defense = Unit.BaseStats.Get(EBioStat::Defense);
            if (ImGui::SliderFloat("Defense", &Defense, 0.0f, 100.0f, "%.0f"))
            {
                Unit.BaseStats.Set(EBioStat::Defense, Defense);
            }

            float Speed = Unit.BaseStats.Get(EBioStat::Speed);
            if (ImGui::SliderFloat("Speed", &Speed, 0.0f, 100.0f, "%.0f"))
            {
                Unit.BaseStats.Set(EBioStat::Speed, Speed);
                const int32_t NewRange = GetMoveRange(Unit);
                Unit.RemainingMovePoints = std::min(Unit.RemainingMovePoints, NewRange);
            }

            if (ImGui::Button("Take 10 Damage"))
            {
                Unit.TakeDamage(10);
            }
            ImGui::SameLine();
            if (ImGui::Button("Heal 10"))
            {
                const float MaxHitPoints = Unit.GetEffectiveStats().Get(EBioStat::HitPoints);
                Unit.CurrentHitPoints = std::clamp(Unit.CurrentHitPoints + 10.0f, 0.0f, MaxHitPoints);
            }

            if (ImGui::Button("Remove Unit"))
            {
                State.Units.erase(State.Units.begin() + State.SelectedUnitIndex);
                State.SelectedUnitIndex = -1;
            }

            const FStatBlock Effective = Unit.GetEffectiveStats();
            const float EffectiveHp = Effective.Get(EBioStat::HitPoints);
            const float EffectiveAttack = Effective.Get(EBioStat::Attack);
            const float EffectiveDefense = Effective.Get(EBioStat::Defense);
            const float EffectiveSpeed = Effective.Get(EBioStat::Speed);
            const int32_t EffectiveMoveRange = GetMoveRange(Unit);
            const float DodgeChance = BioStatMath::ComputeDodgeChance(EffectiveSpeed) * 100.0f;
            const float Mitigation = BioStatMath::ComputeDefenseMitigation(EffectiveDefense) * 100.0f;
            ImGui::Separator();
            ImGui::Text("Effective Stats");
            ImGui::Text("  Max HP: %.0f", EffectiveHp);
            ImGui::Text("  Current HP: %.0f", Unit.CurrentHitPoints);
            ImGui::Text("  Attack: %.0f", EffectiveAttack);
            ImGui::Text("  Defense: %.0f (Mitigation %.0f%%)", EffectiveDefense, Mitigation);
            ImGui::Text("  Speed: %.0f (Move %d, Dodge %.0f%%)", EffectiveSpeed, EffectiveMoveRange, DodgeChance);
        }
        else
        {
            ImGui::Text("Select a unit to edit stats.");
        }
        ImGui::End();

        ImGui::Begin("All Unit Stats");
        if (ImGui::BeginTable("UnitStatsTable", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Team");
            ImGui::TableSetupColumn("HP");
            ImGui::TableSetupColumn("ATK");
            ImGui::TableSetupColumn("DEF");
            ImGui::TableSetupColumn("SPD");
            ImGui::TableSetupColumn("Move");
            ImGui::TableSetupColumn("Remain");
            ImGui::TableSetupColumn("Pos");
            ImGui::TableHeadersRow();

            for (const FUnitInstance& UnitInstance : State.Units)
            {
                const BioUnitBase& Unit = UnitInstance.Unit;
                const FStatBlock Stats = Unit.GetEffectiveStats();
                const float Hp = Unit.CurrentHitPoints;
                const float MaxHp = Stats.Get(EBioStat::HitPoints);
                const int32_t MoveRange = GetMoveRange(Unit);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", UnitInstance.Name.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%d", Unit.TeamID);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.0f/%.0f", Hp, MaxHp);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.0f", Stats.Get(EBioStat::Attack));
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%.0f", Stats.Get(EBioStat::Defense));
                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%.0f", Stats.Get(EBioStat::Speed));
                ImGui::TableSetColumnIndex(6);
                ImGui::Text("%d", MoveRange);
                ImGui::TableSetColumnIndex(7);
                ImGui::Text("%d", Unit.RemainingMovePoints);
                ImGui::TableSetColumnIndex(8);
                ImGui::Text("(%d,%d)", Unit.GridCoordinates.X, Unit.GridCoordinates.Y);
            }
            ImGui::EndTable();
        }
        ImGui::End();

        ImGui::Begin("Units");
        for (int Index = 0; Index < static_cast<int>(State.Units.size()); ++Index)
        {
            const FUnitInstance& Unit = State.Units[Index];
            const bool Selected = (Index == State.SelectedUnitIndex);
            const float MaxHp = Unit.Unit.GetEffectiveStats().Get(EBioStat::HitPoints);
            const float CurrentHp = Unit.Unit.CurrentHitPoints;
            std::string Label = Unit.Name + " [T" + std::to_string(Unit.Unit.TeamID) + "] (" +
                std::to_string(Unit.Unit.GridCoordinates.X) + ", " + std::to_string(Unit.Unit.GridCoordinates.Y) +
                ") HP " + std::to_string(static_cast<int>(CurrentHp)) + "/" + std::to_string(static_cast<int>(MaxHp)) +
                " MP " + std::to_string(Unit.Unit.RemainingMovePoints);
            if (ImGui::Selectable(Label.c_str(), Selected))
            {
                State.SelectedUnitIndex = Index;
            }
        }
        ImGui::End();
    }
}

int main()
{
    WNDCLASSEXA WindowClass = {};
    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.style = CS_CLASSDC;
    WindowClass.lpfnWndProc = MainWindowProc;
    WindowClass.hInstance = GetModuleHandleA(nullptr);
    WindowClass.lpszClassName = "BioStandaloneWindow";
    RegisterClassExA(&WindowClass);

    HWND Window = CreateWindowA(
        WindowClass.lpszClassName,
        "Bio Standalone (ImGui + DX11)",
        WS_OVERLAPPEDWINDOW,
        100,
        100,
        1280,
        720,
        nullptr,
        nullptr,
        WindowClass.hInstance,
        nullptr);

    if (!CreateDeviceD3D(Window))
    {
        CleanupDeviceD3D();
        UnregisterClassA(WindowClass.lpszClassName, WindowClass.hInstance);
        return 1;
    }

    ShowWindow(Window, SW_SHOWDEFAULT);
    UpdateWindow(Window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& IO = ImGui::GetIO();
    IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(Window);
    ImGui_ImplDX11_Init(GDevice, GDeviceContext);

    FGameState GameState;
    InitializeGame(GameState);

    bool Running = true;
    while (Running)
    {
        MSG Message;
        while (PeekMessageA(&Message, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&Message);
            DispatchMessageA(&Message);
            if (Message.message == WM_QUIT)
            {
                Running = false;
            }
        }
        if (!Running)
        {
            break;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderGameUi(GameState);

        ImGui::Render();
        const float ClearColor[4] = { 0.08f, 0.08f, 0.10f, 1.0f };
        GDeviceContext->OMSetRenderTargets(1, &GMainRenderTargetView, nullptr);
        GDeviceContext->ClearRenderTargetView(GMainRenderTargetView, ClearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        GSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(Window);
    UnregisterClassA(WindowClass.lpszClassName, WindowClass.hInstance);

    return 0;
}
