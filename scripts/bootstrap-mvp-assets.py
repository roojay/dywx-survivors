"""Run inside UE Editor Python commandlet, never ordinary CPython.
Creates cooked materials, sampled courtyard textures and audio for runtime meshes.
Idempotent: rebuilds graphs, saves assets, then writes a success receipt.
"""
import json
import runpy
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir())
RECEIPT = ROOT / 'Saved/MVP/material-bootstrap.json'
RECEIPT.parent.mkdir(parents=True, exist_ok=True)
RECEIPT.unlink(missing_ok=True)
# A package with missing authored geometry must never pass via diagnostic fallbacks.
runpy.run_path(str(ROOT / 'scripts/validate-mvp-render-contract.py'), run_name='__main__')
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def node(material, cls, x, y):
    result = mel.create_material_expression(material, cls, x, y)
    if result is None:
        raise RuntimeError('Failed to create ' + cls.__name__)
    return result


def connect(source, output, target, input_name):
    if not mel.connect_material_expressions(source, output, target, input_name):
        raise RuntimeError('Material connection failed: ' + input_name)


def build(name, vfx, texture=None):
    path = '/Game/MVP/' + name
    material = unreal.load_asset(path)
    if material is None:
        material = asset_tools.create_asset(name, '/Game/MVP', unreal.Material, unreal.MaterialFactoryNew())
    if not isinstance(material, unreal.Material):
        raise RuntimeError('Expected material at ' + path)
    mel.delete_all_material_expressions(material)
    material.set_editor_property('two_sided', True)
    material.set_editor_property('used_with_instanced_static_meshes', True)
    material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT if vfx else unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    color = node(material, unreal.MaterialExpressionVertexColor, -600, 0)
    tint = node(material, unreal.MaterialExpressionVectorParameter, -600, 200)
    tint.set_editor_property('parameter_name', 'Tint')
    tint.set_editor_property('default_value', unreal.LinearColor(1, 1, 1, 1))
    multiply = node(material, unreal.MaterialExpressionMultiply, -350, 0)
    # VertexColor exposes combined RGB as the unnamed first output in UE 5.8.
    connect(color, '', multiply, 'A')
    connect(tint, 'RGB', multiply, 'B')
    if texture is not None:
        sample = node(material, unreal.MaterialExpressionTextureSampleParameter2D, -650, -250)
        sample.set_editor_property('parameter_name', 'SurfaceTexture')
        sample.set_editor_property('texture', texture)
        # UV0 comes from the validated geometry JSON, never a constant zero UV.
        texcoord = node(material, unreal.MaterialExpressionTextureCoordinate, -900, -250)
        connect(texcoord, '', sample, 'UVs')
        textured = node(material, unreal.MaterialExpressionMultiply, -150, -200)
        connect(sample, 'RGB', textured, 'A')
        connect(multiply, '', textured, 'B')
        multiply = textured
    if vfx:
        glow = node(material, unreal.MaterialExpressionScalarParameter, -350, 200)
        glow.set_editor_property('parameter_name', 'Glow')
        glow.set_editor_property('default_value', 1.8)
        emission = node(material, unreal.MaterialExpressionMultiply, -100, 0)
        connect(multiply, '', emission, 'A')
        connect(glow, '', emission, 'B')
        assert mel.connect_material_property(emission, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        assert mel.connect_material_property(multiply, '', unreal.MaterialProperty.MP_BASE_COLOR)
        roughness = node(material, unreal.MaterialExpressionConstant, -350, 400)
        roughness.set_editor_property('r', 0.86)
        assert mel.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
        # A small ambient term keeps night silhouettes readable without skylight/Lumen.
        ambient = node(material, unreal.MaterialExpressionMultiply, -100, 200)
        ambient.set_editor_property('const_b', 0.22)
        connect(multiply, '', ambient, 'A')
        assert mel.connect_material_property(ambient, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError('Failed to save ' + path)
    return path


# Import source artwork before creating the material; missing textures fail closed.
texture_paths = []
textures = {}
for source_name, asset_name in [('courtyard_stone', 'T_CourtyardStone'), ('courtyard_rune', 'T_CourtyardRune')]:
    source = ROOT / 'Content/MVP/TextureSource' / (source_name + '.png')
    if not source.is_file():
        raise RuntimeError('Missing authored texture: ' + str(source))
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(source))
    task.set_editor_property('destination_path', '/Game/MVP/Textures')
    task.set_editor_property('destination_name', asset_name)
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', True)
    asset_tools.import_asset_tasks([task])
    texture = unreal.load_asset('/Game/MVP/Textures/' + asset_name)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError('Failed authored texture import: ' + asset_name)
    texture.set_editor_property('srgb', True)
    # Generated sources need not be powers of two (currently 1254 square).
    # Stretch at texture build time, NOT pad: padding would introduce UV seams.
    texture.set_editor_property('power_of_two_mode', unreal.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    texture.set_editor_property('filter', unreal.TextureFilter.TF_TRILINEAR)
    texture.set_editor_property('address_x', unreal.TextureAddress.TA_WRAP)
    texture.set_editor_property('address_y', unreal.TextureAddress.TA_WRAP)
    # Small fixed atlas pair stays resident. No runtime density/streaming guesswork.
    texture.set_editor_property('never_stream', True)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
        raise RuntimeError('Failed saving texture: ' + asset_name)
    textures[source_name] = texture
    texture_paths.append('/Game/MVP/Textures/' + asset_name)
paths = [build('M_Surface', False), build('M_VFX', True),
         build('M_Courtyard', False, textures['courtyard_stone'])] + texture_paths
# Original source WAVs are imported ahead of cooking; no shipping WAV parser.
audio_paths = []
required_cues = {'palm', 'sword', 'thunder', 'hit', 'pickup', 'dash', 'upgrade', 'boss', 'win', 'loss'}
sources = {source.stem: source for source in (ROOT / 'Content/MVP/AudioSource').glob('*.wav')}
missing = required_cues - sources.keys()
if missing:
    raise RuntimeError('Missing required original audio sources: ' + ', '.join(sorted(missing)))
for cue in sorted(required_cues):
    source = sources[cue]
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(source))
    task.set_editor_property('destination_path', '/Game/MVP/Audio')
    task.set_editor_property('destination_name', source.stem)
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', True)
    asset_tools.import_asset_tasks([task])
    imported = list(task.get_editor_property('imported_object_paths'))
    if not imported:
        raise RuntimeError('Audio import failed: ' + str(source))
    for asset_path in imported:
        sound = unreal.load_asset(asset_path)
        if not isinstance(sound, unreal.SoundWave):
            raise RuntimeError('Expected SoundWave: ' + asset_path)
        if not unreal.EditorAssetLibrary.save_loaded_asset(sound, only_if_is_dirty=False):
            raise RuntimeError('Failed to save sound: ' + asset_path)
    expected_path = '/Game/MVP/Audio/' + cue
    if not isinstance(unreal.load_asset(expected_path), unreal.SoundWave):
        raise RuntimeError('Sound missing at expected runtime path: ' + expected_path)
    audio_paths.extend(imported)
paths.extend(audio_paths)
RECEIPT.write_text(json.dumps({'status': 'generated', 'assets': paths, 'ue_build_render': 'not_run'}, indent=2), encoding='utf-8')
unreal.log('MVP_MATERIAL_BOOTSTRAP_OK: ' + ', '.join(paths))
