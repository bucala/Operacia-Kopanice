"""Import the licensed Lucide sound icon without reimporting existing HUD assets."""
from pathlib import Path
import unreal as ue

image = Path(ue.Paths.project_dir()) / 'ArtSource/Generated/TacticalHUD/T_volume_2.png'
assert image.is_file(), image
task = ue.AssetImportTask()
task.filename = str(image)
task.destination_path = '/Game/Kopanice/ReferenceHUD'
task.destination_name = 'T_volume_2'
task.automated = True
task.replace_existing = True
task.save = True
ue.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = ue.load_asset(task.destination_path + '/' + task.destination_name)
assert isinstance(texture, ue.Texture2D)
texture.set_editor_property('compression_settings', ue.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property('lod_group', ue.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property('mip_gen_settings', ue.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property('srgb', True)
ue.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
ue.log('OK_DISTRACTION_HUD_IMPORTED: one icon')
