"""Deterministic terrain/cobbles in Blender; all authoring coordinates below are cm."""
import bpy
import math
import random
from pathlib import Path

output = Path(__file__).resolve().parent/'Generated/WinterGround'
output.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
rng = random.Random(1944)

def mesh(name, vertices, faces, material):
    data=bpy.data.meshes.new(name)
    # FBX converts Blender's handedness by flipping Y on UE import.
    # Pre-flip the authored world coordinates and winding to match the game grid.
    data.from_pydata([(x/100,-y/100,z/100) for x,y,z in vertices],[],[tuple(reversed(f)) for f in faces])
    data.update()
    obj=bpy.data.objects.new(name,data)
    bpy.context.collection.objects.link(obj)
    data.materials.append(material)
    for face in data.polygons:
        face.use_smooth=True
    return obj

def export(obj):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=str(output/(obj.name+'.fbx')),use_selection=True,
        object_types={'MESH'},mesh_smooth_type='FACE',axis_forward='-Y',axis_up='Z',bake_anim=False)

snow=bpy.data.materials.new('OK_GroundSnow')
stone=bpy.data.materials.new('OK_GroundStone')
roads=[ [(-1300,1260),(540,1260),(790,760),(1450,720),(1620,180),(2800,-400)],
        [(540,1260),(400,780),(300,450)],
        [(1450,720),(1660,1260),(2500,1600)],
        [(900,450),(900,990)],[(1260,450),(1260,990)] ]

def path_distance(x,y):
    best=1e9
    for route in roads:
        for (ax,ay),(bx,by) in zip(route,route[1:]):
            dx,dy=bx-ax,by-ay
            t=max(0,min(1,((x-ax)*dx+(y-ay)*dy)/(dx*dx+dy*dy)))
            best=min(best,math.hypot(x-ax-t*dx,y-ay-t*dy))
    return best

def height(x,y):
    # Inside the logical board, keep the surface below the common Z=0 picking plane.
    outside=max(0,-90-x,x-1890,-90-y,y-1530)
    hills=min(1,outside/850)*(38+28*math.sin(x*.0027)*math.cos(y*.0031))
    micro=1.2*math.sin(x*.019)*math.cos(y*.023)
    river_center=1080+min(1,max(0,-90-y,y-1530)/400)*70*math.sin(y*.004)
    river=abs(x-river_center)
    channel=1-max(0,min(1,(river-72)/32))
    road=1-max(0,min(1,(path_distance(x,y)-90)/45))
    return -4+micro+hills-57*channel-3*road

vertices=[]
faces=[]
step=30
count=401
for j in range(count):
    y=-5100+j*step
    for i in range(count):
        x=-5100+i*step
        vertices.append((x,y,height(x,y)))
for j in range(count-1):
    for i in range(count-1):
        a=j*count+i
        faces.append((a,a+1,a+count+1,a+count))
export(mesh('SM_OK_WinterGround',vertices,faces,snow))

vertices=[]
faces=[]
for row,y in enumerate(range(-800,1900,19)):
    for x0 in range(-1500,2800,26):
        x=x0+(row%2)*13+rng.uniform(-2,2)
        yy=y+rng.uniform(-2,2)
        distance=path_distance(x,yy)
        if distance>112+rng.uniform(-15,15) or 978<x<1182:
            continue
        if 90<x<810 and -90<yy<630:
            continue
        if distance>80 and rng.random()<.35:
            continue
        z=height(x,yy)+rng.uniform(2,4)
        w,h=rng.uniform(10,12),rng.uniform(7,8)
        outline=[(-w+2,-h),(w-2,-h),(w,-h+2),(w,h-2),(w-2,h),(-w+2,h),(-w,h-2),(-w,-h+2)]
        base=len(vertices)
        for dx,dy in outline:
            vertices.append((x+dx,yy+dy,z-4))
        for dx,dy in outline:
            vertices.append((x+dx*.82,yy+dy*.82,z))
        faces.append(tuple(base+8+i for i in range(8)))
        for i in range(8):
            k=(i+1)%8
            faces.append((base+i,base+k,base+8+k,base+8+i))
export(mesh('SM_OK_WinterCobbles',vertices,faces,stone))
bpy.ops.wm.save_as_mainfile(filepath=str(output/'WinterGround.blend'))
print('OK_WINTER_GROUND: terrain and %d cobbles exported'%(len(vertices)//16),flush=True)
