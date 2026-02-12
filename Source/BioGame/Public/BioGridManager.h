#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BioGridManager.generated.h"

UCLASS()
class ABioGridManager : public AActor
{
    GENERATED_BODY()

public:
    ABioGridManager();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Grid")
    FIntPoint GridSize = FIntPoint(8, 8);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Grid")
    float TileSize = 100.0f;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Bio|Grid")
    int32 GetManhattanDistance(FIntPoint A, FIntPoint B) const;
    virtual int32 GetManhattanDistance_Implementation(FIntPoint A, FIntPoint B) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Bio|Grid")
    FIntPoint WorldToGrid(FVector WorldLoc) const;
    virtual FIntPoint WorldToGrid_Implementation(FVector WorldLoc) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Bio|Grid")
    FVector GridToWorld(FIntPoint GridLoc) const;
    virtual FVector GridToWorld_Implementation(FIntPoint GridLoc) const;
};
