#include "DywxUIPlayerController.h"
#include "SDywxRoot.h"
#include "MVP/DywxMVPWorld.h"
#include "MVP/Simulation.h"
#include "MVP/DywxRunSession.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "GenericPlatform/GenericWindow.h"
#include "DywxPreviewBudget.h"

DEFINE_LOG_CATEGORY_STATIC(LogDywxPreviewBudget, Log, All);
#include "Components/InputComponent.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetSystemLibrary.h"

void ADywxUIPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (IsLocalController() && GetWorld() && GetWorld()->GetGameViewport())
    {
        MVPWorld = GetWorld()->SpawnActor<ADywxMVPWorld>();
        if (auto* RunSession = Session())
        {
            RunSession->InitializeProfile();
            RunSession->SetBeforeSimulationTick(FSimpleDelegate::CreateUObject(this, &ADywxUIPlayerController::PrepareSimulationTick));
            if (MVPWorld) MVPWorld->InitializePresentation(RunSession, this);
        }
        RootWidget = SNew(SDywxRoot).Owner(this);
        GetWorld()->GetGameViewport()->AddViewportWidgetContent(RootWidget.ToSharedRef(), 10);
        ApplyUIInput(false, false);
    }
}
void ADywxUIPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (InputComponent)
    {
        InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ADywxUIPlayerController::HandleEscape).bExecuteWhenPaused = true;
        InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ADywxUIPlayerController::UpgradeOne);
        InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ADywxUIPlayerController::UpgradeTwo);
        InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ADywxUIPlayerController::UpgradeThree);
        InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ADywxUIPlayerController::HandleDash);
    }
}
void ADywxUIPlayerController::HandleEscape() { if (RootWidget.IsValid()) RootWidget->HandleEscape(); }
void ADywxUIPlayerController::HandleDash() { if (RootWidget.IsValid()) RootWidget->HandleDash(); }
void ADywxUIPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* RunSession = Session()) RunSession->StopRun();
    if (MVPWorld) MVPWorld->StopRun();
    if (auto* RunSession = Session()) RunSession->ClearBeforeSimulationTick();
    StopPreviewBudget();
    if (RootWidget.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(RootWidget.ToSharedRef());
    RootWidget.Reset();
    Super::EndPlay(Reason);
}
void ADywxUIPlayerController::UpdateBattleHUD(const FDywxHUDState& State)
{
    if (RootWidget.IsValid()) RootWidget->SetHUDState(State);
}
void ADywxUIPlayerController::BeginLiveHUD(const FDywxHUDState& State)
{
    // Root owns navigation and SyncInput, including a focus-loss pause modal.
    // Do not overwrite its resulting input mode with an unconditional live mode.
    if (RootWidget.IsValid()) RootWidget->BeginLiveHUD(State);
    else ApplyUIInput(true, false);
}
void ADywxUIPlayerController::ShowMainMenu()
{
    if (auto* RunSession = Session()) RunSession->StopRun();
    if (MVPWorld) MVPWorld->StopRun();
    bDashQueued = false;
    if (RootWidget.IsValid()) RootWidget->ReturnToMenu();
    SetUIPaused(false);
    ApplyUIInput(false, false);
    OnReturnToMenu.Broadcast();
}
void ADywxUIPlayerController::SetEndlessUnlocked(bool bUnlocked)
{
    if (RootWidget.IsValid()) RootWidget->SetEndlessUnlocked(bUnlocked);
}
const dywx::mvp::CampaignProfile& ADywxUIPlayerController::CampaignProfile() const
{
    static const dywx::mvp::CampaignProfile Empty;
    return Session() ? Session()->GetCampaignProfile() : Empty;
}
bool ADywxUIPlayerController::PurchaseLegacy(dywx::mvp::LegacyUnlock U) { return Session() && Session()->PurchaseLegacy(U); }
bool ADywxUIPlayerController::PurchasePermanent(dywx::mvp::PermanentStat S) { return Session() && Session()->PurchasePermanent(S); }
bool ADywxUIPlayerController::SaveRun() { return Session() && Session()->SaveRun(); }
bool ADywxUIPlayerController::HasSavedRun(int32 H) const { return Session() && Session()->HasSavedRun(H); }
bool ADywxUIPlayerController::ResumeRun(int32 H)
{
    if (!MVPWorld || !Session()) return false;
    if (!Session()->ResumeRun(H)) return false;
    MVPWorld->ActivateRun();
    const auto& C=Simulation()->GetCampaign().GetState();
    LastHero=H; LastMap=int32(Simulation()->GetArena().cave); LastMode=int32(C.mode); LastRisk=C.riskMask;
    BeginLiveHUD(FDywxHUDState()); RefreshRunUI(); return true;
}
void ADywxUIPlayerController::RequestRun(int32 CharacterIndex, int32 MapIndex, int32 ModeIndex, uint32 RiskMask)
{
    if (!IsLocalController() || !GetWorld() || GetNetMode() != NM_Standalone || !Session() || !MVPWorld) return;
    if (Session() && Session()->HasSavedRun(CharacterIndex)) { if(RootWidget.IsValid())RootWidget->ShowNotice(TEXT("此修士已有进行中存档，请先续接；未覆盖任何存档。")); return; }
    if (!Session()->StartFullRun(CharacterIndex,MapIndex,ModeIndex,RiskMask))
    {
        // A failed retry keeps the completed run and its pending settlement visible.
        // Likewise a repeated start cannot navigate away from a still-active run.
        if (RootWidget.IsValid()) {
            if (!Session()->IsRunActive()) RootWidget->ReturnToMenu();
            RootWidget->ShowNotice(TEXT("无法开始：检查修士、洞天、模式、劫数解锁或存档状态。原修行未被替换。"));
        }
        StartPreviewBudget(); return;
    }
    MVPWorld->ActivateRun();
    LastHero=CharacterIndex; LastMap=MapIndex; LastMode=ModeIndex; LastRisk=RiskMask;
    BeginLiveHUD(FDywxHUDState()); RefreshRunUI();
    OnRunRequested.Broadcast(CharacterIndex, MapIndex, ModeIndex);
}
void ADywxUIPlayerController::RequestDash() {
    if (Simulation() && Simulation()->IsCombatFrozen()) return;
    bDashQueued = true; OnDashRequested.Broadcast();
}
void ADywxUIPlayerController::ClearGameplayInput()
{
    bDashQueued = false;
    if (PlayerInput) PlayerInput->FlushPressedKeys();
    if (Session()) Session()->SetInput({});
}
void ADywxUIPlayerController::SetUIPaused(bool bPaused)
{
    // Also clear immediately for focus-loss requests, before the input mode changes.
    if (bPaused) ClearGameplayInput();
    if (Session()) Session()->Pause(bPaused);
    OnPauseChanged.Broadcast(bPaused);
}
void ADywxUIPlayerController::ApplyUIInput(bool bLive, bool bModalOpen)
{
    // A live pause is an overlay over the real scene, not a preview screen.
    bLive = bLive || (Session() && Session()->IsRunActive());
    bGameplayInputAllowed = bLive && !bModalOpen;
    StartPreviewBudget();
    bShowMouseCursor = true; // Upgrade cards remain clickable while combat continues.
    if (!bLive || bModalOpen)
    {
        // Economy is UIOnly without pausing the Rest simulation. Clear on mode
        // entry too: key releases while UIOnly may never reach PlayerInput.
        ClearGameplayInput();
        FInputModeUIOnly Input;
        if (RootWidget.IsValid()) Input.SetWidgetToFocus(RootWidget);
        Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(Input);
    }
    else
    {
        FInputModeGameAndUI Input;
        if (RootWidget.IsValid()) Input.SetWidgetToFocus(RootWidget);
        Input.SetHideCursorDuringCapture(bSavedHideCursorDuringCapture);
        Input.SetLockMouseToViewportBehavior(SavedMouseLockMode);
        SetInputMode(Input);
        if (GetWorld() && GetWorld()->GetGameViewport())
        {
            GetWorld()->GetGameViewport()->SetMouseCaptureMode(SavedMouseCaptureMode);
            GetWorld()->GetGameViewport()->SetIgnoreInput(bSavedIgnoreInput);
        }
    }
    // Refresh last: focus-loss navigation may install a modal input mode recursively.
    RefreshPreviewBudget();
}
void ADywxUIPlayerController::ConfirmQuit()
{
    UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void ADywxUIPlayerController::StartPreviewBudget()
{
    if (bPreviewBudgetActive || !IsLocalController() || !GetWorld()) return;
    UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
    if (!Viewport) return;
    PreviewViewport = Viewport;
    bSavedWorldRenderingDisabled = Viewport->bDisableWorldRendering;
    bSavedMouseCursor = bShowMouseCursor;
    bSavedIgnoreInput = Viewport->IgnoreInput();
    bSavedHideCursorDuringCapture = Viewport->HideCursorDuringCapture();
    SavedMouseCaptureMode = Viewport->GetMouseCaptureMode();
    SavedMouseLockMode = Viewport->GetMouseLockMode();
    SavedMaxFPS = GEngine ? GEngine->GetMaxFPS() : 0.f;
    AppliedMaxFPS = -1.f;
    bPreviewBudgetActive = true;
    // Slate still draws; do not submit the entirely obscured Entry/preview world.
    Viewport->bDisableWorldRendering = !(Session() && Session()->IsRunActive());
    if (FSlateApplication::IsInitialized())
        ActivationHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(
            this, &ADywxUIPlayerController::HandleApplicationActivation);
    // Core ticker continues when the world is paused. Also detects minimize/restore,
    // which is not identical to application activation (e.g. multiple app windows).
    PreviewTicker = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &ADywxUIPlayerController::TickPreviewBudget), 0.25f);
    // ApplyUIInput refreshes after installing the input mode, never during setup.
}

void ADywxUIPlayerController::StopPreviewBudget()
{
    if (!bPreviewBudgetActive) return;
    bPreviewBudgetActive = false;
    FTSTicker::GetCoreTicker().RemoveTicker(PreviewTicker);
    PreviewTicker.Reset();
    if (FSlateApplication::IsInitialized())
        FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(ActivationHandle);
    ActivationHandle.Reset();
    if (UGameViewportClient* Viewport = PreviewViewport.Get())
    {
        Viewport->bDisableWorldRendering = bSavedWorldRenderingDisabled;
        Viewport->SetIgnoreInput(bSavedIgnoreInput);
        Viewport->SetMouseCaptureMode(SavedMouseCaptureMode);
        Viewport->SetMouseLockMode(SavedMouseLockMode);
        Viewport->SetHideCursorDuringCapture(bSavedHideCursorDuringCapture);
    }
    bShowMouseCursor = bSavedMouseCursor;
    // Never replace a newer cap supplied by gameplay/settings while the menu was open.
    if (!GIsEditor && GEngine && GEngine->GetMaxFPS() == AppliedMaxFPS)
        GEngine->SetMaxFPS(SavedMaxFPS);
    UE_LOG(LogDywxPreviewBudget, Log, TEXT("Preview released: maxFPS=%.1f; world/input restored"),
        GEngine ? GEngine->GetMaxFPS() : -1.f);
    PreviewViewport.Reset();
}

void ADywxUIPlayerController::HandleApplicationActivation(bool /*bActive*/)
{
    // Read the authoritative current Slate/window state, not time since last input.
    RefreshPreviewBudget();
}

bool ADywxUIPlayerController::TickPreviewBudget(float /*DeltaSeconds*/)
{
    RefreshPreviewBudget();
    return bPreviewBudgetActive;
}

void ADywxUIPlayerController::RefreshPreviewBudget()
{
    if (!bPreviewBudgetActive || !GEngine || !FSlateApplication::IsInitialized()) return;
    UGameViewportClient* Viewport = PreviewViewport.Get();
    if (!Viewport) return;
    const TSharedPtr<SWindow> Window = Viewport->GetWindow();
    const bool bMinimized = Window.IsValid() && Window->GetNativeWindow().IsValid()
        && Window->GetNativeWindow()->IsMinimized();
    const bool bActive = FSlateApplication::Get().IsActive()
        && (!Window.IsValid() || !Window->GetNativeWindow().IsValid()
            || Window->GetNativeWindow()->IsForegroundWindow());
    const bool bWasForeground = bWindowForeground;
    bWindowForeground = bActive && !bMinimized;
    const bool bRunActive = Session() && Session()->IsRunActive();
    Viewport->bDisableWorldRendering = bRunActive ? bSavedWorldRenderingDisabled : true;
    if (bRunActive && !bWindowForeground && (bWasForeground || !Session()->GetSnapshot().paused))
    {
        ClearGameplayInput();
        Session()->Pause(true);
        if (RootWidget.IsValid()) RootWidget->HandleFocusLoss();
    }
    const float CurrentMaxFPS = GEngine->GetMaxFPS();
    const bool bExternalCapChanged = AppliedMaxFPS >= 0.f && CurrentMaxFPS != AppliedMaxFPS;
    if (!GIsEditor)
        SavedMaxFPS = Dywx::PreviewBaseline(SavedMaxFPS, AppliedMaxFPS, CurrentMaxFPS);
    const float TargetFPS = Dywx::PreviewFPS(bActive, bMinimized, SavedMaxFPS);
    // PIE shares its engine with the editor: do not impose a process-wide editor cap.
    if (!GIsEditor && (bExternalCapChanged || !FMath::IsNearlyEqual(AppliedMaxFPS, TargetFPS)))
    {
        GEngine->SetMaxFPS(TargetFPS);
        AppliedMaxFPS = GEngine->GetMaxFPS();
        UE_LOG(LogDywxPreviewBudget, Log,
            TEXT("Preview budget: active=%d minimized=%d requestedFPS=%.1f readbackFPS=%.1f worldDisabled=%d"),
            bActive, bMinimized, TargetFPS, AppliedMaxFPS, Viewport->bDisableWorldRendering);
    }
}


UDywxRunSession* ADywxUIPlayerController::Session() const
{
    return IsLocalController() && GetWorld() && GetNetMode() == NM_Standalone
        ? GetWorld()->GetSubsystem<UDywxRunSession>() : nullptr;
}
const dywx::mvp::Simulation* ADywxUIPlayerController::Simulation() const { return Session() ? Session()->GetSimulation() : nullptr; }
void ADywxUIPlayerController::RefreshRunUI() { if (Session() && RootWidget.IsValid()) RootWidget->SetSnapshot(Session()->GetSnapshot()); }
void ADywxUIPlayerController::RetryRun() { RequestRun(LastHero,LastMap,LastMode,LastRisk); }
void ADywxUIPlayerController::ChooseUpgrade(int32 Index) { if (Simulation() && !Simulation()->GetSnapshot().paused) { Session()->ChooseUpgrade(Index); RefreshRunUI(); } }
void ADywxUIPlayerController::UpgradeOne() { ChooseUpgrade(0); }
void ADywxUIPlayerController::UpgradeTwo() { ChooseUpgrade(1); }
void ADywxUIPlayerController::UpgradeThree() { ChooseUpgrade(2); }
void ADywxUIPlayerController::PrepareSimulationTick()
{
    RefreshPreviewBudget();
    if (!Session()) return;
    // Reading upgrade choices must not queue movement/dash for the resume frame.
    // Core owns the freeze even if this controller or the Slate widget is absent.
    if (Simulation() && Simulation()->IsAwaitingUpgrade()) {
        ClearGameplayInput();
        return;
    }
    dywx::mvp::InputState Input;
    if (bGameplayInputAllowed && bWindowForeground && !Session()->GetSnapshot().paused && FSlateApplication::IsInitialized() && FSlateApplication::Get().IsActive())
    {
        Input.move.x = float(IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right)) - float(IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left));
        Input.move.y = float(IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Up)) - float(IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down));
        Input.dash = bDashQueued || IsInputKeyDown(EKeys::SpaceBar);
    }
    bDashQueued = false;
    Session()->SetInput(Input);
}
void ADywxUIPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if (!IsLocalController() || !Session()) return;
    HUDRefreshElapsed += DeltaTime;
    if (HUDRefreshElapsed >= .05f) { HUDRefreshElapsed = 0.f; RefreshRunUI(); }
}
