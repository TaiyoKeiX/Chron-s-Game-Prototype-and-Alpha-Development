#include "ScreenMainMenu.h"
#include "GameState.h"

#include "imgui.h"

#include <windows.h>

void RenderMainMenu(FGameState& State)
{
    const ImGuiViewport* Viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(Viewport->WorkPos);
    ImGui::SetNextWindowSize(Viewport->WorkSize);

    ImGuiWindowFlags Flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("MainMenu", nullptr, Flags);

    const float WindowWidth = ImGui::GetWindowWidth();
    const float WindowHeight = ImGui::GetWindowHeight();

    // Title
    {
        const char* Title = "CHRON'S GAME";
        const ImVec2 TitleSize = ImGui::CalcTextSize(Title);
        ImGui::SetCursorPos(ImVec2((WindowWidth - TitleSize.x) * 0.5f, WindowHeight * 0.15f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.75f, 0.95f, 1.0f));
        ImGui::TextUnformatted(Title);
        ImGui::PopStyleColor();
    }

    // Subtitle
    {
        const char* Subtitle = "Immune System Tactics";
        const ImVec2 SubSize = ImGui::CalcTextSize(Subtitle);
        ImGui::SetCursorPos(ImVec2((WindowWidth - SubSize.x) * 0.5f, WindowHeight * 0.15f + 28.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.60f, 0.65f, 1.0f));
        ImGui::TextUnformatted(Subtitle);
        ImGui::PopStyleColor();
    }

    const float BtnWidth = 220.0f;
    const float BtnHeight = 40.0f;
    const float BtnX = (WindowWidth - BtnWidth) * 0.5f;
    float BtnY = WindowHeight * 0.40f;
    const float BtnSpacing = 12.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

    // Play
    ImGui::SetCursorPos(ImVec2(BtnX, BtnY));
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.45f, 0.75f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.25f, 0.55f, 0.85f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.15f, 0.35f, 0.65f, 1.0f));
    if (ImGui::Button("Play", ImVec2(BtnWidth, BtnHeight)))
    {
        State.CurrentScreen = EGameScreen::LevelSelect;
    }
    ImGui::PopStyleColor(3);

    // Upgrade
    BtnY += BtnHeight + BtnSpacing;
    ImGui::SetCursorPos(ImVec2(BtnX, BtnY));
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.50f, 0.35f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.60f, 0.45f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.40f, 0.28f, 0.10f, 1.0f));
    if (ImGui::Button("Upgrade", ImVec2(BtnWidth, BtnHeight)))
    {
        State.CurrentScreen = EGameScreen::CharacterUpgrade;
    }
    ImGui::PopStyleColor(3);

    // Quit
    BtnY += BtnHeight + BtnSpacing;
    ImGui::SetCursorPos(ImVec2(BtnX, BtnY));
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.18f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.65f, 0.25f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.45f, 0.12f, 0.12f, 1.0f));
    if (ImGui::Button("Quit", ImVec2(BtnWidth, BtnHeight)))
    {
        PostQuitMessage(0);
    }
    ImGui::PopStyleColor(3);

    ImGui::PopStyleVar();

    // Footer
    {
        const char* Footer = "Prototype Build";
        const ImVec2 FooterSize = ImGui::CalcTextSize(Footer);
        ImGui::SetCursorPos(ImVec2((WindowWidth - FooterSize.x) * 0.5f, WindowHeight - 40.0f));
        ImGui::TextDisabled("%s", Footer);
    }

    ImGui::End();
}
