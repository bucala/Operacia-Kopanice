"""Validate persisted terrain and forest assets in a fresh editor process."""
import unreal as ue

root='/Game/Kopanice/WinterGround'
for name,material in [('SM_OK_WinterGround','M_Snow'),('SM_OK_WinterCobbles','M_Stone')]:
    mesh=ue.load_asset(root+'/'+name)
    mat=ue.load_asset(root+'/'+material)
    assert isinstance(mesh,ue.StaticMesh) and isinstance(mat,ue.Material),name
    assert mesh.get_material(0)==mat,name+' persisted material'
    assert not mesh.get_editor_property('nanite_settings').enabled,name+' exact surface'
    if name=='SM_OK_WinterGround':
        b=mesh.get_bounds()
        assert abs(b.origin.x-900)<1 and abs(b.origin.y-900)<1,'World alignment'
overlay=ue.load_asset(root+'/M_TacticalOverlay')
assert overlay.get_editor_property('blend_mode')==ue.BlendMode.BLEND_TRANSLUCENT
root='/Game/Kopanice/Supplied/Forest'
mat=ue.load_asset(root+'/M_Forest')
for name,height in [('SM_OK_SnowFir',480),('SM_OK_SnowFirSmall',380),('SM_OK_SnowShrub',100),('SM_OK_ForestRock',100)]:
    mesh=ue.load_asset(root+'/'+name)
    assert isinstance(mesh,ue.StaticMesh),name
    assert mesh.get_material(0)==mat,name+' shared atlas'
    assert abs(mesh.get_bounds().box_extent.z*2-height)<1,name+' dimensions'
    settings=mesh.get_editor_property('nanite_settings')
    assert settings.enabled,name+' Nanite'
    assert settings.shape_preservation==ue.NaniteShapePreservation.PRESERVE_AREA,name+' preservation'
for name in ('T_BaseColor','T_MetalRough','T_Normal'):
    texture=ue.load_asset(root+'/'+name)
    assert texture.srgb==(name=='T_BaseColor'),name+' color space'
ue.log('OK_WINTER_ENVIRONMENT_VALIDATED: terrain, overlays and four forest meshes')
