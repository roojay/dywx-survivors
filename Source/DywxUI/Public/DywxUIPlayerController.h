#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DywxHUDState.h"
#include "DywxUIPlayerController.generated.h"

class SDywxRoot;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDywxRunRequested, int32, CharacterIndex, int32, MapIndex, int32, ModeIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDywxUIAction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDywxPauseChanged, bool, bPaused);

UCLASS()
class DYWXUI_API ADywxUIPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    // Call on the game thread. Does not simulate health, combat, time or cooldowns.
    UFUNCTION(BlueprintCallable, Category="道衍万象|UI") void UpdateBattleHUD(const FDywxHUDState& State);
    // Replaces the preview background with the actual world and passes movement input through.
    UFUNCTION(BlueprintCallable, Category="道衍万象|UI") void BeginLiveHUD(const FDywxHUDState& State);
    UFUNCTION(BlueprintCallable, Category="道衍万象|UI") void ShowMainMenu();
    UFUNCTION(BlueprintCallable, Category="道衍万象|UI") void SetEndlessUnlocked(bool bUnlocked);
    UPROPERTY(BlueprintAssignable, Category="道衍万象|UI") FDywxRunRequested OnRunRequested;
    UPROPERTY(BlueprintAssignable, Category="道衍万象|UI") FDywxUIAction OnDashRequested;
    UPROPERTY(BlueprintAssignable, Category="道衍万象|UI") FDywxPauseChanged OnPauseChanged;
    UPROPERTY(BlueprintAssignable, Category="道衍万象|UI") FDywxUIAction OnReturnToMenu;
    void RequestRun(int32 Character, int32 Map, int32 Mode);
    void RequestDash();
    void SetUIPaused(bool bPaused);
    void ApplyUIInput(bool bLive, bool bModalOpen);
    void ConfirmQuit();
private:
    TSharedPtr<SDywxRoot> RootWidget;
    void HandleEscape();
    void HandleDash();
};
