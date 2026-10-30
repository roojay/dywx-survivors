# VFX readability refinement — offline evidence only

The recorded winning trace (SHA256 `6c34c75aaf538c69977981a2b41a6c020e4830fa93bed66f7169dd2b0ea1b39d`) showed dense orange double-ring trails obscuring the hero and enemies. This pass changes presentation only; no Simulation code, damage, effect radii, warning timings, RNG, or recorded snapshots were changed.

## Adapter changes

- `DywxMVPVFX.cpp` owns startup geometry and effect submissions. Existing runtime ISM batches remain the rendering mechanism. No per-frame mesh generation, new shader/material dependency, Niagara system, or actor per effect.
- Friendly tiny burn patches use three sparse, dark-amber ground strokes instead of two neon rings. An 0.8-unit visual cell coalesces overlapping patches. This is deliberately dim opaque geometry, not a claimed soft particle shader.
- Co-located large friendly burns and following areas use one faint maximum-radius boundary around the player. Other large stationary friendly fields retain a faint boundary.
- Friendly effect submissions have a fixed **96-instance ceiling** including the boundary. This is not a claim that the entire scene has 96 instances; enemies, projectiles, pickups, critical hostile telegraphs and other existing batches are separate.
- Hostile effect submissions bypass that ceiling. Red outer boundaries retain authoritative centers/radii; rectangular firewall and line-warning geometry remain. Hostile ground marks are above friendly ground marks. Decorative inner rings were removed.
- Melee uses tapered segmented arcs; orbiting blades no longer have individual ring halos. Friendly chains are narrow and short-lived; lightning warning outlines precede brief vertical impacts.
- No per-enemy duplicate burn ring. Existing hero identification ring remains dimmed.

## Evidence and limitations

`Art/Blender/MVP/trace-preview-before-vfx/` preserves the previous clip/contact sheet/metadata. The revised offline renderer reads the exact same snapshot file and mirrors the hierarchy changes; it never invents entities or kills. Co-location is evaluated geometrically where the trace does not record `followPlayer`. Source meshes remain rigid in this diagnostic video, not the runtime pose-animation proof.

Frames 0 and 49 show a visibly clear hero silhouette and separated enemy bodies, with red hazards prominent over sparse amber marks; frame 99 retains the actual terminal WON state. The renderer permanently labels every frame `BLENDER OFFLINE / SIMULATION TRACE / NOT UE RUNTIME`.

`python scripts/validate-mvp-render-contract.py` passes geometry/staging/source contracts, including extraction wiring, the friendly budget, hostile bypass, burn coalescing, and absence of old inner double rings. `python -m py_compile Art/Blender/MVP/render_trace.py` passes.

**Unreal compile, asset bootstrap, launch, rendering, material appearance, GPU utilization, and gameplay acceptance: not_run.** Offline CPU render duration is not a runtime performance measurement. The next real-UE check must establish visibility under the actual camera/materials and confirm frame time/GPU usage with the existing preview frame cap.
