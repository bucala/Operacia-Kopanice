"""Add distance-filtered snow normals without changing road/river transition masks."""
import unreal as ue

lib = ue.MaterialEditingLibrary
for name in ('Snow', 'ForestSnow'):
    material = ue.load_asset('/Game/Kopanice/WinterGround/M_' + name)
    assert material
    normal = lib.get_material_property_input_node(material, ue.MaterialProperty.MP_NORMAL)
    assert isinstance(normal, ue.MaterialExpressionCustom), name
    normal.set_editor_property('code', '''
float footprint=max(length(ddx(P.xy)),length(ddy(P.xy)));
float detail=1-smoothstep(2,12,footprint);
float broadX=.035*cos(P.x*.047+sin(P.y*.033));
float broadY=.045*cos(P.y*.061);
float fineX=.075*sin(P.x*.42+sin(P.y*.31))*detail;
float fineY=.065*sin(P.y*.39+sin(P.x*.27))*detail;
return normalize(float3(broadX+fineX,broadY+fineY,1));
''')
    lib.recompile_material(material)
    ue.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    ue.log('OK_SNOW_MICRODETAIL: ' + name)

# The new perimeter crate clusters are HISM instances, not standalone actors.
wood = ue.load_asset('/Game/Kopanice/WinterGround/M_Wood')
assert wood
lib.set_material_usage(wood, ue.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
lib.recompile_material(wood)
ue.EditorAssetLibrary.save_loaded_asset(wood, only_if_is_dirty=False)
ue.log('OK_DRESSING_MATERIAL: wood instancing enabled')
