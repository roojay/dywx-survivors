# Independent UE integration source review

2026-10-30. Scope: `Source/DywxUI`, runtime geometry, bootstrap and Windows packaging wiring against the paired gameplay/config baseline. This is **source/API review, not UHT/UBT, editor, cook, GPU or Windows execution**. No UE installation or Windows device was used.

## Findings and disposition

1. **Malformed runtime geometry (high confidence, fixed in source during review).** Initial loader cast arbitrary JSON doubles to `int32` before checking integer/range/finite constraints and narrowed arbitrary coordinates to floats. Out-of-range conversion was undefined; huge coordinates could enter mesh building as infinity. Current loader checks schema version, allowlisted nonduplicate names, array bounds, typed numeric conversion, finite bounded positions/colors/normals, and integral in-range indices before casting. Required runtime failure-injection tests still need UE.
2. **Bootstrap could report success with missing content (high confidence, fixed in source during review).** Empty/missing AudioSource glob produced a success receipt and runtime missing sound loads were silent. Geometry was not a bootstrap prerequisite either, allowing a diagnostic-silhouette package to count as success. Current bootstrap runs geometry/staging validation, requires all ten cue sources, verifies imported SoundWave types and saves/runtime paths; PowerShell verifies all ten sound asset files. Runtime now logs missing resources.
3. **Mesh build return was ignored (high confidence, fixed in source during review).** UE5.8 `BuildFromMeshDescriptions` returns bool. Current source checks its result and rejects empty/all-degenerate descriptions before registering successful meshes. Invalid-resource fallback is not acceptance of authored visuals.

## Wiring checked in source

- Runtime module dependencies include Core/CoreUObject/Engine/InputCore and private Slate, SlateCore, ImageWrapper, RenderCore, MeshDescription, StaticMeshDescription, Json. Reflected headers put generated include last. No new reflected standard-library members; pure-C++ Simulation/Types are ordinary members. C++20 enabled; no explicit exceptions/RTTI-dependent paths found. This does not replace UHT or linking.
- Controller RequestRun releases preview budget, spawns/reuses the actual world actor, starts authoritative simulation, and installs live HUD. Live battle deliberately omits the background image. There is a genuine actor/component scene path, not a static preview.
- Actor takes/restores view target and gameplay FPS ownership; camera yaw −90 plus core `(x,y)`→UE`(100x,-100y)` agrees with D/right and W/up. Orthographic dimensions/follow are computed from current viewport. Aspect extremes and camera near-plane still need rendered verification.
- Controller polls movement and supplies a queued dash; world delegates fixed-step accumulation to Simulation (no second accumulator). UI snapshot sampling is 20Hz, copies progression/player/statistics but not entity vectors, and scans enemies only for boss health. Render uses authoritative snapshot each actor tick.
- Pause freezes Simulation, not Unreal world; render freezes visual time. Live modal input retains gameplay budget rather than re-entering the hidden-world menu budget. Menu stop precedes preview budget acquisition. EndPlay stops run, removes core ticker/delegates and root Slate widget.
- UI buttons carry rebuild epochs; upgrade buttons additionally check run generation/current offer. Synchronous commands refresh snapshots and rebuild. Retry increments simulation generation and renderer clears ID-based transient history. Focus changes, rapid clicks, held keys across pause/alt-tab remain target-runtime checks, not source-proven behavior.
- ISMs have physics/overlap disabled; core is sole collision authority. Six obstacle centers/radii are shared through GetArenaObstacles. Authored irregular rock silhouette approximates its circular core collider rather than proving pixel-perfect collision; arena-edge/pillar placement is consistent with outer boundary intent.
- Runtime meshes, MIDs, ISMs, sounds and concurrency have UPROPERTY strong ownership; Slate textures use TStrongObjectPtr. ISM raw batch pointers have parallel owned component refs. Components registered with AddInstanceComponent; restart reuses scene rather than accumulating actors/components.
- Runtime mesh build uses named material slot, vertex attributes, fast-build path, no simple collision. Material bootstrap sets ISM usage **before save/cook**, builds both materials, and is invoked between editor build and BuildCookRun. PythonScriptPlugin/EditorScriptingUtilities are enabled only for Editor targets. Geometry stages NonUFS; `/Game/MVP` is always cooked.

## Official API checks

- [UE5.8 UStaticMesh](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UStaticMesh?lang=en-US): bool BuildFromMeshDescriptions with MeshDescription list and params.
- [UE5.8 build parameters](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UStaticMesh/FBuildMeshDescriptionsParams): fast path mandatory outside editor; CPU/collision flags available.
- [UE5.8 FJsonValue](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Json/FJsonValue): typed TryGet methods; AsNumber defaults to zero on incompatible values, so it is not validation.
- [UE5.8 Python index](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/genindex-U): Material used_with_instanced_static_meshes property.
- [UE5.8 editor Python](https://dev.epicgames.com/documentation/unreal-engine/scripting-the-unreal-editor-using-python): commandlet syntax and required enabled Python plugin.

## Required target validation before playable-build acceptance

1. UHT/UBT both Editor and game; execute bootstrap and inspect material/audio assets and receipt; cook/package with warnings reviewed.
2. Launch packaged output without source/editor assets. Verify every resource, colored ISMs (not fallback checkerboards), normals/lighting, world camera, Chinese UI and audio.
3. Repeat menu→both heroes→pause/resume→rest/equip/refine/undo→boss/death/win→retry/menu; alt-tab/minimize/resize and held movement/dash keys. Force GC during each phase and teardown/map reload.
4. Corrupt copies of geometry: wrong version/type, huge/fractional index, oversized/NaN-like coordinates, duplicate/reserved names, missing required mesh, empty/all-degenerate triangles. Verify controlled logs/fallback, not crashes.
5. Observe each telegraph before actual harm, movement against all six colliders and edges, and camera framing at minimum/wide/tall aspect ratios. Profile 100+ enemies and saturated effects on target hardware; a 60FPS cap is not a performance result.

## Final local evidence

Independent rerun: `python scripts/validate-mvp-render-contract.py` passed with **15 meshes / 15,440 triangles**; `git diff --check` passed. These validate data and source contracts only. Renderer owner fixes above were re-read, including pre-read 32MiB file cap. No additional high-confidence compile/API defect was found, but only real UHT/UBT can establish compilation. All target validation gates remain open.

## Profile persistence follow-up (source review)

`FDywxMVPProfile` and actor integration reviewed after initial report:

- Loader bounds file size to64KiB, requires version1, clamps finite pity to0–20 before integer cast, reads only boolean elements into fixed-size discovery arrays. Missing/corrupt/unsupported input falls back to empty discoveries and zero pity; no arbitrary array indexing or unbounded profile-state copy. Caller warns if an existing profile fails to load; ordinary first-run absence is quiet.
- Serialized fields are version, pityWaves, spells/relics/fusions discovery flags only. Although the in-memory holder is ProgressionState, Observe copies only those fields; Apply invokes only SetPityWaves and MergeDiscoveries. Inventory, XP, affixes, spell ranks, combat state and hero choice do not persist. A new hero retains its freshly reset starter rather than receiving old build vectors.
- Actor saves/observes active old run **before** Simulation.Start resets it; loads once per actor, then applies persisted values **after** Start and refreshes snapshot. StopRun captures current values; EndPlay also retries a dirty save after an earlier StopRun. Changed wave/phase and a10-second interval capture progress during an active run. Thus ordinary retry/menu transitions preserve pity instead of resetting it to zero. Load-once also preserves latest in-memory discoveries/pity after read-only save failures.
- Save leaves dirty state set on serialization/write/move failure; actor warns and retries at subsequent boundary/timer. Gameplay remains available with in-memory state. No assertion here establishes actual filesystem behavior.
- **Crash-atomic replacement is not established.** Code writes a same-directory temporary then IFileManager::Move with replace. [UE5.8 IFileManager](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Core/IFileManager) promises move/rename, not atomic replacement/durability; [IPlatformFile](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Core/IPlatformFile) documents that its MoveFile does not overwrite existing files. Do not describe the present mechanism as guaranteed crash-atomic. Crash/power-loss recovery, simultaneous game instances, readonly directories and packaged Saved-path behavior require target tests. Current lightweight profile is best-effort local persistence, not a transactional save system.

Target profile matrix: absent/empty/badJSON/wrongversion/oversized/fractionalpity/out-of-range pity/mixed-type flags; readonly profile and directory; retry/hero switch after discoveries and pity advances/resets; orderly exit/relaunch; interrupted temp-write/replace. UE profile IO tests are **not_run**.
