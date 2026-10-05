"""Blender: derive lightweight skinned meshes from the reviewed FBX sources.

blender -b --python rig_characters.py -- <Supplied directory> <output directory>
Original meshes and textures are left intact. Coordinates are metres, facing +X.
"""
import bpy
import json
import sys
from pathlib import Path
from mathutils import Vector

source, output = map(Path, sys.argv[sys.argv.index('--') + 1:])
output.mkdir(parents=True, exist_ok=True)
report = []
for name in ('Partisan', 'Officer'):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(source/name/('SM_OK_'+name+'.fbx')))
    obj = next(o for o in bpy.context.scene.objects if o.type == 'MESH')
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    original = len(obj.data.polygons)
    reduction = obj.modifiers.new('Runtime topology', 'DECIMATE')
    reduction.ratio = min(1.0, 28000 / original)
    bpy.ops.object.modifier_apply(modifier=reduction.name)
    coords = [v.co for v in obj.data.vertices]
    minimum = min(v.z for v in coords)
    height = max(v.z for v in coords)-minimum
    for v in obj.data.vertices:
        v.co.z -= minimum
        v.co *= 1.8 / height
    bones = [('root', None, (0,0,0), (0,0,.1)),
             ('pelvis', 'root', (0,0,.91), (0,0,1.1)),
             ('chest', 'pelvis', (0,0,1.1), (0,0,1.48)),
             ('head', 'chest', (0,0,1.48), (0,0,1.77))]
    for side, sign in [('l',1), ('r',-1)]:
        bones += [(f'thigh_{side}', 'pelvis', (0,.105*sign,.91), (0,.12*sign,.48)),
                  (f'calf_{side}', f'thigh_{side}', (0,.12*sign,.48), (0,.13*sign,.10)),
                  (f'foot_{side}', f'calf_{side}', (0,.13*sign,.10), (.17,.13*sign,.05)),
                  (f'arm_{side}', 'chest', (0,.22*sign,1.43), (0,.29*sign,1.12)),
                  (f'forearm_{side}', f'arm_{side}', (0,.29*sign,1.12), (.02,.31*sign,.84))]
    arm_data = bpy.data.armatures.new('OK_Humanoid')
    rig = bpy.data.objects.new('OK_Rig', arm_data)
    bpy.context.collection.objects.link(rig)
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    for bone_name, parent, head, tail in bones:
        bone = arm_data.edit_bones.new(bone_name)
        bone.head, bone.tail = head, tail
        if parent:
            bone.parent = arm_data.edit_bones[parent]
    bpy.ops.object.mode_set(mode='OBJECT')
    groups = {b[0]: obj.vertex_groups.new(name=b[0]) for b in bones}
    segments = {b[0]: (Vector(b[2]), Vector(b[3])) for b in bones}
    def distance(p, segment):
        a,b = segment
        t = max(0,min(1,(p-a).dot(b-a)/(b-a).length_squared))
        return (p-a-t*(b-a)).length
    for vertex in obj.data.vertices:
        p=vertex.co
        side='l' if p.y >= 0 else 'r'
        if p.z > 1.5:
            candidates=['head','chest']
        elif abs(p.y) > .225 and p.z > .76:
            candidates=[f'arm_{side}',f'forearm_{side}','chest']
        elif p.z < .85:
            # Softly share the coat centre between legs instead of tearing a seam.
            candidates=[f'thigh_{side}',f'calf_{side}',f'foot_{side}']
            if name=='Officer' and p.z > .24 and abs(p.y)<.08:
                candidates=['thigh_l','thigh_r','pelvis']
        else:
            candidates=['pelvis','chest']
        weights=[(b,1/max(.025,distance(p,segments[b]))**4) for b in candidates]
        total=sum(w for _,w in weights)
        for b,w in weights:
            groups[b].add([vertex.index],w/total,'REPLACE')
    modifier=obj.modifiers.new('Humanoid skin','ARMATURE')
    modifier.object=rig
    obj.parent=rig
    obj.name='SK_OK_'+name
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    rig.select_set(True)
    bpy.context.view_layer.objects.active=rig
    bpy.ops.export_scene.fbx(filepath=str(output/(obj.name+'.fbx')),use_selection=True,
        object_types={'ARMATURE','MESH'},add_leaf_bones=False,bake_anim=False,
        axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE',path_mode='STRIP')
    bpy.ops.wm.save_as_mainfile(filepath=str(output/(name+'_Rig.blend')))
    report.append(dict(name=name,height_cm=180,source_polygons=original,
                       runtime_polygons=len(obj.data.polygons),bones=len(bones)))
    print('OK_RIG_EXPORTED',report[-1],flush=True)
(output/'character_sizes.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
