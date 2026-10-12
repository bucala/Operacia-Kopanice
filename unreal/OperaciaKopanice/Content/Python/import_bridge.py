"""Import the calibrated Rustbound bridge and preserve its packed PBR textures."""
import json
from pathlib import Path
import unreal as ue

root = '/Game/Kopanice/Supplied/Bridge'
source = Path(ue.Paths.project_dir()) / 'ArtSource/Supplied/Bridge'
manifest = json.loads((source / 'manifest.json').read_text(encoding='utf-8'))
tools = ue.AssetToolsHelpers.get_asset_tools()
lib = ue.MaterialEditingLibrary
tasks = []
for filename, name in [(a['mesh'] + '.fbx', a['mesh']) for a in manifest['assets']] + [
        ('Image_0.png', 'T_BaseColor'), ('Image_1.png', 'T_MetalRough'), ('Image_2.png', 'T_Normal')]:
    task = ue.AssetImportTask()
    task.filename = str(source / filename)
    task.destination_path = root
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    if filename.endswith('.fbx'):
        options = ue.FbxImportUI()
        options.import_mesh = True
        options.import_materials = False
        options.import_textures = False
        options.import_as_skeletal = False
        options.automated_import_should_detect_type = False
        options.mesh_type_to_import = ue.FBXImportType.FBXIT_STATIC_MESH
        options.static_mesh_import_data.combine_meshes = True
        options.static_mesh_import_data.generate_lightmap_u_vs = False
        options.static_mesh_import_data.auto_generate_collision = False
        options.static_mesh_import_data.build_nanite = True
        task.options = options
    tasks.append(task)
tools.import_asset_tasks(tasks)
material = ue.load_asset(root + '/M_Bridge') if ue.EditorAssetLibrary.does_asset_exist(root + '/M_Bridge') else None
if not material:
    material = tools.create_asset('M_Bridge', root, ue.Material, ue.MaterialFactoryNew())
lib.delete_all_material_expressions(material)
samples = {}
for name, sampler in [('T_BaseColor', ue.MaterialSamplerType.SAMPLERTYPE_COLOR),
                      ('T_MetalRough', ue.MaterialSamplerType.SAMPLERTYPE_MASKS),
                      ('T_Normal', ue.MaterialSamplerType.SAMPLERTYPE_NORMAL)]:
    texture = ue.load_asset(root + '/' + name)
    assert isinstance(texture, ue.Texture2D), name
    texture.set_editor_property('srgb', name == 'T_BaseColor')
    if name == 'T_MetalRough':
        texture.set_editor_property('compression_settings', ue.TextureCompressionSettings.TC_MASKS)
    if name == 'T_Normal':
        texture.set_editor_property('compression_settings', ue.TextureCompressionSettings.TC_NORMALMAP)
        texture.set_editor_property('flip_green_channel', True)
    ue.EditorAssetLibrary.save_loaded_asset(texture)
    sample = lib.create_material_expression(material, ue.MaterialExpressionTextureSample)
    sample.set_editor_property('texture', texture)
    sample.set_editor_property('sampler_type', sampler)
    samples[name] = sample
position = lib.create_material_expression(material, ue.MaterialExpressionWorldPosition)
normal = lib.create_material_expression(material, ue.MaterialExpressionVertexNormalWS)
coverage = lib.create_material_expression(material, ue.MaterialExpressionCustom)
inputs = []
for name in ('P', 'N'):
    pin = ue.CustomInput()
    pin.set_editor_property('input_name', name)
    inputs.append(pin)
coverage.set_editor_property('inputs', inputs)
coverage.set_editor_property('output_type', ue.CustomMaterialOutputType.CMOT_FLOAT1)
coverage.set_editor_property('code', """
float patch=.5+.5*sin(P.x*.047+sin(P.y*.035))*cos(P.y*.028);
return smoothstep(.65,.95,N.z)*smoothstep(.26,.8,patch)*.55;
""")
assert lib.connect_material_expressions(position, '', coverage, 'P')
assert lib.connect_material_expressions(normal, '', coverage, 'N')
snow = lib.create_material_expression(material, ue.MaterialExpressionConstant3Vector)
snow.set_editor_property('constant', ue.LinearColor(.72, .80, .86))
color = lib.create_material_expression(material, ue.MaterialExpressionLinearInterpolate)
assert lib.connect_material_expressions(samples['T_BaseColor'], 'RGB', color, 'A')
assert lib.connect_material_expressions(snow, '', color, 'B')
assert lib.connect_material_expressions(coverage, '', color, 'Alpha')
assert lib.connect_material_property(color, '', ue.MaterialProperty.MP_BASE_COLOR)
roughness = lib.create_material_expression(material, ue.MaterialExpressionLinearInterpolate)
roughness.set_editor_property('const_b', .9)
assert lib.connect_material_expressions(samples['T_MetalRough'], 'G', roughness, 'A')
assert lib.connect_material_expressions(coverage, '', roughness, 'Alpha')
assert lib.connect_material_property(roughness, '', ue.MaterialProperty.MP_ROUGHNESS)
clear = lib.create_material_expression(material, ue.MaterialExpressionOneMinus)
assert lib.connect_material_expressions(coverage, '', clear, '')
metal = lib.create_material_expression(material, ue.MaterialExpressionMultiply)
assert lib.connect_material_expressions(samples['T_MetalRough'], 'B', metal, 'A')
assert lib.connect_material_expressions(clear, '', metal, 'B')
assert lib.connect_material_property(metal, '', ue.MaterialProperty.MP_METALLIC)
assert lib.connect_material_property(samples['T_Normal'], 'RGB', ue.MaterialProperty.MP_NORMAL)
material.set_editor_property('used_with_nanite', True)
lib.layout_material_expressions(material)
lib.recompile_material(material)
ue.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
for asset in manifest['assets']:
    mesh = ue.load_asset(root + '/' + asset['mesh'])
    assert isinstance(mesh, ue.StaticMesh), asset['mesh']
    for index in range(len(mesh.static_materials)):
        mesh.set_material(index, material)
    ue.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    ue.log('OK_BRIDGE_IMPORTED: %s bounds=%s' % (asset['mesh'], mesh.get_bounding_box()))
ue.log('OK_BRIDGE_IMPORT: PASS')
