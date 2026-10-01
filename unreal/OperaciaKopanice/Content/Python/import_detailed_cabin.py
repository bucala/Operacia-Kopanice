"""Import the generated Blender cabin into a dedicated UE asset folder."""
from pathlib import Path
import unreal as ue

root='/Game/Kopanice/DetailedCabin'
source=Path(ue.Paths.project_dir())/'ArtSource/Generated/SM_OK_CabinDetailed.fbx'
if not source.is_file(): raise RuntimeError('Run ArtSource/build_cabin.py first')
task=ue.AssetImportTask()
task.filename=str(source)
task.destination_path=root
task.destination_name='SM_OK_CabinDetailed'
task.automated=True
task.replace_existing=True
task.save=True
options=ue.FbxImportUI()
options.import_mesh=True
options.import_materials=True
options.import_textures=False
options.import_as_skeletal=False
options.automated_import_should_detect_type=False
options.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH
options.static_mesh_import_data.combine_meshes=True
options.static_mesh_import_data.generate_lightmap_u_vs=False
options.static_mesh_import_data.auto_generate_collision=True
options.static_mesh_import_data.build_nanite=True
task.options=options
ue.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
mesh=ue.load_asset(root+'/SM_OK_CabinDetailed')
if not isinstance(mesh,ue.StaticMesh): raise RuntimeError('Static mesh import failed')

palette={}
for i in range(5): palette['OK_Wood_%d'%i]=(.13+i*.018,.105+i*.016,.078+i*.014)
for i in range(3): palette['OK_Stone_%d'%i]=(.22+i*.035,.235+i*.035,.24+i*.035)
palette.update(OK_Snow=(.78,.86,.92),OK_Glass=(.07,.14,.18),OK_Iron=(.045,.052,.057))
tools=ue.AssetToolsHelpers.get_asset_tools()
lib=ue.MaterialEditingLibrary
materials={}
for name,color in palette.items():
    mat=ue.load_asset(root+'/M_'+name) if ue.EditorAssetLibrary.does_asset_exist(root+'/M_'+name) else None
    if not mat: mat=tools.create_asset('M_'+name,root,ue.Material,ue.MaterialFactoryNew())
    lib.delete_all_material_expressions(mat)
    pos=lib.create_material_expression(mat,ue.MaterialExpressionWorldPosition)
    grain=lib.create_material_expression(mat,ue.MaterialExpressionCustom)
    custom_input=ue.CustomInput()
    custom_input.set_editor_property('input_name','P')
    grain.set_editor_property('inputs',[custom_input])
    grain.set_editor_property('output_type',ue.CustomMaterialOutputType.CMOT_FLOAT3)
    is_wood=name.startswith('OK_Wood')
    pattern='0.72+0.18*sin(P.z*2.8+sin(P.x*.08)*2)+0.10*sin(P.z*12.7+P.y*.1)' if is_wood else '0.92+0.06*sin(P.x*1.1)*sin(P.y*1.3)*sin(P.z*1.7)'
    grain.set_editor_property('code','return float3(%s,%s,%s)*(%s);'%(*color,pattern))
    lib.connect_material_expressions(pos,'',grain,'P')
    lib.connect_material_property(grain,'',ue.MaterialProperty.MP_BASE_COLOR)
    rough=lib.create_material_expression(mat,ue.MaterialExpressionConstant)
    rough.set_editor_property('r',.25 if name=='OK_Glass' else .85)
    lib.connect_material_property(rough,'',ue.MaterialProperty.MP_ROUGHNESS)
    lib.layout_material_expressions(mat)
    lib.recompile_material(mat)
    ue.EditorAssetLibrary.save_loaded_asset(mat)
    materials[name]=mat
slots=mesh.get_editor_property('static_materials')
for index,slot in enumerate(slots):
    name=str(slot.get_editor_property('imported_material_slot_name'))
    if name not in materials: raise RuntimeError('Unknown material slot: '+name)
    slot.set_editor_property('material_interface',materials[name])
    slots[index]=slot
mesh.set_editor_property('static_materials',slots)
ue.EditorAssetLibrary.save_loaded_asset(mesh)
bounds=mesh.get_bounds()
ue.log('OK_CABIN_IMPORTED bounds=%s slots=%d'%(bounds.box_extent,len(slots)))
