#pragma once
#include "CoreMinimal.h"
#include "DywxHUDState.generated.h"

USTRUCT(BlueprintType)
struct DYWXUI_API FDywxHUDState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float Health = 100.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float MaxHealth = 100.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float Experience = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float ExperienceToNext = 100.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float ElapsedSeconds = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") int32 Wave = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float DashCooldownRemaining = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float DashCooldownTotal = 6.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") TArray<int32> SkillLevels = {1, 0, 0, 0, 0, 0};
};
