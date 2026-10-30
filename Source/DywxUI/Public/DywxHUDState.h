#pragma once
#include "CoreMinimal.h"
#include "DywxHUDState.generated.h"

USTRUCT(BlueprintType)
struct DYWXUI_API FDywxHUDState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float Health = 780.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float MaxHealth = 1000.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float Experience = 320.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float ExperienceToNext = 1000.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float ElapsedSeconds = 102.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") int32 Wave = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float DashCooldownRemaining = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float DashCooldownTotal = 3.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") TArray<int32> SkillLevels = {2, 1, 1};
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") bool bHasStone = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") bool bHasMirror = true;
};
