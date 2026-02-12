#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

enum class ECellType : uint8_t
{
    Aggressor,
    Guardian,
    Memory
};

struct FIntPoint
{
    int32_t X = 0;
    int32_t Y = 0;

    constexpr FIntPoint() = default;
    constexpr FIntPoint(int32_t InX, int32_t InY) : X(InX), Y(InY) {}

    static const FIntPoint ZeroValue;
};

struct FVector
{
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;

    constexpr FVector() = default;
    constexpr FVector(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}
};

inline FVector operator+(const FVector& A, const FVector& B)
{
    return FVector(A.X + B.X, A.Y + B.Y, A.Z + B.Z);
}

inline FVector operator-(const FVector& A, const FVector& B)
{
    return FVector(A.X - B.X, A.Y - B.Y, A.Z - B.Z);
}

enum class EBioStat : uint8_t
{
    HitPoints,
    Attack,
    Defense,
    Speed,
    Count
};

struct FStatBlock
{
    std::array<float, static_cast<size_t>(EBioStat::Count)> Values{};

    float Get(EBioStat Stat) const;
    void Set(EBioStat Stat, float Value);
    void Add(EBioStat Stat, float Value);
    void AddBlock(const FStatBlock& Other);
    void ClampNonNegative();
};

namespace BioStatMath
{
    float ComputeMoveRange(float Speed);
    float ComputeDodgeChance(float Speed);
    float ComputeDefenseMitigation(float Defense);
}

class UBioMutation
{
public:
    std::string MutationName;
    std::string Description;
    FStatBlock StatModifiers;
};
