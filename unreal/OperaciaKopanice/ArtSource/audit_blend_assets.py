"""Read-only audit of supplied .blend files; writes reports outside the source folder."""
import bpy
import json
import sys
from pathlib import Path
args=sys.argv[sys.argv.index('--')+1:]
source,out=map(Path,args)
out.mkdir(parents=True,exist_ok=True)
report=[]
for path in sorted(source.glob('*.blend')):
    bpy.ops.wm.open_mainfile(filepath=str(path),load_ui=False)
    row={'file':path.name,'objects':[],'images':[],'materials':[]}
    for o in bpy.context.scene.objects:
        if o.type=='MESH':
            row['objects'].append({'name':o.name,'vertices':len(o.data.vertices),'polygons':len(o.data.polygons),
                'dimensions':list(o.dimensions),'location':list(o.location),'rotation':list(o.rotation_euler),
                'uv_layers':len(o.data.uv_layers),'materials':[m.name if m else None for m in o.data.materials]})
    for m in bpy.data.materials:
        row['materials'].append({'name':m.name,'textures':[
            {'node':n.name,'image':n.image.name if n.image else None}
            for n in m.node_tree.nodes if n.type=='TEX_IMAGE'] if m.use_nodes else []})
    for im in bpy.data.images:
        row['images'].append({'name':im.name,'size':list(im.size),'packed':bool(im.packed_file),'path':im.filepath})
    report.append(row)
    print('AUDITED '+path.name,flush=True)
(out/'blend_audit.json').write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding='utf-8')
print('AUDIT_COMPLETE files=%d'%len(report),flush=True)
