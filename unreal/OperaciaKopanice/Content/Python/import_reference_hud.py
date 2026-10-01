"""Import original role illustrations and licensed Lucide UI textures."""
from pathlib import Path
import unreal as ue

source = Path(ue.Paths.project_dir()) / 'ArtSource/Generated/ReferenceHUD'
root = '/Game/Kopanice/ReferenceHUD'
tools = ue.AssetToolsHelpers.get_asset_tools()
for image in sorted(source.glob('T_*.png')):
    task = ue.AssetImportTask()
    task.filename = str(image)
    task.destination_path = root
    task.destination_name = image.stem
    task.automated = True
    task.replace_existing = True
    task.save = True
    tools.import_asset_tasks([task])
    texture = ue.load_asset(root + '/' + image.stem)
    assert isinstance(texture, ue.Texture2D), image.name
    texture.set_editor_property('compression_settings', ue.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group', ue.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('mip_gen_settings', ue.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('srgb', True)
    ue.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
assert ue.load_asset(root + '/T_Officer') and ue.load_asset(root + '/T_Partisan')
ue.log('OK_REFERENCE_HUD_IMPORTED')
