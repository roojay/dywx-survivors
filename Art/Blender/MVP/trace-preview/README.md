# Offline simulation trace preview — NOT Unreal runtime

`simulation-trace-offline-not-ue.mp4` is a diagnostic 10-second, 10 FPS illustration of 100 recorded snapshots from an actual pure-C++ Simulation run. The winning Sword run uses seed 12, bot policy 0; this clip is its final 626.441–636.272 seconds. It is not an Unreal gameplay capture, a UE compilation result, a performance measurement, or final game art.

The Blender renderer reads the entire JSON once, then directly uses each snapshot's hero, enemy, projectile and effect positions. Entity disappearance and terminal WON state are taken from the trace. Red shows hostile areas and warnings, orange friendly fire, purple friendly thunder. Hostile boundaries preserve core effect areas. Friendly presentation uses sparse dark-amber burn marks, coalesced cells, one nearby aura boundary, thin chains and brief vertical lightning; these are bounded visual embellishments, not changed damage areas. This is not final VFX. Recorded headings orient rigid meshes; there is no skeletal animation or invented attacks, kills, enemy spawns, or pickups. Blue-gray tiles, rocks and shrine boundary are the existing authored meshes in the adapter's layout. Shrine props may remain outside this clip's follow camera. Camera pitch is 62 degrees, screen viewport 32×18 core units, player-centered and clamped to the arena.

Each frame is permanently watermarked `BLENDER OFFLINE / SIMULATION TRACE / NOT UE RUNTIME`. The top strip displays only trace time, HP, phase, and boss phase. The readability-v2 presentation replaces excessive friendly double rings; original video/contact sheet/metadata are preserved in `../trace-preview-before-vfx/`. Hostile warnings bypass the 96-instance friendly-effect embellishment budget. No production HUD is implied.

See `docs/mvp/vfx-readability-refinement.md` for the exact scope and source-only checks. The retained PNGs are selected evidence frames; full render frames are temporary and excluded from Git.

Tool: official offline Blender 5.2.3 LTS, Cycles CPU, 8 samples with denoising, 960×540. No GPU, graphics driver installation, external service, or fabricated background. FFmpeg encodes H.264/yuv420p at 10 FPS. Render duration and exact trace/renderer/mesh SHA256 values are in `render-metadata.json`. The copied `simulation-source-manifest.json` records source fingerprints for the trace producer. Frames 0, 49 and 99 were inspected before the full render, confirming follow camera, visible effects, readable watermark, and correct terminal disappearance.

Reproduce from repository root:

```sh
/path/to/blender-offline -b -t 8 --python Art/Blender/MVP/render_trace.py
ffmpeg -framerate 10 -i Art/Blender/MVP/trace-preview/frame-%04d.png \
  -c:v libx264 -preset medium -crf 20 -pix_fmt yuv420p -movflags +faststart \
  Art/Blender/MVP/trace-preview/simulation-trace-offline-not-ue.mp4
```
