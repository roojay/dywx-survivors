"""Original deterministic mesh art; Blender 4.x/5.x offline CPU, no downloads."""
import bpy, math, random, json, os
from mathutils import Vector
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'Content/MVP/Geometry'; HERE=Path(__file__).resolve().parent
random.seed(73)
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
# Linear-light palette; subdued stone and ivory, saturated accents restricted to details.
P={'ivory':(.72,.69,.59,1),'white':(.9,.88,.79,1),'cloth':(.27,.30,.33,1),'ink':(.018,.024,.032,1),'skin':(.56,.32,.20,1),'gold':(.57,.30,.065,1),'brightgold':(.95,.56,.12,1),'blue':(.10,.42,.65,1),'stone':(.055,.075,.105,1),'edge':(.12,.16,.21,1),'demon':(.12,.105,.12,1),'bone':(.32,.31,.30,1),'red':(.31,.035,.022,1),'fire':(.95,.21,.025,1),'purple':(.35,.12,.68,1),'eye':(.85,.63,.21,1)}
M={}
for k,c in P.items():
 m=bpy.data.materials.new(k); m.diffuse_color=c; m.use_nodes=True
 bs=m.node_tree.nodes.get('Principled BSDF'); bs.inputs['Base Color'].default_value=c; bs.inputs['Roughness'].default_value=.48 if 'gold' in k else .76
 if k in ['gold','brightgold']: bs.inputs['Metallic'].default_value=.6
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
 o=bpy.context.object; o.scale=scale; return finish(o,mat)
def rod(a,b,r1,r2,mat,n=8):
 d=Vector(b)-Vector(a); bpy.ops.mesh.primitive_cone_add(vertices=n,radius1=r1,radius2=r2,depth=d.length,location=(Vector(a)+Vector(b))/2)
 o=bpy.context.object; o.rotation_euler=d.to_track_quat('Z','Y').to_euler(); return finish(o,mat)
def box(pos,scale,mat,bevel=.04):
 bpy.ops.mesh.primitive_cube_add(size=2,location=pos);o=bpy.context.object;o.scale=scale
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  mod=o.modifiers.new('soft chisel','BEVEL');mod.width=bevel;mod.segments=1
  bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
 return finish(o,mat,False)
def mesh(v,f,mat):
 d=bpy.data.meshes.new('sculpted');d.from_pydata(v,[],f);d.update();o=bpy.data.objects.new('sculpted',d);bpy.context.collection.objects.link(o);return finish(o,mat,False)
def horn(points,r,mat,n=7):
 for i in range(len(points)-1):rod(points[i],points[i+1],r*(1-i/(len(points)-1)),r*(1-(i+1)/(len(points)-1)),mat,n)
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
 bpy.ops.object.select_all(action='DESELECT')
 for o in parts:o.select_set(True)
 bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
 bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
 # Bake world transform into vertices, then pivot at feet/world origin.
 for v in o.data.vertices:v.co=o.matrix_world@v.co
 o.matrix_world.identity(); assets[name]=o;parts.clear();o.hide_render=True;o.hide_set(True)
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
   horn([(.35-k*.11,s*.25,.89),(.15-k*.13,s*(.30+.018*k),1.02),(-.12-k*.15,s*(.32+.02*k),.96),(-.30-k*.16,s*(.29+.025*k),1.04)],.07,'ivory' if boss else 'gold',7)
 for i in range(5):
  horn([(.35-i*.13,0,.96),(.17-i*.14,0,1.10),(-.05-i*.16,0,1.04),(-.2-i*.18,0,1.14)],.085,'ivory' if boss else 'brightgold',7)
 horn([(-.48,0,.68),(-.85,0,.87),(-1.02,.16,1.03)],.07,'bone' if boss else 'gold')
 if boss:
  for i in range(4):
   for s in [-1,1]:
    ell((-.36+i*.17,s*.13,.94),(.12,.115,.075),'stone',8,5)
    leaf((-.40+i*.16,s*.23,.92),(-.3+i*.16,s*.31,.69),.025,'purple',.013)
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
# Export dense indexed triangles and linear palette. Smooth vertex normals included.
result={'version':1,'coordinateSystem':'RH_Z_UP_X_FORWARD','winding':'CCW_OUTWARD','units':'metres','ueConversion':'(x,-y,z)*100; reverse triangle indices 1 and 2','meshes':[]}
for name,o in assets.items():
 d=o.data;d.calc_loop_triangles();vs=[];ns=[];cs=[];ts=[];dedup={}
 for t in d.loop_triangles:
  a,b,c=[d.vertices[i].co for i in t.vertices]
  if (b-a).cross(c-a).length<1e-9:continue
  inds=[]
  for vi in t.vertices:
   v=d.vertices[vi];normal=v.normal if d.polygons[t.polygon_index].use_smooth else t.normal
   color=tuple(d.materials[t.material_index].diffuse_color)
   key=(vi,tuple(round(n,6) for n in normal),color)
   if key not in dedup:
    dedup[key]=len(vs);vs.append([round(x,6) for x in v.co]);ns.append([round(x,6) for x in normal]);cs.append(list(color))
   inds.append(dedup[key])
  ts.append(inds)
 result['meshes'].append({'name':name,'vertices':vs,'triangles':ts,'colors':cs,'normals':ns})
OUT.mkdir(parents=True,exist_ok=True);(OUT/'scene.json').write_text(json.dumps(result,separators=(',',':')))
bpy.ops.wm.save_as_mainfile(filepath=str(HERE/'mvp-walk-poses.blend'))
if os.environ.get('MVP_EXPORT_ONLY')=='1':raise SystemExit(0)
# Preview scenes use actual exported-source meshes. No matte paintings or concept backgrounds.
def instance(name,pos=(0,0,0),rot=0,scale=1):
 src=assets[name];o=bpy.data.objects.new(name+'_display',src.data);bpy.context.collection.objects.link(o);o.location=pos;o.rotation_euler.z=rot;o.scale=(scale,)*3;return o
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=24;scene.cycles.use_denoising=True
scene.world.color=(.012,.02,.045);scene.view_settings.view_transform='AgX'
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
 x=(i%5)*3.1-6.2;y=(i//5)*3.4-3.4
 displays.append(instance(name,(x,y,0),-.6))
 # restrained label geometry, not part of exported assets
 bpy.ops.object.text_add(location=(x-.9,y-.9,.005));txt=bpy.context.object;txt.data.body=name.replace('enemy_','');txt.data.size=.21;txt.data.extrude=0;txt.data.materials.append(M['ivory']);displays.append(txt)
box((0,0,-.17),(9,6,.12),'ink',.1);ground=parts.pop();displays.append(ground)
c=camera((11,-18,23),(0,0,0),20);scene.render.filepath=str(HERE/'contact-sheet.png');bpy.ops.wm.save_as_mainfile(filepath=str(HERE/'mvp-meshes.blend'));bpy.ops.render.render(write_still=True)
for o in displays:o.hide_render=True
# Arena staging: tessellated dark stone, shrine boundary, mobile threats.
for x in range(-8,9):
 for y in range(-6,7):instance('tile',(x,y,0),0)
for x in [-7,7]:
 for y in [-5,0,5]:instance('pillar',(x,y,0))
 for y in [-3,3]:instance('shrine_banner',(x,y,0),math.pi/2)
instance('shrine_arch',(0,5.7,0),math.pi/2)
for p in [(-6,-3,0),(6,3,0),(-5,5,0)]:instance('rock',p,random.random()*6,.7)
instance('hero_body',(0,0,0),.2,1.2);instance('hero_sword',(.7,.6,1),.5)
for i in range(22):
 a=i*2.399; r=2.3+(i%4)*.72;instance('enemy_swarm',(math.cos(a)*r,math.sin(a)*r,0),a+math.pi)
for name,p,r in [('enemy_boss',(4,3,0),3.4),('enemy_charger',(-4,1,0),0),('enemy_charger',(3,-3,0),2),('enemy_rangedbird',(-3,4,0),-.8),('enemy_rangedbird',(4,-1,0),2),('enemy_summoner',(-5,-2,0),0),('enemy_supporttotem',(2,4,0),0),('enemy_spirit',(-1,-1.5,0),-.5),('enemy_exploder',(-2,2,0),0)]:instance(name,p,r)
for x in [-7,7]:
 for y in [-5,0,5]:
  bpy.ops.object.light_add(type='POINT',location=(x,y,2.65));bpy.context.object.data.energy=55;bpy.context.object.data.color=(1,.28,.055);bpy.context.object.data.shadow_soft_size=.6
c.location=(12,-17,20);c.rotation_euler=(Vector((0,0,0))-c.location).to_track_quat('-Z','Y').to_euler();c.data.ortho_scale=19
scene.render.resolution_x=1600;scene.render.resolution_y=1100;scene.render.filepath=str(HERE/'arena-preview.png')
bpy.ops.wm.save_as_mainfile(filepath=str(HERE/'mvp-arena-preview.blend'));bpy.ops.render.render(write_still=True)
print('EXPORTED',[(m['name'],len(m['vertices']),len(m['triangles'])) for m in result['meshes']])
