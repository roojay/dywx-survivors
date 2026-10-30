# Static UI preview rendering budget

## Evidence and scope

The reported 99% GPU utilization has **not** been reproduced or profiled here.
The previous controller did not cap frames, and it continued to render its world
under the full-screen preview. These are source-confirmed waste risks, not a
proven explanation for the user's GPU reading. No GPU utilization improvement is
claimed until the packaged Windows build is measured.

Only the static menu/character/map/battle-preview path is changed. No renderer
quality CVars, Lumen settings, driver settings, OS settings, or live-gameplay
quality defaults are changed.

## Lifecycle

- On first entry into preview, save current engine max FPS, viewport world-render
  flag, cursor visibility, and viewport input/capture/lock settings.
- Suppress world rendering, but keep the Slate UI rendering normally.
- In standalone/packaged builds, impose a maximum of 60 FPS foreground, 15 FPS
  inactive, 5 FPS minimized. Keep an existing stricter positive user limit.
- Slate application-activation delegate refreshes the budget. A 0.25-second core
  ticker checks native-window minimization and recovery even if the world pauses.
- On `BeginLiveHUD`, restore preview-owned state and unregister both hooks. The
  established live `GameAndUI` route remains, using saved mouse capture/lock
  preferences. Pause modals remain interactive and do not disable world rendering.
- On return to a static menu, capture live settings afresh and apply preview policy.
- On controller teardown during preview, restore saved state and remove callbacks.
  Cap restoration is conditional on it still matching our last applied value, so
  a newer external cap is not overwritten on release. While preview is active,
  an external cap edit becomes the new saved baseline before reapplying the
  preview budget, so later focus changes and release preserve that preference.
- PIE does not change engine max FPS because the engine is shared with the editor.
  Standalone/package is required for FPS acceptance. No benchmark/fixed-time-step
  launch overrides should be used for this acceptance.

`LogDywxPreviewBudget` records requested/read-back cap, active/minimized state,
world-render flag, and release. Readback proves a configured cap, **not measured
FPS or GPU utilization**.

## Verified in the dot environment

`bash scripts/test-preview-budget.sh` compiles the dependency-free policy under
C++20 with warnings as errors and tests all three state-dependent limits, external-cap baseline adoption,
and lower pre-existing limits. This does not compile the Unreal controller.

## Pending Windows acceptance (requires available computer and permission)

1. Compile/package with the project's UE version; retain UBT and package logs.
2. Run the packaged executable, record process identity, GPU, display resolution,
   refresh rate, and UI DPI. Measure FPS and per-process GPU utilization for each
   of the four static UI screens after warm-up. Expect FPS no higher than 60
   (or a saved stricter limit), not a particular absolute GPU percentage.
3. Alt-tab away: verify <=15 FPS and logs. Minimize: verify <=5 FPS and logs.
   Restore and refocus: verify responsive input and return to <=60 FPS.
4. Open/close a preview modal and repeat focus/minimize transitions; verify that
   no pause state prevents budget updates.
5. Use the live-HUD integration harness/actual gameplay to verify visible world,
   restored frame limit, capture/lock settings, movement, HUD/pause input, then
   return to menu and repeat. The current UI demo alone cannot validate real combat.
6. Destroy/recreate the controller or transition levels; verify no stale callbacks
   and no retained world-render suppression. Test an existing stricter 30 FPS cap.
7. If GPU remains high at the measured cap, profile Slate draw cost, resolution,
   texture upload and GPU frame time rather than assuming the cap solved it.

API references: [UGameViewportClient](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UGameViewportClient),
[FGenericWindow](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/ApplicationCore/FGenericWindow),
[FTSTicker](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Core/FTSTicker).
