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

# Full-game runtime wiring. Source contracts are not UE compilation/render evidence.
header = (ROOT/'Source/DywxUI/Public/MVP/DywxMVPWorld.h').read_text()
runtime_header = (ROOT/'Source/DywxRuntime/Public/MVP/DywxRunSession.h').read_text()
runtime = (ROOT/'Source/DywxRuntime/Private/MVP/DywxRunSession.cpp').read_text()
def function(name, source=cpp):
    start = source.index(name)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[brace:end]
for api in ('InitializeProfile', 'StartFullRun', 'GetCampaignProfile', 'PurchaseLegacy',
            'PurchasePermanent', 'SaveRun', 'ResumeRun', 'HasSavedRun'):
    assert api in runtime_header and api not in header
assert 'InitializePresentation' in header
assert 'const auto& Simulation=*Session->GetSimulation();' in cpp
assert 'Simulation.Step' not in cpp
arena = function('void ADywxMVPWorld::RebuildArena')
assert 'Simulation.GetArena()' in arena and 'Arena.obstacles' in arena
assert 'ClearInstances()' in arena and 'Allocated=0' in arena
assert 'Arena.veins' in arena and 'Arena.hazards' in arena
assert 'cornerCut' in arena and 'Corners[(I+1)%8]' in arena
assert 'LoadGeometry' not in arena and 'BuildMesh' not in arena
render = function('void ADywxMVPWorld::Render(float')
assert 'RebuildArena' not in render and 'BuildMesh' not in render and 'MakeBatch' not in render
start = function('bool UDywxRunSession::StartFullRun', runtime)
assert start.index('AllocateRunId()') < start.index('Simulation=MoveTemp(Candidate)')
assert 'if(!RunId) return false;' in start
settle = function('bool UDywxRunSession::SaveProfile', runtime)
assert 'seenAnomalies[I]=true' in settle
assert settle.index('CandidateProfile.Save()') < settle.index('Simulation=MoveTemp(CandidateSimulation)')
resume = function('bool UDywxRunSession::ResumeRun', runtime)
assert resume.index('RestoreCheckpoint') < resume.index('ConsumeCheckpoint') < resume.index('Simulation=MoveTemp(Candidate)')
for element in ('Metal', 'Wood', 'Water', 'Earth', 'Fire', 'Thunder'):
    assert f'Element::{element}' in cpp + vfx
assert 'E.form==Form::Beast' in vfx and 'HeroId::Beast' in render
assert 'RelicGrade%d' in render and 'CollectedGrade' in render
assert 'Vision' in render and 'AnomalyId::Array' in render
print('PASS: full-game world, persistence ordering, seeded arena and bounded six-element/companion presentation source contracts')

# Anomaly halo eligibility follows current gameplay modifiers, not a blanket ring.
affected = function('bool AnomalyAffectsEnemy')
for condition in ('Mods.currency>1.f', 'Mods.spawn>1.f', 'Mods.chargers>1.f',
                  'Role==EnemyRole::Charger', 'Mods.ranged>1.f', 'Role==EnemyRole::Ranged',
                  'Mods.beasts>1.f', 'Role==EnemyRole::SpiritBeast', 'Mods.xp>1.f'):
    assert condition in affected
assert 'if(!State.active) return false' in affected
assert 'case AnomalyId::Lightning: case AnomalyId::Array: case AnomalyId::Count: return false' in affected
assert 'AnomalyAffectsEnemy(Anomaly.State(),Anomaly.Modifiers(S.player.position),E.role)' in render
assert 'AnomalyColors' in vfx and 'AnomalyHalo%d' in vfx and 'unit_faint_boundary' in vfx
assert 'Mods.playerPower>1.f' in render and 'Mods.thunderPower>1.f' in render
print('PASS: six-color low-priority anomaly halos gated by actual modifiers/roles; no fictitious lightning/array enemy buffs')
