"""Import only the seven new native tactical HUD icons (Lucide ISC license)."""
from pathlib import Path
import unreal as ue

source = Path(ue.Paths.project_dir()) / 'ArtSource/Generated/TacticalHUD'
names = ('layers', 'check', 'x', 'heart_pulse', 'save', 'folder_open', 'map')
for name in names:
    image = source / ('T_' + name + '.png')
    assert image.is_file(), image
    task = ue.AssetImportTask()
    task.filename = str(image)
    task.destination_path = '/Game/Kopanice/ReferenceHUD'
    task.destination_name = 'T_' + name
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
ue.log('OK_TACTICAL_HUD_IMPORTED: seven icons')
