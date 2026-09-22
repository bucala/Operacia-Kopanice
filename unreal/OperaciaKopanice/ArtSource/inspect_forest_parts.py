"""Inspect loose components of the supplied forest kit, without saving the source."""
import bpy
import json
import sys
from pathlib import Path
from mathutils import Vector

source,report=sys.argv[sys.argv.index('--')+1:]
bpy.ops.wm.open_mainfile(filepath=source,load_ui=False)
obj=next(o for o in bpy.context.scene.objects if o.type=='MESH')
bpy.ops.object.select_all(action='DESELECT')
obj.select_set(True)
bpy.context.view_layer.objects.active=obj
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.separate(type='LOOSE')
bpy.ops.object.mode_set(mode='OBJECT')
parts=[]
for obj in bpy.context.scene.objects:
    if obj.type!='MESH' or len(obj.data.polygons)<2000:
        continue
    corners=[obj.matrix_world@Vector(v) for v in obj.bound_box]
    low=[min(v[i] for v in corners) for i in range(3)]
    high=[max(v[i] for v in corners) for i in range(3)]
    parts.append({'name':obj.name,'polygons':len(obj.data.polygons),'low':low,'high':high})
Path(report).write_text(json.dumps(sorted(parts,key=lambda a:-a['polygons']),indent=2),encoding='utf-8')
print('OK_FOREST_PARTS',len(parts),flush=True)
