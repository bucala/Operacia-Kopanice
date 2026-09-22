"""Create terrain materials and a translucent tactical grid; import authored terrain."""
from pathlib import Path
import unreal as ue

root='/Game/Kopanice/WinterGround'
source=Path(ue.Paths.project_dir())/'ArtSource/Generated/WinterGround'
tools=ue.AssetToolsHelpers.get_asset_tools()
lib=ue.MaterialEditingLibrary

def material(name):
    path=root+'/'+name
    mat=ue.load_asset(path) if ue.EditorAssetLibrary.does_asset_exist(path) else None
    if not mat:
        mat=tools.create_asset(name,root,ue.Material,ue.MaterialFactoryNew())
    lib.delete_all_material_expressions(mat)
    return mat

def constant(mat,value,prop):
    node=lib.create_material_expression(mat,ue.MaterialExpressionConstant)
    node.set_editor_property('r',value)
    lib.connect_material_property(node,'',prop)

materials={}
for name,color in [('Snow',(.72,.81,.88)),('Stone',(.23,.27,.29)),('Water',(.055,.13,.17))]:
    mat=material('M_'+name)
    pos=lib.create_material_expression(mat,ue.MaterialExpressionWorldPosition)
    node=lib.create_material_expression(mat,ue.MaterialExpressionCustom)
    inp=ue.CustomInput()
    inp.set_editor_property('input_name','P')
    node.set_editor_property('inputs',[inp])
    node.set_editor_property('output_type',ue.CustomMaterialOutputType.CMOT_FLOAT3)
    if name=='Stone':
        pattern='0.75+0.25*frac(sin(dot(floor(P.xy/18),float2(127.1,311.7)))*43758.5453)+.06*sin(P.x*2.1)*sin(P.y*1.7)'
    else:
        pattern='.97+.018*sin(P.x*.009+sin(P.y*.007))+.012*sin(P.x*1.91)*sin(P.y*1.89)'
    node.set_editor_property('code','return float3(%s,%s,%s)*(%s);'%(*color,pattern))
    lib.connect_material_expressions(pos,'',node,'P')
    lib.connect_material_property(node,'',ue.MaterialProperty.MP_BASE_COLOR)
    constant(mat,.28 if name=='Water' else .86,ue.MaterialProperty.MP_ROUGHNESS)
    if name=='Water':
        time=lib.create_material_expression(mat,ue.MaterialExpressionTime)
        tick=ue.CustomInput()
        tick.set_editor_property('input_name','T')
        node.set_editor_property('inputs',[inp,tick])
        node.set_editor_property('code',"""
float a=sin(P.y*.10-T*3.2+sin(P.x*.07+T*.6));
float b=sin(P.y*.23-T*5.1+P.x*.08);
float ribbon=pow(saturate(.5+.5*sin(P.x*.12+sin(P.y*.018-T*.8))),12);
float foam=pow(saturate(a*.5+b*.2+.3),6)*.13;
return float3(.045,.12,.15)+float3(.035,.065,.07)*(a*.5+.5)+foam+ribbon*.025;
""")
        lib.connect_material_expressions(time,'',node,'T')
        lib.connect_material_expressions(pos,'',node,'P')
        normal=lib.create_material_expression(mat,ue.MaterialExpressionCustom)
        normal.set_editor_property('inputs',[inp,tick])
        normal.set_editor_property('output_type',ue.CustomMaterialOutputType.CMOT_FLOAT3)
        normal.set_editor_property('code','return normalize(float3(.12*cos(P.x*.07+P.y*.1-T*3.2),.2*cos(P.y*.23-T*5.1),1));')
        lib.connect_material_expressions(pos,'',normal,'P')
        lib.connect_material_expressions(time,'',normal,'T')
        lib.connect_material_property(normal,'',ue.MaterialProperty.MP_NORMAL)
        # A little self-lit scattered light keeps flowing detail visible under the bridge.
        glow=lib.create_material_expression(mat,ue.MaterialExpressionMultiply)
        glow.set_editor_property('const_b',.18)
        lib.connect_material_expressions(node,'',glow,'A')
        lib.connect_material_property(glow,'',ue.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(mat)
    ue.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
    materials[name]=mat

overlay=material('M_TacticalOverlay')
overlay.set_editor_property('blend_mode',ue.BlendMode.BLEND_TRANSLUCENT)
overlay.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_UNLIT)
overlay.set_editor_property('two_sided',True)
color=lib.create_material_expression(overlay,ue.MaterialExpressionVectorParameter)
color.set_editor_property('parameter_name','Color')
color.set_editor_property('default_value',ue.LinearColor(.8,.87,.92,1))
lib.connect_material_property(color,'',ue.MaterialProperty.MP_EMISSIVE_COLOR)
fill=lib.create_material_expression(overlay,ue.MaterialExpressionScalarParameter)
fill.set_editor_property('parameter_name','Fill')
fill.set_editor_property('default_value',0)
uv=lib.create_material_expression(overlay,ue.MaterialExpressionTextureCoordinate)
alpha=lib.create_material_expression(overlay,ue.MaterialExpressionCustom)
inputs=[]
for name in ['UV','Fill','GridVisibility']:
    inp=ue.CustomInput()
    inp.set_editor_property('input_name',name)
    inputs.append(inp)
alpha.set_editor_property('inputs',inputs)
alpha.set_editor_property('output_type',ue.CustomMaterialOutputType.CMOT_FLOAT1)
alpha.set_editor_property('code','float d=min(min(UV.x,1-UV.x),min(UV.y,1-UV.y)); float w=max(fwidth(d),.0005); return max(Fill,(1-smoothstep(.001,.001+w,d))*.19*GridVisibility);')
lib.connect_material_expressions(uv,'',alpha,'UV')
lib.connect_material_expressions(fill,'',alpha,'Fill')
grid=lib.create_material_expression(overlay,ue.MaterialExpressionScalarParameter)
grid.set_editor_property('parameter_name','GridVisibility')
grid.set_editor_property('default_value',1)
lib.connect_material_expressions(grid,'',alpha,'GridVisibility')
lib.connect_material_property(alpha,'',ue.MaterialProperty.MP_OPACITY)
lib.recompile_material(overlay)
ue.EditorAssetLibrary.save_loaded_asset(overlay,only_if_is_dirty=False)

for name,mat in [('SM_OK_WinterGround',materials['Snow']),('SM_OK_WinterCobbles',materials['Stone'])]:
    task=ue.AssetImportTask()
    task.filename=str(source/(name+'.fbx'))
    task.destination_path=root
    task.destination_name=name
    task.automated=True
    task.replace_existing=True
    task.save=True
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
    # Keep the shallow terrain/cobble surface exact. Detailed supplied buildings
    # still use Nanite; these modest terrain meshes do not need cluster reduction.
    options.static_mesh_import_data.build_nanite=False
    task.options=options
    tools.import_asset_tasks([task])
    mesh=ue.load_asset(root+'/'+name)
    assert isinstance(mesh,ue.StaticMesh),name
    settings=mesh.get_editor_property('nanite_settings')
    settings.enabled=False
    mesh.set_editor_property('nanite_settings',settings)
    mesh.set_material(0,mat)
    ue.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False)
    bounds=mesh.get_bounds()
    if name=='SM_OK_WinterGround':
        assert abs(bounds.origin.x-900)<1 and abs(bounds.origin.y-900)<1, 'FBX world-coordinate mismatch'
    else:
        assert bounds.origin.y>0, 'Cobbles mirrored across the river'
    ue.log('OK_GROUND_BOUNDS %s %s'%(name,mesh.get_bounds()))
ue.log('OK_WINTER_GROUND_IMPORTED')
