"""Blender background export of optional user-supplied mission props.

blender -b --python prepare_mission_props.py -- <3D_models folder> <ArtSource/Supplied>
Original .blend files are never saved or overwritten.
"""
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector

source, output = map(Path, sys.argv[sys.argv.index('--') + 1:])
presets = [
    ('CabinAlt', 'Meshy_AI_Winterwood_Cabin_0915154730_texture.blend', 720, 0),
    ('CommandVan', 'Meshy_AI_Olive_Command_Caravan_0915152829_texture.blend', 900, 90),
]
manifest = []
for name, filename, length, yaw in presets:
    bpy.ops.wm.open_mainfile(filepath=str(source / filename))
    bpy.ops.object.select_all(action='DESELECT')
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    if not meshes:
        raise RuntimeError('No mesh: ' + filename)
    for obj in meshes:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.rotation_euler.z += math.radians(yaw)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    bpy.context.view_layer.update()
    points = [obj.matrix_world @ Vector(p) for p in obj.bound_box]
    low = Vector(tuple(min(p[i] for p in points) for i in range(3)))
    high = Vector(tuple(max(p[i] for p in points) for i in range(3)))
    center = Vector(((low.x + high.x) / 2, (low.y + high.y) / 2, low.z))
    # Geometry is exported in centimetres, with a centred floor pivot.
    matrix = obj.matrix_world.copy()
    factor = length / (high.x - low.x)
    for vertex in obj.data.vertices:
        vertex.co = (matrix @ vertex.co - center) * factor
    obj.matrix_world.identity()
    triangles = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    if triangles > 180000:
        modifier = obj.modifiers.new('MissionDetailBudget', 'DECIMATE')
        modifier.ratio = 180000 / triangles
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    folder = output / name
    folder.mkdir(parents=True, exist_ok=True)
    for image_name in ('Image_0', 'Image_1', 'Image_2'):
        image = bpy.data.images.get(image_name)
        if image is None:
            raise RuntimeError('Missing texture: ' + image_name)
        image.scale(1024, 1024)
        image.filepath_raw = str(folder / (image_name + '.png'))
        image.file_format = 'PNG'
        image.save()
    mesh_name = 'SM_OK_' + name
    obj.name = mesh_name
    bpy.context.scene.unit_settings.system = 'METRIC'
    bpy.context.scene.unit_settings.scale_length = 0.01
    bpy.ops.export_scene.fbx(filepath=str(folder / (mesh_name + '.fbx')),
        use_selection=True, object_types={'MESH'}, apply_unit_scale=True,
        axis_forward='-Y', axis_up='Z', bake_anim=False, use_mesh_modifiers=True)
    manifest.append({'name': name, 'mesh': mesh_name, 'source': filename,
        'triangles': sum(len(p.vertices) - 2 for p in obj.data.polygons),
        'texture_size': 1024})
output.mkdir(parents=True, exist_ok=True)
(output / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
print('OK_MISSION_PROPS', json.dumps(manifest))
