#include "ScreenBattleResult.h"
#include "GameState.h"
#include "LevelData.h"

#include "imgui.h"

void RenderBattleResult(FGameState& State)
{
    const ImGuiViewport* Viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(Viewport->WorkPos);
    ImGui::SetNextWindowSize(Viewport->WorkSize);

    ImGuiWindowFlags Flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("BattleResult", nullptr, Flags);

    const float WindowWidth = ImGui::GetWindowWidth();
    const float WindowHeight = ImGui::GetWindowHeight();

    if (State.BattleWon)
    {
        // Victory
        {
            const char* Title = "VICTORY!";
            const ImVec2 TitleSize = ImGui::CalcTextSize(Title);
            ImGui::SetCursorPos(ImVec2((WindowWidth - TitleSize.x) * 0.5f, WindowHeight * 0.20f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.90f, 0.40f, 1.0f));
            ImGui::TextUnformatted(Title);
            ImGui::PopStyleColor();
        }

        const FLevelData* Level = GetLevel(State.CurrentLevel);
        const char* LevelName = Level ? Level->Name.c_str() : "Unknown";

        {
            char Subtitle[128];
            snprintf(Subtitle, sizeof(Subtitle), "Mission %d: %s - Complete", State.CurrentLevel, LevelName);
            const ImVec2 SubSize = ImGui::CalcTextSize(Subtitle);
            ImGui::SetCursorPos(ImVec2((WindowWidth - SubSize.x) * 0.5f, WindowHeight * 0.20f + 30.0f));
            ImGui::TextDisabled("%s", Subtitle);
        }

        ImGui::SetCursorPos(ImVec2(WindowWidth * 0.35f, WindowHeight * 0.38f));
        ImGui::TextColored(ImVec4(0.90f, 0.80f, 0.20f, 1.0f), "Rewards:");
        ImGui::SetCursorPosX(WindowWidth * 0.35f);
        ImGui::Text("+1 Skill Point");
        ImGui::SetCursorPosX(WindowWidth * 0.35f);
        ImGui::Text("+2 Stat Points");
    }
    else
    {
        // Defeat
        {
            const char* Title = "DEFEAT";
            const ImVec2 TitleSize = ImGui::CalcTextSize(Title);
            ImGui::SetCursorPos(ImVec2((WindowWidth - TitleSize.x) * 0.5f, WindowHeight * 0.20f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.25f, 0.25f, 1.0f));
            ImGui::TextUnformatted(Title);
            ImGui::PopStyleColor();
        }

        {
            const char* Subtitle = "Your immune cells have been overrun.";
            const ImVec2 SubSize = ImGui::CalcTextSize(Subtitle);
            ImGui::SetCursorPos(ImVec2((WindowWidth - SubSize.x) * 0.5f, WindowHeight * 0.20f + 30.0f));
            ImGui::TextDisabled("%s", Subtitle);
        }
    }

    // Buttons
    const float BtnWidth = 180.0f;
    const float BtnHeight = 36.0f;
    const float BtnX = (WindowWidth - BtnWidth) * 0.5f;
    float BtnY = WindowHeight * 0.60f;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

    ImGui::SetCursorPos(ImVec2(BtnX, BtnY));
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.45f, 0.75f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.25f, 0.55f, 0.85f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.15f, 0.35f, 0.65f, 1.0f));
    if (ImGui::Button("Continue", ImVec2(BtnWidth, BtnHeight)))
    {
        State.CurrentScreen = EGameScreen::LevelSelect;
    }
    ImGui::PopStyleColor(3);

    BtnY += BtnHeight + 10.0f;
    ImGui::SetCursorPos(ImVec2(BtnX, BtnY));
    if (ImGui::Button("Main Menu", ImVec2(BtnWidth, BtnHeight)))
    {
        State.CurrentScreen = EGameScreen::MainMenu;
    }

    ImGui::PopStyleVar();

    ImGui::End();
}
