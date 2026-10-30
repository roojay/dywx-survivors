# NPR study — actual mesh staging, offline only

This pass applies the article's two-point lighting and compositional unity principles to the existing mesh arena, not to a generated concept painting. Source: [80 Level — Amber Rutherford, introducing Zha Art (crzyzhaa), How to Create Better NPR Art in Blender](https://80.lv/articles/watch-this-video-to-learn-how-to-create-better-npr-art-in-blender). Only the article body was available for this study; the embedded video was not retrieved or watched. This is an independent application of the article’s stated principles, not a step-by-step reproduction of the video. No third-party shaders, textures, models, downloads or additional software are required.

## Review first

- `before-after.png`: frozen original art versus refreshed mesh art + NPR staging, **same camera, positions and scales**. This compares the combined art pass, not lighting alone.
- `before.png`, `after.png`: 1600×1100 annotated actual CPU renders.
- `before-960x540.png`, `after-960x540.png`: downsample to 960×660 then center crop. This tests reduced-scale silhouette reading; it is **not** a real gameplay camera or runtime screenshot. Original arena hero scale is inherited at 1.2; this script does not enlarge it.
- `source-before.blend`: frozen pre-pass source so later source rebuilds cannot contaminate the baseline.
- `after.blend`: editable final staging, with refreshed meshes imported from `../mvp-walk-poses.blend`.
- `*-evidence.json`: camera, source hash, resolution, CPU samples and replacement counts.

## Reproduce from repository root

```sh
../dywx-blender-tools/bin/blender-offline -b -t 8 --python Art/Blender/MVP/npr/render_npr.py -- --before
../dywx-blender-tools/bin/blender-offline -b -t 8 --python Art/Blender/MVP/npr/render_npr.py -- --refresh-meshes
python3 Art/Blender/MVP/npr/make_review.py
```

Run the base mesh export first if rebuilding everything; the final render imports 15 named base assets and replaces their staged instances. `--draft` uses half resolution and 12 samples; finals use Cycles CPU, 24 samples, denoise, four bounces. The unchanged baseline also uses four bounces so the comparison uses matching render settings. Raw PNGs exist for technical reproducibility; all review-facing images carry the offline watermark.

## Artistic decisions and port boundary

- Two broad sources: cool key at (-1,-3,7), 1000 W / 4 m; subdued amber rim at (2,6,7), 500 W / 4 m. Existing local braziers remain, so “two-point” describes the **main** lights, not all scene lights.
- Ink-blue environment replaces the neutral gray surround. Near key direction sculpts creatures; the warm side is deliberately weaker to avoid turning the boss mane into the main focal point.
- Mist pass: linear camera distance, start 22 m, depth 22 m, blended by only 0.055 into muted blue. No volume shader, no godray cone across the combat area, no bloom veil. Visible tile edges and threats take priority over copying every effect in the tutorial.
- Existing real low-poly silhouettes remain identifiable. This is a restrained stylized mesh pass, **not** a claim to fully hand-painted/shader-to-RGB NPR.
- Mesh silhouettes and per-face color variation come from the refreshed shared source/export and can be consumed by the runtime. The staging script never edits `scene.json` or source geometry.
- Light direction, color ratios, desaturated ground and focal hierarchy can guide Unreal lighting/materials. Blender wattage is **not** a one-to-one Unreal parameter mapping.
- AgX, Blender world, Cycles shadows/denoise and the compositor mist are **offline preview only**. They are not imported into UE, not proof of a runtime material, and not packaged performance evidence. UE would need its own exposure/tonemapping and subtle depth-fog implementation plus actual 960×540 runtime review before visual parity could be claimed.

No gameplay rules, collision, positions, camera or actor scale are changed by this script. No commit or push is included.

Validation: `python3 Art/Blender/MVP/npr/verify_review.py` passed. Final after render saved in 20.2 s CPU in this environment; this is an offline authoring cost, not a game-frame benchmark. Read-only thumbnail/cache and headless EGL warnings were nonfatal; PNGs and blends saved and were inspected.
