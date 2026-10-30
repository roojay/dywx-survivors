# MVP gameplay mesh art — original procedural source

## Deliverables

- `Content/MVP/Geometry/scene.json` (repository-relative): 31 indexed triangle meshes (15 bases + 16 authored walk poses), vertex colours and normals; no bitmap environment pretending to be a world.
- `build_scene.py`: original deterministic Blender geometry construction and CPU preview scene generator. No downloaded scripts, external mesh assets, textures, plugins or paid API calls.
- `mvp-meshes.blend`: editable source meshes, materials and laid-out contact sheet.
- `mvp-arena-preview.blend`: playable-view staging of the same meshes; illustrative placement, not a level imported into Unreal.
- `contact-sheet.png`, `arena-preview.png`: actual CPU renders, visually inspected after the second art pass.
- `validate_geometry.py`, `validation.json`: reproducible structural, finite-value, normals, indices, area, winding-volume and triangle-budget checks.

All geometry and scripts in this directory were authored for this project. Blender is the creation tool, not an asset licensor. There are no third-party art attribution obligations for this mesh set. The concept images in `docs/` are visual direction only, not baked textures or mesh inputs.

## Runtime contract (agreed with UE renderer)

`version: 1`; `meshes` contains `name`, `vertices: [[x,y,z],…]`, `triangles: [[a,b,c],…]`, `colors: [[r,g,b,a],…]`, optional `normals: [[x,y,z],…]`. Colours are **linear**, not sRGB. Colours and normals have exactly one entry per vertex. Vertex splits preserve hard clothing edges and smooth organic shading.

Blender right-handed coordinates, Z up, +X facing forward; counter-clockwise outside-facing triangles. Unit = metre / one simulation grid unit. Unreal imports `(X,-Y,Z)*100`, transforms normals `(Nx,-Ny,Nz)`, and **reverses triangle indices 1 and 2** to compensate for the reflected Y axis. Do not reverse twice.

- `hero_body`: common ivory-robed cultivator, black hair/topknot, gold cuffs, split robe and visible boots, ~1.793 m tall. Actual feet touch Z=0.
- `hero_sword`: separate floating jian weapon, **not a second humanoid**. +X blade direction; origin near hilt/guard. Renderer attaches above the ground beside `hero_body`; can reuse for sword projectiles.
- `enemy_swarm`, `enemy_exploder`, `enemy_charger`, `enemy_rangedbird`, `enemy_supporttotem`, `enemy_summoner`, `enemy_boss`, `enemy_spirit`.
- `rock`: approximately 1 m base radius, `pillar`: 1 m footprint and 2.83 m high, `tile`: 0.98 m square; tile top around Z=0 and thickness below ground.

The parent approved relaxed geometry budgets during visual refinement: hero <5k triangles, regular enemies/props <3k, boss <10k. Actual highest counts remain hero 2116, charger 2432 and boss 3024; see validation for all 15 assets.

Actor origin is ground/feet. Floating bird and fiery spirit are **intentionally elevated** within their mesh bounds. Boss bounds are ~3.34 × 1.62 × 2.33 m. Exact bounds/counts are in `validation.json`.

## Reproduce

From repository root, using the existing official offline Blender installation:

```sh
../dywx-blender-tools/bin/blender-offline -b -t 8 --python Art/Blender/MVP/build_scene.py
python3 Art/Blender/MVP/validate_geometry.py
python3 Art/Blender/MVP/validate_walk_runtime.py
```

Blender 5.2.3 LTS was used. Render backend: Cycles CPU, 24 samples with denoising. Fixed geometry random seed: 73. The harmless thumbnail-cache write warning in the restricted dot environment does not affect `.blend`, `.json` or PNG outputs.

## Honest scope / art QA

This is a lightweight **stylized low-poly MVP** mesh kit, not the high-detail concept illustration reproduced in 3D. Actual mesh silhouettes were checked at gameplay scale: crouched two-legged swarm, four-legged horned charger, large ivory-maned boss, spread-wing bird, upright totem, low shell summoner, and small gold fox are distinguishable. Hero pale cloth remains distinct from dark enemies/stone. Refinement passes replaced the initial toy-like hero proportions with an approximately 1:7 head/body ratio, long hair, asymmetric casting pose, split coat and visible boots; narrowed the demon skull and lengthened its limbs; replaced crown-like beast feathers with swept curved mane strands; and added darker stone, hairline floor cracks, hanging vermilion banners, a paifang entrance and torch bowls. Preview lighting is blue moonlight with restrained local amber, not neutral studio daylight.

`shrine_banner` (about 3.05 m tall) and `shrine_arch` (about 4.42 m tall, 5.3 m span along Y) are optional additional shrine props. Their frontage is +X; rotate 90 degrees for a north-edge entrance. The cloth banner has actual volume/wave geometry but is rigid.

Hero and swarm now use eight **authored mesh-pose animation** variants each: small part-group joint rotations, alternating boot lift, arm counter-swing and hero coat/hair lag. This is **not skeletal animation**: no skeletal rig, runtime skin weights or skeletal animation clips are supplied. Other characters remain rigid. Runtime chooses one pose from actual frame-to-frame displacement (idle uses base), with stable swarm ID phase; it owns heading, ground shadows, VFX and hit feedback. Blender previews use authored metallic/roughness material values and studio lights; the runtime vertex-colour bootstrap material approximates this and is not yet visually certified in an Unreal build. CPU preview is not evidence of packaged game performance.

These are disconnected but closed component forms assembled for economical runtime meshes, not watertight subdivision-ready character sculpts. The robe front is intentionally open. Validation asserts positive aggregate signed volume and no degenerate triangles, but does not claim every mesh is a single closed manifold.

## Authored walk-pose pass

`hero_walk_0..7` and `enemy_swarm_walk_0..7` use the same schema and per-mesh budgets as their bases. The original 15 base meshes were compared against the pre-animation `.blend`: exact vertex-position and polygon-geometry multisets match (Blender join can reorder indices). Pose normals are recalculated by Blender after deformation. Source mesh-part groups are authoring aids, not runtime skinning.

Reproduce the separate animation-source file and actual contact sheet without touching the previous arena preview:

```sh
MVP_EXPORT_ONLY=1 ../dywx-blender-tools/bin/blender-offline -b -t 8 --python Art/Blender/MVP/build_scene.py
../dywx-blender-tools/bin/blender-offline -b -t 8 --python Art/Blender/MVP/render_walk_poses.py
../dywx-blender-tools/bin/blender-offline -b -t 2 --python Art/Blender/MVP/validate_walk_bases.py
python3 Art/Blender/MVP/validate_geometry.py
python3 Art/Blender/MVP/validate_walk_runtime.py
```

`walk-contact-sheet.png` shows all 16 baked poses from `mvp-walk-poses.blend`; inspected for coherent boots, claws, arms, silhouette and alternating stride. These are actual Blender CPU frames, not Unreal screenshots. No paid models, downloads, third-party art/code, new installs or shaders were used.

Startup data is about 4.86 MB / 31 meshes / 45,680 triangles (all variants included); exact bytes are in `validation.json`. Loader accepts only explicitly named bases and the 16 variants, with a 64-mesh and 32-MiB file cap. Each actor emits exactly one body instance; meshes/batches are built only at scene startup. Pose/base hero and swarm batches trim historical tails to at most eight spare instances each, bounding retained occupancy to current actor count + 144 across these 18 buckets rather than eight full historical crowds. Gait advances by actual distance, not a fixed clock: one hero cycle per 1.5 simulation units and one swarm cycle per 1.1 units (scaled for elites). Hero bob follows the same gait phase. ID modulo eight seeds swarm phase; per-ID phases are removed on death and cleared on run/generation reset. Pausing or blocking movement cannot advance phase. This is a deliberately economical stylized MVP treatment, not foot-locked motion matching. Physical coordinates, collision, balance and simulation are unchanged.

Unreal compilation, runtime appearance, startup time, GPU cost and packaged performance remain **not_run** (no UE installation here). Source/data checks and Blender CPU art review do not certify those.
