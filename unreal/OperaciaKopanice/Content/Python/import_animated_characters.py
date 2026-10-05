"""Import derived rigs, reusing supplied textures; pass -OKRigSource=<folder>."""
import unreal as ue
from pathlib import Path

command=ue.SystemLibrary.get_command_line()
source=Path(command.split('-OKRigSource=')[1].split(' -')[0].strip('" '))
for name in ('Partisan','Officer'):
    root='/Game/Kopanice/Animated/'+name
    task=ue.AssetImportTask()
    task.filename=str(source/('SK_OK_'+name+'.fbx'))
    task.destination_path=root
    task.destination_name='SK_OK_'+name
    task.automated=True
    task.replace_existing=True
    task.save=True
    options=ue.FbxImportUI()
    options.import_mesh=True
    options.import_as_skeletal=True
    options.import_materials=False
    options.import_textures=False
    options.import_animations=False
    options.create_physics_asset=False
    options.automated_import_should_detect_type=False
    options.mesh_type_to_import=ue.FBXImportType.FBXIT_SKELETAL_MESH
    task.options=options
    ue.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh=ue.load_asset(root+'/SK_OK_'+name)
    assert isinstance(mesh,ue.SkeletalMesh),name
    mat=ue.load_asset('/Game/Kopanice/Supplied/'+name+'/M_'+name)
    ue.MaterialEditingLibrary.set_material_usage(mat,ue.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    ue.EditorAssetLibrary.save_loaded_asset(mat)
    slots=mesh.get_editor_property('materials')
    for slot in slots:
        slot.set_editor_property('material_interface',mat)
    mesh.set_editor_property('materials',slots)
    ue.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False)
    ue.log('OK_ANIMATED_IMPORTED '+name)
