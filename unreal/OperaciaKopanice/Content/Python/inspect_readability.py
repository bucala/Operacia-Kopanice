"""Read-only material/rig assignment check for the shadow readability pass."""
import unreal as ue

for name in ('Cabin', 'CabinAlt', 'Officer', 'Partisan'):
    root = '/Game/Kopanice/Supplied/' + name
    mat = ue.load_asset(root + '/M_' + name)
    for key in ('AlbedoGamma', 'AlbedoGain', 'NormalStrength', 'Emission'):
        ue.log('OK_PBR_PARAMETER: %s %s=%s' % (name, key, ue.MaterialEditingLibrary.get_material_default_scalar_parameter_value(mat, key)))
    if name in ('Officer', 'Partisan'):
        mesh = ue.load_asset('/Game/Kopanice/Animated/' + name + '/SK_OK_' + name)
        for slot in mesh.get_editor_property('materials'):
            material = slot.get_editor_property('material_interface')
            ue.log('OK_RIG_MATERIAL: %s %s' % (name, material.get_path_name() if material else 'NONE'))
