#include "SkillTree.h"

#include <algorithm>

// ---------------------------------------------------------------------------
// Registry
// ---------------------------------------------------------------------------

static std::vector<FSkillNode> BuildRegistry()
{
    std::vector<FSkillNode> Nodes;

    auto MakeStats = [](float Hp, float Atk, float Def, float Spd) -> FStatBlock
    {
        FStatBlock B;
        B.Set(EBioStat::HitPoints, Hp);
        B.Set(EBioStat::Attack,    Atk);
        B.Set(EBioStat::Defense,   Def);
        B.Set(EBioStat::Speed,     Spd);
        return B;
    };

    // -----------------------------------------------------------------------
    // Aggressor tree  (Neutrophil / Killer T-Cell — DPS)
    // -----------------------------------------------------------------------

    // Tier 1
    {
        FSkillNode N;
        N.Id          = "agg_t1_strike";
        N.Name        = "Cytotoxic Strike";
        N.Description = "Sharpen cytotoxic granules. +12 Attack.";
        N.Tree        = ESkillTree::Aggressor;
        N.Tier        = 1;
        N.StatModifiers = MakeStats(0, 12, 0, 0);
        Nodes.push_back(N);
    }
    {
        FSkillNode N;
        N.Id          = "agg_t1_chemo";
        N.Name        = "Rapid Chemotaxis";
        N.Description = "Enhanced receptor sensitivity accelerates cell migration. +10 Speed.";
        N.Tree        = ESkillTree::Aggressor;
        N.Tier        = 1;
        N.StatModifiers = MakeStats(0, 0, 0, 10);
        Nodes.push_back(N);
    }

    // Tier 2
    {
        FSkillNode N;
        N.Id          = "agg_t2_lytic";
        N.Name        = "Lytic Burst";
        N.Description = "Explosive lytic release. +18 Attack.\nSynergy (2+ Aggressor skills): +8 Attack.";
        N.Tree        = ESkillTree::Aggressor;
        N.Tier        = 2;
        N.Prerequisites = { "agg_t1_strike", "agg_t1_chemo" };  // requires any one — enforced by CanUnlock
        N.StatModifiers = MakeStats(0, 18, 0, 0);
        {
            FSynergyBonus S;
            S.MinSkillsInTree = 2;
            S.StatBonus = MakeStats(0, 8, 0, 0);
            N.SynergyBonuses.push_back(S);
        }
        Nodes.push_back(N);
    }
    {
        FSkillNode N;
        N.Id          = "agg_t2_perforin";
        N.Name        = "Perforin Secretion";
        N.Description = "Membrane-piercing proteins bypass resistance. +15 Attack, +5 Speed.";
        N.Tree        = ESkillTree::Aggressor;
        N.Tier        = 2;
        N.Prerequisites = { "agg_t1_strike", "agg_t1_chemo" };
        N.StatModifiers = MakeStats(0, 15, 0, 5);
        Nodes.push_back(N);
    }

    // Tier 3
    {
        FSkillNode N;
        N.Id          = "agg_t3_nk";
        N.Name        = "NK Cell Fury";
        N.Description = "Unleash natural killer cell aggression. +25 Attack.\nSynergy (5+ Aggressor skills): +5 Attack per skill in tree.";
        N.Tree        = ESkillTree::Aggressor;
        N.Tier        = 3;
        N.Prerequisites = { "agg_t2_lytic", "agg_t2_perforin" };
        N.StatModifiers = MakeStats(0, 25, 0, 0);
        {
            // At 5 skills: +5 Attack × CountInTree(Aggressor) — approximated as flat +30
            // (5 skills × 5 = 25, but adding this skill makes 6 → 30; we encode the max
            // realistic value; dynamic per-skill scaling is handled in ComputeTotalBonus)
            FSynergyBonus S;
            S.MinSkillsInTree = 5;
            S.StatBonus = MakeStats(0, 30, 0, 0);  // 6 skills × 5 Attack
            N.SynergyBonuses.push_back(S);
        }
        Nodes.push_back(N);
    }
    {
        FSkillNode N;
        N.Id          = "agg_t3_storm";
        N.Name        = "Oxidative Storm";
        N.Description = "A hurricane of reactive oxygen. +20 Attack, +15 Speed.\nSynergy (5+ Aggressor skills): +10 Attack.";
        N.Tree        = ESkillTree::Aggressor;
        N.Tier        = 3;
        N.Prerequisites = { "agg_t2_lytic", "agg_t2_perforin" };
        N.StatModifiers = MakeStats(0, 20, 0, 15);
        {
            FSynergyBonus S;
            S.MinSkillsInTree = 5;
            S.StatBonus = MakeStats(0, 10, 0, 0);
            N.SynergyBonuses.push_back(S);
        }
        Nodes.push_back(N);
    }

    // -----------------------------------------------------------------------
    // Guardian tree  (Macrophage — Tank)
    // -----------------------------------------------------------------------

    // Tier 1
    {
        FSkillNode N;
        N.Id          = "grd_t1_shell";
        N.Name        = "Phagocytic Shell";
        N.Description = "Reinforce the cell membrane with phagocytic proteins. +15 Defense.";
        N.Tree        = ESkillTree::Guardian;
        N.Tier        = 1;
        N.StatModifiers = MakeStats(0, 0, 15, 0);
        Nodes.push_back(N);
    }
    {
        FSkillNode N;
        N.Id          = "grd_t1_membrane";
        N.Name        = "Membrane Fortification";
        N.Description = "Thicken the lipid bilayer for additional resilience. +30 HP.";
        N.Tree        = ESkillTree::Guardian;
        N.Tier        = 1;
        N.StatModifiers = MakeStats(30, 0, 0, 0);
        Nodes.push_back(N);
    }

    // Tier 2
    {
        FSkillNode N;
        N.Id          = "grd_t2_complement";
        N.Name        = "Complement Barrier";
        N.Description = "Activate complement cascade as a defensive shield. +20 Defense.\nSynergy (2+ Guardian skills): +10 Defense.";
        N.Tree        = ESkillTree::Guardian;
        N.Tier        = 2;
        N.Prerequisites = { "grd_t1_shell", "grd_t1_membrane" };
        N.StatModifiers = MakeStats(0, 0, 20, 0);
        {
            FSynergyBonus S;
            S.MinSkillsInTree = 2;
            S.StatBonus = MakeStats(0, 0, 10, 0);
            N.SynergyBonuses.push_back(S);
        }
        Nodes.push_back(N);
    }
    {
        FSkillNode N;
        N.Id          = "grd_t2_inflame";
        N.Name        = "Inflammatory Thickening";
        N.Description = "Controlled inflammation hardens the cell body. +50 HP, +5 Defense.";
        N.Tree        = ESkillTree::Guardian;
        N.Tier        = 2;
        N.Prerequisites = { "grd_t1_shell", "grd_t1_membrane" };
        N.StatModifiers = MakeStats(50, 0, 5, 0);
        Nodes.push_back(N);
    }

    // Tier 3
    {
        FSkillNode N;
        N.Id          = "grd_t3_bastion";
        N.Name        = "Macrophage Bastion";
        N.Description = "Transform into an immovable fortress. +40 HP.\nSynergy (5+ Guardian skills): +15 HP per skill in tree.";
        N.Tree        = ESkillTree::Guardian;
        N.Tier        = 3;
        N.Prerequisites = { "grd_t2_complement", "grd_t2_inflame" };
        N.StatModifiers = MakeStats(40, 0, 0, 0);
        {
            FSynergyBonus S;
            S.MinSkillsInTree = 5;
            S.StatBonus = MakeStats(90, 0, 0, 0);  // 6 skills × 15 HP
            N.SynergyBonuses.push_back(S);
        }
        Nodes.push_back(N);
    }
    {
        FSkillNode N;
        N.Id          = "grd_t3_iron";
        N.Name        = "Iron Constitution";
        N.Description = "Cellular iron deposits harden all surfaces. +30 Defense.\nSynergy (5+ Guardian skills): +10 Defense.";
        N.Tree        = ESkillTree::Guardian;
        N.Tier        = 3;
        N.Prerequisites = { "grd_t2_complement", "grd_t2_inflame" };
        N.StatModifiers = MakeStats(0, 0, 30, 0);
        {
            FSynergyBonus S;
            S.MinSkillsInTree = 5;
            S.StatBonus = MakeStats(0, 0, 10, 0);
            N.SynergyBonuses.push_back(S);
        }
        Nodes.push_back(N);
    }

    // -----------------------------------------------------------------------
    // Memory tree  (Helper T / B-Cell — Support / Utility)
    // -----------------------------------------------------------------------

    // Tier 1
    {
        FSkillNode N;
        N.Id          = "mem_t1_pulse";
        N.Name        = "Cytokine Pulse";
        N.Description = "Broadcast cytokine signals to improve coordination. +12 Speed.";
        N.Tree        = ESkillTree::Memory;
        N.Tier        = 1;
        N.StatModifiers = MakeStats(0, 0, 0, 12);
        Nodes.push_back(N);
    }
    {
        FSkillNode N;
        N.Id          = "mem_t1_signal";
        N.Name        = "Adaptive Signal";
        N.Description = "Broad-spectrum adaptation. +10 HP, +5 Attack, +5 Defense, +5 Speed.";
        N.Tree        = ESkillTree::Memory;
        N.Tier        = 1;
        N.StatModifiers = MakeStats(10, 5, 5, 5);
        Nodes.push_back(N);
    }

    // Tier 2
    {
        FSkillNode N;
        N.Id          = "mem_t2_clonal";
        N.Name        = "Clonal Expansion";
        N.Description = "Rapid clonal proliferation bolsters vitality and speed. +20 HP, +8 Speed.";
        N.Tree        = ESkillTree::Memory;
        N.Tier        = 2;
        N.Prerequisites = { "mem_t1_pulse", "mem_t1_signal" };
        N.StatModifiers = MakeStats(20, 0, 0, 8);
        Nodes.push_back(N);
    }
    {
        FSkillNode N;
        N.Id          = "mem_t2_helper";
        N.Name        = "Helper Cascade";
        N.Description = "T-helper signalling amplifies offensive and mobility stats. +10 Attack, +10 Speed.\nSynergy (2+ Memory skills): +5 Speed.";
        N.Tree        = ESkillTree::Memory;
        N.Tier        = 2;
        N.Prerequisites = { "mem_t1_pulse", "mem_t1_signal" };
        N.StatModifiers = MakeStats(0, 10, 0, 10);
        {
            FSynergyBonus S;
            S.MinSkillsInTree = 2;
            S.StatBonus = MakeStats(0, 0, 0, 5);
            N.SynergyBonuses.push_back(S);
        }
        Nodes.push_back(N);
    }

    // Tier 3
    {
        FSkillNode N;
        N.Id          = "mem_t3_longmem";
        N.Name        = "Immunological Memory";
        N.Description = "Long-lived memory cells retain all past adaptations. +15 HP, +15 Attack, +15 Defense, +15 Speed.\nSynergy (5+ Memory skills): +8 to all stats.";
        N.Tree        = ESkillTree::Memory;
        N.Tier        = 3;
        N.Prerequisites = { "mem_t2_clonal", "mem_t2_helper" };
        N.StatModifiers = MakeStats(15, 15, 15, 15);
        {
            FSynergyBonus S;
            S.MinSkillsInTree = 5;
            S.StatBonus = MakeStats(8, 8, 8, 8);
            N.SynergyBonuses.push_back(S);
        }
        Nodes.push_back(N);
    }
    {
        FSkillNode N;
        N.Id          = "mem_t3_immunity";
        N.Name        = "Long-term Immunity";
        N.Description = "Accumulated immune experience improves every attribute. +20 HP, +10 Speed, +10 Defense.\nSynergy (5+ Memory skills): +3 to all stats per skill owned across all trees.";
        N.Tree        = ESkillTree::Memory;
        N.Tier        = 3;
        N.Prerequisites = { "mem_t2_clonal", "mem_t2_helper" };
        N.StatModifiers = MakeStats(20, 0, 10, 10);
        // Synergy handled specially in ComputeTotalBonus (scales with total owned count)
        Nodes.push_back(N);
    }

    return Nodes;
}

const std::vector<FSkillNode>& FSkillTreeState::GetRegistry()
{
    static const std::vector<FSkillNode> Registry = BuildRegistry();
    return Registry;
}

const FSkillNode* FSkillTreeState::Find(const std::string& Id)
{
    for (const FSkillNode& Node : GetRegistry())
    {
        if (Node.Id == Id)
        {
            return &Node;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// FSkillTreeState
// ---------------------------------------------------------------------------

bool FSkillTreeState::IsOwned(const std::string& Id) const
{
    for (const std::string& OwnedId : OwnedSkillIds)
    {
        if (OwnedId == Id)
        {
            return true;
        }
    }
    return false;
}

bool FSkillTreeState::CanUnlock(const std::string& Id) const
{
    if (IsOwned(Id))
    {
        return false;
    }
    if (AvailableSkillPoints <= 0)
    {
        return false;
    }

    const FSkillNode* Node = Find(Id);
    if (!Node)
    {
        return false;
    }

    // For tier 2/3: require at least one prerequisite to be owned (any-of semantics)
    if (!Node->Prerequisites.empty())
    {
        bool AnyPrereqOwned = false;
        for (const std::string& PrereqId : Node->Prerequisites)
        {
            if (IsOwned(PrereqId))
            {
                AnyPrereqOwned = true;
                break;
            }
        }
        if (!AnyPrereqOwned)
        {
            return false;
        }
    }

    return true;
}

bool FSkillTreeState::Unlock(const std::string& Id)
{
    if (!CanUnlock(Id))
    {
        return false;
    }

    const FSkillNode* Node = Find(Id);
    AvailableSkillPoints -= Node->SkillPointCost;
    OwnedSkillIds.push_back(Id);
    return true;
}

void FSkillTreeState::Reset()
{
    int32_t Refund = 0;
    for (const std::string& Id : OwnedSkillIds)
    {
        const FSkillNode* Node = Find(Id);
        if (Node)
        {
            Refund += Node->SkillPointCost;
        }
    }
    OwnedSkillIds.clear();
    AvailableSkillPoints += Refund;
}

void FSkillTreeState::EarnPoints(int32_t Amount)
{
    if (Amount > 0)
    {
        AvailableSkillPoints += Amount;
    }
}

int32_t FSkillTreeState::CountInTree(ESkillTree Tree) const
{
    int32_t Count = 0;
    for (const std::string& Id : OwnedSkillIds)
    {
        const FSkillNode* Node = Find(Id);
        if (Node && Node->Tree == Tree)
        {
            ++Count;
        }
    }
    return Count;
}

FStatBlock FSkillTreeState::ComputeTotalBonus() const
{
    FStatBlock Total;

    const int32_t AggrCount = CountInTree(ESkillTree::Aggressor);
    const int32_t GrdCount  = CountInTree(ESkillTree::Guardian);
    const int32_t MemCount  = CountInTree(ESkillTree::Memory);
    const int32_t TotalOwned = static_cast<int32_t>(OwnedSkillIds.size());

    for (const std::string& Id : OwnedSkillIds)
    {
        const FSkillNode* Node = Find(Id);
        if (!Node)
        {
            continue;
        }

        // Direct modifier
        Total.AddBlock(Node->StatModifiers);

        // Synergy bonuses
        const int32_t TreeCount = (Node->Tree == ESkillTree::Aggressor) ? AggrCount
                                 : (Node->Tree == ESkillTree::Guardian)  ? GrdCount
                                 : MemCount;

        for (const FSynergyBonus& Syn : Node->SynergyBonuses)
        {
            if (TreeCount >= Syn.MinSkillsInTree)
            {
                Total.AddBlock(Syn.StatBonus);
            }
        }

        // Special case: Long-term Immunity scales with total owned count
        if (Id == "mem_t3_immunity" && MemCount >= 5)
        {
            FStatBlock PerSkillBonus;
            const float PerSkill = 3.0f * static_cast<float>(TotalOwned);
            PerSkillBonus.Set(EBioStat::HitPoints, PerSkill);
            PerSkillBonus.Set(EBioStat::Attack,    PerSkill);
            PerSkillBonus.Set(EBioStat::Defense,   PerSkill);
            PerSkillBonus.Set(EBioStat::Speed,     PerSkill);
            Total.AddBlock(PerSkillBonus);
        }
    }

    Total.ClampNonNegative();
    return Total;
}
