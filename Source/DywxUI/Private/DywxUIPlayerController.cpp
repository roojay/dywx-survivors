#include "DywxUIPlayerController.h"
#include "SDywxRoot.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
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
void ADywxUIPlayerController::RequestRun(int32 Character, int32 Map, int32 Mode)
{
    OnRunRequested.Broadcast(Character, Map, Mode);
}
void ADywxUIPlayerController::RequestDash() { OnDashRequested.Broadcast(); }
void ADywxUIPlayerController::SetUIPaused(bool bPaused)
{
    // The owning game decides whether to pause a world or only open its local menu.
    OnPauseChanged.Broadcast(bPaused);
}
void ADywxUIPlayerController::ApplyUIInput(bool bLive, bool bModalOpen)
{
    bShowMouseCursor = !bLive || bModalOpen;
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
        Input.SetHideCursorDuringCapture(false);
        Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(Input);
    }
}
void ADywxUIPlayerController::ConfirmQuit()
{
    UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
