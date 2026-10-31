#!/usr/bin/env python3
"""Static lifecycle regressions only. Not a UE compile, focus event, or GPU test."""
from pathlib import Path
R = Path(__file__).resolve().parents[1]
w = (R/'Source/DywxUI/Private/MVP/DywxMVPWorld.cpp').read_text()
h = (R/'Source/DywxUI/Public/MVP/DywxMVPWorld.h').read_text()
c = (R/'Source/DywxUI/Private/DywxUIPlayerController.cpp').read_text()
u = (R/'Source/DywxUI/Private/SDywxRoot.h').read_text()
def body(source, signature):
    start = source.index('{', source.index(signature))
    depth = 1
    for end in range(start+1, len(source)):
        depth += (source[end] == '{') - (source[end] == '}')
        if not depth:
            return source[start:end+1]
    raise AssertionError(signature)
root = (R/'Source/DywxUI/Private/SDywxRoot.cpp').read_text()
begin_hud = body(c, 'void ADywxUIPlayerController::BeginLiveHUD')
apply_input = body(c, 'void ADywxUIPlayerController::ApplyUIInput')
session = (R/'Source/DywxRuntime/Private/MVP/DywxRunSession.cpp').read_text()
budget = body(c, 'void ADywxUIPlayerController::RefreshPreviewBudget')
tick = body(session, 'void UDywxRunSession::Tick')
prepare = body(c, 'void ADywxUIPlayerController::PrepareSimulationTick')
stop = body(w, 'void ADywxMVPWorld::StopRun')
release = body(c, 'void ADywxUIPlayerController::StopPreviewBudget')
pause = body(c, 'void ADywxUIPlayerController::SetUIPaused')
clear = body(c, 'void ADywxUIPlayerController::ClearGameplayInput')
ui_only = body(body(c, 'void ADywxUIPlayerController::ApplyUIInput'), 'if (!bLive || bModalOpen)')
close_modal = body(root, 'FReply SDywxRoot::CloseModal')
checks = {
    'explicit resume cannot undo a nested focus-loss pause': close_modal.index('Owner->SetUIPaused(false)') < close_modal.index('SyncInput()'),
    'live HUD preserves navigation-selected modal input mode': 'if (RootWidget.IsValid()) RootWidget->BeginLiveHUD(State);\n    else ApplyUIInput(true, false);' in begin_hud and begin_hud.count('ApplyUIInput(') == 1 and 'Rebuild();SyncInput();' in body(root, 'void SDywxRoot::BeginLiveHUD') and 'Nav.modal!=dywx::Modal::None' in body(root, 'void SDywxRoot::SyncInput'),
    'focus navigation cannot be overwritten after nested input-mode change': 'RefreshPreviewBudget()' not in body(c, 'void ADywxUIPlayerController::StartPreviewBudget') and apply_input.rstrip().endswith('RefreshPreviewBudget();\n}') and 'Rebuild();SyncInput();' in body(root, 'void SDywxRoot::HandleFocusLoss'),
    'pause entry clears queued dash and stale held keys': 'if (bPaused) ClearGameplayInput()' in pause and all(s in clear for s in ['bDashQueued = false', 'if (PlayerInput) PlayerInput->FlushPressedKeys()', 'Session()->SetInput({})']) and '#include "GameFramework/PlayerInput.h"' in c,
    'UIOnly entry clears keys even for non-pausing economy': ui_only.index('ClearGameplayInput()') < ui_only.index('SetInputMode(Input)'),
    'flush only through clear helper, not each player tick or upgrade': c.count('PlayerInput->FlushPressedKeys()') == 1 and 'ClearGameplayInput' not in body(c, 'void ADywxUIPlayerController::PlayerTick') and 'ClearGameplayInput' not in body(c, 'void ADywxUIPlayerController::ChooseUpgrade'),
    'one live integration owner; world renderer never steps': 'Simulation.Step' not in w and tick.count('Simulation.Step') == 1 and 'Simulation.Step' not in c,
    'local focus/input sampled immediately before simulation step': tick.index('BeforeSimulationTick.ExecuteIfBound()') < tick.index('Simulation.Step') and prepare.index('RefreshPreviewBudget()') < prepare.index('Session()->SetInput(Input)') and 'SetBeforeSimulationTick(FSimpleDelegate::CreateUObject(this, &ADywxUIPlayerController::PrepareSimulationTick))' in c,
    'native minimized/foreground and Slate active': all(s in budget for s in ['IsMinimized()', 'IsForegroundWindow()', 'IsActive()']),
    'shared budget preserves external baseline and accepted cap': all(s in budget for s in ['Dywx::PreviewBaseline', 'Dywx::PreviewFPS', 'AppliedMaxFPS = GEngine->GetMaxFPS()', 'bExternalCapChanged ||']),
    'PIE cap excluded': 'if (!GIsEditor && (bExternalCapChanged ||' in budget and '!GIsEditor && GEngine' in release,
    'focus loss pauses once per transition and never auto-resumes': all(s in budget for s in ['bWasForeground || !Session()->GetSnapshot().paused', 'ClearGameplayInput()', 'Session()->Pause(true)', 'RootWidget->HandleFocusLoss()']) and 'Pause(false)' not in budget,
    'existing pause menu is idempotent, not Escape toggle': 'void HandleFocusLoss();' in u and 'Nav.FocusLoss()' in (R/'Source/DywxUI/Private/SDywxRoot.cpp').read_text(),
    'release restores only owned exact cap and prior view': all(s in release for s in ['GEngine->GetMaxFPS() == AppliedMaxFPS', 'GEngine->SetMaxFPS(SavedMaxFPS)']) and all(s in stop for s in ['PC->GetViewTarget()==this', 'PreviousView.IsValid()', 'SetActorTickEnabled(false)']),
    'live pause preserves scene instead of preview suppression': 'Viewport->bDisableWorldRendering = bRunActive ? bSavedWorldRenderingDisabled : true' in budget and 'bDisableWorldRendering' not in w,
    'modal and background input not sampled': 'bGameplayInputAllowed && bWindowForeground && !Session()->GetSnapshot().paused' in prepare,
    'single budget remains bound across menu/run transitions': c.count('AddTicker(') == 1 and 'StopPreviewBudget()' not in body(c, 'void ADywxUIPlayerController::RequestRun') and 'StopPreviewBudget()' not in body(c, 'void ADywxUIPlayerController::ShowMainMenu'),
    'world released before menu reacquires preview': body(c, 'void ADywxUIPlayerController::ShowMainMenu').index('MVPWorld->StopRun()') < body(c, 'void ADywxUIPlayerController::ShowMainMenu').index('RootWidget->ReturnToMenu()'),
    'pre-step delegate removed during teardown': 'ClearBeforeSimulationTick()' in body(c, 'void ADywxUIPlayerController::EndPlay'),
}
for label, passed in checks.items():
    assert passed, label
    print('PASS:', label)
print('NOT_RUN: Unreal compile, actual focus/minimize/input transitions, rendered pause scene, FPS/GPU measurements')
