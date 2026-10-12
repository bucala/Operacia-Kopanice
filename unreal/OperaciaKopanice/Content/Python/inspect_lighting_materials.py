"""Export lighting diagnostic textures; never modify authored asset materials."""
from pathlib import Path
import unreal as ue

output = Path(ue.Paths.project_saved_dir()) / 'LightingReview'
output.mkdir(parents=True, exist_ok=True)
for name in ('Cabin', 'CabinAlt', 'Partisan', 'Officer'):
    root = '/Game/Kopanice/Supplied/' + name
    material = ue.load_asset(root + '/M_' + name)
    assert isinstance(material, ue.Material), name
    for prop in (ue.MaterialProperty.MP_BASE_COLOR, ue.MaterialProperty.MP_METALLIC,
                 ue.MaterialProperty.MP_ROUGHNESS, ue.MaterialProperty.MP_AMBIENT_OCCLUSION):
        node = ue.MaterialEditingLibrary.get_material_property_input_node(material, prop)
        ue.log('OK_LIGHTING_MATERIAL: %s %s %s' % (name, prop, node))
    for texture in ('T_BaseColor', 'T_MetalRough'):
        asset = ue.load_asset(root + '/' + texture)
        assert isinstance(asset, ue.Texture2D), name + texture
        task = ue.AssetExportTask()
        task.object = asset
        task.filename = str(output / (name + '_' + texture + '.png'))
        task.exporter = ue.TextureExporterPNG()
        task.automated = True
        task.prompt = False
        task.replace_identical = True
        assert ue.Exporter.run_asset_export_task(task), task.filename
        ue.log('OK_LIGHTING_TEXTURE: %s srgb=%s' % (task.filename, asset.srgb))
