#!/usr/bin/env python3
"""Dependency-free geometry contract + budget verification."""
import json, math, hashlib
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
PATH=ROOT/'Content/MVP/Geometry/scene.json'
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def sub(a,b):return [x-y for x,y in zip(a,b)]
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def run():
 d=json.loads(PATH.read_text());assert d['version']==1
 expected={'hero_body','hero_sword','enemy_swarm','enemy_exploder','enemy_charger','enemy_rangedbird','enemy_supporttotem','enemy_summoner','enemy_boss','enemy_spirit','rock','pillar','tile','shrine_banner','shrine_arch'}
 expected.update(f'{prefix}_{i}' for prefix in ['hero_walk','enemy_swarm_walk'] for i in range(8))
 assert len(d['meshes'])==len(expected)<=64
 assert {m['name'] for m in d['meshes']}==expected
 assert PATH.stat().st_size<32*1024*1024
 assert d['coordinateSystem']=='RH_Z_UP_X_FORWARD' and d['winding']=='CCW_OUTWARD'
 report={'source':'Content/MVP/Geometry/scene.json','sha256':hashlib.sha256(PATH.read_bytes()).hexdigest(),'meshCount':len(d['meshes']),'rawBytes':PATH.stat().st_size,'passed':True,'meshes':[]}
 for m in d['meshes']:
  v=m['vertices'];n=m['normals'];c=m['colors'];t=m['triangles'];assert len(v)==len(n)==len(c)>0
  assert all(len(x)==3 and all(math.isfinite(z) for z in x) for x in v+n)
  assert all(len(x)==4 and all(0<=z<=1 for z in x) for x in c)
  assert all(abs(dot(x,x)-1)<.005 for x in n)
  signed_volume=0
  for tr in t:
   assert len(tr)==len(set(tr))==3 and all(isinstance(i,int) and 0<=i<len(v) for i in tr)
   a,b,cc=[v[i] for i in tr];normal=cross(sub(b,a),sub(cc,a));assert dot(normal,normal)>1e-18,(m['name'],tr)
   signed_volume+=dot(a,cross(b,cc))/6
  limit=10000 if m['name']=='enemy_boss' else 5000 if m['name'].startswith('hero') else 3000
  assert len(t)<limit
  bounds={'min':[min(x[i] for x in v) for i in range(3)],'max':[max(x[i] for x in v) for i in range(3)]}
  assert signed_volume>0,(m['name'],signed_volume)
  if '_walk_' in m['name']:assert bounds['min'][2]>=0,(m['name'],'below ground')
  report['meshes'].append({'name':m['name'],'vertices':len(v),'triangles':len(t),'triangleBudgetExclusive':limit,'boundsMetres':bounds,'signedVolume':round(signed_volume,6)})
 by_name={m['name']:m for m in d['meshes']}
 for base,prefix in [('hero_body','hero_walk'),('enemy_swarm','enemy_swarm_walk')]:
  assert all(len(by_name[f'{prefix}_{i}']['triangles'])==len(by_name[base]['triangles']) for i in range(8))
  assert by_name[f'{prefix}_2']['vertices']!=by_name[f'{prefix}_6']['vertices']
 (HERE/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report,indent=2))
if __name__=='__main__':run()
