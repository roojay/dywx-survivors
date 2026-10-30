#include "DywxUIPlayerController.h"
#include "SDywxRoot.h"
#include "MVP/DywxMVPWorld.h"
#include "MVP/Simulation.h"
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
    if (MVPWorld) MVPWorld->StopRun();
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
    if (RootWidget.IsValid()) RootWidget->BeginLiveHUD(State);
    ApplyUIInput(true, false);
}
void ADywxUIPlayerController::ShowMainMenu()
{
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
void ADywxUIPlayerController::RequestRun(int32 CharacterIndex, int32 MapIndex, int32 ModeIndex)
{
    if (MapIndex != 0 || ModeIndex != 0 || CharacterIndex < 0 || CharacterIndex > 1 || !GetWorld()) return;
    LastHero = CharacterIndex;
    StopPreviewBudget();
    if (!MVPWorld) MVPWorld = GetWorld()->SpawnActor<ADywxMVPWorld>();
    if (!MVPWorld) { StartPreviewBudget(); return; }
    MVPWorld->StartRun(CharacterIndex == 0 ? dywx::mvp::HeroId::Body : dywx::mvp::HeroId::Sword, uint64(FDateTime::UtcNow().GetTicks()));
    BeginLiveHUD(FDywxHUDState());
    RefreshRunUI();
    OnRunRequested.Broadcast(CharacterIndex, MapIndex, ModeIndex);
}
void ADywxUIPlayerController::RequestDash() { bDashQueued = true; OnDashRequested.Broadcast(); }
void ADywxUIPlayerController::SetUIPaused(bool bPaused)
{
    if (bPaused)
    {
        // Pause entry is an event (including the one-shot focus-loss request),
        // not a per-frame flush. UIOnly may never deliver held-key releases.
        bDashQueued = false;
        if (PlayerInput) PlayerInput->FlushPressedKeys();
        if (MVPWorld) MVPWorld->SetInput({});
    }
    if (Simulation()) Simulation()->Pause(bPaused);
    OnPauseChanged.Broadcast(bPaused);
}
void ADywxUIPlayerController::ApplyUIInput(bool bLive, bool bModalOpen)
{
    // A live pause is an overlay over the real scene, not a preview screen.
    bLive = bLive || (MVPWorld && MVPWorld->IsRunActive());
    if (bLive) StopPreviewBudget();
    else StartPreviewBudget();
    bShowMouseCursor = true; // Upgrade cards remain clickable while combat continues.
    if (!bLive || bModalOpen)
    {
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
}
void ADywxUIPlayerController::ConfirmQuit()
{
    UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void ADywxUIPlayerController::StartPreviewBudget()
{
    if (bPreviewBudgetActive || (MVPWorld && MVPWorld->IsRunActive()) || !IsLocalController() || !GetWorld()) return;
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
    Viewport->bDisableWorldRendering = true;
    if (FSlateApplication::IsInitialized())
        ActivationHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(
            this, &ADywxUIPlayerController::HandleApplicationActivation);
    // Core ticker continues when the world is paused. Also detects minimize/restore,
    // which is not identical to application activation (e.g. multiple app windows).
    PreviewTicker = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &ADywxUIPlayerController::TickPreviewBudget), 0.25f);
    RefreshPreviewBudget();
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


dywx::mvp::Simulation* ADywxUIPlayerController::Simulation() { return MVPWorld ? &MVPWorld->GetSimulation() : nullptr; }
void ADywxUIPlayerController::RefreshRunUI() { if (MVPWorld && RootWidget.IsValid()) RootWidget->SetSnapshot(MVPWorld->GetSnapshot()); }
void ADywxUIPlayerController::RetryRun() { RequestRun(LastHero,0,0); }
void ADywxUIPlayerController::ChooseUpgrade(int32 Index) { if (Simulation() && !Simulation()->GetSnapshot().paused) { Simulation()->ChooseUpgrade(Index); RefreshRunUI(); } }
void ADywxUIPlayerController::UpgradeOne() { ChooseUpgrade(0); }
void ADywxUIPlayerController::UpgradeTwo() { ChooseUpgrade(1); }
void ADywxUIPlayerController::UpgradeThree() { ChooseUpgrade(2); }
void ADywxUIPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if (!MVPWorld) return;
    if (MVPWorld->IsRunActive() && RootWidget.IsValid() && MVPWorld->ConsumeFocusPauseRequest())
        RootWidget->HandleFocusLoss();
    dywx::mvp::InputState Input;
    if (MVPWorld->IsWindowForeground() && !MVPWorld->GetSnapshot().paused && FSlateApplication::IsInitialized() && FSlateApplication::Get().IsActive())
    {
        Input.move.x = float(IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right)) - float(IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left));
        Input.move.y = float(IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Up)) - float(IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down));
        Input.dash = bDashQueued || IsInputKeyDown(EKeys::SpaceBar);
    }
    bDashQueued = false;
    MVPWorld->SetInput(Input);
    HUDRefreshElapsed += DeltaTime;
    if (HUDRefreshElapsed >= .05f) { HUDRefreshElapsed = 0.f; RefreshRunUI(); }
}
