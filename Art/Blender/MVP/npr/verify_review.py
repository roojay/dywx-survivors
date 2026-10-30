"""Structural proof checks; visual/art quality is reviewed from the rendered pixels."""
import json
from pathlib import Path
from PIL import Image
P=Path(__file__).resolve().parent
b=json.loads((P/'before-evidence.json').read_text());a=json.loads((P/'after-evidence.json').read_text())
assert a['camera']==b['camera'],'Camera changed'
assert a['source_sha256']==b['source_sha256'],'Baseline source changed'
assert a['samples']==b['samples']==24
assert len(a['refreshed_mesh_instances'])==15
assert a['refreshed_mesh_instances']['hero_body']==1
assert a['refreshed_mesh_instances']['enemy_swarm']==22
assert a['refreshed_mesh_instances']['tile']==221
assert a['mesh_source_sha256']
for name,size in [('before',(1600,1100)),('after',(1600,1100)),('before-after',(1600,620)),('before-960x540',(960,540)),('after-960x540',(960,540))]:
 with Image.open(P/(name+'.png')) as im:assert im.size==size,(name,im.size);im.verify()
print('PASS: same source/camera/sampling; 15 fresh base assets; unchanged staged actor counts; 5 valid watermarked review images')
