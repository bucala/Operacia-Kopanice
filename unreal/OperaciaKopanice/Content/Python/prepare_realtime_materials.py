"""Persist Nanite material usage instead of recompiling default-material fallbacks each launch."""
import unreal as ue

changed = 0
for path in ue.EditorAssetLibrary.list_assets('/Game/Kopanice/Supplied', recursive=True):
    # Only material assets, not large meshes/textures, need to be loaded.
    if '/M_' not in path:
        continue
    material = ue.load_asset(path)
    if not isinstance(material, ue.Material):
        continue
    material.set_editor_property('used_with_nanite', True)
    ue.MaterialEditingLibrary.recompile_material(material)
    ue.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    assert material.get_editor_property('used_with_nanite')
    changed += 1
assert changed >= 7, changed
ue.log('OK_RT_MATERIALS: prepared {} Nanite materials'.format(changed))
