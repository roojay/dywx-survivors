# Built-in image_gen prompts

All three calls used the user-provided target-style reference as a supporting reference image, not an edit target. The gameplay screenshot was inspected separately. No external paid image API was used.

## courtyard_stone.png — transparent_background=false

Use case: stylized-concept. Production asset: one square seamless tileable albedo texture for a 3D Chinese dark-fantasy temple courtyard floor in Unreal Engine. Use reference only for stone material style, NOT the scene composition. Flat orthographic top-down texture only, edge-to-edge grey-blue weathered flagstone with irregular medium large slabs, subtle chisel marks, small cracks, delicate worn edges, sparse warm ochre dust in narrow seams. Rich sculpted painterly material detail, believable rough stone, restrained blue-grey palette, enough midtone brightness to read under game lighting. No black thick grid, no tiny noisy cobblestones, no lava, no objects, no shadows or directional lighting baked in, no characters, no text, no borders, no UI. Seamless left-right and top-bottom edges. 2048x2048 square.

## courtyard_rune.png — transparent_background=false

Use case: stylized-concept. Production asset: square texture decal for the central carved stone medallion of a 3D Chinese mythological temple arena. Use supplied target only for material/style. Flat orthographic directly overhead view, perfectly centered circular ceremonial stone disk nearly filling square, interlocking subtly engraved bronze-gold concentric rings, central yin-yang symbol and eight trigram symbols, decorative abstract archaic sigil marks, weathered dark cool blue-grey stone and understated warm brass inlay. A readable large design with fine chiselled edges. Square corners same stone tone, opaque image, no scene surroundings, no perspective, no characters, no floating magic particles, no fire, no letters/labels or readable text, no cast shadows. 2048x2048 square albedo intended to be mapped on actual game floor geometry, not a game screenshot.

## hud-skills.png — transparent_background=true

Use case: stylized-concept. Production game UI texture atlas, exactly six circular skill medallion icons arranged in a precise 3 columns by 2 rows equal-square grid, 1536x1024 canvas. Reference shows the desired finely rendered dark Chinese fantasy style, copy no surrounding scene. Each medallion centered in its cell, equal diameter80percent ofcellwidth with generous transparent gutters. Clockwise ornate darkbronze metallic thin rim, black-charcoal inner disc, bright readable central illustration. Row1 left flaming orange palm/sweeping fire slash; row1middle golden sword with violet energy trail; row1right purple spiral/vortex. Row2left golden Taoist talisman paper with abstract seal marks; row2middle silver wind spiral; row2right seated meditating immortal silhouette backed by golden aura. Detailed painterly 3D material style with crisp luminous silhouettes readable at64px. No captions, no typography, no numbers, no extra icons, no UI bars. Absolutely transparent outside the six isolated round medallions; maintain exact regular grid.

## Actual outputs

Stone/rune actually returned 1254×1254 RGB. Atlas returned1536×1024 RGBA. Prompts are requests, not proof of the requested resolution, exact motifs or seamlessness; actual-byte metadata and verification are recorded separately.
