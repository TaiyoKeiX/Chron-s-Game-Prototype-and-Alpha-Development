#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BioTypes.h"
#include "BioUnitBase.generated.h"

class UStaticMeshComponent;
class UWidgetComponent;

UCLASS()
class ABioUnitBase : public AActor
{
    GENERATED_BODY()

public:
    ABioUnitBase();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Components")
    UStaticMeshComponent* UnitMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Components")
    UWidgetComponent* UnitWidget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Unit")
    FBiologicalStats BaseStats;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Unit")
    TArray<UBioMutation*> AppliedMutations;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Unit")
    FIntPoint GridCoordinates = FIntPoint::ZeroValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Unit")
    int32 TeamID = 0;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Bio|Unit")
    FBiologicalStats GetEffectiveStats() const;
    virtual FBiologicalStats GetEffectiveStats_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Bio|Unit")
    void MoveToGrid(FIntPoint NewCoords);
    virtual void MoveToGrid_Implementation(FIntPoint NewCoords);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Bio|Unit")
    void TakeDamage(int32 Amount);
    virtual void TakeDamage_Implementation(int32 Amount);
};
