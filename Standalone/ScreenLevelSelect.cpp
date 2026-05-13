#include "ScreenLevelSelect.h"
#include "GameState.h"
#include "LevelData.h"
#include "BattleLogic.h"

#include "imgui.h"

void RenderLevelSelect(FGameState& State)
{
    const ImGuiViewport* Viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(Viewport->WorkPos);
    ImGui::SetNextWindowSize(Viewport->WorkSize);

    ImGuiWindowFlags Flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("LevelSelect", nullptr, Flags);

    const float WindowWidth = ImGui::GetWindowWidth();

    // Title
    {
        const char* Title = "SELECT MISSION";
        const ImVec2 TitleSize = ImGui::CalcTextSize(Title);
        ImGui::SetCursorPosX((WindowWidth - TitleSize.x) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.75f, 0.95f, 1.0f));
        ImGui::TextUnformatted(Title);
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const auto& Levels = GetAllLevels();
    const float CardWidth = 500.0f;
    const float CardX = (WindowWidth - CardWidth) * 0.5f;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

    for (const FLevelData& Level : Levels)
    {
        const bool Unlocked = Level.LevelNumber <= State.Profile.HighestLevelCompleted + 1;
        const bool Completed = Level.LevelNumber <= State.Profile.HighestLevelCompleted;

        ImGui::SetCursorPosX(CardX);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.15f, 1.0f));
        const std::string ChildId = "Level_" + std::to_string(Level.LevelNumber);
        ImGui::BeginChild(ChildId.c_str(), ImVec2(CardWidth, 80.0f), true);

        // Level header
        if (Completed)
        {
            ImGui::TextColored(ImVec4(0.40f, 0.85f, 0.45f, 1.0f), "Mission %d: %s  [COMPLETE]",
                Level.LevelNumber, Level.Name.c_str());
        }
        else if (Unlocked)
        {
            ImGui::TextColored(ImVec4(0.90f, 0.85f, 0.30f, 1.0f), "Mission %d: %s",
                Level.LevelNumber, Level.Name.c_str());
        }
        else
        {
            ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.45f, 1.0f), "Mission %d: LOCKED",
                Level.LevelNumber);
        }

        if (Unlocked)
        {
            ImGui::TextDisabled("%s", Level.Description.c_str());
            ImGui::Text("Enemies: %d  |  Grid: %dx%d", Level.EnemyCount, Level.GridWidth, Level.GridHeight);

            ImGui::SameLine(CardWidth - 90.0f);
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.45f, 0.75f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.25f, 0.55f, 0.85f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.15f, 0.35f, 0.65f, 1.0f));
            const std::string BtnLabel = "Deploy##" + std::to_string(Level.LevelNumber);
            if (ImGui::Button(BtnLabel.c_str(), ImVec2(72.0f, 24.0f)))
            {
                State.CurrentLevel = Level.LevelNumber;
                Battle::InitializeBattle(State, Level);
                State.CurrentScreen = EGameScreen::Battle;
            }
            ImGui::PopStyleColor(3);
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    ImGui::PopStyleVar();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Back button
    {
        const float BtnWidth = 120.0f;
        ImGui::SetCursorPosX((WindowWidth - BtnWidth) * 0.5f);
        if (ImGui::Button("Back", ImVec2(BtnWidth, 32.0f)))
        {
            State.CurrentScreen = EGameScreen::MainMenu;
        }
    }

    ImGui::End();
}
