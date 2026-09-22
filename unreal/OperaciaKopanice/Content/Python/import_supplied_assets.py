"""Import prepared, textured Blender assets into a separate Nanite library."""
import json
from pathlib import Path
import unreal as ue

source = Path(ue.Paths.project_dir()) / 'ArtSource/Supplied'
manifest = json.loads((source / 'manifest.json').read_text(encoding='utf-8'))
tools = ue.AssetToolsHelpers.get_asset_tools()
lib = ue.MaterialEditingLibrary
for asset in manifest:
    name = asset['name']
    root = '/Game/Kopanice/Supplied/' + name
    tasks = []
    for filename, destination in [(asset['mesh']+'.fbx', asset['mesh']),
                                   ('Image_0.png', 'T_BaseColor'),
                                   ('Image_1.png', 'T_MetalRough'),
                                   ('Image_2.png', 'T_Normal')]:
        task = ue.AssetImportTask()
        task.filename = str(source/name/filename)
        task.destination_path = root
        task.destination_name = destination
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
    if '-OKMaterialsOnly' not in ue.SystemLibrary.get_command_line():
        tools.import_asset_tasks(tasks)
    mesh = ue.load_asset(root+'/'+asset['mesh'])
    if not isinstance(mesh, ue.StaticMesh):
        raise RuntimeError('Missing imported mesh: '+name)
    path = root+'/M_'+name
    mat = ue.load_asset(path) if ue.EditorAssetLibrary.does_asset_exist(path) else None
    if not mat:
        mat = tools.create_asset('M_'+name,root,ue.Material,ue.MaterialFactoryNew())
    lib.delete_all_material_expressions(mat)
    for texname, sampler, connections in [
        ('T_BaseColor', ue.MaterialSamplerType.SAMPLERTYPE_COLOR, [('RGB',ue.MaterialProperty.MP_BASE_COLOR)]),
        ('T_MetalRough', ue.MaterialSamplerType.SAMPLERTYPE_MASKS, [('G',ue.MaterialProperty.MP_ROUGHNESS),('B',ue.MaterialProperty.MP_METALLIC)]),
        ('T_Normal', ue.MaterialSamplerType.SAMPLERTYPE_NORMAL, [('RGB',ue.MaterialProperty.MP_NORMAL)])]:
        tex = ue.load_asset(root+'/'+texname)
        if not isinstance(tex,ue.Texture2D):
            raise RuntimeError('Missing texture: '+texname)
        tex.set_editor_property('srgb',texname=='T_BaseColor')
        if texname=='T_Normal':
            tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_NORMALMAP)
            # Blender tangent normals are OpenGL; UE expects DirectX green direction.
            tex.set_editor_property('flip_green_channel',True)
        elif texname=='T_MetalRough':
            tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_MASKS)
        ue.EditorAssetLibrary.save_loaded_asset(tex)
        node = lib.create_material_expression(mat,ue.MaterialExpressionTextureSample)
        node.set_editor_property('texture',tex)
        node.set_editor_property('sampler_type',sampler)
        for output, prop in connections:
            lib.connect_material_property(node,output,prop)
    lib.layout_material_expressions(mat)
    lib.recompile_material(mat)
    ue.EditorAssetLibrary.save_loaded_asset(mat)
    for index in range(len(mesh.static_materials)):
        mesh.set_material(index,mat)
    ue.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    ue.log('OK_SUPPLIED_IMPORTED %s bounds=%s'%(name,mesh.get_bounds().box_extent))
