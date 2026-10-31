#pragma once
#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Engine/EngineBaseTypes.h"
#include "GameFramework/PlayerController.h"
#include "DywxHUDState.h"
#include "DywxUIPlayerController.generated.h"

class SDywxRoot;
class ADywxMVPWorld;
namespace dywx::mvp { class Simulation; struct CampaignProfile; enum class LegacyUnlock; enum class PermanentStat; }
class UGameViewportClient;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDywxRunRequested, int32, CharacterIndex, int32, MapIndex, int32, ModeIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDywxUIAction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDywxPauseChanged, bool, bPaused);

UCLASS()
class DYWXUI_API ADywxUIPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual void PlayerTick(float DeltaTime) override;
    dywx::mvp::Simulation* Simulation();
    void RefreshRunUI();
    void RetryRun();
    const dywx::mvp::CampaignProfile& CampaignProfile() const;
    bool PurchaseLegacy(dywx::mvp::LegacyUnlock Unlock);
    bool PurchasePermanent(dywx::mvp::PermanentStat Stat);
    bool SaveRun();
    bool ResumeRun(int32 Hero);
    bool HasSavedRun(int32 Hero) const;
    void ChooseUpgrade(int32 Index);
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
    void RequestRun(int32 CharacterIndex, int32 MapIndex, int32 ModeIndex, uint32 RiskMask=0);
    void RequestDash();
    void SetUIPaused(bool bPaused);
    void ApplyUIInput(bool bLive, bool bModalOpen);
    void ConfirmQuit();
private:
    TSharedPtr<SDywxRoot> RootWidget;
    UPROPERTY() TObjectPtr<ADywxMVPWorld> MVPWorld;
    void ClearGameplayInput();
    bool bDashQueued = false;
    float HUDRefreshElapsed = 0.f;
    int32 LastHero = 0, LastMap = 0, LastMode = 0;
    uint32 LastRisk = 0;
    void UpgradeOne();
    void UpgradeTwo();
    void UpgradeThree();
    // Preview owns these overrides only until live gameplay resumes or this controller exits.
    TWeakObjectPtr<UGameViewportClient> PreviewViewport;
    FTSTicker::FDelegateHandle PreviewTicker;
    FDelegateHandle ActivationHandle;
    bool bPreviewBudgetActive = false;
    bool bSavedWorldRenderingDisabled = false;
    bool bSavedMouseCursor = false;
    bool bSavedIgnoreInput = false;
    bool bSavedHideCursorDuringCapture = false;
    EMouseCaptureMode SavedMouseCaptureMode = EMouseCaptureMode::NoCapture;
    EMouseLockMode SavedMouseLockMode = EMouseLockMode::DoNotLock;
    float SavedMaxFPS = 0.f;
    float AppliedMaxFPS = -1.f;
    void StartPreviewBudget();
    void StopPreviewBudget();
    void RefreshPreviewBudget();
    bool TickPreviewBudget(float DeltaSeconds);
    void HandleApplicationActivation(bool bActive);
    void HandleEscape();
    void HandleDash();
};
