# UE world adapter / render contract

## What is implemented in source

`ADywxMVPWorld` is a native actor that owns the pure C++ `Simulation`. The controller supplies core-space movement (+X right, +Y up) and dash. `Simulation::Step` owns the 60 Hz fixed accumulator; the actor supplies capped frame delta, not another accumulator. Snapshot phase/pause drives visual freezing; upgrade offers do not independently pause combat.

The scene is geometry, not a battle screenshot: 96×54 arena, stone tile instances, six rocks at the **same shared coordinates/radii as core collision**, outside-boundary pillars, hero and eight enemy silhouettes, dynamic projectiles, spell areas and telegraphs, relic/XP pickups and short ID-tracked death/collect effects. One actor ticks. Enemies are not actors/UObjects. Batches reuse allocated instance slots and hide inactive slots. Effects/pickups obey core caps and transient bursts cap at 96.

- 1 core unit = 100 UE cm; core `(x,y)` → UE `(100x,-100y,0)`.
- Camera: orthographic, pitch −62°, yaw −90°, 3200 cm minimum width, expands on wide aspect, follows and clamps to arena bounds. No camera shake obscures warnings.
- Red outlines: hostile warning/active hazards. Orange/violet: friendly fire/thunder. White/gold hero, teal XP. Warning must not display as impact. `Lightning` is a **radial field**, `Line` is a segment, `FireWall` is the authoritative shrinking rectangular perimeter.
- Two inexpensive shadowless directional lights; simple opaque materials, geometry drop shadows; no Lumen, Niagara, navigation, or physics requirement.
- Non-PIE gameplay cap is min(existing positive cap,60); restore only if the actor still owns the applied cap. Release preview cap **before** StartRun; StopRun **before** returning to menu.
- Audio imports original PCM files into cooked SoundWaves. Discrete cues share four-voice concurrency and a 120 ms cue gate. No per-enemy sound storm.

## Automatic assets (must run before play/cook)

The authored geometry is `Content/MVP/Geometry/scene.json`. JSON stores Blender right-handed CCW triangles, metre vertices, linear RGBA colors, optional smooth normals. Loader reflects Y and swaps indices 1/2; creates runtime `UStaticMesh` with `BuildFromMeshDescriptions`, `bFastBuild=true`, no collision. Normals are reflected too. Core is the sole collision authority. The loader rejects missing version, unknown/duplicate mesh names, malformed/nonfinite arrays, nonintegral/out-of-range triangle indices, overlarge files/meshes, and empty/failed mesh builds. Geometry is explicitly staged NonUFS, so the JSON does not depend on editor files in packaged builds.

Material shaders cannot be invented at shipping runtime. Run:

```powershell
# First build DywxSurvivorsEditor Win64 Development with UE 5.8.
.\scripts\Build-MVPAssets.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8'
```

This invokes UE Python (NullRHI commandlet), builds `/Game/MVP/M_Surface` (vertex color×Tint, lit/rough, small ambient contribution) and `/Game/MVP/M_VFX` (opaque, unlit/emissive), sets ISM usage before cooking, validates the authored JSON, requires and imports all ten original `Content/MVP/AudioSource/*.wav` cues, verifies SoundWave types/paths, saves assets, and emits a receipt. `package-windows.ps1` invokes it after the editor build and fails if receipt/material/SoundWave files are missing. `/Game/MVP` is explicitly always-cooked; the Python/editor plugins are editor-target only. No manual asset authoring is required.

If users bypass bootstrap, missing materials log errors and Unreal's diagnostic default material is shown; missing geometry logs an error and diagnostic silhouettes appear. **These are recovery diagnostics, not acceptance of the intended art.** No Engine/BasicShapes meshes or engine debug materials are dependencies.

## Local discoveries and pity

The world owns one `FDywxMVPProfile` for `Saved/MVP/profile.json`. It loads once; active retries first observe/save the old state, then reset simulation and merge discoveries/pity into the new progression. Phase/wave boundaries, a 10-second cadence, StopRun and EndPlay observe/save; the helper skips writes when clean. Save failures are logged and in-memory state survives retries. Existing invalid profiles log a warning. This persists only discovery flags and rare-drop pity, not inventory, combat position, power, or a resumable run. Actual UE filesystem behavior is not yet executed here.

## Evidence and limits

- `python scripts/validate-mvp-render-contract.py`: validates actual JSON arrays, limits, normals, colors, triangle indices, conversion finiteness, staging and critical source contracts.
- `python -m py_compile scripts/bootstrap-mvp-assets.py`: Python syntax only; does not exercise `unreal`.
- UE UHT/UBT, commandlet execution, material compilation, cook, packaged rendering/input/audio, GPU frame timings: **not_run** in the dot environment (no UE/GPU). 60 is a cap, **not** a measured FPS claim.
- Flat/procedural mesh poses have bob/orientation animation, not a skeletal animation set. GPU appearance and packaged resource loads require real UE validation; CPU art contact sheets are not UE screenshots.

## Official API references used

- [UStaticMesh build parameters](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UStaticMesh/FBuildMeshDescriptionsParams): fast-build path is mandatory in non-editor builds; collision/CPU access flags.
- [Build from Static Mesh Descriptions](https://dev.epicgames.com/documentation/unreal-engine/BlueprintAPI/StaticMesh/BuildfromStaticMeshDescriptions): runtime static mesh descriptions contract.
- [MaterialExpressionVertexColor](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialExpressionVertexColor?application_version=5.5): editor Python graph node.
- [Material Python API](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/Material?application_version=5.6): ISM material usage compiles supporting shader permutations.
