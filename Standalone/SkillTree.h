#pragma once

#include "BioTypes.h"

#include <cstdint>
#include <string>
#include <vector>

enum class ESkillTree : uint8_t
{
    Aggressor,
    Guardian,
    Memory
};

// A conditional bonus that activates when the player owns enough skills in a tree
struct FSynergyBonus
{
    int32_t MinSkillsInTree = 0;
    FStatBlock StatBonus;
};

struct FSkillNode
{
    std::string Id;
    std::string Name;
    std::string Description;
    ESkillTree Tree;
    int32_t Tier = 1;                            // 1, 2, or 3
    std::vector<std::string> Prerequisites;      // IDs of skills that must be owned first
    FStatBlock StatModifiers;                    // direct stat bonus on unlock
    std::vector<FSynergyBonus> SynergyBonuses;  // conditional bonuses based on tree depth
    int32_t SkillPointCost = 1;
};

struct FSkillTreeState
{
    std::vector<std::string> OwnedSkillIds;
    int32_t AvailableSkillPoints = 3;

    bool IsOwned(const std::string& Id) const;
    bool CanUnlock(const std::string& Id) const;
    bool Unlock(const std::string& Id);
    void EarnPoints(int32_t Amount);
    int32_t CountInTree(ESkillTree Tree) const;

    // Refunds all invested skill points and clears owned skills
    void Reset();

    // Returns the sum of all direct modifiers plus all active synergy bonuses
    FStatBlock ComputeTotalBonus() const;

    static const std::vector<FSkillNode>& GetRegistry();
    static const FSkillNode* Find(const std::string& Id);
};
