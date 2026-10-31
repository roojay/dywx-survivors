# Windows Shipping Trace removal and courtyard art integration

The player package now defaults to Win64 Shipping with UE Trace disabled. The new courtyard art is integrated, compiled, cooked, and archived with UE 5.8.4. The new art package has **not** received GUI or visual acceptance; the older Trace-only package's successful interaction checks do not certify the new art.

## Source and build identity

- Trace-only source: `4a657aa4cb25af00f228ef97cd449c152e3c0488` (normal main push verified).
- Art import: `1917c24d3776a4a927b02395d81b5ef98fe23e4f`, tree `3db08acbd5e9ce272fb1a16a02c9cdd42a4a9f1c`.
- Art checkpoint archive: `92256cc3557ab2cc47deb7e02dac8e4954e80ed0`; delta bundle SHA256 `a83342b0878ff0f97ed991696692f17f47a016f3f68db463aa157ec0d8672c3f`. Bundle verify and git fsck passed before integration.
- Compiled code: merge `1e65b528234aacd59b8ba32d27abbc502439139d` (art plus Trace changes, no conflict).
- Build contents: `acf43552a1b40f8ace1a789abc0958d081c8b48c`. This adds the 15 actual bootstrap-generated UE material, texture, and audio assets to the compiled code commit; no C++ change follows compilation.
- Engine: UE 5.8.4 CL58902052, installed `D:/epic/UE_5.8`; MSVC 14.50.35725, Windows SDK 10.0.26100.0.
- Build: `scripts/package-windows.ps1`, default Shipping; Editor remains Development for asset bootstrap.

## Trace result

Setting `bEnableTrace=false` for a Development game target was tested and rejected by UBT because installed shared UnrealGame build products require the same Trace/debugger flags. No engine files, build-environment override, credentials, firewall, or system security settings were changed. Shipping uses the installed engine's supported Trace-disabled configuration. Generated Shipping definitions contain `UE_TRACE_ENABLED 0`, `UE_TRACE_FORCE_ENABLED 0`, and `UE_BUILD_SHIPPING 1`.

Development game PID 69084 previously listened on TCP `0.0.0.0:1985`. After the Shipping change, the real Shipping game PID 99148 and bootstrap PID 92628 had **no TCP listeners and no UDP endpoints**, both at menu and during battle. The top-level EXE was launched without any Trace command-line workaround. The new art Shipping game PID 94156 and bootstrap PID 86732 also had no TCP listeners or UDP endpoints at startup. See the separate listener JSON evidence. This checks the process endpoints, not merely the absence of a firewall popup.

The Trace-only Shipping build was actually operated: menu, default body character, Thunder Marsh quick mode, battle, directional movement, dash (cooldown observed), pause/resume, natural death, result scrolling, retry (fresh 120/120 HP, wave 1), and return to menu/normal exit. These observations used the successfully authorized application handle. Screenshots remain in local `trace-disabled-20261031/`.

The previous firewall prompt did not expose its expanded application path, so repeated prompts cannot be conclusively attributed to path changes. Two prior executable paths were used in earlier test batches. No claim is made that a rebuild alone caused repeated prompts.

## Art build and resource checks

| Stage | Result | Evidence |
| --- | --- | --- |
| Art restore, HEAD/tree, bundle verify, fsck | PASS | expected IDs above |
| UE Editor/UHT/UBT | PASS | `fullgame-build-art/UBT-Editor.log` |
| Material/texture/audio bootstrap | PASS | `MVP_MATERIAL_BOOTSTRAP_OK`; 15 saved assets |
| Win64 Shipping UBT | PASS | `fullgame-build-art/UAT/UBA-DywxSurvivorsEditor-Win64-Development.txt` includes the Shipping target |
| Cook/Stage/Archive | PASS | `fullgame-build-art/UAT-console.log`; BuildCookRun 154.44 s, exit 0 |
| Courtyard material and textures | PASS | staged UFS manifest includes M_Courtyard, T_CourtyardStone, T_CourtyardRune |
| Geometry and HUD atlas | PASS | packaged scene.json and hud-skills.png match source SHA256 |
| Local geometry/material source contracts | PASS | 38 meshes, 62,268 triangles; unique UObject naming and render contracts |
| New art process startup and endpoints | PASS | live Shipping PID and empty TCP/UDP endpoint lists |
| New art GUI battle / retry / saves | NOT_RUN | old authorized handle invalid after restarting game |
| New art 720p / 1080p screenshots, layout, shadows, frame time and GPU performance | NOT_RUN | no new window authorization requested |
| Windows Pillow font / atlas tests | NOT_RUN | `ModuleNotFoundError: No module named 'PIL'`; no dependency installed |
| Full natural campaign / all heroes and maps | NOT_RUN | outside this build and startup check |

No compilation or material API fix was needed for the imported art. Non-fatal C4305 double-to-float warnings remain in art C++ literals. Shipping uses its standard `USE_LOGGING_IN_SHIPPING=0`; lack of a runtime log is not proof that all runtime paths are correct. Cook and bootstrap logs are available separately.

`Art/Blender/MVP/arena-preview.png` is the offline Blender preview. It is not a UE screenshot. The prior revision's screenshots and frame measurements must not be used as evidence for this new art package.

## Current interaction limitation

The original Shipping CUA authorization succeeded and was used. After that game exited normally and the integrated package started at the same executable path, the old window handle's read-only `getAXState()` returned `foreground window did not report a process id`. The task explicitly prohibited triggering another approval card. Consequently no new `getApp` request or alternate UI automation was attempted. The new-art GUI check is deferred, not denied, and there is no pending approval request from this step. Build, resource checks, source publication, and local delivery were completed independently.

## Local delivery and rollback

The fixed runnable entry remains `D:/dev/Codex/2026-10-30/task-2/fullgame-package-winfix/Windows/DywxSurvivors.exe`. It now contains the integrated art Shipping build. Launch the top-level EXE with the full directory present; do not copy only the executable.

The original Development crash-fix package is preserved under `trace-disabled-20261031/rollback-Windows-f92a037/`. The tested Trace-only Shipping package is preserved under `trace-disabled-20261031/rollback-Windows-shipping-4a657aa/` and `fullgame-package-notrace/Windows/`. Tests used isolated user directories; real saves were not deleted or edited.

The complete local ZIP, byte count, SHA256, CRC result, and file manifest are recorded in `package-summary.json`. The ZIP contains the required runtime files, no PDBs or private test saves. No executable, Release, or Space upload was performed.
