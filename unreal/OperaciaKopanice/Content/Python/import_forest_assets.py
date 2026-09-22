"""Import the extracted forest pieces with one shared PBR atlas material."""
from pathlib import Path
import json
import unreal as ue

root='/Game/Kopanice/Supplied/Forest'
source=Path(ue.Paths.project_dir())/'ArtSource/Supplied/Forest'
assets=json.loads((source/'manifest.json').read_text(encoding='utf-8'))['assets']
tools=ue.AssetToolsHelpers.get_asset_tools()
lib=ue.MaterialEditingLibrary
for filename,destination in [(a['mesh']+'.fbx',a['mesh']) for a in assets]+[
    ('Image_0.png','T_BaseColor'),('Image_1.png','T_MetalRough'),('Image_2.png','T_Normal')]:
    task=ue.AssetImportTask()
    task.filename=str(source/filename)
    task.destination_path=root
    task.destination_name=destination
    task.automated=True
    task.replace_existing=True
    task.save=True
    if filename.endswith('.fbx'):
        options=ue.FbxImportUI()
        options.import_mesh=True
        options.import_materials=False
        options.import_textures=False
        options.import_as_skeletal=False
        options.automated_import_should_detect_type=False
        options.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH
        options.static_mesh_import_data.combine_meshes=True
        options.static_mesh_import_data.generate_lightmap_u_vs=False
        options.static_mesh_import_data.auto_generate_collision=False
        options.static_mesh_import_data.build_nanite=True
        task.options=options
    tools.import_asset_tasks([task])
path=root+'/M_Forest'
mat=ue.load_asset(path) if ue.EditorAssetLibrary.does_asset_exist(path) else None
if not mat: mat=tools.create_asset('M_Forest',root,ue.Material,ue.MaterialFactoryNew())
lib.delete_all_material_expressions(mat)
for name,sampler,connections in [
    ('T_BaseColor',ue.MaterialSamplerType.SAMPLERTYPE_COLOR,[('RGB',ue.MaterialProperty.MP_BASE_COLOR)]),
    ('T_MetalRough',ue.MaterialSamplerType.SAMPLERTYPE_MASKS,[('G',ue.MaterialProperty.MP_ROUGHNESS),('B',ue.MaterialProperty.MP_METALLIC)]),
    ('T_Normal',ue.MaterialSamplerType.SAMPLERTYPE_NORMAL,[('RGB',ue.MaterialProperty.MP_NORMAL)])]:
    tex=ue.load_asset(root+'/'+name)
    assert isinstance(tex,ue.Texture2D),name
    tex.set_editor_property('srgb',name=='T_BaseColor')
    if name=='T_Normal':
        tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property('flip_green_channel',True)
    if name=='T_MetalRough': tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_MASKS)
    ue.EditorAssetLibrary.save_loaded_asset(tex,only_if_is_dirty=False)
    node=lib.create_material_expression(mat,ue.MaterialExpressionTextureSample)
    node.set_editor_property('texture',tex)
    node.set_editor_property('sampler_type',sampler)
    for pin,prop in connections: lib.connect_material_property(node,pin,prop)
lib.layout_material_expressions(mat)
lib.recompile_material(mat)
ue.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
for asset in assets:
    mesh=ue.load_asset(root+'/'+asset['mesh'])
    assert isinstance(mesh,ue.StaticMesh),asset['mesh']
    mesh.set_material(0,mat)
    settings=mesh.get_editor_property('nanite_settings')
    settings.set_editor_property('shape_preservation',ue.NaniteShapePreservation.PRESERVE_AREA)
    mesh.set_editor_property('nanite_settings',settings)
    ue.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False)
    ue.log('OK_FOREST_IMPORTED '+asset['mesh'])
