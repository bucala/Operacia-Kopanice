"""Blender 5.1 background generator. Original geometry based on the user's house1.png.
Run: blender -b --python build_cabin.py -- <output-directory>
"""
import bpy
import bmesh
import math
import random
import sys
from pathlib import Path
from mathutils import Vector, noise

OUT = Path(sys.argv[sys.argv.index('--') + 1])
OUT.mkdir(parents=True, exist_ok=True)
random.seed(1944)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = 1.0

def material(name, color, roughness=0.85):
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*color, 1)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = (*color, 1)
    bsdf.inputs['Roughness'].default_value = roughness
    return m

wood = [material('OK_Wood_%d' % i, (0.13+i*.018, .105+i*.016, .078+i*.014)) for i in range(5)]
stone = [material('OK_Stone_%d' % i, (.22+i*.035, .235+i*.035, .24+i*.035)) for i in range(3)]
snow = material('OK_Snow', (.78,.86,.92))
glass = material('OK_Glass', (.07,.14,.18), .23)
iron = material('OK_Iron', (.045,.052,.057), .5)
objects = []

def box(name, p, size, mat, bevel=.018, rot=(0,0,0)):
    bpy.ops.mesh.primitive_cube_add(size=1, location=p)
    o = bpy.context.object
    o.name = name
    o.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    o.rotation_euler = rot
    o.data.materials.append(mat)
    if bevel:
        mod=o.modifiers.new('Worn edges','BEVEL')
        mod.width=bevel
        mod.segments=2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    objects.append(o)
    return o

def cut_spans(a, b, holes):
    spans=[(a,b)]
    for left,right in holes:
        updated=[]
        for start,end in spans:
            if right<=start or left>=end:
                updated.append((start,end))
            else:
                if left>start: updated.append((start,left))
                if right<end: updated.append((right,end))
        spans=updated
    return spans

# Masonry has individual joints and uneven faces, rather than a solid pedestal.
for row in range(3):
    for side in (-1,1):
        for i in range(10):
            t=-2.7+i*.6
            for axis in (0,1):
                p=(t,side*2.95,.1+row*.16) if axis==0 else (side*2.95,t,.1+row*.16)
                size=(.57,.34,.145) if axis==0 else (.34,.57,.145)
                box('Foundation stone', p, size, random.choice(stone), .025,
                    (0,0,random.uniform(-.025,.025)))

# Horizontal log courses, with genuine door/window gaps in the wall geometry.
for row in range(16):
    z=.58+row*.18
    for side in (-1,1):
        holes=[]
        if side==1 and z<2.52: holes.append((-.53,.53))
        if 1.34<z<2.35: holes += [(-2.22,-1.28),(1.28,2.22)]
        for a,b in cut_spans(-3.08,3.08,holes):
            box('Facade timber', ((a+b)/2,side*2.98,z), (b-a,.25,.174),
                random.choice(wood), .035, (0,random.uniform(-.004,.004),0))
        sideholes=[(-.5,.5)] if 1.34<z<2.35 else []
        for a,b in cut_spans(-3.06,3.06,sideholes):
            box('Side timber', (side*2.98,(a+b)/2,z), (.25,b-a,.174),random.choice(wood), .035)

def window(x,y,rotate=False):
    def part(p,size,mat):
        outward=1 if y>0 else -1
        if rotate:
            p=(y+outward*(p[1]+.16),x+p[0],p[2])
            size=(size[1],size[0],size[2])
        else: p=(x+p[0],y+outward*(p[1]+.16),p[2])
        box('Window',p,size,mat,.012)
    part((0,0,1.85),(.9,.05,.96),glass)
    for a in (-.51,.51): part((a,.035,1.85),(.075,.15,1.15),wood[3])
    for z in (1.30,2.4): part((0,.035,z),(1.1,.15,.075),wood[3])
    part((0,.055,1.85),(.045,.12,1),wood[2])
    part((0,.06,1.85),(1,.12,.045),wood[2])
    part((0,.08,1.26),(1.2,.3,.07),wood[3])
    part((0,.08,1.31),(1.16,.27,.055),snow)

for y in (-3,3):
    for x in (-1.75,1.75): window(x,y)
for x in (-3,3): window(0,x,True)
for i in range(7): box('Door board',(-.45+i*.15,3.02,1.48),(.142,.09,1.96),wood[i%5],.008)
for z in (.8,2.1): box('Door hinge',(-.32,3.085,z),(.36,.035,.05),iron,.004)
box('Latch',(.34,3.09,1.48),(.04,.06,.15),iron,.006)
for i in range(3): box('Entry step',(0,3.14+i*.18,.36-i*.1),(1.25,.45,.15),stone[i],.025)

eave=3.42
run=3.45
slope=.78
pitch=math.atan(slope)
# Planked triangular gables with a small loft ventilation opening.
for i in range(31):
    x=-3+i*.2
    h=(run-abs(x))*slope
    for side in (-1,1):
        if abs(x)<.3:
            box('Loft lower board',(x,side*3,eave+(h-.48)/2),(.19,.14,h-.48),wood[i%5],.008)
        else: box('Gable board',(x,side*3,eave+h/2),(.19,.14,h),wood[i%5],.008)

for side in (-1,1):
    for j in range(17):
        y=-3.3+j*.4125
        box('Roof rafter',(side*run/2,y,eave+run*slope/2),
            (run/math.cos(pitch),.11,.14),wood[1],.012,(0,side*pitch,0))
    for i in range(10):
        u=(i+.5)/10
        x=side*run*u
        for j in range(22):
            y=-3.4+(j+.5)*6.8/22
            # Deliberately damaged boards where the snow surface exposes the rafters.
            damaged=side==1 and .48<u<.88 and .65<y<1.6
            if damaged and (i+j)%3!=0: continue
            box('Roof board',(x,y,eave+(run-abs(x))*slope+.09),
                (run/10/math.cos(pitch)+.045,6.8/22-.008,.035),random.choice(wood),.005,
                (0,side*pitch,random.uniform(-.008,.008)))
    # Tessellated snow surface with irregular boundaries and exposed roof patches.
    verts=[]; faces=[]; nu=64; nv=112
    for i in range(nu+1):
        u=i/nu
        for j in range(nv+1):
            v=j/nv
            y=-3.48+6.96*v
            edge=run+.055*math.sin(y*4)+.035*math.sin(y*11)
            x=side*u*edge
            n=noise.noise_vector(Vector((x*2.4,y*2.4,1))).x
            z=eave+(run-abs(x))*slope+.21+.065*n+.021*math.sin(y*8+x*7)
            verts.append((x,y,z))
    for i in range(nu):
        for j in range(nv):
            u=(i+.5)/nu; y=-3.48+6.96*(j+.5)/nv
            hole=side==1 and ((u-.67)/.25)**2+((y-1.12)/.72)**2<1+.12*math.sin(y*14)+.08*math.sin(u*53)
            notch=side==1 and u>.83+.03*math.sin(y*12) and -1.7<y<-.8
            if hole or notch: continue
            a=i*(nv+1)+j
            face=(a,a+1,a+nv+2,a+nv+1)
            faces.append(tuple(reversed(face)) if side==1 else face)
    mesh=bpy.data.meshes.new('Snow geometry'); mesh.from_pydata(verts,[],faces); mesh.update()
    o=bpy.data.objects.new('Sculpted roof snow',mesh); bpy.context.collection.objects.link(o)
    o.data.materials.append(snow)
    for poly in mesh.polygons: poly.use_smooth=True
    bpy.context.view_layer.objects.active=o
    mod=o.modifiers.new('Snow thickness','SOLIDIFY'); mod.thickness=.14
    bpy.ops.object.modifier_apply(modifier=mod.name)
    mod=o.modifiers.new('Soft snow edges','BEVEL'); mod.width=.025; mod.segments=3
    bpy.ops.object.modifier_apply(modifier=mod.name)
    objects.append(o)

# Hollow stone chimney, with no cap sealing the opening.
for row in range(6):
    z=5.0+row*.16
    for side in (-1,1):
        box('Chimney',(1+side*.23,-1.4,z),(.13,.58,.15),stone[row%3],.012)
        box('Chimney',(1,-1.4+side*.23,z),(.35,.13,.15),stone[row%3],.012)

bpy.ops.object.select_all(action='DESELECT')
for o in objects: o.select_set(True)
bpy.context.view_layer.objects.active=objects[0]
bpy.ops.object.join()
cabin=bpy.context.object; cabin.name='SM_OK_CabinDetailed'
bpy.context.scene.cursor.location=(0,0,0)
bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
bm=bmesh.new(); bm.from_mesh(cabin.data)
bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
bm.to_mesh(cabin.data); bm.free()
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.smart_project(angle_limit=1.15,island_margin=.008)
bpy.ops.object.mode_set(mode='OBJECT')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'CabinDetailed.blend'))
bpy.ops.export_scene.fbx(filepath=str(OUT/'SM_OK_CabinDetailed.fbx'),use_selection=True,
    object_types={'MESH'},apply_unit_scale=True,axis_forward='-Y',axis_up='Z',bake_anim=False,mesh_smooth_type='FACE')
print('OK_CABIN_EXPORT vertices=%d polygons=%d' % (len(cabin.data.vertices),len(cabin.data.polygons)))
