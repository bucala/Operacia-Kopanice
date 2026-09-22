"""Render a supplied asset read-only, with consistent neutral lighting."""
import bpy
import sys
from mathutils import Vector

source,output=sys.argv[sys.argv.index('--')+1:]
bpy.ops.wm.open_mainfile(filepath=source,load_ui=False)
objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
if not objects:
    objects=[o for o in bpy.data.objects if o.type=='MESH']
    for obj in objects:
        bpy.context.scene.collection.objects.link(obj)
points=[o.matrix_world@Vector(c) for o in objects for c in o.bound_box]
low=Vector(tuple(min(p[i] for p in points) for i in range(3)))
high=Vector(tuple(max(p[i] for p in points) for i in range(3)))
target=(low+high)*.5
extent=max(high-low)
for obj in list(bpy.context.scene.objects):
    if obj not in objects:
        bpy.data.objects.remove(obj,do_unlink=True)
scene=bpy.context.scene
scene.render.engine='CYCLES'
scene.cycles.samples=16
scene.cycles.use_denoising=True
world=bpy.data.worlds.new('Review')
world.use_nodes=True
world.node_tree.nodes['Background'].inputs['Color'].default_value=(.26,.3,.35,1)
world.node_tree.nodes['Background'].inputs['Strength'].default_value=.8
scene.world=world
bpy.ops.object.camera_add(location=target+Vector((1.2,-1.7,2.4))*extent)
camera=bpy.context.object
camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO'
camera.data.ortho_scale=extent*1.55
scene.camera=camera
bpy.ops.object.light_add(type='AREA',location=target+Vector((1,-2,3))*extent)
light=bpy.context.object
light.data.energy=180*extent*extent
light.data.size=extent*2
light.rotation_euler=(target-light.location).to_track_quat('-Z','Y').to_euler()
scene.render.resolution_x=1200
scene.render.resolution_y=1200
scene.render.resolution_percentage=100
scene.render.filepath=output
bpy.ops.render.render(write_still=True)
