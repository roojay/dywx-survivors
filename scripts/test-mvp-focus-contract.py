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
budget = body(w, 'void ADywxMVPWorld::RefreshRunBudget')
tick = body(w, 'void ADywxMVPWorld::Tick')
stop = body(w, 'void ADywxMVPWorld::StopRun')
pause = body(c, 'void ADywxUIPlayerController::SetUIPaused')
clear = body(c, 'void ADywxUIPlayerController::ClearGameplayInput')
ui_only = body(body(c, 'void ADywxUIPlayerController::ApplyUIInput'), 'if (!bLive || bModalOpen)')
checks = {
    'pause entry clears queued dash and stale held keys': 'if (bPaused) ClearGameplayInput()' in pause and all(s in clear for s in ['bDashQueued = false', 'if (PlayerInput) PlayerInput->FlushPressedKeys()', 'MVPWorld->SetInput({})']) and '#include \"GameFramework/PlayerInput.h\"' in c,
    'UIOnly entry clears keys even for non-pausing economy': ui_only.index('ClearGameplayInput()') < ui_only.index('SetInputMode(Input)'),
    'flush only at transition entry, not each tick or upgrade': c.count('PlayerInput->FlushPressedKeys()') == 1 and 'FlushPressedKeys' not in body(c, 'void ADywxUIPlayerController::PlayerTick') and 'FlushPressedKeys' not in body(c, 'void ADywxUIPlayerController::ChooseUpgrade') and 'ClearGameplayInput' not in body(c, 'void ADywxUIPlayerController::PlayerTick') and 'ClearGameplayInput' not in body(c, 'void ADywxUIPlayerController::ChooseUpgrade'),
    'one live tick owner; no live ticker/delegate': 'AddTicker' not in w and 'AddUObject' not in w,
    'focus checked before simulation step': tick.index('RefreshRunBudget()') < tick.index('Simulation.Step'),
    'native minimized/foreground and Slate active': all(s in budget for s in ['IsMinimized()', 'IsForegroundWindow()', 'IsActive()']),
    'shared budget preserves external baseline and accepted cap': all(s in budget for s in ['Dywx::PreviewBaseline', 'Dywx::PreviewFPS', 'AppliedFPS=GEngine->GetMaxFPS()', 'bExternalCapChanged ||']),
    'PIE cap excluded': 'if(GIsEditor || !GEngine) return' in budget,
    'focus loss pauses, clears input, and never auto-resumes': all(s in budget for s in ['Input={}', 'if(!GetSnapshot().paused)', 'Simulation.Pause(true)', 'bFocusPauseRequested=true']) and 'Pause(false)' not in w,
    'sticky request survives tick ordering and short focus loss': 'ConsumeFocusPauseRequest' in h and 'bFocusPauseRequested=false' in h and 'MVPWorld->ConsumeFocusPauseRequest()' in c,
    'existing pause menu is idempotent, not Escape toggle': 'void HandleFocusLoss();' in u and 'Nav.FocusLoss()' in (R/'Source/DywxUI/Private/SDywxRoot.cpp').read_text() and 'RootWidget->HandleFocusLoss()' in c,
    'stop restores only owned exact cap and prior view': all(s in stop for s in ['GEngine->GetMaxFPS()==AppliedFPS', 'GEngine->SetMaxFPS(SavedFPS)', 'PC->GetViewTarget()==this', 'PreviousView.IsValid()', 'SetActorTickEnabled(false)']),
    'preview never starts over live paused scene': 'MVPWorld && MVPWorld->IsRunActive()' in body(c, 'void ADywxUIPlayerController::StartPreviewBudget') and 'bDisableWorldRendering' not in w,
    'preview released before world starts': body(c, 'void ADywxUIPlayerController::RequestRun').index('StopPreviewBudget()') < body(c, 'void ADywxUIPlayerController::RequestRun').index('MVPWorld->StartFullRun'),
    'world released before menu reacquires preview': body(c, 'void ADywxUIPlayerController::ShowMainMenu').index('MVPWorld->StopRun()') < body(c, 'void ADywxUIPlayerController::ShowMainMenu').index('RootWidget->ReturnToMenu()'),
}
for label, passed in checks.items():
    assert passed, label
    print('PASS:', label)
print('NOT_RUN: Unreal compile, actual focus/minimize/input transitions, rendered pause scene, FPS/GPU measurements')
