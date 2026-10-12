"""Validate playable bridge packages without requiring the original Blender file."""
import unreal as ue

root = '/Game/Kopanice/Supplied/Bridge/'
for name in ('SM_OK_BridgeSpan', 'SM_OK_BridgeSupports', 'SM_OK_BridgeRubble'):
    mesh = ue.load_asset(root + name)
    assert isinstance(mesh, ue.StaticMesh), name
    assert mesh.get_editor_property('nanite_settings').enabled, name + ' Nanite'
    assert all(slot.material_interface.get_path_name().startswith(root + 'M_Bridge') for slot in mesh.static_materials)
    bounds = mesh.get_bounding_box()
    if name == 'SM_OK_BridgeSpan':
        assert abs(bounds.max.x - bounds.min.x - 459.87) < 1, 'Span length'
        assert abs(bounds.max.y - bounds.min.y - 260) < 1, 'Deck width'
        assert abs(bounds.max.z - 90) < 1, 'Handrail height'
    if name == 'SM_OK_BridgeSupports':
        assert bounds.max.z < 0, 'Supports must stay below the deck pivot'
    ue.log('OK_BRIDGE_VALIDATED: ' + name)
for name in ('T_BaseColor', 'T_MetalRough', 'T_Normal'):
    texture = ue.load_asset(root + name)
    assert isinstance(texture, ue.Texture2D), name
    assert texture.srgb == (name == 'T_BaseColor'), name + ' color space'
    assert texture.blueprint_get_size_x() == 1024, name + ' budget'
normal = ue.load_asset(root + 'T_Normal')
assert normal.flip_green_channel, 'DirectX normal convention'
material = ue.load_asset(root + 'M_Bridge')
assert material.get_editor_property('used_with_nanite')
assert ue.MaterialEditingLibrary.get_material_property_input_node(material, ue.MaterialProperty.MP_EMISSIVE_COLOR) is None
ue.log('OK_BRIDGE_VALIDATION: PASS')
