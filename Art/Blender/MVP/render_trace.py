"""Offline diagnostic view of recorded C++ snapshots, never UE runtime footage.
Run: blender-offline -b -t 8 --python render_trace.py -- --indices 0,49,99
Omit --indices for complete sequence. No synthetic entities, interpolated frames,
rigging, kill counts, or inferred inventory. Optional direction fallback points at hero.
"""
import bpy, math, json, argparse, sys, hashlib, time
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]; HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--trace',default=str(ROOT/'docs/mvp/playthrough-winning-trace.json'));p.add_argument('--out',default=str(HERE/'trace-preview'));p.add_argument('--indices');p.add_argument('--samples',type=int,default=8)
a=p.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
out=Path(a.out);out.mkdir(parents=True,exist_ok=True);raw=Path(a.trace).read_bytes();trace=json.loads(raw);assert trace['schema']=='dywx-mvp-trace-v1'
start=time.monotonic();bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
names=['hero_body','hero_sword','enemy_swarm','enemy_exploder','enemy_charger','enemy_rangedbird','enemy_supporttotem','enemy_summoner','enemy_boss','enemy_spirit','rock','pillar','tile','shrine_banner','shrine_arch']
with bpy.data.libraries.load(str(HERE/'mvp-meshes.blend'),link=False) as (src,dst):dst.objects=list(names)
assets={o.name:o.data for o in dst.objects}
def inst(name,pos,scale=1,angle=0):
 o=bpy.data.objects.new(name,assets[name]);bpy.context.collection.objects.link(o);o.location=pos;o.scale=(scale,)*3;o.rotation_euler.z=angle;return o
def mat(name,c,emission=0):
 m=bpy.data.materials.new(name);m.diffuse_color=(*c,1);m.use_nodes=True;b=m.node_tree.nodes.get('Principled BSDF');b.inputs['Base Color'].default_value=(*c,1);b.inputs['Roughness'].default_value=.85;b.inputs['Emission Color'].default_value=(*c,1);b.inputs['Emission Strength'].default_value=emission;return m
red=mat('trace hostile red',(1,.025,.018),1.5);orange=mat('trace friendly fire',(1,.24,.015),1.5);purple=mat('trace friendly thunder',(.45,.13,1),1.5);gold=mat('trace player',(.9,.66,.15),1);white=mat('trace labels',(.9,.95,1),1);ground=mat('trace ground',(.025,.04,.065))
def box(pos,scale,m):
 bpy.ops.mesh.primitive_cube_add(size=1,location=pos);o=bpy.context.object;o.scale=scale;o.data.materials.append(m);return o
box((0,0,-.25),(104,62,.3),ground)
for x in range(-46,47,4):
 for y in range(-25,26,4):inst('tile',(x,y,-.04),4)
for x,y,r in [(-18,-9,1.6),(15,11,1.8),(-30,15,2),(30,-15,2),(-7,19,1.5),(7,-19,1.5)]:inst('rock',(x,y,0),r,math.radians(x*17))
for x in range(-44,45,8):
 for y in [-29,29]:inst('pillar',(x,y,0),1.1)
for y in range(-20,21,8):
 for x in [-50,50]:inst('pillar',(x,y,0),1.1)
inst('shrine_arch',(0,30,0),1.5,math.pi/2)
for x in [-12,12]:inst('shrine_banner',(x,29,0),1.5,math.pi/2)
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=a.samples;scene.cycles.use_denoising=True;scene.render.threads_mode='FIXED';scene.render.threads=8
scene.render.resolution_x=960;scene.render.resolution_y=540;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.world.color=(.04,.055,.085);scene.view_settings.view_transform='AgX'
for angle,energy,color in [((.6,-.5,-.7),2.2,(.63,.75,1)),((-.7,.2,-.4),1.1,(1,.68,.38))]:
 bpy.ops.object.light_add(type='SUN');o=bpy.context.object;o.rotation_euler=Vector(angle).to_track_quat('-Z','Y').to_euler();o.data.energy=energy;o.data.color=color;o.data.angle=.12
bpy.ops.object.camera_add();camera=bpy.context.object;camera.data.type='ORTHO';camera.data.ortho_scale=32;camera.data.clip_end=200;scene.camera=camera
# Looking from -Y, Z-up. Matches adapter 62deg pitch, 32x18 screen units.
camera.rotation_euler=(math.radians(28),0,0)
def label(name,body,x,y,size):
 d=bpy.data.curves.new(name,'FONT');d.body=body;d.size=size;d.materials.append(white);o=bpy.data.objects.new(name,d);bpy.context.collection.objects.link(o);o.parent=camera;o.location=(x,y,-1);return o
header=label('snapshot metadata','',-15.3,7.6,.43)
label('mandatory watermark','BLENDER OFFLINE  /  SIMULATION TRACE  /  NOT UE RUNTIME',-15.3,-8.25,.43)
label('diagnostic legend','RED: hostile / warning    ORANGE: fire    PURPLE: thunder    rigid mesh visualization',-15.3,-7.65,.32)
# Dark strips, parented to camera, keep watermark readable in every frame.
for y,h in [(8,1.25),(-7.95,1.5)]:
 o=box((0,0,0),(32,h,.01),mat('label backing',(.006,.009,.016)));o.parent=camera;o.location=(0,y,-1.1)
dynamic=[]
def keep(o,name):o.name=name;dynamic.append(o);return o
def line(x,y,ex,ey,width,m,z=.08):
 dx,dy=ex-x,ey-y;o=box(((x+ex)/2,(y+ey)/2,z),(max(.01,math.hypot(dx,dy)),width,.045),m);o.rotation_euler.z=math.atan2(dy,dx);dynamic.append(o);return o
def ring(x,y,r,m,z=.08,width=.045):
 n=64;verts=[];faces=[]
 for i in range(n):
  t=i*2*math.pi/n
  for rr in [max(0,r-width),r+width]:verts.append((x+rr*math.cos(t),y+rr*math.sin(t),z))
 for i in range(n):faces.append((2*i,2*i+1,2*((i+1)%n)+1,2*((i+1)%n)))
 d=bpy.data.meshes.new('trace ring');d.from_pydata(verts,[],faces);d.materials.append(m);o=bpy.data.objects.new('ring',d);bpy.context.collection.objects.link(o);dynamic.append(o);return o
ember=mat('trace dark amber embers',(.22,.055,.008),.8)
faint=mat('trace faint boundary',(.16,.075,.025),.6)
thunder_faint=mat('trace faint thunder boundary',(.12,.055,.24),.6)
def embers(x,y,r,angle):
 verts=[];faces=[]
 for j in range(3):
  t=j*2*math.pi/3;cx,cy=math.cos(t)*.35,math.sin(t)*.35
  n=len(verts)
  for dx,dy in [(-.18,-.03),(.18,-.03),(.08,.03)]:
   xx,yy=(cx+dx)*r,(cy+dy)*r;verts.append((x+xx*math.cos(angle)-yy*math.sin(angle),y+xx*math.sin(angle)+yy*math.cos(angle),.03))
  faces.append((n,n+1,n+2))
 d=bpy.data.meshes.new('sparse ember strokes');d.from_pydata(verts,[],faces);d.materials.append(ember);o=bpy.data.objects.new('embers',d);bpy.context.collection.objects.link(o);dynamic.append(o)
roles=names[2:10];phase=['TITLE','WAVE','REST','BOSS','WON','DEAD']
indices=[int(i) for i in a.indices.split(',')] if a.indices else range(len(trace['frames']))
for index in indices:
 for o in dynamic:
  d=o.data;bpy.data.objects.remove(o,do_unlink=True)
  if d.users==0:bpy.data.meshes.remove(d)
 dynamic.clear();f=trace['frames'][index];hero=f['player'];px,py=hero['x'],hero['y'];focus=Vector((max(-32,min(32,px)),max(-27+9/math.sin(math.radians(62)),min(27-9/math.sin(math.radians(62)),py)),0));camera.location=focus+Vector((0,-45*math.cos(math.radians(62)),45*math.sin(math.radians(62))))
 keep(inst('hero_body',(px,py,0),1,math.atan2(hero['facingY'],hero['facingX'])),'hero');ring(px,py,.65,gold,.04,.018)
 if trace.get('hero')==1:keep(inst('hero_sword',(px+.45,py,1.25),.8,math.atan2(hero['facingY'],hero['facingX'])),'hero sword')
 for e in f['enemies']:
  angle=math.atan2(e.get('directionY',py-e['y']),e.get('directionX',px-e['x']));keep(inst(roles[e['role']],(e['x'],e['y'],0),1.55 if e.get('tier')==2 else 1,angle),'enemy_'+str(e['id']))
 for e in f['projectiles']:
  m=red if e['hostile'] else purple if e.get('element',1 if e['source'] in [1,5,8,9] else 0)==1 else orange
  bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=e['radius'],location=(e['x'],e['y'],.65));o=bpy.context.object;o.data.materials.append(m);keep(o,'projectile_'+str(e['id'])+'_source_'+str(e['source']))
 def is_aura(e):return not e['hostile'] and e['kind']!=4 and (e.get('followPlayer',False) or (e['kind']==2 and e['radius']>1 and (e['x']-px)**2+(e['y']-py)**2<.64))
 friendly_count=0;burn_cells=set();aura=max((e['radius'] for e in f['effects'] if is_aura(e)),default=0)
 if aura: ring(px,py,aura,faint,.03,aura*.0075);friendly_count+=1
 for e in f['effects']:
  x,y,r=e['x'],e['y'],max(.05,e['radius']);warning=e['warning']>0;hostile=e['hostile'];age=e.get('duration',e['life'])-e['life'];m=red if hostile else purple if e.get('element',1 if e['source'] in [1,5,8,9] else 0)==1 else orange;begin=len(dynamic);z=.08 if hostile else .03
  if is_aura(e):continue
  if not hostile and friendly_count>=96:continue
  if not hostile and e['kind']==2:
   if r>1:ring(x,y,r,faint,z,r*.0075)
   else:
    cell=(math.floor(x/.8),math.floor(y/.8))
    if cell in burn_cells:continue
    burn_cells.add(cell);embers(x,y,min(r,.65),math.radians(-e['id']%180))
  elif e['kind']==6:
   elapsed=0 if warning else max(0,e.get('duration',6)-e['life']);xx,yy=48-elapsed*5,27-elapsed*2.5
   for sign in [-1,1]:line(sign*(xx-1),-yy,sign*(xx-1),yy,.12,m);line(-xx,sign*(yy-1),xx,sign*(yy-1),.12,m)
  elif e['kind']==1:
   if not hostile and age>.18:continue
   ex,ey=e['endX'],e['endY'];dx,dy=ex-x,ey-y;length=max(.001,math.hypot(dx,dy))
   if warning:
    for sign in [-1,1]:line(x-sign*dy/length*r,y+sign*dx/length*r,ex-sign*dy/length*r,ey+sign*dx/length*r,.08,m,z)
    line(ex-dy/length*r,ey+dx/length*r,ex+dy/length*r,ey-dx/length*r,.08,m,z)
   else:line(x,y,ex,ey,.08 if hostile else .035,m,z)
  elif e['kind']==4 and e['source']==4:
   keep(inst('hero_sword',(x,y,.63),.7,math.atan2(y-py,x-px)+math.pi/2),'orbiting flame blade')
  elif e['kind']==4:
   angle=math.atan2(e['endY']-y,e['endX']-x)
   for j in range(12):
    if not hostile and friendly_count+len(dynamic)-begin>=96:break
    aa=angle-math.pi/3+j*math.pi/18;bb=aa+math.pi/18;line(x+r*math.cos(aa),y+r*math.sin(aa),x+r*math.cos(bb),y+r*math.sin(bb),.09 if hostile else .025+.04*math.sin(math.pi*(j+.5)/12),m,z)
  elif hostile:ring(x,y,r,m,z)
  elif e['kind']==3:
   if warning:ring(x,y,r,thunder_faint,z,r*.0075)
   elif age<.18:dynamic.append(box((x,y,1.13),(.045,.045,2.2),m))
  else:ring(x,y,r,faint,z,r*.0075)
  if not hostile:friendly_count+=len(dynamic)-begin
  for o in dynamic[begin:]:o.name='effect_'+str(e['id'])+'_source_'+str(e['source'])
 header.data.body=f"CORE SNAPSHOT   t={f['time']:.2f}s   HP {hero['hp']:.1f}   {phase[f['phase']]}   boss phase {f['bossPhase']}"
 scene.render.filepath=str(out/f'frame-{index:04d}.png');bpy.ops.render.render(write_still=True)
meta={'tool':bpy.app.version_string,'engine':'Cycles CPU','samples':a.samples,'resolution':[960,540],'sampleHz':trace['sampleHz'],'framesRendered':list(indices),'presentation':'readability-v2: coalesced sparse amber burn marks; 96 friendly instance budget; hostile boundaries unbudgeted; no simulation changes','traceSHA256':hashlib.sha256(raw).hexdigest(),'sourceSHA256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'meshBlendSHA256':hashlib.sha256((HERE/'mvp-meshes.blend').read_bytes()).hexdigest(),'renderSeconds':time.monotonic()-start,'limitations':['Offline simulation snapshot illustration, not Unreal runtime, compilation, performance or final-quality evidence','Rigid source meshes; no rigging or animation beyond recorded transforms','No absent inventory, pickups, kills or extra spawns inferred','Camera approximately matches adapter; no production HUD'],'tracePath':str(a.trace)}
(out/'render-metadata.json').write_text(json.dumps(meta,indent=2));print('TRACE_RENDER_DONE',json.dumps(meta))
