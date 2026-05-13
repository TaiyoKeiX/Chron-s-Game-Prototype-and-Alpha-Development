#include "ScreenCharacterUpgrade.h"
#include "GameState.h"

#include "imgui.h"

static void RenderSkillTreePanel(FGameState& State)
{
    FSkillTreeState& Tree = State.Profile.SkillTree;

    ImGui::Text("Skill Points: %d", Tree.AvailableSkillPoints);
    ImGui::SameLine();
    const int32_t OwnedCount = static_cast<int32_t>(Tree.OwnedSkillIds.size());
    if (OwnedCount > 0)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.50f, 0.18f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.65f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.40f, 0.12f, 0.12f, 1.0f));
        if (ImGui::Button("Reset Skills"))
        {
            Tree.Reset();
        }
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Refunds all %d invested skill point(s).", OwnedCount);
        }
    }
    ImGui::Separator();

    const FStatBlock Bonus = Tree.ComputeTotalBonus();
    ImGui::Text("Skill Bonuses: HP +%.0f  ATK +%.0f  DEF +%.0f  SPD +%.0f",
        Bonus.Get(EBioStat::HitPoints), Bonus.Get(EBioStat::Attack),
        Bonus.Get(EBioStat::Defense), Bonus.Get(EBioStat::Speed));
    ImGui::Separator();

    const char* TreeNames[3]       = { "Aggressor", "Guardian", "Memory" };
    const ESkillTree Trees[3]      = { ESkillTree::Aggressor, ESkillTree::Guardian, ESkillTree::Memory };
    const ImVec4 HeaderColors[3]   = {
        ImVec4(0.90f, 0.30f, 0.30f, 1.0f),
        ImVec4(0.30f, 0.55f, 0.90f, 1.0f),
        ImVec4(0.30f, 0.80f, 0.40f, 1.0f)
    };

    const float TotalWidth  = ImGui::GetContentRegionAvail().x;
    const float ChildWidth  = (TotalWidth - 16.0f) / 3.0f;
    const float ChildHeight = ImGui::GetContentRegionAvail().y;

    const auto& Registry = FSkillTreeState::GetRegistry();

    for (int TreeIdx = 0; TreeIdx < 3; ++TreeIdx)
    {
        if (TreeIdx > 0) ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f, 0.11f, 0.14f, 1.0f));
        ImGui::BeginChild(TreeNames[TreeIdx], ImVec2(ChildWidth, ChildHeight), true);

        ImGui::TextColored(HeaderColors[TreeIdx], "%s", TreeNames[TreeIdx]);
        ImGui::Text("Depth: %d / 6", Tree.CountInTree(Trees[TreeIdx]));
        ImGui::Separator();

        for (int Tier = 1; Tier <= 3; ++Tier)
        {
            ImGui::TextDisabled("-- Tier %d --", Tier);

            for (const FSkillNode& Node : Registry)
            {
                if (Node.Tree != Trees[TreeIdx] || Node.Tier != Tier) continue;

                const bool Owned      = Tree.IsOwned(Node.Id);
                const bool Unlockable = Tree.CanUnlock(Node.Id);

                ImVec4 BtnColor;
                if (Owned)
                    BtnColor = ImVec4(0.15f, 0.52f, 0.18f, 1.0f);
                else if (Unlockable)
                    BtnColor = ImVec4(0.62f, 0.50f, 0.04f, 1.0f);
                else
                    BtnColor = ImVec4(0.28f, 0.28f, 0.28f, 1.0f);

                const ImVec4 BtnHover = ImVec4(BtnColor.x + 0.08f, BtnColor.y + 0.08f, BtnColor.z + 0.08f, 1.0f);

                ImGui::PushStyleColor(ImGuiCol_Button,        BtnColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  Owned ? BtnColor : BtnHover);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,   BtnColor);

                const float BtnWidth = ImGui::GetContentRegionAvail().x;
                if (ImGui::Button(Node.Name.c_str(), ImVec2(BtnWidth, 0.0f)) && Unlockable)
                {
                    Tree.Unlock(Node.Id);
                }

                ImGui::PopStyleColor(3);

                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(Node.Name.c_str());
                    ImGui::Separator();
                    ImGui::TextWrapped("%s", Node.Description.c_str());
                    ImGui::Spacing();
                    const FStatBlock& M = Node.StatModifiers;
                    if (M.Get(EBioStat::HitPoints) != 0.0f) ImGui::Text("HP:  +%.0f", M.Get(EBioStat::HitPoints));
                    if (M.Get(EBioStat::Attack)    != 0.0f) ImGui::Text("ATK: +%.0f", M.Get(EBioStat::Attack));
                    if (M.Get(EBioStat::Defense)   != 0.0f) ImGui::Text("DEF: +%.0f", M.Get(EBioStat::Defense));
                    if (M.Get(EBioStat::Speed)     != 0.0f) ImGui::Text("SPD: +%.0f", M.Get(EBioStat::Speed));
                    ImGui::Spacing();
                    if (Owned)
                        ImGui::TextColored(ImVec4(0.40f, 0.90f, 0.45f, 1.0f), "OWNED");
                    else if (Unlockable)
                        ImGui::TextColored(ImVec4(0.90f, 0.80f, 0.20f, 1.0f), "Click to unlock  (cost: %d SP)", Node.SkillPointCost);
                    else
                        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f), "Requires a Tier %d skill first", Node.Tier - 1);
                    ImGui::EndTooltip();
                }
            }

            ImGui::Spacing();
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
}

static void RenderStatAllocationPanel(FGameState& State)
{
    FPlayerProfile& Profile = State.Profile;

    ImGui::Text("Stat Points Available: %d", Profile.StatPoints);
    ImGui::SameLine();
    ImGui::TextDisabled("(+2 per mission victory)");

    bool HasAllocated = false;
    for (size_t I = 0; I < static_cast<size_t>(EBioStat::Count); ++I)
    {
        if (Profile.StatAllocation[I] > 0) { HasAllocated = true; break; }
    }

    if (HasAllocated)
    {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.50f, 0.18f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.65f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.40f, 0.12f, 0.12f, 1.0f));
        if (ImGui::Button("Reset Stats"))
        {
            Profile.ResetStatAllocation();
        }
        ImGui::PopStyleColor(3);
    }

    ImGui::Separator();

    const char* StatNames[4] = { "Hit Points", "Attack", "Defense", "Speed" };
    const float Multipliers[4] = { 10.0f, 3.0f, 3.0f, 3.0f };

    for (size_t I = 0; I < static_cast<size_t>(EBioStat::Count); ++I)
    {
        const EBioStat Stat = static_cast<EBioStat>(I);
        const int32_t Allocated = Profile.StatAllocation[I];
        const float BonusValue = static_cast<float>(Allocated) * Multipliers[I];

        ImGui::Text("%-10s  +%.0f  (%d pts)", StatNames[I], BonusValue, Allocated);
        ImGui::SameLine();

        const std::string MinusLabel = std::string("-##stat") + std::to_string(I);
        const std::string PlusLabel  = std::string("+##stat") + std::to_string(I);

        if (Allocated > 0)
        {
            if (ImGui::SmallButton(MinusLabel.c_str()))
            {
                Profile.DeallocateStatPoint(Stat);
            }
        }
        else
        {
            ImGui::TextDisabled(" -");
        }
        ImGui::SameLine();

        if (Profile.StatPoints > 0)
        {
            if (ImGui::SmallButton(PlusLabel.c_str()))
            {
                Profile.AllocateStatPoint(Stat);
            }
        }
        else
        {
            ImGui::TextDisabled(" +");
        }
    }

    ImGui::Separator();

    // Show combined total bonuses
    const FStatBlock SkillBonus = Profile.SkillTree.ComputeTotalBonus();
    const FStatBlock StatBonus = Profile.ComputeStatAllocationBonus();
    ImGui::TextColored(ImVec4(0.70f, 0.85f, 0.95f, 1.0f), "Total Bonuses (Skills + Stats):");
    ImGui::Text("  HP:  +%.0f", SkillBonus.Get(EBioStat::HitPoints) + StatBonus.Get(EBioStat::HitPoints));
    ImGui::Text("  ATK: +%.0f", SkillBonus.Get(EBioStat::Attack)    + StatBonus.Get(EBioStat::Attack));
    ImGui::Text("  DEF: +%.0f", SkillBonus.Get(EBioStat::Defense)   + StatBonus.Get(EBioStat::Defense));
    ImGui::Text("  SPD: +%.0f", SkillBonus.Get(EBioStat::Speed)     + StatBonus.Get(EBioStat::Speed));
}

void RenderCharacterUpgrade(FGameState& State)
{
    const ImGuiViewport* Viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(Viewport->WorkPos);
    ImGui::SetNextWindowSize(Viewport->WorkSize);

    ImGuiWindowFlags Flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("CharacterUpgrade", nullptr, Flags);

    const float WindowWidth = ImGui::GetWindowWidth();

    // Title
    {
        const char* Title = "UPGRADE";
        const ImVec2 TitleSize = ImGui::CalcTextSize(Title);
        ImGui::SetCursorPosX((WindowWidth - TitleSize.x) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.75f, 0.25f, 1.0f));
        ImGui::TextUnformatted(Title);
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const float AvailHeight = ImGui::GetContentRegionAvail().y - 50.0f;

    // Stat allocation panel (top section)
    ImGui::BeginChild("StatAlloc", ImVec2(0.0f, 200.0f), true);
    RenderStatAllocationPanel(State);
    ImGui::EndChild();

    ImGui::Spacing();

    // Skill tree panel (bottom section, fills remaining space)
    ImGui::BeginChild("SkillTree", ImVec2(0.0f, AvailHeight - 210.0f), true);
    RenderSkillTreePanel(State);
    ImGui::EndChild();

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
