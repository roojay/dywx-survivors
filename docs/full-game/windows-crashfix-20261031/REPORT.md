# Windows battle-entry crash fix — 2026-10-31

The packaged battle-entry crash is fixed and the repaired package has been exercised beyond startup. Build source: `f92a037cbbcb897d11a0cc945e06a470c34433dc`; UE 5.8.4 CL 58902052, Windows Development, MSVC 14.50 and SDK 10.0.26100.

This supersedes the interaction-NOT_RUN state in `../windows-validation-20261031/REPORT.md`. The previous package built from 739466 is known to crash on first battle entry and should not be used. 739466 to 655e01 changed only validation documents/status, not runtime source/config/assets.

## Confirmed cause and repairs

The original 11:17:23 UTC fatal names `InstancedStaticMeshComponent ...rock` replacing `StaticMesh ...rock`, with MakeBatch -> BuildScene -> ActivateRun -> RequestRun -> SDywxRoot::Primary. These objects shared an Outer; FName is case-insensitive. Ten resource/batch keys overlap, including Rock/rock, Pillar/pillar and eight Enemy roles. Components now use NAME_None so Unreal assigns unique UObject names; business map keys remain unchanged. A source-contract regression enumerates the overlaps. Actual packaged scene construction created all 37 meshes and 94 batches without the fatal.

The first repaired entry exposed two independent runtime faults, both fixed before final validation:

- Runtime fast-built untextured, zero-UV meshes had uninitialized UVChannelData, causing a handled StaticMesh.cpp streaming ensure. The slot now explicitly initializes zero UV density. Final runs have no such ensure.
- Manual exposure was applying default physical camera f/4, 1/60s, ISO100 to relative scene lights. Disabling postprocessing diagnostically revealed the scene. The final code disables physical-camera exposure only; final validation uses normal postprocessing, no rendering workaround command.

Original crash XML, minidump, log, matching PDB, profile and old package were preserved locally. None of the crash dump, private profile or full crash context is published.

## Actual packaged validation

- PASS: fresh menu -> Sword -> ThunderMarsh/Quick -> visible battle; discrete WASD input, dash cooldown/displacement and automatic attacks.
- PASS: natural deaths at 26s/24 kills and 30s/34 kills, fatal-hit description, scroll to both result actions, retry returning to fresh HP/wave.
- PASS: 720p and 1080p sampled HUD, pause and results; Chinese Sarasa glyphs rendered. Scrollable panels require scrolling at the small viewport. This is functional layout sampling, not full visual-quality acceptance.
- PASS: minimize causes pause and readback 5 FPS; restore leaves pause open. Foreground 60, background 15 FPS readbacks; previous unchanged lower-cap check remains 10 FPS.
- PASS with an isolated fixture: resume wave 2 with pending upgrade, choose key 1 (palm rank 1 -> 2), choose event, unequip/re-equip relic, save-and-leave, exit process, restart, resume wave 3 with rank 2 and XP 15/31 preserved. The fixture uses no spawns and 4-second waves; it does not demonstrate natural progression or balance. Its source is included only for reproducibility; no fixture save is in the executable ZIP.
- PASS: all three final normal-render runs construct the scene and exit normally, with no Fatal, Ensure or LogDywxMVPWorld Error. Prior CMake 22/22 and targeted MSVC ASan remain applicable; current changes touch only UE presentation, generated assets and the render contract.
- NOT_RUN: complete natural campaign, every hero/map/boss combination, sustained hold-input test, long-duration performance/stress, filesystem failure/power-loss injection, human gameplay/balance and full art acceptance.

## Performance sample

RTX 3080, 1920x1080, 3,733 foreground frames spanning pause, live combat and death result: frame mean 16.6742ms, P95 16.6707ms, max 41.4793ms; GPU mean 2.5470ms, P95 3.1861ms, max 3.7745ms. This is a mixed smoke capture, not a large-wave stress benchmark. Raw CSV remains local.

## Firewall finding

A live process socket query observed TCP 0.0.0.0:1985 owned by the game and no UDP endpoints. The runtime log says `LogTrace: Control listening on port 1985`; UE TraceLog Control.cpp and Trace/Config.h identify it as Development tracing control. Mounting UDP/TCP Messaging plugins alone does not establish an active transport. No system firewall, credentials, proxy or security settings were changed; no network plugins were blindly removed.

The exact reason for each historical prompt is not proven. Windows program rules use the full executable path, so different extraction/test paths can require different rules. Rebuild alone is not enough to establish this explanation. Reusing the current path did not produce an observed new prompt in these tests. A future non-development distribution can avoid development trace services; this fix keeps the validated Development build.

References: [Epic Trace control](https://dev.epicgames.com/documentation/unreal-engine/trace?application_version=4.27), [Microsoft firewall rules](https://learn.microsoft.com/en-us/windows/security/operating-system-security/network-security/windows-firewall/rules), [Epic auto exposure](https://dev.epicgames.com/documentation/unreal-engine/auto-exposure-in-unreal-engine).

## Local deliverable

`dywx-survivors-fullgame-win64-crashfix-f92a037.zip`: 372,916,730 bytes, SHA256 `7ec2f06430ba8a5cc22602b2f9d1761997270195f9549eb18891af94b7dbfe24`. CRC check passed, 62 entries; no PDB/private saves. Extract the full directory and launch `Windows/DywxSurvivors.exe`; keep DLL/Engine/Content files together. Package was not uploaded to Release, Space or any external file service. Source commit f92a037 was pushed normally and verified on remote main. This report/status commit changes no compiled source, configuration or game assets.
