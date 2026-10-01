"""Fail the commandlet if an integrated asset or its PBR dependencies are missing."""
import unreal as ue

for name in ('Cabin', 'Car', 'Partisan'):
    root = '/Game/Kopanice/Supplied/' + name
    mesh = ue.load_asset(root+'/SM_OK_'+name)
    assert isinstance(mesh, ue.StaticMesh), name
    assert mesh.get_editor_property('nanite_settings').enabled, name+' Nanite disabled'
    size = mesh.get_bounds().box_extent * 2
    if name == 'Partisan':
        assert abs(size.z-180)<1, str(size)
        assert max(size.x,size.y)<120, 'Display plinth was not removed'
    elif name == 'Cabin':
        assert abs(max(size.x,size.y)-330)<1, str(size)
    else:
        assert abs(max(size.x,size.y)-520)<1, str(size)
    mat = ue.load_asset(root+'/M_'+name)
    assert isinstance(mat,ue.Material), name+' material'
    assert all(slot.material_interface==mat for slot in mesh.static_materials), name+' slots'
    for texname in ('T_BaseColor','T_MetalRough','T_Normal'):
        tex = ue.load_asset(root+'/'+texname)
        assert isinstance(tex,ue.Texture2D), name+' '+texname
        assert tex.srgb == (texname=='T_BaseColor'), name+' color space'
        if texname=='T_Normal':
            assert tex.flip_green_channel, name+' tangent convention'
    ue.log('OK_SUPPLIED_VALIDATED '+name)
