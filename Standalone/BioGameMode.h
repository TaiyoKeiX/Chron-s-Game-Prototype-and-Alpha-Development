#pragma once

#include <cstdint>

enum class ETurnPhase : uint8_t
{
    PlayerInput,
    EnemyAI,
    Environment
};

class BioGameMode
{
public:
    ETurnPhase CurrentPhase = ETurnPhase::PlayerInput;
    int32_t TurnCount = 1;

    void AdvanceTurn();
    void EndPlayerTurn();
};
