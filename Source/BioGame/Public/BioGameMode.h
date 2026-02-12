#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BioGameMode.generated.h"

UENUM(BlueprintType)
enum class ETurnPhase : uint8
{
    PlayerInput UMETA(DisplayName = "Player Input"),
    Execution UMETA(DisplayName = "Execution"),
    EnemyAI UMETA(DisplayName = "Enemy AI"),
    Environment_Inflammation UMETA(DisplayName = "Environment: Inflammation")
};

UCLASS()
class ABioGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ABioGameMode();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Turn")
    ETurnPhase CurrentPhase = ETurnPhase::PlayerInput;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Turn")
    int32 TurnCount = 1;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Bio|Turn")
    void AdvanceTurn();
    virtual void AdvanceTurn_Implementation();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Bio|Turn")
    void EndPlayerTurn();
    virtual void EndPlayerTurn_Implementation();
};
