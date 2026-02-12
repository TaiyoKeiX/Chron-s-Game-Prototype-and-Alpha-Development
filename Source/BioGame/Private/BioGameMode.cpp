#include "BioGameMode.h"

ABioGameMode::ABioGameMode()
{
    CurrentPhase = ETurnPhase::PlayerInput;
    TurnCount = 1;
}

void ABioGameMode::AdvanceTurn_Implementation()
{
    switch (CurrentPhase)
    {
        case ETurnPhase::PlayerInput:
            CurrentPhase = ETurnPhase::Execution;
            break;
        case ETurnPhase::Execution:
            CurrentPhase = ETurnPhase::EnemyAI;
            break;
        case ETurnPhase::EnemyAI:
            CurrentPhase = ETurnPhase::Environment_Inflammation;
            break;
        case ETurnPhase::Environment_Inflammation:
        default:
            CurrentPhase = ETurnPhase::PlayerInput;
            ++TurnCount;
            break;
    }
}

void ABioGameMode::EndPlayerTurn_Implementation()
{
    if (CurrentPhase == ETurnPhase::PlayerInput)
    {
        AdvanceTurn();
    }
}
