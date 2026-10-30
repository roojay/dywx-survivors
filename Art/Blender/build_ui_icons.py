"""Reproducible original mesh-based UI glyphs. Run with Blender --background --python.
Uses no external assets, network, add-ons or MCP. Blender 5.2.3 tested.
"""
import bpy
import math
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / 'Content' / 'UI' / 'Images'
OUTPUT.mkdir(parents=True, exist_ok=True)
SOURCE = Path(__file__).resolve().parent
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.render.engine = 'CYCLES'
scene.cycles.device = 'CPU'
scene.cycles.samples = 32
scene.cycles.use_denoising = True
scene.render.resolution_x = 512
scene.render.resolution_y = 512
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.render.image_settings.color_mode = 'RGBA'
scene.render.film_transparent = True
scene.render.image_settings.color_depth = '8'
scene.world.color = (0.16, 0.19, 0.24)
scene.view_settings.view_transform = 'AgX'

def material(name, color, metallic=0.75, roughness=0.28, glow=0):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = (*color, 1)
    bsdf.inputs['Metallic'].default_value = metallic
    bsdf.inputs['Roughness'].default_value = roughness
    if glow:
        bsdf.inputs['Emission Color'].default_value = (*color, 1)
        bsdf.inputs['Emission Strength'].default_value = glow
    return m

gold = material('Warm antique gold', (0.96, 0.51, 0.12), glow=0.12)
silver = material('Moon silver', (0.72, 0.85, 1.0), glow=0.08)
pale = material('Warm luminous edge', (1.0, 0.83, 0.43), glow=0.7)
blue = material('Dash moon white', (0.55, 0.78, 1.0), glow=0.3)
stone = material('Iron slate', (0.18, 0.21, 0.26), 0.2, 0.6)
glass = material('Jade mirror face', (0.42, 0.82, 0.76), 0.75, 0.15, 0.12)

bpy.ops.object.camera_add(location=(0, -0.25, 6))
camera = bpy.context.object
camera.name = 'UI orthographic camera'
camera.rotation_euler = (Vector((0, 0, 0)) - camera.location).to_track_quat('-Z', 'Y').to_euler()
camera.data.type = 'ORTHO'
camera.data.ortho_scale = 3.65
scene.camera = camera
for name, location, energy, size, color in [
    ('Warm key', (-3, 2, 5), 650, 4, (1.0, .79, .48)),
    ('Cool fill', (3, 1, 4), 450, 3, (.55, .73, 1.0)),
    ('Edge light', (0, -3, 3), 500, 2, (1.0, .95, .8)),
]:
    bpy.ops.object.light_add(type='AREA', location=location)
    light = bpy.context.object
    light.name = name
    light.data.energy = energy
    light.data.shape = 'DISK'
    light.data.size = size
    light.data.color = color
    light.rotation_euler = (-light.location).to_track_quat('-Z', 'Y').to_euler()

collections = []
current = None

def begin(name):
    global current
    current = bpy.data.collections.new(name)
    scene.collection.children.link(current)
    collections.append(current)

def put(obj, name, mat):
    obj.name = name
    for c in list(obj.users_collection):
        c.objects.unlink(obj)
    current.objects.link(obj)
    obj.data.materials.append(mat)
    return obj

def polygon(name, points, mat, depth=.13):
    n = len(points)
    vertices = [(x, y, -depth/2) for x, y in points] + [(x, y, depth/2) for x, y in points]
    faces = [tuple(reversed(range(n))), tuple(range(n,2*n))]
    faces += [(i, (i+1)%n, (i+1)%n+n, i+n) for i in range(n)]
    mesh = bpy.data.meshes.new(name + ' mesh')
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    ob = bpy.data.objects.new(name, mesh)
    current.objects.link(ob)
    ob.data.materials.append(mat)
    bevel = ob.modifiers.new('Soft forged edges', 'BEVEL')
    bevel.width = .035
    bevel.segments = 3
    ob.modifiers.new('Weighted normals', 'WEIGHTED_NORMAL')
    return ob

def arc(name, radius, a, b, width, mat, center=(0,0), rotate=0):
    # Tapered solid crescent, separate front/back faces and bevelled metal edge.
    count = 40
    outer, inner = [], []
    for i in range(count + 1):
        t = i / count
        angle = math.radians(a + (b-a)*t + rotate)
        w = width * math.sin(math.pi*t)**.7 + .008
        outer.append((center[0]+radius*math.cos(angle), center[1]+radius*math.sin(angle)))
        inner.append((center[0]+(radius-w)*math.cos(angle), center[1]+(radius-w)*math.sin(angle)))
    return polygon(name, outer + list(reversed(inner)), mat)

def tube(name, points, radius, mat):
    data = bpy.data.curves.new(name, 'CURVE')
    data.dimensions = '3D'
    data.resolution_u = 2
    data.bevel_depth = radius
    data.bevel_resolution = 4
    spline = data.splines.new('POLY')
    spline.points.add(len(points)-1)
    for p, xyz in zip(spline.points, points): p.co = (*xyz,1)
    return curve_object(name, data, mat)

def curve_object(name, data, mat):
    ob = bpy.data.objects.new(name, data)
    current.objects.link(ob)
    data.materials.append(mat)
    return ob

def torus(name, radius, thickness, mat, z=0):
    bpy.ops.mesh.primitive_torus_add(major_radius=radius, minor_radius=thickness, major_segments=96, minor_segments=12, location=(0,0,z))
    return put(bpy.context.object,name,mat)

begin('slash')
for i, x in enumerate([-.45,0,.45]):
    ob = arc('Forged slash '+str(i), 1.4, 195, 325, .22, gold, center=(x,.45-i*.13))
    ob.rotation_euler.z = math.radians(-35)

begin('orbit')
for i in range(3):
    arc('Orbit blade '+str(i), 1.35, 5, 112, .35, silver, rotate=120*i)
    arc('Inner sweep '+str(i), .75, -10, 88, .15, silver, rotate=120*i)

begin('arrow')
polygon('Piercing arrow', [(-1.25,-1.1),(.48,.62),(.12,.7),(1.3,1.3),(.7,.11),(.63,.47),(-1.1,-1.25)], silver)
polygon('Left fletching', [(-1.25,-.63),(-.72,-.48),(-.42,-.65),(-1.03,-1.06)], gold)
polygon('Right fletching', [(-.63,-1.25),(-.48,-.72),(-.65,-.42),(-1.06,-1.03)], gold)
tube('Arrow central energy', [(-.88,-.88,.13),(.94,.94,.13)], .025, pale)

begin('dash')
# Human movement silhouette, distinct from auto-attack blades.
bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=12, radius=.22, location=(.35,.94,0))
put(bpy.context.object,'Runner head',blue)
for name, pts, radius in [
 ('Torso',[(.22,.62,.02),(-.12,.05,.02)],.14),
 ('Forward arm',[(.14,.5,0),(.54,.25,0),(.94,.55,0)],.10),
 ('Rear arm',[(.1,.46,0),(-.43,.6,0),(-.72,.25,0)],.10),
 ('Forward leg',[(-.12,.05,0),(.4,-.38,0),(.72,-.98,0)],.12),
 ('Rear leg',[(-.12,.05,0),(-.57,-.55,0),(-1.1,-.72,0)],.12),
]: tube(name,pts,radius,blue)
for i in range(3): tube('Motion streak '+str(i), [(-1.45,.85-i*.42,0),(-.83,.85-i*.42,0)], .035, silver)

begin('stone')
bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=1, location=(0,0,0))
ob = put(bpy.context.object,'Faceted whetstone',stone)
ob.scale = (.72,1.25,.32)
ob.rotation_euler.z = -.3
for name, pts in [
 ('Gold mineral seam',[(-.25,-.93,.27),(-.08,-.35,.36),(.11,.2,.35),(.04,.99,.26)]),
 ('Branch vein',[(-.08,-.35,.36),(.38,-.07,.31),(.46,.25,.24)]),
]: tube(name,pts,.028,gold)

begin('mirror')
bpy.ops.mesh.primitive_cylinder_add(vertices=64, radius=1.03, depth=.12, location=(0,0,0))
put(bpy.context.object,'Reflective jade face',glass)
torus('Outer cast bronze rim',1.13,.095,gold)
torus('Inner luminous ring',.98,.028,pale,.1)
for i in range(8):
    a=i*math.pi/4
    points=[]
    for r,da in [(1.17,-.07),(1.40,0),(1.17,.07)]: points.append((r*math.cos(a+da),r*math.sin(a+da)))
    polygon('Compass ornament '+str(i),points,gold)
# A shaped reflection highlight, actual luminous geometry.
arc('Mirror reflection',.74,48,125,.12,pale)

for collection in collections:
    collection.hide_render = True
    collection.hide_viewport = True
for collection in collections:
    collection.hide_render = False
    collection.hide_viewport = False
    scene.render.filepath = str(OUTPUT / ('icon-'+collection.name+'.png'))
    bpy.ops.render.render(write_still=True)
    collection.hide_render = True
    collection.hide_viewport = True
collections[0].hide_render = False
collections[0].hide_viewport = False
bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE / 'ui-icons.blend'))
print('DYWX_UI_ICONS_COMPLETE', OUTPUT)
