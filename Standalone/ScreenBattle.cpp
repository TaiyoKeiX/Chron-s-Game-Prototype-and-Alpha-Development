#include "ScreenBattle.h"
#include "GameState.h"
#include "BattleLogic.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <string>

// Geometry helpers for isometric tile hit-testing
static float Cross2D(ImVec2 A, ImVec2 B, ImVec2 C)
{
    return (B.x - A.x) * (C.y - A.y) - (B.y - A.y) * (C.x - A.x);
}

static bool IsPointInTriangle(ImVec2 P, ImVec2 A, ImVec2 B, ImVec2 C)
{
    const float C1 = Cross2D(A, B, P);
    const float C2 = Cross2D(B, C, P);
    const float C3 = Cross2D(C, A, P);
    const bool HasNeg = (C1 < 0.0f) || (C2 < 0.0f) || (C3 < 0.0f);
    const bool HasPos = (C1 > 0.0f) || (C2 > 0.0f) || (C3 > 0.0f);
    return !(HasNeg && HasPos);
}

static bool IsPointInQuad(ImVec2 P, ImVec2 A, ImVec2 B, ImVec2 C, ImVec2 D)
{
    return IsPointInTriangle(P, A, B, C) || IsPointInTriangle(P, A, C, D);
}

// Auto-turn state machine
static void UpdateAutoTurn(FGameState& State)
{
    if (State.AutoTurnState == EAutoTurnState::Idle)
    {
        return;
    }

    const float Dt = ImGui::GetIO().DeltaTime;
    State.AutoTurnTimer -= Dt;
    if (State.AutoTurnTimer > 0.0f)
    {
        return;
    }

    switch (State.AutoTurnState)
    {
    case EAutoTurnState::EnemyAI_Running:
    {
        // AI has executed, brief pause to show results
        State.AutoTurnState = EAutoTurnState::EnemyAI_Pause;
        State.AutoTurnTimer = 0.5f;
        break;
    }
    case EAutoTurnState::EnemyAI_Pause:
    {
        // Advance to Environment phase
        State.Mode.AdvanceTurn();
        State.LastPhase = State.Mode.CurrentPhase;
        Battle::HandlePhaseStart(State);
        State.AutoTurnState = EAutoTurnState::Environment_Running;
        State.AutoTurnTimer = 0.6f;
        break;
    }
    case EAutoTurnState::Environment_Running:
    {
        State.AutoTurnState = EAutoTurnState::Environment_Pause;
        State.AutoTurnTimer = 0.4f;
        break;
    }
    case EAutoTurnState::Environment_Pause:
    {
        // Advance back to PlayerInput
        State.Mode.AdvanceTurn();
        State.LastPhase = State.Mode.CurrentPhase;
        Battle::HandlePhaseStart(State);
        State.AutoTurnState = EAutoTurnState::Idle;
        break;
    }
    default:
        State.AutoTurnState = EAutoTurnState::Idle;
        break;
    }
}

static void RenderGameInfoPanel(FGameState& State)
{
    ImGui::Begin("Battle Info");

    ImGui::Text("Phase: %s", Battle::GetPhaseName(State.Mode.CurrentPhase));
    ImGui::Text("Turn: %d", State.Mode.TurnCount);

    const int AlivePlayers = Battle::CountAliveTeam(State, 1);
    const int AliveEnemies = Battle::CountAliveTeam(State, 2);
    const bool GameOver = (AlivePlayers == 0 || AliveEnemies == 0);
    ImGui::Text("Players: %d  Enemies: %d", AlivePlayers, AliveEnemies);

    // Auto-turn status
    if (State.AutoTurnState != EAutoTurnState::Idle)
    {
        const char* StatusText = "Processing...";
        switch (State.AutoTurnState)
        {
        case EAutoTurnState::EnemyAI_Running:
        case EAutoTurnState::EnemyAI_Pause:
            StatusText = "Enemy moving...";
            break;
        case EAutoTurnState::Environment_Running:
        case EAutoTurnState::Environment_Pause:
            StatusText = "Environment effects...";
            break;
        default:
            break;
        }
        ImGui::TextColored(ImVec4(0.95f, 0.80f, 0.20f, 1.0f), "%s", StatusText);
    }

    // End Turn button
    const bool CanAct = !GameOver && State.Mode.CurrentPhase == ETurnPhase::PlayerInput &&
                        State.AutoTurnState == EAutoTurnState::Idle;

    if (!CanAct) ImGui::BeginDisabled();
    if (ImGui::Button("End Turn", ImVec2(120.0f, 30.0f)))
    {
        // Start auto-turn sequence
        State.Mode.AdvanceTurn();
        State.LastPhase = State.Mode.CurrentPhase;
        Battle::ResetMovementForTeam(State, 2);
        Battle::RunEnemyAiStep(State);
        State.AutoTurnState = EAutoTurnState::EnemyAI_Running;
        State.AutoTurnTimer = 0.6f;
    }
    if (!CanAct) ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Retreat", ImVec2(80.0f, 30.0f)))
    {
        State.BattleWon = false;
        State.CurrentScreen = EGameScreen::LevelSelect;
    }

    // Game over detection
    if (GameOver && State.AutoTurnState == EAutoTurnState::Idle)
    {
        if (AlivePlayers == 0)
        {
            ImGui::TextColored(ImVec4(0.85f, 0.25f, 0.25f, 1.0f), "All units lost!");
        }
        else if (AliveEnemies == 0)
        {
            ImGui::TextColored(ImVec4(0.25f, 0.85f, 0.35f, 1.0f), "All enemies eliminated!");
        }

        if (!State.VictoryPointAwarded && AliveEnemies == 0)
        {
            State.BattleWon = true;
            State.VictoryPointAwarded = true;
            State.Profile.SkillTree.EarnPoints(1);
            State.Profile.StatPoints += 2;
            if (State.CurrentLevel > State.Profile.HighestLevelCompleted)
            {
                State.Profile.HighestLevelCompleted = State.CurrentLevel;
            }
        }
        else if (!State.VictoryPointAwarded && AlivePlayers == 0)
        {
            State.BattleWon = false;
            State.VictoryPointAwarded = true;
        }

        if (ImGui::Button("Continue", ImVec2(120.0f, 30.0f)))
        {
            State.CurrentScreen = EGameScreen::BattleResult;
        }
    }

    // Selected unit info
    ImGui::Separator();
    if (State.SelectedUnitIndex >= 0 && State.SelectedUnitIndex < static_cast<int>(State.Units.size()))
    {
        const FUnitInstance& Selected = State.Units[State.SelectedUnitIndex];
        ImGui::Text("Selected: %s", Selected.Name.c_str());
        const int32_t MoveRange = Battle::GetMoveRange(Selected.Unit);
        ImGui::Text("Move Range: %d  |  Remaining: %d", MoveRange, Selected.Unit.RemainingMovePoints);

        const FStatBlock Effective = Selected.Unit.GetEffectiveStats();
        const float DodgeChance = BioStatMath::ComputeDodgeChance(Effective.Get(EBioStat::Speed)) * 100.0f;
        const float Mitigation = BioStatMath::ComputeDefenseMitigation(Effective.Get(EBioStat::Defense)) * 100.0f;

        ImGui::Text("HP: %.0f/%.0f  ATK: %.0f  DEF: %.0f (%.0f%%)  SPD: %.0f (Dodge %.0f%%)",
            Selected.Unit.CurrentHitPoints, Effective.Get(EBioStat::HitPoints),
            Effective.Get(EBioStat::Attack), Effective.Get(EBioStat::Defense), Mitigation,
            Effective.Get(EBioStat::Speed), DodgeChance);
    }
    else
    {
        ImGui::Text("Click a unit on the board to select it.");
    }

    ImGui::End();
}

static void RenderIsometricBoard(FGameState& State)
{
    ImGui::SetNextWindowSize(ImVec2(720.0f, 720.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Board", nullptr, ImGuiWindowFlags_NoCollapse);

    ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
    ImVec2 CanvasSize = ImGui::GetContentRegionAvail();
    if (CanvasSize.x < 200.0f) CanvasSize.x = 200.0f;
    if (CanvasSize.y < 200.0f) CanvasSize.y = 200.0f;

    ImGui::InvisibleButton("board_canvas", CanvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    const bool CanvasHovered = ImGui::IsItemHovered();
    const ImVec2 MousePos = ImGui::GetIO().MousePos;
    const bool Clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    DrawList->AddRectFilled(CanvasPos, ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y),
        ImGui::GetColorU32(ImVec4(0.08f, 0.08f, 0.10f, 1.0f)));
    DrawList->PushClipRect(CanvasPos, ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y), true);

    // Camera controls
    if (CanvasHovered)
    {
        ImGuiIO& IO = ImGui::GetIO();
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            State.CameraPan.x += IO.MouseDelta.x;
            State.CameraPan.y += IO.MouseDelta.y;
        }
        if (IO.MouseWheel != 0.0f)
        {
            if (IO.KeyShift)
            {
                State.CameraYaw += IO.MouseWheel * 0.15f;
            }
            else
            {
                State.CameraZoom *= (1.0f + IO.MouseWheel * 0.1f);
                State.CameraZoom = std::clamp(State.CameraZoom, 0.4f, 3.0f);
            }
        }
        if (ImGui::IsKeyDown(ImGuiKey_Q)) State.CameraYaw -= 0.02f;
        if (ImGui::IsKeyDown(ImGuiKey_E)) State.CameraYaw += 0.02f;
    }

    const float TwoPi = 6.2831853f;
    if (State.CameraYaw > TwoPi) State.CameraYaw -= TwoPi;
    else if (State.CameraYaw < -TwoPi) State.CameraYaw += TwoPi;

    const int32_t GridW = State.Grid.GridSize.X;
    const int32_t GridH = State.Grid.GridSize.Y;
    const float CosYaw = std::cos(State.CameraYaw);
    const float SinYaw = std::sin(State.CameraYaw);
    const float CosPitch = std::cos(State.CameraPitch);

    auto ProjectRaw = [&](float X, float Y)
    {
        const float X1 = X * CosYaw - Y * SinYaw;
        const float Y1 = X * SinYaw + Y * CosYaw;
        const float Y2 = Y1 * CosPitch;
        return ImVec2(X1, Y2);
    };

    ImVec2 Corners[4] =
    {
        ProjectRaw(0.0f, 0.0f),
        ProjectRaw(static_cast<float>(GridW), 0.0f),
        ProjectRaw(0.0f, static_cast<float>(GridH)),
        ProjectRaw(static_cast<float>(GridW), static_cast<float>(GridH))
    };

    float MinX = Corners[0].x, MaxX = Corners[0].x;
    float MinY = Corners[0].y, MaxY = Corners[0].y;
    for (int Index = 1; Index < 4; ++Index)
    {
        MinX = std::min(MinX, Corners[Index].x);
        MaxX = std::max(MaxX, Corners[Index].x);
        MinY = std::min(MinY, Corners[Index].y);
        MaxY = std::max(MaxY, Corners[Index].y);
    }

    const float Margin = 24.0f;
    const float AvailableW = std::max(1.0f, CanvasSize.x - Margin * 2.0f);
    const float AvailableH = std::max(1.0f, CanvasSize.y - Margin * 2.0f);
    const float RawW = std::max(0.001f, MaxX - MinX);
    const float RawH = std::max(0.001f, MaxY - MinY);
    float Scale = std::min(AvailableW / RawW, AvailableH / RawH) * State.CameraZoom;
    Scale = std::clamp(Scale, 8.0f, 160.0f);

    const float BoardW = RawW * Scale;
    const float BoardH = RawH * Scale;
    const float OffsetX = CanvasPos.x + (CanvasSize.x - BoardW) * 0.5f - MinX * Scale + State.CameraPan.x;
    const float OffsetY = CanvasPos.y + (CanvasSize.y - BoardH) * 0.5f - MinY * Scale + State.CameraPan.y;

    auto Project = [&](float X, float Y)
    {
        const ImVec2 Raw = ProjectRaw(X, Y);
        return ImVec2(Raw.x * Scale + OffsetX, Raw.y * Scale + OffsetY);
    };

    struct FTileDraw
    {
        int X = 0;
        int Y = 0;
        float Depth = 0.0f;
        ImVec2 P0, P1, P2, P3, Center;
    };

    std::vector<FTileDraw> Tiles;
    Tiles.reserve(GridW * GridH);

    for (int32_t Y = 0; Y < GridH; ++Y)
    {
        for (int32_t X = 0; X < GridW; ++X)
        {
            const ImVec2 P0 = Project(static_cast<float>(X), static_cast<float>(Y));
            const ImVec2 P1 = Project(static_cast<float>(X + 1), static_cast<float>(Y));
            const ImVec2 P2 = Project(static_cast<float>(X + 1), static_cast<float>(Y + 1));
            const ImVec2 P3 = Project(static_cast<float>(X), static_cast<float>(Y + 1));
            const ImVec2 Center = Project(static_cast<float>(X) + 0.5f, static_cast<float>(Y) + 0.5f);
            const float Depth = (static_cast<float>(X) * SinYaw + static_cast<float>(Y) * CosYaw);
            Tiles.push_back(FTileDraw{ X, Y, Depth, P0, P1, P2, P3, Center });
        }
    }

    std::sort(Tiles.begin(), Tiles.end(), [](const FTileDraw& A, const FTileDraw& B)
    {
        return A.Depth < B.Depth;
    });

    const bool GameOver = (Battle::CountAliveTeam(State, 1) == 0 || Battle::CountAliveTeam(State, 2) == 0);
    const bool PlayerCanMove = !GameOver && State.Mode.CurrentPhase == ETurnPhase::PlayerInput &&
                                State.AutoTurnState == EAutoTurnState::Idle;

    FIntPoint ClickedCell(-1, -1);
    for (const FTileDraw& Tile : Tiles)
    {
        const FIntPoint Cell(Tile.X, Tile.Y);
        const int UnitIndex = Battle::FindUnitAt(State, Cell);
        const bool HasUnit = UnitIndex >= 0;
        const bool IsSelected = (UnitIndex == State.SelectedUnitIndex);
        const bool IsObstacle = Battle::IsObstacleAt(State, Cell);
        const bool IsHealTile = Battle::IsHealingPodAt(State, Cell);
        const bool IsHazardTile = Battle::IsHazardAt(State, Cell);

        const bool Hovered = CanvasHovered && IsPointInQuad(MousePos, Tile.P0, Tile.P1, Tile.P2, Tile.P3);
        if (Hovered && Clicked)
        {
            ClickedCell = Cell;
        }

        ImVec4 BaseColor = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
        if (IsObstacle)
            BaseColor = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
        else if (HasUnit)
        {
            const int TeamId = State.Units[UnitIndex].Unit.TeamID;
            BaseColor = (TeamId == 2) ? ImVec4(0.60f, 0.18f, 0.20f, 1.0f) : ImVec4(0.18f, 0.40f, 0.70f, 1.0f);
        }
        else if (IsHealTile)
            BaseColor = ImVec4(0.18f, 0.55f, 0.30f, 1.0f);
        else if (IsHazardTile)
            BaseColor = ImVec4(0.60f, 0.32f, 0.15f, 1.0f);

        if (IsSelected)
            BaseColor = ImVec4(0.80f, 0.65f, 0.20f, 1.0f);
        else if (Hovered)
            BaseColor = ImVec4(std::min(1.0f, BaseColor.x + 0.08f), std::min(1.0f, BaseColor.y + 0.08f),
                               std::min(1.0f, BaseColor.z + 0.08f), 1.0f);

        DrawList->AddQuadFilled(Tile.P0, Tile.P1, Tile.P2, Tile.P3, ImGui::GetColorU32(BaseColor));
        DrawList->AddQuad(Tile.P0, Tile.P1, Tile.P2, Tile.P3, ImGui::GetColorU32(ImVec4(0.10f, 0.10f, 0.10f, 1.0f)), 1.0f);

        if (IsObstacle)
        {
            DrawList->AddText(ImVec2(Tile.Center.x - 4.0f, Tile.Center.y - 6.0f),
                ImGui::GetColorU32(ImVec4(0.75f, 0.75f, 0.75f, 1.0f)), "#");
        }
        else if (!HasUnit && IsHealTile)
        {
            DrawList->AddText(ImVec2(Tile.Center.x - 4.0f, Tile.Center.y - 6.0f),
                ImGui::GetColorU32(ImVec4(0.90f, 0.95f, 0.90f, 1.0f)), "H");
        }
        else if (!HasUnit && IsHazardTile)
        {
            DrawList->AddText(ImVec2(Tile.Center.x - 4.0f, Tile.Center.y - 6.0f),
                ImGui::GetColorU32(ImVec4(0.95f, 0.85f, 0.75f, 1.0f)), "X");
        }

        if (HasUnit)
        {
            const int TeamId = State.Units[UnitIndex].Unit.TeamID;
            const ImU32 UnitColor = ImGui::GetColorU32((TeamId == 2)
                ? ImVec4(0.85f, 0.25f, 0.28f, 1.0f) : ImVec4(0.25f, 0.55f, 0.90f, 1.0f));
            const float Radius = std::max(3.0f, Scale * 0.18f);
            DrawList->AddCircleFilled(Tile.Center, Radius, UnitColor);
            if (!State.Units[UnitIndex].Name.empty())
            {
                char Label[2] = { State.Units[UnitIndex].Name[0], '\0' };
                DrawList->AddText(ImVec2(Tile.Center.x - 4.0f, Tile.Center.y - 6.0f),
                    ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), Label);
            }
        }
    }

    DrawList->AddText(ImVec2(CanvasPos.x + 8.0f, CanvasPos.y + 8.0f),
        ImGui::GetColorU32(ImVec4(0.8f, 0.8f, 0.85f, 1.0f)),
        "RMB drag: pan | Wheel: zoom | Shift+Wheel or Q/E: rotate");

    DrawList->PopClipRect();

    // Handle click
    if (Clicked && ClickedCell.X >= 0)
    {
        const int UnitIndex = Battle::FindUnitAt(State, ClickedCell);
        const bool HasUnit = UnitIndex >= 0;
        const bool IsObstacle = Battle::IsObstacleAt(State, ClickedCell);
        if (HasUnit)
        {
            State.SelectedUnitIndex = UnitIndex;
        }
        else if (PlayerCanMove && !IsObstacle && State.SelectedUnitIndex >= 0)
        {
            BioUnitBase& SelectedUnit = State.Units[State.SelectedUnitIndex].Unit;
            if (SelectedUnit.TeamID == 1)
            {
                const int32_t Range = Battle::GetMoveRange(SelectedUnit);
                const int32_t Dist = State.Grid.GetManhattanDistance(SelectedUnit.GridCoordinates, ClickedCell);
                if (Dist <= Range && Dist <= SelectedUnit.RemainingMovePoints)
                {
                    SelectedUnit.MoveToGrid(ClickedCell);
                    SelectedUnit.ConsumeMovePoints(Dist);
                    Battle::ResolveEncounters(State);
                }
            }
        }
    }

    ImGui::End();
}

static void RenderUnitStatsTable(FGameState& State)
{
    ImGui::Begin("Unit Stats");
    if (ImGui::BeginTable("UnitStatsTable", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Team");
        ImGui::TableSetupColumn("HP");
        ImGui::TableSetupColumn("ATK");
        ImGui::TableSetupColumn("DEF");
        ImGui::TableSetupColumn("SPD");
        ImGui::TableSetupColumn("Move");
        ImGui::TableSetupColumn("MP Left");
        ImGui::TableSetupColumn("Pos");
        ImGui::TableHeadersRow();

        for (int Index = 0; Index < static_cast<int>(State.Units.size()); ++Index)
        {
            const FUnitInstance& UnitInstance = State.Units[Index];
            const BioUnitBase& Unit = UnitInstance.Unit;
            const FStatBlock Stats = Unit.GetEffectiveStats();
            const int32_t MoveRange = Battle::GetMoveRange(Unit);
            const bool IsSelected = (Index == State.SelectedUnitIndex);

            ImGui::TableNextRow();

            if (IsSelected)
            {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImVec4(0.30f, 0.30f, 0.12f, 1.0f)));
            }

            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(UnitInstance.Name.c_str(), IsSelected, ImGuiSelectableFlags_SpanAllColumns))
            {
                State.SelectedUnitIndex = Index;
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", Unit.TeamID == 1 ? "Player" : "Enemy");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.0f/%.0f", Unit.CurrentHitPoints, Stats.Get(EBioStat::HitPoints));
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
}

void RenderBattle(FGameState& State)
{
    // Handle auto-turn phase transitions
    UpdateAutoTurn(State);

    // Handle phase change detection (for manual or auto-turn transitions)
    if (State.Mode.CurrentPhase != State.LastPhase)
    {
        State.LastPhase = State.Mode.CurrentPhase;
        State.EnemyAiExecuted = false;
        Battle::HandlePhaseStart(State);
    }

    RenderGameInfoPanel(State);
    RenderIsometricBoard(State);
    RenderUnitStatsTable(State);
}
