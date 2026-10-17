"""Headless Blender geometry acceptance checks. Run with --python-exit-code 1."""
import hashlib
import json
import runpy
from pathlib import Path
import sys
import bpy
import bmesh
from mathutils import Vector

HERE=Path(__file__).resolve().parent
model=runpy.run_path(str(HERE/'model.py'),run_name='tacpad_model')
root=model['build']()
p=model['P']
parts=[o for o in root.all_objects if o.get('print_part')]
report={'version':model['REFERENCE']['version'],'blender':bpy.app.version_string,
        'stage':'digital checks only; physical fit and USB host enumeration pending',
        'source_sha256':{name:hashlib.sha256((HERE/name).read_text(encoding="utf-8-sig").encode("utf-8")).hexdigest() for name in ('model.py','reference.json')},
        'parts':{},'clearances_mm':{},'intersections_mm3':{}}

def metrics(obj):
    bm=bmesh.new();bm.from_mesh(obj.data);bm.transform(obj.matrix_world)
    nonmanifold=sum(not e.is_manifold for e in bm.edges)
    remaining=set(bm.verts);components=0
    while remaining:
        stack=[remaining.pop()];components+=1
        while stack:
            for edge in stack.pop().link_edges:
                for v in edge.verts:
                    if v in remaining:remaining.remove(v);stack.append(v)
    volume=bm.calc_volume(signed=True);bm.free()
    return {'nonmanifold_edges':nonmanifold,'components':components,'volume_mm3':round(volume,4)}

for o in parts:
    result=metrics(o)
    assert result['nonmanifold_edges']==0,(o.name,result)
    assert result['components']==1,(o.name,result)
    assert result['volume_mm3']>0,(o.name,result)
    ident=o['print_part']
    if ident not in report['parts']: report['parts'][ident]=dict(result,quantity=0)
    report['parts'][ident]['quantity']+=1
assert report['parts']['keycap']['quantity']==9
byid={o['print_part']:o for o in parts}
case=byid['case']
assert all(abs(a-b)<.001 for a,b in zip(case.dimensions,(85,120,22)))
report['enclosure_mm']=list(case.dimensions)
report['key_pitch_mm']=p['pitch']
assert p['pitch']==19
floor=p['height']-p['recess_depth']
clear=report['clearances_mm']
clear['pressed_key_skirt_to_deck']=p['pressed_gap']
clear['key_to_key']=p['pitch']-p['key_size']
clear['encoder_push_to_nut']=p['height']+p['knob_base_height']-.5-model['REFERENCE']['encoder']['nut_top']
clear['mx_socket_total_span_allowance']=p['stem_span']-model['REFERENCE']['switch']['stem_span']
clear['oled_carrier_to_sidewall']=p['width']/2-p['wall']-(abs(p['oled_x'])+49/2)
assert min(clear.values())>.05,clear
assert abs(min((byid['keycap'].matrix_world @ Vector(c)).z for c in byid['keycap'].bound_box)-4-floor-.8)<.01

# Real mesh intersections, not bounding-box guesses. Intentional support contacts
# have zero volume. Switch stems and sockets are checked at rest; both move on press.
def intersection(a,b):
    temp=a.copy();temp.data=a.data.copy();bpy.context.scene.collection.objects.link(temp)
    bpy.context.view_layer.objects.active=temp
    mod=temp.modifiers.new('Acceptance intersection','BOOLEAN');mod.operation='INTERSECT';mod.solver='EXACT';mod.object=b
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bm=bmesh.new();bm.from_mesh(temp.data);volume=abs(bm.calc_volume());bm.free()
    mesh=temp.data;bpy.data.objects.remove(temp,do_unlink=True);bpy.data.meshes.remove(mesh)
    report['intersections_mm3'][a.name+' / '+b.name]=round(volume,5)
    assert volume<.03,(a.name,b.name,volume)

for a,b in [('case','base'),('case','oled-carrier'),('case','controller-retainer'),('base','controller-retainer'),('oled-carrier','controller-retainer'),('knob','knob-bezel')]:
    intersection(byid[a],byid[b])
refs=[o for o in root.all_objects if o.type=='MESH' and (o.name.startswith('REFERENCE /') or '/ reference stem' in o.name)]
for o in refs:
    if o.get('reference')=='foot':continue
    for ident in ('case','base','oled-carrier','controller-retainer'):
        intersection(byid[ident],o)
shaft=next(o for o in refs if o.name=='REFERENCE / encoder shaft')
intersection(byid['knob'],shaft)
for cap in (o for o in parts if o['print_part']=='keycap'):
    stem=next(o for o in refs if '/ reference stem' in o.name and abs(o.location.x-cap.location.x)<.01 and abs(o.location.y-cap.location.y)<.01)
    intersection(cap,stem)
    cap.location.z-=4;bpy.context.view_layer.update()
    intersection(cap,case)
    cap.location.z+=4
bpy.context.view_layer.update()
folder=HERE/'exports'
folder.mkdir(exist_ok=True)
model['export_model'](str(folder/'parts'),stl=True,obj=True,separate_stl=True)
(HERE/'validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('PASS: closed connected printable parts, nine caps, travel, interfaces and reference envelope intersections')
