"""Inspect/export a supplied bridge in background Blender without saving its source.

blender -b --python prepare_bridge.py -- <source.blend> <ArtSource/Supplied/Bridge> [--inspect]
"""
import json
import math
import statistics
import sys
from pathlib import Path

import bpy
import bmesh
from mathutils import Vector
from mathutils.bvhtree import BVHTree

arguments = sys.argv[sys.argv.index('--') + 1:]
source, output = map(Path, arguments[:2])
bpy.ops.wm.open_mainfile(filepath=str(source))
meshes = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH' and not obj.hide_render]
if not meshes:
    raise RuntimeError('No renderable bridge mesh')
points = [obj.matrix_world @ Vector(p) for obj in meshes for p in obj.bound_box]
low = Vector(tuple(min(p[i] for p in points) for i in range(3)))
high = Vector(tuple(max(p[i] for p in points) for i in range(3)))
report = {
    'source': source.name,
    'bounds_min': list(low), 'bounds_max': list(high),
    'objects': [{'name': obj.name, 'vertices': len(obj.data.vertices),
                 'triangles': sum(len(p.vertices) - 2 for p in obj.data.polygons),
                 'uv_layers': len(obj.data.uv_layers),
                 'materials': [slot.material.name if slot.material else None for slot in obj.material_slots]}
                for obj in meshes],
    'images': [{'name': image.name, 'size': list(image.size), 'packed': bool(image.packed_file)}
               for image in bpy.data.images if image.type == 'IMAGE'],
}
print('OK_BRIDGE_INSPECT ' + json.dumps(report))
tree = BVHTree.FromObject(meshes[0], bpy.context.evaluated_depsgraph_get())
for y in (-.2, 0, .2):
    hits = []
    for x in (-.8, -.5, 0, .5, .8):
        hit, normal, _, _ = tree.ray_cast(Vector((x, y, high.z + 1)), Vector((0, 0, -1)))
        hits.append((x, round(hit.z, 4) if hit is not None else None,
                     round(normal.z, 3) if normal is not None else None))
    print('OK_BRIDGE_DECK', y, hits)
if '--preview' in arguments:
    output.mkdir(parents=True, exist_ok=True)
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.display.shading.light = 'STUDIO'
    scene.display.shading.color_type = 'TEXTURE'
    scene.display.shading.show_shadows = True
    scene.display.shading.show_cavity = True
    scene.display.shading.background_type = 'WORLD'
    scene.world.color = (.16, .18, .20)
    camera = bpy.data.objects.new('BridgeReviewCamera', bpy.data.cameras.new('BridgeReviewCamera'))
    scene.collection.objects.link(camera)
    camera.location = (2.8, -3.2, 2)
    camera.rotation_euler = ((low + high) / 2 - camera.location).to_track_quat('-Z', 'Y').to_euler()
    camera.data.type = 'ORTHO'
    camera.data.ortho_scale = 2.8
    scene.camera = camera
    scene.render.resolution_x = 1200
    scene.render.resolution_y = 800
    scene.render.resolution_percentage = 100
    scene.render.filepath = str(output / 'BridgeSource.png')
    bpy.ops.render.render(write_still=True)
if '--inspect' not in arguments:
    output.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action='DESELECT')
    for mesh in meshes:
        mesh.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    if len(meshes) > 1:
        bpy.ops.object.join()
    obj = bpy.context.object
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    tree = BVHTree.FromObject(obj, bpy.context.evaluated_depsgraph_get())
    deck_hits = [tree.ray_cast(Vector((x, 0, high.z + 1)), Vector((0, 0, -1)))[0].z
                 for x in (-.5, 0, .5)]
    deck = statistics.median(deck_hits)
    underside = tree.ray_cast(Vector((0, 0, low.z - 1)), Vector((0, 0, 1)))[0].z
    if underside >= deck:
        raise RuntimeError('Invalid bridge deck section')
    # Preserve horizontal proportions. Raise only the handrail, not the slab/pillars.
    factor = 260 / (high.y - low.y)
    rail_scale = 90 / ((high.z - deck) * factor)
    center = Vector(((low.x + high.x) / 2, (low.y + high.y) / 2, deck))
    for vertex in obj.data.vertices:
        point = (vertex.co - center) * factor
        if point.z > 0:
            point.z *= rail_scale
        vertex.co = point
    split_z = (underside - deck) * factor - .1
    original = obj.data.copy()

    def clipped_mesh(name, planes):
        data = original.copy()
        part = bpy.data.objects.new(name, data)
        bpy.context.collection.objects.link(part)
        bm = bmesh.new()
        bm.from_mesh(data)
        for origin, normal, discard_negative in planes:
            result = bmesh.ops.bisect_plane(bm, geom=list(bm.verts) + list(bm.edges) + list(bm.faces),
                plane_co=origin, plane_no=normal, dist=.001,
                clear_inner=discard_negative, clear_outer=not discard_negative)
            edges = [edge for edge in result['geom_cut'] if isinstance(edge, bmesh.types.BMEdge) and edge.is_boundary]
            if edges:
                bmesh.ops.holes_fill(bm, edges=edges, sides=0)
        bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
        bm.to_mesh(data)
        bm.free()
        return part

    span = clipped_mesh('SM_OK_BridgeSpan', [((0, 0, split_z), (0, 0, 1), True)])
    supports = clipped_mesh('SM_OK_BridgeSupports', [((0, 0, split_z), (0, 0, 1), False)])
    debris = []
    for side in (-1, 1):
        piece = clipped_mesh('BridgeEnd',
            [((0, 0, split_z), (0, 0, 1), True), ((side * 65, 0, 0), (side, 0, 0), True)])
        piece.rotation_euler.y = math.radians(side * 9)
        piece.location.z = -18
        debris.append(piece)
    # Preserve a visible central fragment but do not leave a traversable deck.
    fragment = clipped_mesh('BridgeFragment', [((0, 0, split_z), (0, 0, 1), True),
        ((-40, 0, 0), (1, 0, 0), True), ((25, 0, 0), (1, 0, 0), False),
        ((0, -45, 0), (0, 1, 0), True), ((0, 30, 0), (0, 1, 0), False)])
    fragment.location = (18, 15, -45)
    fragment.rotation_euler = (math.radians(19), math.radians(-24), math.radians(16))
    debris.append(fragment)
    bpy.ops.object.select_all(action='DESELECT')
    for piece in debris:
        piece.select_set(True)
    bpy.context.view_layer.objects.active = debris[0]
    bpy.ops.object.join()
    rubble = bpy.context.object
    rubble.name = 'SM_OK_BridgeRubble'
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    for image_name in ('Image_0', 'Image_1', 'Image_2'):
        image = bpy.data.images.get(image_name)
        if image is None:
            raise RuntimeError('Missing bridge texture: ' + image_name)
        image.scale(1024, 1024)
        image.filepath_raw = str(output / (image_name + '.png'))
        image.file_format = 'PNG'
        image.save()
    bpy.context.scene.unit_settings.system = 'METRIC'
    bpy.context.scene.unit_settings.scale_length = .01
    exported = []
    for part in (span, supports, rubble):
        bpy.ops.object.select_all(action='DESELECT')
        part.select_set(True)
        bpy.context.view_layer.objects.active = part
        bpy.ops.export_scene.fbx(filepath=str(output / (part.name + '.fbx')),
            use_selection=True, object_types={'MESH'}, apply_unit_scale=True,
            axis_forward='-Y', axis_up='Z', bake_anim=False, use_mesh_modifiers=True,
            mesh_smooth_type='FACE')
        exported.append({'mesh': part.name, 'triangles': sum(len(p.vertices) - 2 for p in part.data.polygons)})
    manifest = {'source': source.name, 'texture_size': 1024, 'deck_z': 0,
        'length_cm': (high.x - low.x) * factor, 'width_cm': 260, 'handrail_cm': 90, 'assets': exported}
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print('OK_BRIDGE_EXPORTED ' + json.dumps(manifest))
