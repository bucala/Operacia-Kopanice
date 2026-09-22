"""Extract reviewed loose components; preserve the shared UV atlas and source file."""
import bpy
import json
import sys
from pathlib import Path
from mathutils import Vector

source,output=sys.argv[sys.argv.index('--')+1:]
output=Path(output)
output.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=source,load_ui=False)
obj=next(o for o in bpy.context.scene.objects if o.type=='MESH')
bpy.ops.object.select_all(action='DESELECT')
obj.select_set(True)
bpy.context.view_layer.objects.active=obj
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.separate(type='LOOSE')
bpy.ops.object.mode_set(mode='OBJECT')
for image in bpy.data.images:
    if image.type=='IMAGE' and image.packed_file:
        image.filepath_raw=str(output/(image.name+'.png'))
        image.file_format='PNG'
        image.save()
manifest=[]
for source_name,name,height in [('Mesh_0.002','SM_OK_SnowFir',4.8),
                                ('Mesh_0.007','SM_OK_SnowFirSmall',3.8),
                                ('Mesh_0.013','SM_OK_SnowShrub',1.0),
                                ('Mesh_0.006','SM_OK_ForestRock',1.0)]:
    obj=bpy.data.objects.get(source_name)
    assert obj and obj.type=='MESH',source_name
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active=obj
    obj.scale*=height/obj.dimensions.z
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    points=[obj.matrix_world@Vector(v) for v in obj.bound_box]
    obj.location-=Vector(((min(v.x for v in points)+max(v.x for v in points))/2,
                          (min(v.y for v in points)+max(v.y for v in points))/2,
                          min(v.z for v in points)))
    bpy.ops.object.transform_apply(location=True,rotation=False,scale=False)
    obj.name=name
    bpy.ops.export_scene.fbx(filepath=str(output/(name+'.fbx')),use_selection=True,
        object_types={'MESH'},mesh_smooth_type='FACE',axis_forward='-Y',axis_up='Z',bake_anim=False,path_mode='STRIP')
    manifest.append({'mesh':name,'source_part':source_name,'polygons':len(obj.data.polygons),'dimensions_m':list(obj.dimensions)})
    # Save a reviewable derived Blender file containing only this mesh and its textures.
    review_scene=bpy.data.scenes.new(name)
    review_scene.collection.objects.link(obj)
    bpy.data.libraries.write(str(output/(name+'.blend')),{review_scene},fake_user=True)
    bpy.data.scenes.remove(review_scene)
(output/'manifest.json').write_text(json.dumps({'source':source,'assets':manifest},indent=2),encoding='utf-8')
print('OK_FOREST_EXPORTED',len(manifest),flush=True)
