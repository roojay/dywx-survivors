"""Actual baked-pose source mesh contact sheet; offline CPU, no external assets."""
import bpy, math
from pathlib import Path
from mathutils import Vector
HERE=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(HERE/'mvp-walk-poses.blend'))
scene=bpy.context.scene
for row,prefix in enumerate(['hero_walk','enemy_swarm_walk']):
 for i in range(8):
  src=bpy.data.objects[f'{prefix}_{i}'];o=src.copy();o.data=src.data;bpy.context.collection.objects.link(o)
  o.hide_render=False;o.hide_set(False);o.location=((i-3.5)*1.9,row*3.2,0);o.rotation_euler.z=-math.pi/2
  bpy.ops.object.text_add(location=((i-3.5)*1.9-.5,row*3.2-.9,.01));txt=bpy.context.object;txt.data.body=f'{"Hero" if row==0 else "Swarm"} {i}';txt.data.size=.19;txt.data.materials.append(bpy.data.materials['white'])
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.015));bpy.context.object.data.materials.append(bpy.data.materials['stone'])
for pos,power,size in [((0,-6,10),2000,9),((1,8,8),1800,8)]:
 bpy.ops.object.light_add(type='AREA',location=pos);l=bpy.context.object;l.data.energy=power;l.data.shape='DISK';l.data.size=size;l.rotation_euler=(Vector((0,1,0))-l.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(2,-14,12));cam=bpy.context.object;cam.rotation_euler=(Vector((0,1.2,.5))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=16.5;scene.camera=cam
scene.world.color=(.12,.12,.12);scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=24;scene.cycles.use_denoising=True
scene.view_settings.view_transform='AgX';scene.render.resolution_x=2200;scene.render.resolution_y=950;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(HERE/'walk-contact-sheet.png');bpy.ops.render.render(write_still=True)
