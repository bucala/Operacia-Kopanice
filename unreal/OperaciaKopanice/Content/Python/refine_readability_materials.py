"""Rebuild four owned PBR materials; retain supplied textures and mesh assignments."""
import unreal as ue

lib = ue.MaterialEditingLibrary
profiles = {'Cabin': (.72, 1.02, .7, .65), 'CabinAlt': (.86, 1.0, .75, .65),
            'Officer': (.7, 1.05, .65, .62), 'Partisan': (.84, 1.0, .7, .62)}

for name, (gamma, gain, normal_strength, roughness_min) in profiles.items():
    root = '/Game/Kopanice/Supplied/' + name
    mat = ue.load_asset(root + '/M_' + name)
    assert isinstance(mat, ue.Material), name
    textures = {n: ue.load_asset(root + '/T_' + n) for n in ('BaseColor', 'MetalRough', 'Normal')}
    assert all(isinstance(t, ue.Texture2D) for t in textures.values()), name
    assert textures['BaseColor'].get_editor_property('srgb'), name
    assert not textures['MetalRough'].get_editor_property('srgb'), name
    lib.delete_all_material_expressions(mat)

    def sample(key, sampler):
        node = lib.create_material_expression(mat, ue.MaterialExpressionTextureSample)
        node.set_editor_property('texture', textures[key])
        node.set_editor_property('sampler_type', sampler)
        return node

    def parameter(key, value):
        node = lib.create_material_expression(mat, ue.MaterialExpressionScalarParameter)
        node.set_editor_property('parameter_name', key)
        node.set_editor_property('default_value', value)
        return node

    def custom(code, inputs, output=ue.CustomMaterialOutputType.CMOT_FLOAT3):
        node = lib.create_material_expression(mat, ue.MaterialExpressionCustom)
        node.set_editor_property('code', code)
        node.set_editor_property('output_type', output)
        pins = []
        for key in inputs:
            pin = ue.CustomInput()
            pin.set_editor_property('input_name', key)
            pins.append(pin)
        node.set_editor_property('inputs', pins)
        for key, (source, channel) in inputs.items():
            assert lib.connect_material_expressions(source, channel, node, key)
        return node

    albedo = sample('BaseColor', ue.MaterialSamplerType.SAMPLERTYPE_COLOR)
    packed = sample('MetalRough', ue.MaterialSamplerType.SAMPLERTYPE_MASKS)
    normal = sample('Normal', ue.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    world = lib.create_material_expression(mat, ue.MaterialExpressionWorldPosition)
    up = lib.create_material_expression(mat, ue.MaterialExpressionVertexNormalWS)
    tone = custom('''float3 c=saturate(pow(max(Albedo, .0001), Gamma) * Gain);
float grain=.75+.25*sin(P.x*.071+sin(P.y*.093));
float edge=saturate((SnowHeight-P.z)/26.)*saturate(Up.z*.5+.5)*grain*SnowAmount;
return lerp(c,float3(.78,.84,.89),edge);''',
                  {'Albedo': (albedo, 'RGB'), 'Gamma': (parameter('AlbedoGamma', gamma), ''),
                   'Gain': (parameter('AlbedoGain', gain), ''), 'P': (world, ''), 'Up': (up, ''),
                   'SnowHeight': (parameter('GroundSnowHeight', 18.), ''),
                   'SnowAmount': (parameter('GroundSnowAmount', .65 if name.startswith('Cabin') else .12), '')})
    normals = custom('return normalize(lerp(float3(0,0,1), N, Strength));',
                     {'N': (normal, 'RGB'), 'Strength': (parameter('NormalStrength', normal_strength), '')})
    roughness = custom('return clamp(Roughness, Minimum, .94);',
                       {'Roughness': (packed, 'G'), 'Minimum': (parameter('RoughnessMin', roughness_min), '')},
                       ue.CustomMaterialOutputType.CMOT_FLOAT1)
    assert lib.connect_material_property(tone, '', ue.MaterialProperty.MP_BASE_COLOR)
    assert lib.connect_material_property(normals, '', ue.MaterialProperty.MP_NORMAL)
    assert lib.connect_material_property(roughness, '', ue.MaterialProperty.MP_ROUGHNESS)
    assert lib.connect_material_property(packed, 'B', ue.MaterialProperty.MP_METALLIC)
    assert lib.connect_material_property(parameter('DielectricSpecular', .35), '', ue.MaterialProperty.MP_SPECULAR)
    assert lib.connect_material_property(parameter('Occlusion', 1.), '', ue.MaterialProperty.MP_AMBIENT_OCCLUSION)
    assert lib.connect_material_property(parameter('Emission', 0.), '', ue.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.set_material_usage(mat, ue.MaterialUsage.MATUSAGE_NANITE)
    if name in ('Partisan', 'Officer'):
        lib.set_material_usage(mat, ue.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    lib.layout_material_expressions(mat)
    lib.recompile_material(mat)
    ue.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    ue.log('OK_READABILITY_MATERIAL: %s gamma=%.2f gain=%.2f normal=%.2f rough_min=%.2f emission=0' %
           (name, gamma, gain, normal_strength, roughness_min))
