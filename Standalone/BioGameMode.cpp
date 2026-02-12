#include "BioGameMode.h"

void BioGameMode::AdvanceTurn()
{
    switch (CurrentPhase)
    {
        case ETurnPhase::PlayerInput:
            CurrentPhase = ETurnPhase::EnemyAI;
            break;
        case ETurnPhase::EnemyAI:
            CurrentPhase = ETurnPhase::Environment;
            break;
        case ETurnPhase::Environment:
        default:
            CurrentPhase = ETurnPhase::PlayerInput;
            ++TurnCount;
            break;
    }
}

void BioGameMode::EndPlayerTurn()
{
    if (CurrentPhase == ETurnPhase::PlayerInput)
    {
        AdvanceTurn();
    }
}
