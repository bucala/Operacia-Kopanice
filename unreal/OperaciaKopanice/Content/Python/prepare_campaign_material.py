"""Derive neutral forest snow without Mission 1's world-space river mask."""
import unreal as ue

library = ue.EditorAssetLibrary
path = '/Game/Kopanice/WinterGround/M_ForestSnow'
material = ue.load_asset(path) if library.does_asset_exist(path) else library.duplicate_asset(
    '/Game/Kopanice/WinterGround/M_Snow', path)
if not material:
    raise RuntimeError('Forest snow material could not be created')
node = ue.MaterialEditingLibrary.get_material_property_input_node(material, ue.MaterialProperty.MP_BASE_COLOR)
if not isinstance(node, ue.MaterialExpressionCustom):
    raise RuntimeError('Unexpected snow material graph')
node.set_editor_property('code', '''
float2 q=P.xy*.004; float2 i=floor(q), f=frac(q); f=f*f*(3-2*f);
float4 h=frac(sin(float4(dot(i,float2(127.1,311.7)),dot(i+float2(1,0),float2(127.1,311.7)),dot(i+float2(0,1),float2(127.1,311.7)),dot(i+1,float2(127.1,311.7))))*43758.5453);
float n=lerp(lerp(h.x,h.y,f.x),lerp(h.z,h.w,f.x),f.y);
return lerp(float3(.62,.71,.79),float3(.86,.89,.92),.52+n*.48);
''')
ue.MaterialEditingLibrary.recompile_material(material)
library.save_loaded_asset(material, only_if_is_dirty=False)
ue.log('OK_CAMPAIGN_MATERIAL: forest snow saved')
for name in ('CabinAlt', 'CommandVan', 'Forest'):
    mat = ue.load_asset('/Game/Kopanice/Supplied/{0}/M_{0}'.format(name))
    if not mat:
        raise RuntimeError('Missing campaign material: ' + name)
    ue.MaterialEditingLibrary.set_material_usage(mat, ue.MaterialUsage.MATUSAGE_NANITE)
    if name == 'Forest':
        ue.MaterialEditingLibrary.set_material_usage(mat, ue.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    ue.MaterialEditingLibrary.recompile_material(mat)
    library.save_loaded_asset(mat, only_if_is_dirty=False)
    ue.log('OK_CAMPAIGN_MATERIAL: saved usage flags ' + name)
