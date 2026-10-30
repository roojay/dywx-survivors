#!/usr/bin/env python3
"""Focused static art/material compatibility gate, NOT an Unreal shader test."""
import ast
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
scene = json.loads((ROOT / 'Content/MVP/Geometry/scene.json').read_text())
bootstrap = (ROOT / 'scripts/bootstrap-mvp-assets.py').read_text()
world = (ROOT / 'Source/DywxUI/Private/MVP/DywxMVPWorld.cpp').read_text()
# Parse rather than importing a commandlet that requires the real Unreal module.
ast.parse(bootstrap)
assert "unreal.BlendMode.BLEND_OPAQUE" in bootstrap
assert "unreal.MaterialShadingModel.MSM_DEFAULT_LIT" in bootstrap
assert "connect(color, 'RGB', multiply, 'A')" in bootstrap
assert "connect(tint, 'RGB', multiply, 'B')" in bootstrap
assert "roughness.set_editor_property('r', 0.86)" in bootstrap
assert "ambient.set_editor_property('const_b', 0.22)" in bootstrap
assert 'MP_OPACITY' not in bootstrap
assert 'MP_METALLIC' not in bootstrap  # Blender metallic is NOT exported.
assert 'Colors[Id]=C.IsValidIndex(Index)?C[Index]' in world
assert 'float(X[3])' in world  # Loader does preserve RGBA; A remains unused.
assert 'TEXT("MoonLight")' in world and 'TEXT("WarmRim")' in world
assert 'Sun->SetCastShadows(false)' in world and 'Fill->SetCastShadows(false)' in world

by_name = {m['name']: m for m in scene['meshes']}
assert scene['version'] == 1 and len(by_name) == 31
for mesh in scene['meshes']:
    assert set(mesh) == {'name', 'vertices', 'triangles', 'colors', 'normals'}
    assert len(mesh['vertices']) == len(mesh['colors']) == len(mesh['normals'])
    for color in mesh['colors']:
        assert len(color) == 4 and all(math.isfinite(c) and 0 <= c <= 1 for c in color)
        assert color[3] == 1, (mesh['name'], 'alpha is not material metadata')

# Area-weighted linear base-color luminance, not screen luminance. This checks
# exported value hierarchy independently of vertex density, lights and tonemapping.
def luminance(mesh):
    total = area_sum = 0.0
    for tri in mesh['triangles']:
        a, b, c = [mesh['vertices'][i] for i in tri]
        u, v = [[p[k] - a[k] for k in range(3)] for p in (b, c)]
        cross = [u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0]]
        area = math.sqrt(sum(x*x for x in cross)) / 2
        value = sum(sum(w*c for w,c in zip((.2126,.7152,.0722), mesh['colors'][i][:3])) for i in tri) / 3
        total += area * value
        area_sum += area
    assert area_sum > 0
    return total / area_sum

values = {name: luminance(by_name[name]) for name in ('hero_body', 'enemy_swarm', 'tile')}
assert values['hero_body'] > 1.5 * values['enemy_swarm'], values
assert values['hero_body'] > 1.5 * values['tile'], values
print(json.dumps({'status':'passed', 'scope':'static exported art + material source contracts only',
                  'meshes':len(by_name), 'areaWeightedBaseColorLuminance':values,
                  'ue_shader_compile':'not_run', 'ue_render':'not_run', 'gpu_performance':'not_run'}, indent=2))
