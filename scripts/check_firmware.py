"""Compile and run the actual firmware logic with host doubles; no board needed."""
from pathlib import Path
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]

def main():
    fallback = Path.home()/'.platformio/packages/toolchain-gccmingw32/bin/g++.exe'
    compiler = os.environ.get('CXX') or shutil.which('g++') or (str(fallback) if fallback.exists() else None)
    if not compiler:
        raise SystemExit('Install g++ or set CXX to its executable path.')
    out = ROOT/'.pio/host-checks'
    out.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env['PATH'] = str(Path(compiler).parent)+os.pathsep+env.get('PATH','')
    for name in ('minimax_test','oled_smoke_test','hid_smoke_test','profile_smoke_test'):
        exe = out/(name+('.exe' if os.name=='nt' else ''))
        subprocess.run([compiler,'-std=c++11','-O2','-Wall','-Wextra','-Werror',
                        '-Itests/host','-Iinclude',f'tests/{name}.cpp','-o',str(exe)],cwd=ROOT,check=True,env=env)
        subprocess.run([str(exe)],cwd=ROOT,check=True,env=env)
    rows = subprocess.check_output([str(exe),'--dump'],cwd=ROOT,env=env,text=True).splitlines()
    lines = ['# Profile keymaps', '', 'Generated from the compiled `include/keymap.h`. Run `python scripts/check_firmware.py --write-keymap` after changing bindings.', '',
             'Tap the encoder in Macro mode to cycle profiles. Hold 1.5 seconds for XO. Physical legends match Coding; the OLED shows the selected profile and action.', '']
    previous = None
    for row in rows:
        profile, key, label, shortcut = row.split('\t')
        if profile != previous:
            lines.extend([f'## {profile}', '', '| Key | OLED label | Shortcut / consumer action |', '|---:|---|---|'])
            previous = profile
        lines.append(f'| {key} | {label} | {shortcut.replace(chr(96), "&#96;")} |')
        if key == '9': lines.append('')
    rendered = '\n'.join(lines)
    path = ROOT/'docs/keymap.md'
    if '--write-keymap' in sys.argv:
        path.write_text(rendered,encoding='utf-8')
    elif not path.exists() or path.read_text(encoding='utf-8') != rendered:
        raise SystemExit('Keymap docs are stale. Run python scripts/check_firmware.py --write-keymap')
    node = shutil.which('node')
    if not node:
        raise SystemExit('Install Node.js to check the compiled OLED WASM.')
    subprocess.run([node,'tests/oled_chip_test.cjs'],cwd=ROOT,check=True)
    print('PASS: all firmware host and Wokwi OLED checks')

if __name__=='__main__':
    main()
