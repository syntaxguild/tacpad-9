"""Package locally verified outputs. Run checks/builds/rendering before this script."""
from pathlib import Path
import hashlib
import json
import shutil
import struct
import zipfile

ROOT=Path(__file__).resolve().parents[1]
version=(ROOT/'VERSION').read_text(encoding='utf-8-sig').strip()
out=ROOT/'dist';out.mkdir(exist_ok=True)
report=json.loads((ROOT/'mechanical/validation.json').read_text())
assert report['version']==version

def bundle(suffix,files,extra=None):
    path=out/f'Tacpad9-v{version}-{suffix}.zip'
    with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
        for file,name in files:
            assert file.is_file(),f'Missing required artifact: {file}'
            archive.write(file,name)
        for name,data in (extra or {}).items():archive.writestr(name,data)
    with zipfile.ZipFile(path) as archive:assert archive.testzip() is None
    return path

cad=[]
for pattern in ('*.py','*.json','exports/*.blend','exports/parts/*'):
    for file in sorted((ROOT/'mechanical').glob(pattern)):
        cad.append((file,file.relative_to(ROOT).as_posix()))
assert any(n.endswith('.blend') for _,n in cad)
for ident,part in report['parts'].items():
    file=ROOT/'mechanical/exports/parts'/f'{ident}.stl'
    data=file.read_bytes();triangles=struct.unpack_from('<I',data,80)[0]
    assert len(data)==84+50*triangles and triangles>0,file
cad.append((ROOT/'docs/assets/primadon-logo.png','docs/assets/primadon-logo.png'))
manifest=json.dumps({'version':version,'units':'mm','parts':{k:v['quantity'] for k,v in report['parts'].items()},
                     'note':'Print nine keycaps. Fit coupon is optional. Prototype fit remains untested.'},indent=2)
outputs=[bundle('CAD',cad,{'PRINT-MANIFEST.json':manifest,'READ-ME.txt':
    'Tacpad 9 / Primadon - digital prototype\nOpen mechanical/exports/Tacpad_9_Primadon.blend in Blender 4.x.\nSTLs: mechanical/exports/parts/ (mm; one cap file, print nine).\nRebuild from extracted root: blender -b --python mechanical/render.py\nModel: mechanical/model.py; keep reference.json beside it.\nMeasure actual parts and print the coupon before the full enclosure.\nSee the repository docs for assembly and validation limits.\n'})]
images=[(f,f.name) for f in sorted((ROOT/'docs/assets').glob('tacpad9-*.png'))]
assert len(images)==4
outputs.append(bundle('Images',images))

build=ROOT/'.pio/build/tacpad_s3'
firmware={name:(build/name).read_bytes() for name in ('bootloader.bin','partitions.bin','firmware.bin')}
firmware['boot_app0.bin']=(Path.home()/'.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin').read_bytes()
assert firmware['firmware.bin'][0]==0xe9
assert struct.unpack_from('<H',firmware['firmware.bin'],12)[0]==9,'Expected ESP32-S3 image'
# Validate the app address/length against the generated partition table.
entries=[struct.unpack_from('<HBBII16sI',firmware['partitions.bin'],i) for i in range(0,0xc00,32)]
app=next(e for e in entries if e[0]==0x50aa and e[1]==0 and e[2] in (0,0x10))
assert app[3]==0x10000 and len(firmware['firmware.bin'])<=app[4]
offsets={'bootloader.bin':0,'partitions.bin':0x8000,'boot_app0.bin':0xe000,'firmware.bin':0x10000}
merged=bytearray(b'\xff'*(0x10000+len(firmware['firmware.bin'])))
last_end=0
for name,offset in offsets.items():
    assert offset>=last_end
    merged[offset:offset+len(firmware[name])]=firmware[name]
    last_end=offset+len(firmware[name])
extra=dict(firmware)
extra['Tacpad9-merged.bin']=bytes(merged)
extra['flash-layout.json']=json.dumps({'chip':'esp32s3','flash_size':'4MB','version':version,'offsets':offsets},indent=2)
extra['READ-ME.txt']='''Tacpad 9 / ESP32-S3 FH4R2 - digital prototype, physical USB testing pending.
Use a data cable. Hold BOOT while plugging in for the first flash.
Tap/release encoder: Coding / Blender / Media; hold 1.5 s: Macro / XO.
Profile choice is stored in NVS; a * on OLED means the save failed.
Install esptool and run, replacing COM5 with your actual port:
python -m esptool --chip esp32s3 --port COM5 write_flash 0x0 Tacpad9-merged.bin
Reset/replug after flashing. This merged image contains bootloader, partition
 table, initial OTA data and application. The separate application firmware.bin
belongs at 0x10000, not 0x0. Do not flash this bundle to a classic ESP32 or C3.
Writing this full merged image can reset saved NVS settings, including profile.
Recommended alternative: build/upload the source with pio run -e tacpad_s3 -t upload.
'''
outputs.append(bundle('USB-Firmware',[],extra))
(out/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.name+'\n' for f in outputs),encoding='utf-8')
for file in outputs:print(f'{file.name}: {file.stat().st_size:,} bytes')
print('PASS: bundle CRCs, STL sizes, S3 image target and flash layout')
