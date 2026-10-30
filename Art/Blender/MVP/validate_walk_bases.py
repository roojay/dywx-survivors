import bpy,json
from pathlib import Path
root=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(root/'mvp-meshes.blend'))
names=['hero_body','hero_sword','enemy_swarm','enemy_exploder','enemy_charger','enemy_rangedbird','enemy_supporttotem','enemy_summoner','enemy_boss','enemy_spirit','rock','pillar','tile','shrine_banner','shrine_arch']
def signature(o):
 d=o.data
 return (sorted(tuple(v.co) for v in d.vertices),sorted(tuple(sorted(tuple(d.vertices[i].co) for i in p.vertices)) for p in d.polygons))
before={n:signature(bpy.data.objects[n]) for n in names}
bpy.ops.wm.open_mainfile(filepath=str(root/'mvp-walk-poses.blend'))
for n in names:assert before[n]==signature(bpy.data.objects[n]),n
print('PASS: current contact-source vs pose-source: all 15 bases have exact vertex-position/polygon geometry equality; not an old-art baseline; join index order may differ')
(root/'walk-base-comparison.json').write_text(json.dumps({'status':'passed','base_meshes':names,'check':'exact coordinate/polygon geometry multiset: current contact-source mvp-meshes.blend vs current pose-source mvp-walk-poses.blend; not an old-art baseline; vertex indices may reorder'},indent=2)+'\n')
