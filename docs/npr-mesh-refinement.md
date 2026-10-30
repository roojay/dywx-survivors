# NPR mesh refinement — 2026-10-30

## Source and intent

The referenced 80 Level article introduces a Blender NPR workflow focused on two-point lighting, compositing unity, fog and god rays. It is **not** a specific mesh-authoring recipe. The following original mesh changes are our project-specific response, not techniques falsely attributed to that article. No downloaded assets, installers, paid APIs or third-party code were used.

Actual pre-pass `arena-preview.png` and `contact-sheet.png` pixels were inspected before editing. Their smooth component spheres, segmented cone manes and shiny gold read like assembled toys. The aim is a restrained ink/charcoal enemy family, warm ivory hero, aged bronze and small red-orange danger accents.

## Transferable changes

- Broad demon/stone shell masses use chiseled superellipsoid planes rather than smooth bead shading. Cloth and armor rods use hard surface normals; face and small glowing accents retain appropriate smooth shading.
- Horns, hair and beast manes are continuous swept polygon surfaces instead of overlapping individual cones. This removes joint collars and reduces triangles.
- Authored three-value face groups provide broad dark/mid/light patches, with sparse stone edge wear. These are material colors in Blender **and exported linear vertex RGB**, not preview-only effects or procedural runtime shaders.
- Demon values are darker; stone is less blue; gold becomes matte aged bronze. Boss mane has its own muted ochre palette rather than the hero's ivory. Red-orange fire and violet accents are restricted and less saturated.
- Original base envelopes are retained explicitly in the authoring script; all 15 base XYZ extrema agree with the original export within 0.000002 metres. Bounds preservation is an art placement check, not a physics-engine test. No simulation, collision or Unreal code is changed by this mesh pass.
- All 31 mesh names, coordinate/winding schema, ground origins, eight hero poses and eight swarm poses remain. Pose copies are generated from the refined bases and retain part-group articulation.
- Exported color alpha stays **1**, not roughness. Blender uses .84 roughness for bronze and .90 for other materials; the existing opaque UE .86 bootstrap approximates these without changing its contract. Blender preview lighting/material response is not UE certification.

## Cost and validation

| Metric, all bases and variants | Before | After |
|---|---:|---:|
| Triangles | 45,680 | 43,066 |
| Exported split vertices | 34,850 | 99,854 |
| JSON bytes | 4,857,902 | 9,147,202 |

Hard facet normals/value boundaries require more vertex splits despite fewer triangles. Export colors are rounded to six decimals (matching position/normal precision), reducing the first pass from 12,943,309 to 9,147,202 bytes without removing facets. Array payload bytes after refinement: positions 2,775,618; normals 2,911,373; colors 2,763,409; triangle indices 694,439. This remains within the 32 MiB loader cap, but actual UE parse/startup/GPU cost is **not_run**.

Current base triangles: hero 1,996; swarm 1,588; charger 2,074; boss 2,666. All 31 meshes pass geometry validation (finite coordinates, unit normals, indices, nondegenerate triangles, positive aggregate volume, explicit budgets, grounded poses and pose/base triangle counts). Distance-linked gait source validation passes. Blender base/pose source comparison passes exact position/polygon multiset equality for all 15 bases. `git diff --check` passes. These are not runtime animation certifications.

Reproduce using the commands in `Art/Blender/MVP/README.md`. Full `build_scene.py` generates JSON, three source `.blend` files, contact and arena previews. `render_walk_poses.py` generates the 16-pose sheet. `validate_walk_bases.py` checks exact base position/polygon multiset equality between the regenerated contact-source and pose-source files; this comparison establishes source consistency, not equality to the old art.

## Visual QA and honest limitations

Refined contact, arena and all-16-pose sheets were inspected at full resolution; boots, cuffs, claws and alternating stride remain coherent. Contact review confirms: coherent faceted shells, continuous mane silhouettes, distinct pale hero and dark enemy families, retained shrine/weapon silhouettes. The illustrative arena uses the pre-existing 1.2× hero staging scale; it is not evidence of exact gameplay proportions. Meshes remain economical disconnected closed components, not production character sculpts or skeletal rigs. Other than hero/swarm baked gait, actors remain rigid. Separate `Art/Blender/MVP/npr/` scene work demonstrates the article's atmosphere/compositing ideas and must not be described as an implemented Unreal post-process.

Unreal compilation, packaged runtime, real gameplay appearance, collision integration, startup time and GPU performance remain **not_run**.
