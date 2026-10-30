#!/usr/bin/env python3
"""Data/contract checks only. This does not compile UE or certify GPU rendering."""
import json
import math
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
scene = json.loads((ROOT/'Content/MVP/Geometry/scene.json').read_text())
required = {'hero_body','hero_sword','enemy_swarm','enemy_exploder','enemy_charger','enemy_rangedbird','enemy_supporttotem','enemy_summoner','enemy_boss','enemy_spirit','rock','pillar','tile'}
assert scene['version'] == 1
assert {m['name'] for m in scene['meshes']} >= required
assert len({m['name'] for m in scene['meshes']}) == len(scene['meshes'])
triangles = 0
for mesh in scene['meshes']:
    vertices = mesh['vertices']
    assert len(vertices) == len(mesh['colors']) == len(mesh['normals'])
    assert len(vertices) <= 100000 and len(mesh['triangles']) <= 100000
    assert all(len(v)==3 and all(math.isfinite(x) and abs(x)<=1000 for x in v) for v in vertices)
    assert all(len(c)==4 and all(math.isfinite(x) and 0<=x<=1 for x in c) for c in mesh['colors'])
    assert all(len(t)==3 and all(isinstance(i,int) and 0<=i<len(vertices) for i in t) for t in mesh['triangles'])
    assert all(abs(sum(x*x for x in n)-1)<.02 for n in mesh['normals'])
    # Reflect Y and reverse winding exactly as runtime loader does.
    converted = [(x*100,-y*100,z*100) for x,y,z in vertices]
    for a,c,b in mesh['triangles']:
        assert all(math.isfinite(x) for index in (a,b,c) for x in converted[index])
    nondegenerate = 0
    for a,b,c in mesh['triangles']:
        u=[vertices[b][k]-vertices[a][k] for k in range(3)]
        v=[vertices[c][k]-vertices[a][k] for k in range(3)]
        cross=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
        nondegenerate += sum(x*x for x in cross)>1e-16
    assert nondegenerate > 0
    triangles += len(mesh['triangles'])
config = (ROOT/'Config/DefaultGame.ini').read_text()
assert 'DirectoriesToAlwaysStageAsNonUFS=(Path="MVP/Geometry")' in config
assert 'DirectoriesToAlwaysCook=(Path="/Game/MVP")' in config
cpp = (ROOT/'Source/DywxUI/Private/MVP/DywxMVPWorld.cpp').read_text()
assert 'Params.bFastBuild=true' in cpp
assert 'ECollisionEnabled::NoCollision' in cpp
assert 'SoundConcurrency->Concurrency.MaxCount=4' in cpp
assert 'PreviousEnemies.Reset(); PreviousPickups.Reset(); Transients.Reset()' in cpp
assert 'GetArenaObstacles()' in cpp
vfx = (ROOT/'Source/DywxUI/Private/MVP/DywxMVPVFX.cpp').read_text()
assert 'EffectKind::FireWall' in vfx and 'EffectKind::Slash' in vfx
assert 'RenderVFX(S,NewPalm,NewThunder,MaxEffect)' in cpp
assert 'FriendlyInstanceBudget=96' in vfx and 'E.hostile || FriendlyInstances<FriendlyInstanceBudget' in vfx
assert 'BurnCells.Contains(Cell)' in vfx and 'AuraBoundary' in vfx
assert 'Radius*.75f' not in vfx
assert 'Simulation.' not in vfx and 'rand(' not in vfx
assert 'Build-MVPAssets.ps1' in (ROOT/'scripts/package-windows.ps1').read_text()
print(json.dumps({'status':'passed','mesh_count':len(scene['meshes']),'triangles':triangles,'scope':'JSON + staging + source contracts only','ue_compile':'not_run','ue_render':'not_run'},indent=2))
