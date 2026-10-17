"""Render genuine service and exploded views from the same product geometry."""
import runpy
import sys
from pathlib import Path
import bpy

HERE=Path(__file__).resolve().parent
studio=runpy.run_path(str(HERE/'render.py'),run_name='tacpad_studio')
scene,model,_=studio['build_studio']()
root=bpy.data.collections['Tacpad 9']
stage=next(c for c in scene.collection.children if c.name.startswith('Studio /'))
floor=bpy.data.objects['Studio / seamless charcoal floor']
floor.hide_render=True
original={o:(o.location.copy(),o.hide_render) for o in list(root.all_objects)}
for o in list(root.all_objects):
    if o.get('print_part')=='base' or o.get('reference')=='foot':o.hide_render=True
cam=studio['camera']('03 / SERVICE / underside',(125,-175,-220),(0,8,12),stage,ortho=205)
scene.camera=cam
# A dedicated lower softbox makes real internal walls and mounts readable.
studio['area_light']('Service / lower key',(-90,-70,-180),(0,10,12),1_000_000,145,(.92,.96,1),stage)
scene.render.resolution_x,scene.render.resolution_y=2240,1680
scene.render.resolution_percentage=100
scene.cycles.samples=96
scene.render.filepath=str(HERE.parent/'docs/assets/tacpad9-service.png')
if '--exploded-only' not in sys.argv:
    bpy.ops.render.render(write_still=True)
if '--service-only' in sys.argv:
    sys.exit(0)
for o,(loc,hidden) in original.items():o.location=loc;o.hide_render=hidden
floor.hide_render=False
floor.location.z=-17.1
for o in list(root.all_objects):
    ident=o.get('print_part')
    if ident=='fit-coupon':continue
    dz=0
    if ident=='base' or o.get('reference')=='foot':dz=-16
    elif ident=='case':dz=22
    elif ident=='keycap':dz=38
    elif ident=='knob':dz=40
    elif ident=='knob-bezel':dz=22
    elif ident=='oled-carrier':dz=6
    elif ident=='controller-retainer':dz=4
    elif o.name.startswith('Switch '):dz=22
    elif o.name.startswith('OLED '):dz=28
    elif o.name.startswith('Knob index'):dz=40
    elif o.get('reference')=='display':dz=12
    elif o.get(studio['STUDIO_TAG']):
        dz=38 if o.name.startswith('Print /') and o.location.z>25 else 22
    o.location.z+=dz
bpy.context.view_layer.update()
cam=studio['camera']('04 / EXPLODED',(170,-240,235),(0,3,22),stage,ortho=223)
scene.camera=cam
scene.render.resolution_x,scene.render.resolution_y=2560,1920
scene.render.filepath=str(HERE.parent/'docs/assets/tacpad9-exploded.png')
bpy.ops.render.render(write_still=True)
print('PASS: service and exploded product views rendered')
