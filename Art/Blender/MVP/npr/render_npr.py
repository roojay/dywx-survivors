"""Reproducible offline NPR staging; never overwrites the original scene or export."""
import bpy, sys, json, hashlib, argparse
from pathlib import Path
from mathutils import Vector
P=Path(__file__).resolve().parent
ap=argparse.ArgumentParser();ap.add_argument('--draft',action='store_true');ap.add_argument('--refresh-meshes',action='store_true');ap.add_argument('--before',action='store_true');args=ap.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
source=P/'source-before.blend';bpy.ops.wm.open_mainfile(filepath=str(source));s=bpy.context.scene
camera={'location':list(s.camera.location),'rotation':list(s.camera.rotation_euler),'ortho_scale':s.camera.data.ortho_scale}
replaced={}
mesh_source_hash=None
if args.refresh_meshes and not args.before:
 fresh=P.parent/'mvp-walk-poses.blend'
 mesh_source_hash=hashlib.sha256(fresh.read_bytes()).hexdigest()
 with bpy.data.libraries.load(str(fresh),link=False) as (data,target):
  target.objects=[n for n in data.objects if n in ['hero_body','hero_sword','enemy_swarm','enemy_exploder','enemy_charger','enemy_rangedbird','enemy_supporttotem','enemy_summoner','enemy_boss','enemy_spirit','tile','pillar','rock','shrine_banner','shrine_arch']]
 for src in target.objects:
  if src is None:continue
  # Library appending may suffix object names; source name prefix still unambiguous.
  base=src.name.split('.')[0]
  targets=[o for o in s.objects if o.type=='MESH' and o.name.startswith(base+'_display') and not o.hide_render]
  for o in targets:o.data=src.data
  replaced[base]=len(targets)
 if len(replaced)!=15:raise RuntimeError(f'Missing refreshed bases: {replaced}')
if not args.before:
 for o in list(s.objects):
  if o.type=='LIGHT' and o.data.type=='AREA':bpy.data.objects.remove(o,do_unlink=True)
 def area(name,pos,target,power,size,color):
  d=bpy.data.lights.new(name,'AREA');d.energy=power;d.shape='DISK';d.size=size;d.color=color
  o=bpy.data.objects.new(name,d);s.collection.objects.link(o);o.location=pos;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
 area('NPR cool moon key',(-1,-3,7),(0,0,0),1000,4,(.63,.75,1.0))
 area('NPR restrained amber rim',(2,6,7),(0,1,0),500,4,(1.0,.55,.28))
 s.world.use_nodes=True;bg=s.world.node_tree.nodes.get('Background');bg.inputs['Color'].default_value=(.025,.038,.065,1);bg.inputs['Strength'].default_value=.32
 # Depth-based subtle atmosphere: no volume steps, no opaque screen fog.
 s.view_layers[0].use_pass_mist=True;s.world.mist_settings.start=22;s.world.mist_settings.depth=22;s.world.mist_settings.falloff='LINEAR'
 s.use_nodes=True
 # Blender 5 stores compositor trees as node groups.
 tree=getattr(s,'node_tree',None)
 if tree is None:
  tree=bpy.data.node_groups.new('NPR restrained depth unity','CompositorNodeTree');s.compositing_node_group=tree
 tree.nodes.clear();n=tree.nodes;l=tree.links
 render=n.new('CompositorNodeRLayers');amount=n.new('ShaderNodeMath');amount.operation='MULTIPLY';amount.inputs[1].default_value=.055;l.new(render.outputs['Mist'],amount.inputs[0])
 mix=n.new('ShaderNodeMixRGB');mix.blend_type='MIX';mix.inputs[2].default_value=(.07,.105,.16,1);l.new(amount.outputs[0],mix.inputs[0]);l.new(render.outputs['Image'],mix.inputs[1])
 # Blender 5 group output replaces the legacy Composite node.
 try:
  out=n.new('CompositorNodeComposite');l.new(mix.outputs[0],out.inputs[0])
 except RuntimeError:
  tree.interface.new_socket(name='Image',in_out='OUTPUT',socket_type='NodeSocketColor');out=n.new('NodeGroupOutput');l.new(mix.outputs[0],out.inputs['Image'])
 s.view_settings.exposure=-.1
s.render.engine='CYCLES';s.cycles.device='CPU';s.cycles.samples=12 if args.draft else 24;s.cycles.use_denoising=True;s.cycles.max_bounces=4
s.render.resolution_x=1600;s.render.resolution_y=1100;s.render.resolution_percentage=50 if args.draft else 100
s.render.image_settings.file_format='PNG'
name=('before' if args.before else 'after')+('-draft' if args.draft else '')
s.render.filepath=str(P/(name+'-raw.png'));s.render.film_transparent=False
if not args.draft:bpy.ops.wm.save_as_mainfile(filepath=str(P/(name+'.blend')))
bpy.ops.render.render(write_still=True)
(P/(name+'-evidence.json')).write_text(json.dumps({'source':str(source.relative_to(P.parents[3])),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'camera':camera,'resolution':[s.render.resolution_x,s.render.resolution_y],'percentage':s.render.resolution_percentage,'samples':s.cycles.samples,'backend':'Cycles CPU','refreshed_mesh_instances':replaced,'mesh_source_sha256':mesh_source_hash,'offline_not_ue':True},indent=2))
