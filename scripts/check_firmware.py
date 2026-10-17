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
    for name in ('minimax_test','oled_smoke_test','hid_smoke_test'):
        exe = out/(name+('.exe' if os.name=='nt' else ''))
        subprocess.run([compiler,'-std=c++11','-O2','-Wall','-Wextra','-Werror',
                        '-Itests/host','-Iinclude',f'tests/{name}.cpp','-o',str(exe)],cwd=ROOT,check=True,env=env)
        subprocess.run([str(exe)],cwd=ROOT,check=True,env=env)
    node = shutil.which('node')
    if not node:
        raise SystemExit('Install Node.js to check the compiled OLED WASM.')
    subprocess.run([node,'tests/oled_chip_test.cjs'],cwd=ROOT,check=True)
    print('PASS: all firmware host and Wokwi OLED checks')

if __name__=='__main__':
    main()
