#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BioTypes.generated.h"

UENUM(BlueprintType)
enum class ECellType : uint8
{
    Aggressor UMETA(DisplayName = "Aggressor (DPS)"),
    Guardian UMETA(DisplayName = "Guardian (Tank)"),
    Memory UMETA(DisplayName = "Memory (Support)")
};

USTRUCT(BlueprintType)
struct FBiologicalStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Stats")
    int32 MaxHealth = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Stats")
    int32 CurrentHealth = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Stats")
    int32 AttackPower = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Stats")
    int32 Defense = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Stats")
    int32 MoveRange = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Stats")
    float InflammationCost = 0.0f;
};

UCLASS(BlueprintType)
class UBioMutation : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Mutation")
    FString MutationName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Mutation", meta = (MultiLine = true))
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio|Mutation")
    FBiologicalStats StatModifiers;
};
