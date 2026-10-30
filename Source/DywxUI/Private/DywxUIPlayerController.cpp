#include "DywxUIPlayerController.h"
#include "SDywxRoot.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "GenericPlatform/GenericWindow.h"
#include "DywxPreviewBudget.h"

DEFINE_LOG_CATEGORY_STATIC(LogDywxPreviewBudget, Log, All);
#include "Components/InputComponent.h"
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
        InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ADywxUIPlayerController::HandleDash);
    }
}
void ADywxUIPlayerController::HandleEscape() { if (RootWidget.IsValid()) RootWidget->HandleEscape(); }
void ADywxUIPlayerController::HandleDash() { if (RootWidget.IsValid()) RootWidget->HandleDash(); }
void ADywxUIPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
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
    OnRunRequested.Broadcast(CharacterIndex, MapIndex, ModeIndex);
}
void ADywxUIPlayerController::RequestDash() { OnDashRequested.Broadcast(); }
void ADywxUIPlayerController::SetUIPaused(bool bPaused)
{
    // The owning game decides whether to pause a world or only open its local menu.
    OnPauseChanged.Broadcast(bPaused);
}
void ADywxUIPlayerController::ApplyUIInput(bool bLive, bool bModalOpen)
{
    if (bLive) StopPreviewBudget();
    else StartPreviewBudget();
    bShowMouseCursor = !bLive || bModalOpen || bSavedMouseCursor;
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
    if (!GIsEditor && GEngine && FMath::IsNearlyEqual(GEngine->GetMaxFPS(), AppliedMaxFPS))
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
