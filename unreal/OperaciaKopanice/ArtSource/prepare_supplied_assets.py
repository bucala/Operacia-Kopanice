"""Export derived assets without saving changes to the original Blender files."""
import bpy
import bmesh
import json
import math
import sys
from pathlib import Path
from mathutils import Vector

args = sys.argv[sys.argv.index('--') + 1:]
source, output = map(Path, args[:2])
output.mkdir(parents=True, exist_ok=True)
presets = [('Cabin', '*Snowy_Wooden_Cabin*', 3.3),
           ('Car', '*classic_black_car*', 5.2),
           ('Partisan', '*Figur_0915153436*', 1.85)]
manifest = json.loads((output/'manifest.json').read_text(encoding='utf-8')) if len(args)>2 and (output/'manifest.json').exists() else []
for name, pattern, size in presets:
    if len(args)>2 and name != args[2]:
        continue
    original = next(source.glob(pattern + '.blend'))
    bpy.ops.wm.open_mainfile(filepath=str(original), load_ui=False)
    obj = next(o for o in bpy.context.scene.objects if o.type == 'MESH')
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    obj.name = 'SM_OK_' + name
    factor = size / (obj.dimensions.z if name == 'Partisan' else max(obj.dimensions.x, obj.dimensions.y))
    obj.scale *= factor
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    bounds = [obj.matrix_world @ Vector(v) for v in obj.bound_box]
    obj.location -= Vector(((min(v.x for v in bounds)+max(v.x for v in bounds))/2,
                            (min(v.y for v in bounds)+max(v.y for v in bounds))/2,
                            min(v.z for v in bounds)))
    bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)
    if name == 'Partisan':
        # This specific reviewed figurine has a plinth below 0.25 m.
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.bisect_plane(bm, geom=list(bm.verts)+list(bm.edges)+list(bm.faces),
                              plane_co=(0,0,.25), plane_no=(0,0,1), clear_inner=True)
        remaining = set(bm.verts)
        islands = []
        while remaining:
            start = remaining.pop()
            island, stack = {start}, [start]
            while stack:
                v = stack.pop()
                for edge in v.link_edges:
                    other = edge.other_vert(v)
                    if other in remaining:
                        remaining.remove(other)
                        island.add(other)
                        stack.append(other)
            islands.append(island)
        largest = max(islands,key=len)
        bmesh.ops.delete(bm, geom=[v for v in bm.verts if v not in largest], context='VERTS')
        minimum = min(v.co.z for v in bm.verts)
        height = max(v.co.z for v in bm.verts)-minimum
        for v in bm.verts:
            v.co.z -= minimum
            v.co *= 1.8/height
        # Reviewed source faces -Y; game units face +X.
        from mathutils import Matrix
        bmesh.ops.rotate(bm, cent=(0,0,0), matrix=Matrix.Rotation(math.pi/2,3,'Z'), verts=list(bm.verts))
        bm.to_mesh(obj.data)
        bm.free()
        obj.data.update()
        bpy.context.view_layer.update()
    # Nanite retains the high-resolution source; no destructive automatic decimation.
    folder = output / name
    folder.mkdir(exist_ok=True)
    links = []
    for mat in obj.data.materials:
        if mat and mat.use_nodes:
            links += [{'from': l.from_node.name, 'output': l.from_socket.name,
                       'to': l.to_node.name, 'input': l.to_socket.name}
                      for l in mat.node_tree.links]
    for image in bpy.data.images:
        if image.type == 'IMAGE' and image.packed_file:
            image.filepath_raw = str(folder / (image.name + '.png'))
            image.file_format = 'PNG'
            image.save()
    bpy.ops.export_scene.fbx(filepath=str(folder / (obj.name + '.fbx')),
        use_selection=True, object_types={'MESH'}, mesh_smooth_type='FACE',
        axis_forward='-Y', axis_up='Z', apply_unit_scale=True, bake_anim=False,
        path_mode='STRIP')
    scene = bpy.context.scene
    for other in list(scene.objects):
        if other != obj:
            bpy.data.objects.remove(other, do_unlink=True)
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 16
    scene.cycles.use_denoising = True
    scene.world = bpy.data.worlds.new('ReviewWorld')
    scene.world.use_nodes = True
    scene.world.node_tree.nodes['Background'].inputs['Color'].default_value = (.22,.25,.3,1)
    scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value = .7
    target = Vector((0,0,obj.dimensions.z*.45))
    extent = max(obj.dimensions)
    bpy.ops.object.camera_add(location=target+Vector((1.3,-1.7,1.0))*extent)
    camera = bpy.context.object
    camera.rotation_euler = (target-camera.location).to_track_quat('-Z','Y').to_euler()
    camera.data.type = 'ORTHO'
    camera.data.ortho_scale = extent*1.5
    scene.camera = camera
    bpy.ops.object.light_add(type='AREA', location=target+Vector((1,-2,3))*extent)
    light = bpy.context.object
    light.data.energy = 180*extent*extent
    light.data.shape = 'DISK'
    light.data.size = extent*2
    light.rotation_euler = (target-light.location).to_track_quat('-Z','Y').to_euler()
    scene.render.resolution_x = 900
    scene.render.resolution_y = 900
    scene.render.resolution_percentage = 100
    scene.render.filepath = str(folder / 'preview.png')
    bpy.ops.render.render(write_still=True)
    manifest = [a for a in manifest if a['name'] != name]
    manifest.append({'name': name, 'source': str(original), 'mesh': obj.name,
                     'dimensions_m': list(obj.dimensions), 'polygons': len(obj.data.polygons),
                     'material_links': links})
    (output/'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print('OK_SUPPLIED_EXPORTED', name, flush=True)
