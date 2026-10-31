"""Original deterministic mesh art; Blender 4.x/5.x offline CPU, no downloads."""
import bpy, math, random, json, os
from mathutils import Vector
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'Content/MVP/Geometry'; HERE=Path(__file__).resolve().parent
# Original accepted base envelopes: keep art revisions inside the collision/placement contract.
BASE_BOUNDS={'hero_body': [[-0.65, -0.45361, 0.0], [0.499751, 0.45361, 1.793]], 'hero_sword': [[-0.61, -0.19, -0.0], [1.18, 0.19, 0.16]], 'enemy_swarm': [[-0.47, -0.480942, 0.005], [0.49, 0.480942, 1.154528]], 'enemy_exploder': [[-0.32185, -0.338121, 0.19], [0.343637, 0.338121, 1.26]], 'enemy_charger': [[-1.02, -0.431298, 0.005], [0.94, 0.431298, 1.31]], 'enemy_boss': [[-1.785, -0.754772, 0.00875], [1.645, 0.754772, 2.2925]], 'enemy_rangedbird': [[-1.0, -1.0, 0.48], [0.53, 1.0, 1.18]], 'enemy_supporttotem': [[-0.303109, -0.35, 0.0], [0.303109, 0.35, 1.33]], 'enemy_summoner': [[-0.56, -0.433013, 0.06], [0.56, 0.433013, 0.94]], 'enemy_spirit': [[-0.635453, -0.151609, 0.025004], [0.49515, 0.151609, 0.85]], 'rock': [[-0.828994, -0.818085, 0.0], [0.940872, 0.997783, 1.060426]], 'pillar': [[-0.5, -0.5, 0.0], [0.5, 0.5, 2.83]], 'tile': [[-0.49, -0.49, -0.14], [0.49, 0.49, 0.002]], 'shrine_banner': [[-0.24, -0.747063, 0.0], [0.24, 0.747063, 3.05]], 'shrine_arch': [[-0.7, -2.65, 0.0], [0.7, 2.65, 4.42]]}
random.seed(73)
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
# Linear-light palette; subdued stone and ivory, saturated accents restricted to details.
P={'ivory':(.72,.69,.59,1),'white':(.9,.88,.79,1),'cloth':(.27,.30,.33,1),'ink':(.018,.024,.032,1),'skin':(.56,.32,.20,1),'gold':(.36,.22,.075,1),'brightgold':(.64,.37,.105,1),'blue':(.10,.42,.65,1),'stone':(.060,.068,.078,1),'edge':(.12,.135,.15,1),'demon':(.068,.060,.069,1),'bone':(.25,.25,.23,1),'mane':(.65,.61,.52,1),'red':(.31,.035,.022,1),'fire':(.83,.105,.025,1),'purple':(.23,.085,.32,1),'eye':(.85,.63,.21,1)}
M={}
for k,c in P.items():
 m=bpy.data.materials.new(k); m.diffuse_color=c; m.use_nodes=True
 bs=m.node_tree.nodes.get('Principled BSDF'); bs.inputs['Base Color'].default_value=c; bs.inputs['Roughness'].default_value=.84 if 'gold' in k else .9
 if k in ['gold','brightgold']: bs.inputs['Metallic'].default_value=.22
 if k in ['fire','purple','eye','blue']: bs.inputs['Emission Color'].default_value=c; bs.inputs['Emission Strength'].default_value=.28
 M[k]=m
parts=[]; assets={}
def finish(o,mat,smooth=True):
 o.data.materials.append(M[mat]); parts.append(o)
 if smooth:
  for p in o.data.polygons:p.use_smooth=True
 return o

def ell(pos,scale,mat,seg=10,rings=6):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=seg,ring_count=rings,location=pos)
 o=bpy.context.object; o.scale=scale
 # Flatten broad shell/armor planes rather than shading every mass like a bead.
 if mat in {'stone','demon','ink'} and min(scale)>.055:
  for v in o.data.vertices:
   v.co=Vector(tuple(math.copysign(abs(c)**.72,c) for c in v.co))
  return finish(o,mat,mat=='demon')
 return finish(o,mat,mat in {'skin','fire','eye','mane'})
def rod(a,b,r1,r2,mat,n=8):
 d=Vector(b)-Vector(a); bpy.ops.mesh.primitive_cone_add(vertices=n,radius1=r1,radius2=r2,depth=d.length,location=(Vector(a)+Vector(b))/2)
 o=bpy.context.object; o.rotation_euler=d.to_track_quat('Z','Y').to_euler(); return finish(o,mat,mat in {'skin','fire'})
def box(pos,scale,mat,bevel=.04):
 bpy.ops.mesh.primitive_cube_add(size=2,location=pos);o=bpy.context.object;o.scale=scale
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  mod=o.modifiers.new('soft chisel','BEVEL');mod.width=bevel;mod.segments=2
  bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
 return finish(o,mat,False)
def mesh(v,f,mat):
 d=bpy.data.meshes.new('sculpted');d.from_pydata(v,[],f);d.update();o=bpy.data.objects.new('sculpted',d);bpy.context.collection.objects.link(o);return finish(o,mat,False)
def horn(points,r,mat,n=7):
 # One continuous swept surface: no overlapping cone collars at every bend.
 pts=[Vector(p) for p in points];v=[];f=[]
 for i,p in enumerate(pts[:-1]):
  tangent=(pts[min(i+1,len(pts)-1)]-pts[max(0,i-1)]).normalized()
  u=tangent.cross(Vector((0,0,1)))
  if u.length<.01:u=tangent.cross(Vector((0,1,0)))
  u.normalize();w=tangent.cross(u).normalized();radius=r*(1-i/(len(pts)-1))
  for j in range(n):v.append(p+radius*(u*math.cos(2*math.pi*j/n)+w*math.sin(2*math.pi*j/n)))
 f.append(tuple(reversed(range(n))))
 for i in range(len(pts)-2):
  for j in range(n):f.append((i*n+j,i*n+(j+1)%n,(i+1)*n+(j+1)%n,(i+1)*n+j))
 tip=len(v);v.append(pts[-1]);last=(len(pts)-2)*n
 for j in range(n):f.append((last+j,last+(j+1)%n,tip))
 return mesh(v,f,mat)
def leaf(a,b,width,mat,depth=.025):
 # Thick tapering blade / cloth ribbon with intentional ridge.
 a,b=Vector(a),Vector(b); d=b-a; side=d.cross(Vector((0,0,1)))
 if side.length<.01:side=Vector((0,1,0))
 side.normalize();mid=a+d*.45
 v=[a-side*width*.25,a+side*width*.25,mid+side*width,b,mid-side*width,mid+Vector((0,0,depth)),mid-Vector((0,0,depth))]
 return mesh(v,[(0,1,5),(1,2,5),(2,3,5),(3,4,5),(4,0,5),(1,0,6),(2,1,6),(3,2,6),(4,3,6),(0,4,6)],mat)
def ring(pos,r,minor,mat,major=16):
 bpy.ops.mesh.primitive_torus_add(major_segments=major,minor_segments=4,location=pos,major_radius=r,minor_radius=minor)
 return finish(bpy.context.object,mat)
def tag_since(start,name):
 for o in parts[start:]:
  g=o.vertex_groups.new(name=name);g.add(list(range(len(o.data.vertices))),1.0,'REPLACE')
def end(name):
 bpy.context.view_layer.update()
 # Deterministic explicit merge: bpy.ops.object.join orders by internal allocation.
 vertices=[];faces=[];materials=[];face_mats=[];smooth=[];groups={}
 for src in parts:
  offset=len(vertices);vertices.extend([src.matrix_world@v.co for v in src.data.vertices])
  slots=[]
  for mat in src.data.materials:
   if mat not in materials:materials.append(mat)
   slots.append(materials.index(mat))
  for poly in src.data.polygons:
   faces.append(tuple(offset+i for i in poly.vertices));face_mats.append(slots[poly.material_index]);smooth.append(poly.use_smooth)
  for group in src.vertex_groups:
   groups.setdefault(group.name,[]).extend(offset+v.index for v in src.data.vertices if any(g.group==group.index for g in v.groups))
 d=bpy.data.meshes.new(name);d.from_pydata(vertices,[],faces);d.update()
 o=bpy.data.objects.new(name,d);bpy.context.collection.objects.link(o)
 for mat in materials:d.materials.append(mat)
 for poly,mat,sm in zip(d.polygons,face_mats,smooth):poly.material_index=mat;poly.use_smooth=sm
 for key,ids in groups.items():o.vertex_groups.new(name=key).add(ids,1.0,'REPLACE')
 for src in parts:bpy.data.objects.remove(src,do_unlink=True)
 bpy.context.view_layer.objects.active=o
 # Keep original world-axis envelopes exactly; no simulation/collision changes.
 if name in BASE_BOUNDS:
  old_min,old_max=BASE_BOUNDS[name]
  if name=='enemy_boss':old_min=[v/1.75 for v in old_min];old_max=[v/1.75 for v in old_max]
  lo=[min(v.co[i] for v in o.data.vertices) for i in range(3)]
  hi=[max(v.co[i] for v in o.data.vertices) for i in range(3)]
  for v in o.data.vertices:
   for i in range(3):v.co[i]=old_min[i]+(v.co[i]-lo[i])/(hi[i]-lo[i])*(old_max[i]-old_min[i])
 o.data.update()
 # Authored broad facet values, shared by Blender previews and runtime vertex colours.
 # Not ambient-occlusion baking or a shader dependency: restrained hand-painted grouping.
 originals=list(o.data.materials);tone_slots={}
 for face in o.data.polygons:
  base=originals[face.material_index]
  if base.name in {'eye','fire','purple','blue','skin'}:continue
  normal=face.normal
  level=2 if normal.z>.55 else 0 if normal.z<-.25 else 1
  # Sparse edge wear, not random per-vertex noise.
  if base.name in {'stone','edge','demon','bone'} and int(sum(abs(c)*997 for c in face.center))%17==0:level=min(2,level+1)
  key=(base.name,level)
  if key not in tone_slots:
   shade=bpy.data.materials.get(base.name+'_facet_'+str(level))
   if shade is None:
    shade=base.copy();shade.name=base.name+'_facet_'+str(level)
    factor=(.72,.92,1.08)[level]
    shade.diffuse_color=tuple(min(1,c*factor) for c in base.diffuse_color[:3])+(1,)
    shade.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=shade.diffuse_color
   tone_slots[key]=len(o.data.materials);o.data.materials.append(shade)
  face.material_index=tone_slots[key]
 assets[name]=o;parts.clear();o.hide_render=True;o.hide_set(True)
 return o

def robe():
 # Long lean silhouette; open-front coat built as tailored pleated panels, not a cone.
 n=16;v=[]
 for z,rx,ry in [(.32,.20,.31),(.82,.13,.18),(1.05,.14,.17),(1.37,.13,.23)]:
  for i in range(n):
   t=2*math.pi*i/n;fold=1+(.06 if i%2 else -.02)
   v.append((rx*math.cos(t)*fold,ry*math.sin(t)*fold,z+(.10*math.cos(t*3) if z<.4 else 0)))
 f=[]
 for j in range(3):
  for i in range(n):
   if j<2 and i in [0,1,14,15]:continue
   f.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i))
 f.append(tuple(range(3*n,4*n)));mesh(v,f,'ivory')
 for side in [-1,1]:
  leaf((-.04,side*.13,.92),(-.52,side*.34,.28),.115,'white',.02)
  leaf((-.12,side*.12,.95),(-.65,side*.22,.51),.075,'ivory',.02)
  leaf((.13,side*.11,1.37),(.16,side*.04,1.02),.024,'gold',.006)
  # Deliberate walking stance with slimmer ankle, soft boot upper and forward toe.
  start=len(parts)
  hip=(0,side*.10,.88);knee=(side*.08,side*.16,.48);ankle=(side*.14,side*.18,.12)
  rod(hip,knee,.075,.06,'cloth',10);rod(knee,ankle,.06,.045,'ink',10)
  ell((side*.14+.06,side*.18,.055),(.13,.063,.055),'ink',10,5)
  rod((side*.14,side*.18,.13),(side*.14,side*.18,.21),.052,.053,'gold',8)
  tag_since(start,'leg_'+str(side))
robe()
# Approximately 1:7 head/body ratio, chin, nose, brows and swept-back long hair.
rod((0,0,1.36),(0,0,1.49),.047,.048,'skin',10)
ell((.018,0,1.57),(.080,.073,.118),'skin',14,9)
ell((-.026,0,1.635),(.089,.082,.082),'ink',14,8)
ell((-.065,0,1.745),(.040,.04,.048),'ink',10,6)
rod((-.08,0,1.722),(-.08,0,1.742),.046,.046,'gold',10)
for side in [-1,1]:
 rod((.086,side*.02,1.598),(.076,side*.058,1.60),.006,.007,'ink',5)
 rod((.08,side*.025,1.58),(.075,side*.048,1.58),.004,.004,'ink',5)
 for i in range(3):
  horn([(-.07,side*(.035+i*.018),1.65),(-.13,side*(.065+i*.022),1.43),(-.30,side*(.06+i*.025),1.23),(-.47-i*.035,side*.12,1.20+i*.05)],.036,'ink',7)
rod((.085,0,1.574),(.108,0,1.55),.016,.008,'skin',6)
# Asymmetric casting pose. Broad hanging cuff but slender articulated arm.
for side in [-1,1]:
 start=len(parts)
 shoulder=(0,side*.21,1.32);elbow=(.14 if side==1 else -.05,side*.34,1.12);wrist=(.42 if side==1 else .12,side*.37,1.21 if side==1 else .91)
 rod(shoulder,elbow,.085,.11,'white',12)
 rod(elbow,wrist,.068,.045,'cloth',10)
 leaf(elbow,(elbow[0]-.09,elbow[1]+side*.035,elbow[2]-.32),.105,'ivory',.025)
 q=Vector(wrist);d=(q-Vector(elbow)).normalized();rod(q-d*.08,q-d*.015,.054,.05,'gold',10)
 ell(q+d*.035,(.049,.045,.045),'skin',10,6)
 tag_since(start,'arm_'+str(side))
rod((0,0,.96),(0,0,1.025),.18,.18,'ink',16)
box((.169,0,.99),(.018,.052,.031),'gold',.01)
leaf((.16,0,.97),(.17,-.08,.51),.047,'red',.006)
# Layered tailoring: ivory lapels, fluted shoulder cloth and embroidered hem.
for side in [-1,1]:
 for k in range(5):
  leaf((-.05,side*(.12+k*.013),1.31),(-.29-k*.028,side*(.17+k*.024),.42+k*.025),.017,'white',.006)
 horn([(.145,side*.06,1.36),(.17,side*.09,1.17),(.17,side*.025,1.03)],.013,'brightgold',6)
 for k in range(4):
  leaf((-.12-k*.07,side*.18,.49),(-.19-k*.07,side*.20,.42),.016,'gold',.004)
end('hero_body')
# Floating jian, clearly a separate rigid mesh; centre at blade guard, +X blade direction.
leaf((0,0,.10),(1.18,0,.10),.11,'white',.035)
leaf((.04,0,.115),(1.10,0,.115),.028,'blue',.025)
rod((-.28,0,.10),(0,0,.10),.038,.038,'ink',8)
rod((0,-.19,.1),(0,.19,.1),.035,.035,'gold',8)
ell((-.30,0,.1),(.06,.06,.06),'gold',8,5)
horn([(-.3,0,.1),(-.44,.06,.04),(-.61,.04,0)],.025,'blue',5)
end('hero_sword')
# Low predatory demon, narrow skull, long forearms, scapulae and curled horns.
ell((-.15,0,.70),(.23,.24,.29),'demon',12,8)
ell((-.25,0,.82),(.20,.22,.14),'stone',12,6)
ell((.12,0,.85),(.14,.13,.15),'demon',12,7)
ell((.24,0,.78),(.12,.105,.07),'ink',10,6)
for side in [-1,1]:
 start=len(parts)
 rod((-.13,side*.16,.50),(-.27,side*.27,.28),.10,.065,'demon',10)
 rod((-.27,side*.27,.28),(.01,side*.29,.07),.065,.035,'demon',8)
 ell((.07,side*.29,.055),(.13,.065,.05),'ink',8,5)
 tag_since(start,'leg_'+str(side))
 start=len(parts)
 ell((-.07,side*.23,.78),(.095,.09,.12),'stone',10,6)
 rod((-.04,side*.24,.76),(.07,side*.42,.48),.075,.048,'demon',10)
 rod((.07,side*.42,.48),(.38,side*.43,.16),.05,.035,'demon',10)
 rod((.16,side*.425,.39),(.27,side*.43,.26),.056,.043,'bone',8)
 for k in [-1,0,1]:horn([(.35,side*.43+k*.032,.17),(.45,side*.43+k*.04,.065),(.49,side*.43+k*.04,.09)],.018,'bone',5)
 tag_since(start,'arm_'+str(side))
 horn([(.06,side*.10,.96),(-.03,side*.18,1.08),(-.17,side*.23,1.14),(-.22,side*.22,1.08)],.047,'bone',7)
 ell((.238,side*.075,.875),(.018,.031,.009),'fire',8,4)
 rod((.23,side*.045,.90),(.18,side*.12,.925),.022,.028,'bone',6)
 rod((.32,side*.07,.775),(.34,side*.07,.70),.017,0,'bone',5)
 for k in range(3):leaf((-.16-k*.07,side*.15,.87),(-.39-k*.04,side*.28,.94),.035,'bone',.014)
# Ragged pale mane marks the hunched shoulder line at tactical camera distance.
for side in [-1,1]:
 for k in range(5):
  horn([(.04-k*.055,side*.12,.96),(-.07-k*.06,side*.20,1.02),(-.21-k*.045,side*.25,.89)],.034,'mane',5)
end('enemy_swarm')
# Volatile spirit: broken charcoal shell around a hot core with upward licking flares.
ell((0,0,.52),(.25,.25,.33),'fire',10,7)
for i in range(7):
 t=i*2*math.pi/7;x,y=.22*math.cos(t),.22*math.sin(t)
 ell((x,y,.45),(.13,.13,.20),'demon',8,5)
 horn([(x,y,.59),(x*1.3,y*1.3,.84),(x*.6,y*.6,1.1+(i%3)*.08)],.08,'brightgold' if i%2 else 'fire',6)
ring((0,0,.23),.19,.032,'gold',12)
end('enemy_exploder')

def beast(boss=False):
 scale=1.75 if boss else 1
 # Local shape assembled at ordinary size, scaled after join.
 ell((-.08,0,.65),(.53,.30,.34),'stone' if boss else 'demon',12,7)
 ell((.43,0,.76),(.28,.31,.29),'stone' if boss else 'demon',10,7)
 ell((.69,0,.60),(.25,.155,.105),'demon',12,6)
 ell((.74,0,.53),(.19,.13,.035),'ink',10,4)
 for s in [-1,1]:
  for x in [-.4,.32]:
   ell((x,s*.25,.46),(.16,.15,.23),'stone' if boss else 'demon',8,5)
   rod((x,s*.25,.46),(x+.1,s*.28,.11),.10,.075,'demon')
   ell((x+.19,s*.28,.08),(.19,.10,.075),'ink',8,5)
   for k in [-1,0,1]:rod((x+.26,s*.28+k*.055,.075),(x+.37,s*.28+k*.055,.045),.025,0,'bone',5)
  ell((.65,s*.20,.79),(.07,.027,.017),'purple' if boss else 'fire',8,4)
  rod((.62,s*.19,.835),(.49,s*.27,.86),.045,.055,'stone',7)
  for k in range(3):rod((.71+k*.07,s*.125,.58),(.74+k*.07,s*.125,.48),.024,0,'bone',5)
  horn([(.42,s*.22,.92),(.28,s*.36,1.13),(.04,s*.40,1.27),(-.09,s*.38,1.31)],.095,'gold',8)
  for k in range(5):
   horn([(.35-k*.11,s*.25,.89),(.15-k*.13,s*(.30+.018*k),1.02),(-.12-k*.15,s*(.32+.02*k),.96),(-.30-k*.16,s*(.29+.025*k),1.04)],.07,'mane' if boss else 'gold',7)
 for i in range(5):
  horn([(.35-i*.13,0,.96),(.17-i*.14,0,1.10),(-.05-i*.16,0,1.04),(-.2-i*.18,0,1.14)],.085,'mane' if boss else 'brightgold',7)
 horn([(-.48,0,.68),(-.85,0,.87),(-1.02,.16,1.03)],.07,'bone' if boss else 'gold')
 if boss:
  for i in range(4):
   for s in [-1,1]:
    ell((-.36+i*.17,s*.13,.94),(.12,.115,.075),'stone',8,5)
    leaf((-.40+i*.16,s*.23,.92),(-.3+i*.16,s*.31,.69),.025,'purple',.013)
 # Dense overlapping swept mane locks, grown from the shoulder ridge rather than a bead halo.
 for side in [-1,1]:
  for row in range(3):
   for k in range(6):
    x=.43-k*.12; y=side*(.18+row*.045); z=.92-row*.10
    horn([(x,y,z),(x-.10,y+side*.08,z+.09),(x-.29,y+side*.11,z+.03),(x-.43,y+side*.07,z-.04)],.044 if row else .058,'mane' if boss else 'brightgold',5)
 name='enemy_boss' if boss else 'enemy_charger';o=end(name)
 for v in o.data.vertices:v.co*=scale
beast();beast(True)
# Ranged bird: spread layered wings and long flowing tail; no flat billboard.
ell((0,0,.88),(.26,.16,.17),'ink');ell((.24,0,1.03),(.13,.12,.15),'demon')
rod((.32,0,1.03),(.53,0,.99),.075,0,'gold',6)
for s in [-1,1]:
 rod((-.03,s*.11,.92),(-.14,s*.58,1.10),.12,.075,'demon')
 for i in range(5):
  leaf((-.09-i*.045,s*(.23+i*.09),1.02),(-.48-i*.10,s*(.58+i*.10),.91-i*.045),.10,'demon',.035)
  leaf((-.34-i*.07,s*(.43+i*.10),.98),(-.54-i*.10,s*(.60+i*.1),.90-i*.045),.047,'fire')
 rod((0,s*.075,.81),(.07,s*.09,.57),.026,.02,'gold',5)
 ell((.32,s*.08,1.08),(.025,.025,.025),'fire',8,4)
for i in [-1,0,1]:leaf((-.2,i*.06,.88),(-1.0,i*.22,.48),.10,'gold' if i==0 else 'red')
end('enemy_rangedbird')
# Support reliquary: pagoda cap, engraved gold bands, central violet jade.
for z,r1,r2,mat in [(.12,.35,.29,'stone'),(.32,.25,.22,'edge'),(.66,.15,.15,'stone'),(1.04,.33,.12,'gold')]:
 rod((0,0,z-.12),(0,0,z+.12),r1,r2,mat,6)
ell((0,0,.69),(.17,.17,.25),'purple',8,5)
for i in range(4):
 t=i*math.pi/2;x,y=.20*math.cos(t),.20*math.sin(t)
 rod((x,y,.34),(x,y,.91),.045,.035,'gold',6)
ring((0,0,.93),.23,.025,'gold',12);rod((0,0,1.15),(0,0,1.33),.055,0,'gold',6)
end('enemy_supporttotem')
# Summoner shell / tortoise: radial layered carapace with glowing ceremonial obelisk.
ell((0,0,.32),(.43,.36,.26),'demon',10,6)
for i in range(6):
 t=i*math.pi/3
 ell((.25*math.cos(t),.23*math.sin(t),.45),(.22,.18,.13),'stone',8,5)
 leaf((.2*math.cos(t),.2*math.sin(t),.43),(.56*math.cos(t),.5*math.sin(t),.23),.11,'bone')
ring((0,0,.56),.22,.025,'gold',12)
rod((0,0,.54),(0,0,.94),.15,0,'purple',5)
for s in [-1,1]:ell((.39,s*.10,.27),(.04,.04,.035),'eye',8,4)
end('enemy_summoner')
# Gold spirit fox: recognisable head, ears, feet and sweeping tail.
ell((0,0,.32),(.27,.12,.15),'gold',10,6);ell((.23,0,.46),(.14,.12,.14),'brightgold',10,6)
rod((.3,0,.44),(.49,0,.4),.09,.025,'ivory',6)
for s in [-1,1]:
 horn([(.20,s*.08,.54),(.16,s*.12,.76)],.075,'brightgold',6)
 ell((.32,s*.078,.49),(.02,.018,.02),'ink',8,4)
 for x in [-.16,.14]:rod((x,s*.09,.28),(x+.05,s*.10,.03),.04,.025,'gold',6)
horn([(-.21,0,.33),(-.48,0,.44),(-.59,0,.68),(-.38,0,.85)],.15,'brightgold',8)
end('enemy_spirit')
# Angular real rock; proportions deliberately not an unmodified sphere.
bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2,radius=1);o=bpy.context.object
for v in o.data.vertices:
 x,y,z=v.co;v.co=(x*(.84+random.random()*.16),y*(.8+random.random()*.2),max(0,(z+.65)*(.52+random.random()*.2)))
finish(o,'stone',False)
o.data.materials.append(M['edge'])
for face in o.data.polygons:
 if face.index%9==0:face.material_index=1
end('rock')
# Shrine pillar with carved recess, crown and warm brazier.
for z,size,mat in [(.1,(.50,.50,.10),'stone'),(.27,(.43,.43,.08),'edge'),(1.20,(.32,.32,.85),'stone'),(2.08,(.43,.43,.09),'edge'),(2.22,(.37,.37,.06),'stone')]:box((0,0,z),size,mat,.035)
for s in [-1,1]:
 box((s*.328,0,1.22),(.016,.20,.57),'ink',.008)
 for y in [-.24,.24]:rod((s*.34,y,.62),(s*.34,y,1.82),.022,.022,'gold',4)
 for z in [.76,1.18,1.60]:
  leaf((s*.35,-.11,z), (s*.35,.11,z+.2),.045,'gold')
 rod((s*.30,-.30,2.18),(s*.30,-.30,2.52),.065,.035,'stone',6)
 rod((s*.30,.30,2.18),(s*.30,.30,2.52),.065,.035,'stone',6)
rod((0,0,2.24),(0,0,2.36),.15,.27,'gold',12)
ring((0,0,2.36),.25,.035,'gold',12)
for i in range(4):
 t=i*math.pi/2;horn([(.13*math.cos(t),.13*math.sin(t),2.28),(.16*math.cos(t),.16*math.sin(t),2.57),(.03,0,2.83)],.09,'fire',6)
end('pillar')
box((0,0,-.07),(.49,.49,.07),'stone',.025)
# Inlaid hairline crack and small chipped corner, real triangles not a texture.
for a,b in [((-.40,.27,.001),(-.18,.18,.001)),((-.18,.18,.001),(-.12,-.06,.001)),((-.12,-.06,.001),(.12,-.19,.001))]:
 leaf(a,b,.005,'ink',.001)
end('tile')
# Two optional environment meshes: hanging vermilion temple standard and ruined gate.
rod((0,0,.08),(0,0,3.05),.04,.025,'ink',10)
rod((0,-.68,2.9),(0,.68,2.9),.035,.035,'gold',8)
box((0,0,.08),(.24,.24,.08),'stone',.04)
for side in [-1,1]:ell((0,side*.69,2.9),(.06,.06,.06),'gold',8,5)
verts=[];w=.56;rows=9;cols=8
for back in [-1,1]:
 for j in range(rows):
  for i in range(cols):
   y=(i/(cols-1)*2-1)*w;z=2.82-j*.18
   if j==rows-1:z-=.16 if i%2==0 else .02
   x=.07*math.sin(j*.7+i*.55)+back*.006
   verts.append((x,y,z))
faces=[];count=rows*cols
for j in range(rows-1):
 for i in range(cols-1):
  k=j*cols+i;faces.append((k,k+cols,k+cols+1,k+1));faces.append((k+count+1,k+count+cols+1,k+count+cols,k+count))
for j in range(rows-1):
 for i in [0,cols-1]:
  k=j*cols+i;faces.append((k,k+count,k+count+cols,k+cols))
for i in range(cols-1):
 for j in [0,rows-1]:
  k=j*cols+i;faces.append((k,k+1,k+1+count,k+count))
mesh(verts,faces,'red')
# Gold talisman strokes on both front/back faces; not a text/logo decal.
for side in [-1,1]:
 for j in range(4):
  z=2.61-j*.24
  rod((side*.09,-.13,z),(side*.09,.13,z+.10),.015,.015,'gold',5)
 rod((side*.09,0,2.66),(side*.09,0,1.69),.014,.014,'gold',5)
end('shrine_banner')
# Entrance broken paifang: two stepped plinths, layered carved posts, sloping tiled lintels.
for side in [-1,1]:
 y=side*1.8
 for z,sz,mat in [(.12,(.7,.6,.12),'stone'),(.34,(.54,.48,.10),'edge'),(1.95,(.32,.32,1.5),'stone'),(3.54,(.45,.45,.09),'edge')]:box((0,y,z),sz,mat,.04)
 for x in [-.33,.33]:
  box((x,y,1.95),(.012,.19,1.02),'ink',.01)
  for k in range(5):rod((x,y-.12,1.10+k*.35),(x,y+.12,1.26+k*.35),.016,.016,'gold',5)
 for x in [-.48,.48]:rod((x,y-.43,.51),(x,y+.43,.51),.04,.04,'gold',6)
box((0,0,3.70),(.37,2.30,.16),'stone',.06)
box((0,0,3.96),(.48,2.50,.10),'edge',.035)
box((0,0,4.12),(.35,2.20,.08),'stone',.03)
# Stepped eave tips introduce characteristic silhouette without a texture or a fake backdrop.
for side in [-1,1]:
 horn([(0,side*2.1,4.12),(0,side*2.45,4.18),(0,side*2.65,4.42)],.15,'stone',6)
for y in [-1.6,-1.2,-.8,-.4,0,.4,.8,1.2,1.6]:rod((-.40,y,4.06),(.40,y,4.06),.075,.075,'stone',6)
box((.39,0,3.70),(.026,.56,.12),'ink',.01)
for k in range(3):rod((.423,-.35+k*.35,3.64),(.423,-.25+k*.35,3.78),.024,.024,'gold',5)
end('shrine_arch')

# Reference-driven courtyard kit. Decorative silhouettes do not replace collision meshes.
# Tall carved stele, double mouldings, recessed panels and repeated cloud-scroll relief.
for z,sz,mat in [(.10,(.70,.70,.10),'stone'),(.26,(.62,.62,.07),'edge'),(.42,(.55,.55,.09),'stone'),(2.53,(.39,.39,2.02),'stone'),(4.62,(.49,.49,.09),'edge'),(4.82,(.58,.58,.10),'stone'),(5.02,(.48,.48,.08),'edge')]:box((0,0,z),sz,mat,.035)
for axis in range(2):
 for side in [-1,1]:
  def facept(u,z,depth=.405):return (side*depth,u,z) if axis==0 else (u,side*depth,z)
  o=box(facept(0,2.54),(.012,.285,1.77) if axis==0 else (.285,.012,1.77),'ink',.012)
  for u in [-.29,.29]:rod(facept(u,.78,.43),facept(u,4.30,.43),.025,.025,'edge',6)
  for j in range(7):
   z=.93+j*.47
   pts=[facept(-.19,z,.44),facept(.07,z+.08,.445),facept(.17,z+.24,.445),facept(-.01,z+.31,.44),facept(-.13,z+.19,.44)]
   horn(pts,.023,'gold',5)
  for z in [.66,4.42]:rod(facept(-.30,z,.43),facept(.30,z,.43),.035,.035,'edge',6)
for side in [-1,1]:
 horn([(side*.29,0,5.1),(side*.46,0,5.35),(side*.35,0,5.55)],.13,'stone',7)
rod((0,0,5.02),(0,0,5.48),.20,.11,'edge',8)
end('courtyard_pillar')
# Low Chinese stone balustrade; rail length 4m along X, open spaces stay truly open.
box((0,0,.10),(2,.25,.10),'stone',.025)
for x in [-1.85,0,1.85]:
 box((x,0,.64),(.13,.17,.52),'stone',.025)
 box((x,0,1.15),(.19,.21,.06),'edge',.02)
 rod((x,0,1.2),(x,0,1.37),.105,.065,'stone',6)
for z in [.42,.94]:box((0,0,z),(1.88,.11,.075),'edge',.02)
for x in [-1.4,-.9,-.45,.45,.9,1.4]:
 box((x,0,.66),(.045,.07,.24),'stone',.012)
end('courtyard_rail')
# Ritual bronze brazier: three claw feet, hammered belly, rolled lip, curled handles.
for i in range(3):
 a=i*math.tau/3
 horn([(.32*math.cos(a),.32*math.sin(a),.64),(.48*math.cos(a),.48*math.sin(a),.30),(.56*math.cos(a),.56*math.sin(a),.06)],.085,'gold',7)
ell((0,0,.88),(.55,.55,.44),'gold',20,10)
rod((0,0,1.03),(0,0,1.19),.52,.61,'gold',20)
ring((0,0,1.20),.59,.045,'brightgold',24)
rod((0,0,1.20),(0,0,1.23),.52,.52,'ink',20)
for i in range(12):
 a=i*math.tau/12
 ell((.515*math.cos(a),.515*math.sin(a),.91),(.045,.045,.10),'brightgold',6,4)
for side in [-1,1]:
 horn([(0,side*.48,.95),(0,side*.76,1.05),(0,side*.78,1.37),(0,side*.61,1.45),(0,side*.55,1.28)],.062,'gold',8)
for i in range(7):
 a=i*2.399;r=.30 if i else 0
 horn([(r*math.cos(a),r*math.sin(a),1.22),(r*math.cos(a)+.08,r*math.sin(a),1.48),(r*math.cos(a)-.1,r*math.sin(a)+.04,1.72),(r*.6,r*.4,2.0+(i%3)*.09)],.10,'fire' if i%2 else 'eye',7)
end('courtyard_brazier')
# Windswept old pine. Branching swept trunk and clustered pointed needles, not spheres.
P['bark']=(.075,.047,.03,1);P['pine']=(.025,.075,.055,1)
for key in ['bark','pine']:
 m=M['stone'].copy();m.name=key;m.diffuse_color=P[key];m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=P[key];M[key]=m
horn([(0,0,0),(.12,.05,1.0),(-.30,0,2.1),(-.04,.12,3.1),(-.58,.18,4.1),(-.72,.30,5.15)],.37,'bark',10)
for i in range(9):
 a=i*2.399;z=2.35+i*.30;reach=2.5-(i%3)*.30
 base=Vector((-.2,0,z));tip=Vector((math.cos(a)*reach,math.sin(a)*reach,z+.5))
 horn([base,base.lerp(tip,.40)+Vector((0,0,-.18)),base.lerp(tip,.8),tip],.15-i*.008,'bark',7)
 for j in range(4):
  center=base.lerp(tip,.48+j*.17);center.z+=.22
  for k in range(11):
   aa=k*2.399+j;radius=.36+(k%3)*.13
   start=center+Vector((math.cos(aa)*radius*.15,math.sin(aa)*radius*.15,0))
   endp=center+Vector((math.cos(aa)*radius,math.sin(aa)*radius,.12+(k%4)*.065))
   leaf(start,endp,.105,'pine',.045)
for i in range(7):
 a=i*math.tau/7;horn([(0,0,.3),(.45*math.cos(a),.45*math.sin(a),.09),(.9*math.cos(a),.9*math.sin(a),.01)],.13,'bark',7)
end('courtyard_pine')
for j in range(4):box((0,j*.46,.08+j*.15),(2,.29,.08+j*.15),'stone',.025)
end('courtyard_steps')
# UV carrier assets for generated surface maps; intentionally no baked battle artwork.
box((0,0,-.045),(.50,.50,.045),'white',.008)
end('courtyard_slab')
rod((0,0,-.012),(0,0,.006),.5,.5,'white',96)
end('courtyard_medallion')

# Eight baked authored mesh poses, not skeletal animation. Base meshes remain untouched.
# Part groups keep boots, cuffs and claws coherent; small rotations leave the silhouette intact.
base_names=list(assets)
for base,prefix in [('hero_body','hero_walk'),('enemy_swarm','enemy_swarm_walk')]:
 src=assets[base];hero=base=='hero_body'
 for frame in range(8):
  o=src.copy();o.data=src.data.copy();bpy.context.collection.objects.link(o)
  name=f'{prefix}_{frame}';o.name=name;assets[name]=o
  phase=2*math.pi*frame/8
  for group in src.vertex_groups:
   side=int(group.name.split('_')[-1]);arm=group.name.startswith('arm')
   pivot=Vector((0,side*.21,1.32) if hero and arm else (0,side*.10,.88) if hero else (-.04,side*.24,.76) if arm else (-.13,side*.16,.50))
   angle=math.sin(phase)*side*(.14 if arm else .32)*(-1 if arm else 1)
   ids=[v.index for v in src.data.vertices if any(g.group==group.index for g in v.groups)]
   for i in ids:
    q=src.data.vertices[i].co-pivot
    o.data.vertices[i].co=pivot+Vector((q.x*math.cos(angle)+q.z*math.sin(angle),q.y,-q.x*math.sin(angle)+q.z*math.cos(angle)))
   # Ground clearance, faded to zero at hip/shoulder to preserve articulation.
   if not arm or not hero:
    target=.008+(.045*max(0,math.sin(phase)*side) if not arm else 0)
    lift=target-min(o.data.vertices[i].co.z for i in ids)
    if arm:lift=max(0,lift)
    for i in ids:
     v=o.data.vertices[i];v.co.z+=lift*max(0,min(1,(pivot.z-src.data.vertices[i].co.z)/(pivot.z-.08)))
  # Coat hem and swept hair have gentle travel lag, never a whole-body fake translation.
  if hero:
   for v in o.data.vertices:
    if not v.groups and v.co.x<-.12:
     weight=max(0,min(1,(-v.co.x-.12)/.5))
     v.co.y+=math.sin(phase+.6)*.025*weight
  o.data.update()
# Export indexed triangles and linear palette; smooth skin and hard authored facets.
result={'version':1,'coordinateSystem':'RH_Z_UP_X_FORWARD','winding':'CCW_OUTWARD','units':'metres','ueConversion':'(x,-y,z)*100; reverse triangle indices 1 and 2','meshes':[]}
for name,o in assets.items():
 d=o.data;d.calc_loop_triangles();vs=[];ns=[];cs=[];ts=[];uvs=[];dedup={}
 # Stable double-precision triangle normals and sorted sums avoid parallel float drift.
 import numpy as np
 coords=[tuple(round(x,6) for x in v.co) for v in d.vertices];smooth_sums={};tri_normals=[]
 for tr in d.loop_triangles:
  aa,bb,cc=[np.array(coords[i],dtype=float) for i in tr.vertices];nn=np.cross(bb-aa,cc-aa);length=np.linalg.norm(nn)
  nn=nn/length if length>1e-12 else np.array([0.,0.,1.]);tri_normals.append(tuple(nn))
  for vi in tr.vertices:smooth_sums.setdefault(vi,[]).append(tuple(nn))
 smooth_normals={}
 for vi,values in smooth_sums.items():
  nn=np.array([math.fsum(v[j] for v in sorted(values)) for j in range(3)]);length=np.linalg.norm(nn);smooth_normals[vi]=nn/length if length>1e-12 else np.array([0.,0.,1.])
 for t in d.loop_triangles:
  a,b,c=[d.vertices[i].co for i in t.vertices]
  if (b-a).cross(c-a).length<1e-9:continue
  inds=[]
  for vi in t.vertices:
   v=d.vertices[vi];normal=smooth_normals[vi] if d.polygons[t.polygon_index].use_smooth else tri_normals[t.index]
   color=tuple(d.materials[t.material_index].diffuse_color)
   key=(coords[vi],tuple(round(n,6) for n in normal),color)
   if key not in dedup:
    dedup[key]=len(vs);vs.append([round(x,6) for x in v.co]);ns.append([round(x,6) for x in normal]);cs.append([round(x,6) for x in color]);uvs.append([round(v.co.x+.5,6),round(v.co.y+.5,6)])
   inds.append(dedup[key])
  ts.append(inds)
 # Canonical ordering makes byte output independent of Blender join allocation order.
 order=sorted(range(len(vs)),key=lambda i:(vs[i],ns[i],cs[i],uvs[i]))
 remap={old:new for new,old in enumerate(order)}
 vs=[vs[i] for i in order];ns=[ns[i] for i in order];cs=[cs[i] for i in order];uvs=[uvs[i] for i in order]
 canonical=[]
 for tr in ts:
  tr=[remap[i] for i in tr];pivot=tr.index(min(tr));canonical.append(tr[pivot:]+tr[:pivot])
 ts=sorted(canonical)
 result['meshes'].append({'name':name,'vertices':vs,'triangles':ts,'colors':cs,'normals':ns,'uvs':uvs})
OUT.mkdir(parents=True,exist_ok=True);(OUT/'scene.json').write_text(json.dumps(result,separators=(',',':')))
bpy.ops.wm.save_as_mainfile(compress=True,filepath=str(HERE/'mvp-walk-poses.blend'))
if os.environ.get('MVP_EXPORT_ONLY')=='1':raise SystemExit(0)
# Optional generated surface images are bound to real UV meshes in offline evidence.
# Environment variables may override repository paths without changing exported geometry.
def bind_surface(name,envvar,default):
 path=Path(os.environ.get(envvar,str(ROOT/default)))
 if not path.exists():return False
 o=assets[name];d=o.data
 uv=d.uv_layers.new(name='UVMap')
 for poly in d.polygons:
  for li in poly.loop_indices:
   v=d.vertices[d.loops[li].vertex_index].co;uv.data[li].uv=(v.x+.5,v.y+.5)
 m=bpy.data.materials.new(name+'_generated_surface');m.use_nodes=True
 nodes=m.node_tree.nodes;bs=nodes.get('Principled BSDF');bs.inputs['Roughness'].default_value=.91
 tx=nodes.new('ShaderNodeTexImage');tx.image=bpy.data.images.load(str(path));tx.image.pack()
 m.node_tree.links.new(tx.outputs['Color'],bs.inputs['Base Color'])
 bump=nodes.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.16;bump.inputs['Distance'].default_value=.025
 m.node_tree.links.new(tx.outputs['Color'],bump.inputs['Height']);m.node_tree.links.new(bump.outputs['Normal'],bs.inputs['Normal'])
 d.materials.clear();d.materials.append(m)
 for poly in d.polygons:poly.material_index=0
 return True
bind_surface('courtyard_slab','MVP_STONE_TEXTURE','Content/MVP/TextureSource/courtyard_stone.png')
has_rune=bind_surface('courtyard_medallion','MVP_RUNE_TEXTURE','Content/MVP/TextureSource/courtyard_rune.png')
# Preview scenes use actual exported-source meshes. No matte paintings or concept backgrounds.
def instance(name,pos=(0,0,0),rot=0,scale=1):
 src=assets[name];o=bpy.data.objects.new(name+'_display',src.data);bpy.context.collection.objects.link(o);o.location=pos;o.rotation_euler.z=rot;o.scale=(scale,)*3;return o
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=64;scene.cycles.use_denoising=False
scene.world.color=(.012,.02,.045);scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.012,.02,.045,1);scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.35;scene.view_settings.view_transform='AgX'
scene.render.resolution_x=1440;scene.render.resolution_y=1080;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
def camera(pos,target,ortho):
 bpy.ops.object.camera_add(location=pos);c=bpy.context.object;c.rotation_euler=(Vector(target)-c.location).to_track_quat('-Z','Y').to_euler();c.data.type='ORTHO';c.data.ortho_scale=ortho;scene.camera=c
 return c
for pos,power,size,color in [((1,-4,9),550,8,(.56,.68,1)),((-5,3,7),1200,7,(.34,.52,1)),((4,5,5),350,5,(1,.45,.17))]:
 bpy.ops.object.light_add(type='AREA',location=pos);l=bpy.context.object;l.data.energy=power;l.data.shape='DISK';l.data.size=size;l.data.color=color;l.rotation_euler=(-l.location).to_track_quat('-Z','Y').to_euler()
# Contact sheet laid out in world with consistent scale, bonus larger hero for visual inspection.
displays=[]
for i,name in enumerate(base_names):
 x=(i%6)*4.5-11.25;y=(i//6)*5-7.5
 displays.append(instance(name,(x,y,0),-.6))
 # restrained label geometry, not part of exported assets
 bpy.ops.object.text_add(location=(x-.9,y-.9,.005));txt=bpy.context.object;txt.data.body=name.replace('enemy_','');txt.data.size=.21;txt.data.extrude=0;txt.data.materials.append(M['ivory']);displays.append(txt)
box((0,0,-.17),(15,13,.12),'ink',.1);ground=parts.pop();displays.append(ground)
c=camera((11,-18,23),(0,0,1),39);scene.render.filepath=str(HERE/'contact-sheet.png');bpy.ops.wm.save_as_mainfile(compress=True,filepath=str(HERE/'mvp-meshes.blend'));bpy.ops.render.render(write_still=True)
for o in displays:o.hide_render=True
# Arena staging: tessellated dark stone, shrine boundary, mobile threats.
for x in [-6,-2,2,6]:
 for y in [-4,0,4]:instance('courtyard_slab',(x,y,0),0,4)
for x in [-7,7]:
 for y in [-5,0,5]:instance('courtyard_pillar',(x,y,0),0,.85)
 for y in [-3,3]:instance('shrine_banner',(x,y,0),math.pi/2)
instance('shrine_arch',(0,5.7,0),math.pi/2)
for x in [-6,-2,2,6]:
 instance('courtyard_rail',(x,6.2,0));instance('courtyard_rail',(x,-6.2,0))
for x in [-8,8]:
 for y in [-4,0,4]:instance('courtyard_rail',(x,y,0),math.pi/2)
for p in [(-6,-4,0),(6,4,0),(-6,4,0),(6,-4,0)]:instance('courtyard_brazier',p)
instance('courtyard_pine',(-7,4,0),0,1.15)
instance('courtyard_steps',(0,-7.5,-.55))
if has_rune:instance('courtyard_medallion',(0,0,.015),0,5.1)
# Concentric stone relief rings, actual raised geometry.
for radius in [1.8,2.0,2.45,2.55]:ring((0,0,.02),radius,.018,'gold',96)
for i in range(32):
 a=i*math.tau/32
 leaf((2.1*math.cos(a),2.1*math.sin(a),.02),(2.32*math.cos(a),2.32*math.sin(a),.02),.016,'gold',.006)

for p in [(-6,-3,0),(6,3,0),(-5,5,0)]:instance('rock',p,random.random()*6,.7)
instance('hero_body',(0,0,0),.2,1.2);instance('hero_sword',(.7,.6,1),.5)
for i in range(22):
 a=i*2.399; r=2.3+(i%4)*.72;instance('enemy_swarm',(math.cos(a)*r,math.sin(a)*r,0),a+math.pi)
for name,p,r in [('enemy_boss',(4,3,0),3.4),('enemy_charger',(-4,1,0),0),('enemy_charger',(3,-3,0),2),('enemy_rangedbird',(-3,4,0),-.8),('enemy_rangedbird',(4,-1,0),2),('enemy_summoner',(-5,-2,0),0),('enemy_supporttotem',(2,4,0),0),('enemy_spirit',(-1,-1.5,0),-.5),('enemy_exploder',(-2,2,0),0)]:instance(name,p,r)
for x in [-7,7]:
 for y in [-5,0,5]:
  bpy.ops.object.light_add(type='POINT',location=(x,y,2.65));bpy.context.object.data.energy=55;bpy.context.object.data.color=(1,.28,.055);bpy.context.object.data.shadow_soft_size=.6
c.location=(12,-17,17);c.rotation_euler=(Vector((0,0,0))-c.location).to_track_quat('-Z','Y').to_euler();c.data.ortho_scale=19
scene.render.resolution_x=1600;scene.render.resolution_y=1100;scene.render.filepath=str(HERE/'arena-preview.png')
bpy.ops.wm.save_as_mainfile(compress=True,filepath=str(HERE/'mvp-arena-preview.blend'));bpy.ops.render.render(write_still=True)
print('EXPORTED',[(m['name'],len(m['vertices']),len(m['triangles'])) for m in result['meshes']])
