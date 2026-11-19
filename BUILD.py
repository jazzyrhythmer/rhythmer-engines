#!/usr/bin/env python3
"""Build the collected Mac sources without the original developer checkout."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import argparse, json, os, shutil, subprocess

ap=argparse.ArgumentParser()
ap.add_argument('--build-root',type=Path,required=True)
ap.add_argument('--cmake',default='cmake')
ap.add_argument('--jobs',type=int,default=2)
ap.add_argument('--native-only',action='store_true')
ap.add_argument('--stage-only',action='store_true')
a=ap.parse_args()
source=Path(__file__).resolve().parent
build=a.build_root.resolve()
build.mkdir(parents=True,exist_ok=True)
recipes=json.loads((source/'native-build-recipes.json').read_text())
outputs={r[r.index('-o')+1] for r in recipes['objects']+recipes['links']}
prefix='${SOURCE}/upstream/mpc3-native/mac/'
def expand(arg):
    if arg in outputs:
        if not arg.startswith(prefix):raise ValueError('Unexpected output '+arg)
        return str(build/'native'/arg[len(prefix):])
    return arg.replace('${SOURCE}',str(source))
def run(recipe):
    cmd=[expand(x) for x in recipe]
    cmd[0]=subprocess.check_output(['xcrun','--find',cmd[0]],text=True).strip()
    Path(cmd[cmd.index('-o')+1]).parent.mkdir(parents=True,exist_ok=True)
    result=subprocess.run(cmd,cwd=source,capture_output=True,text=True)
    if result.returncode:raise RuntimeError(' '.join(cmd)+'\n'+result.stdout+result.stderr)
if not a.stage_only:
    with ThreadPoolExecutor(max_workers=a.jobs) as pool:
        list(pool.map(run,recipes['objects']))
    for recipe in recipes['links']:run(recipe)
    print('NATIVE BUILD PASS:',len(recipes['objects']),'objects,',len(recipes['links']),'modules',flush=True)
if a.native_only:raise SystemExit(0)
desktop=build/'desktop'
if not a.stage_only:
    subprocess.run([a.cmake,'-S',str(source/'project/desktop'),'-B',str(desktop),
        '-DJUCE_PATH='+str(source/'juce'),'-DCMAKE_BUILD_TYPE=Release',
        '-DCMAKE_OSX_ARCHITECTURES=arm64','-DCMAKE_OSX_DEPLOYMENT_TARGET=13.0'],check=True)
    subprocess.run([a.cmake,'--build',str(desktop),'--target','RhythmerDesktop_AU','RhythmerDesktop_VST3','-j'+str(a.jobs)],check=True)
for fmt,ext in [('AU','component'),('VST3','vst3')]:
    src=desktop/'RhythmerDesktop_artefacts/Release'/fmt/('Rhythmer Engines.'+ext)
    dst=build/'formats'/fmt/src.name
    if dst.exists():shutil.rmtree(dst) # Only generated build-root output.
    dst.parent.mkdir(parents=True,exist_ok=True)
    shutil.copytree(src,dst,symlinks=True)
    resources=dst/'Contents/Resources'
    shutil.copytree(source/'resources-template',resources,dirs_exist_ok=True)
    meta=json.loads((resources/'plugin.json').read_text())
    meta['format']=fmt
    (resources/'plugin.json').write_text(json.dumps(meta,indent=2)+'\n')
    (resources/'Backend').mkdir(exist_ok=True)
    shutil.copy2(build/'native/instrument/rhythmer-engines.dylib',resources/'Backend/rhythmer-engines.dylib')
    for binary in (build/'native/engines').rglob('*'):
        if binary.suffix not in ('.so','.dylib'):continue
        if 'original-build' in binary.parts:continue
        target=resources/'Content/Engines/mac'/binary.relative_to(build/'native/engines')
        target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(binary,target)
    for binary,relative in recipes.get('runtime_aliases',{}).items():
        target=resources/'Content/Engines/mac'/relative
        target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(expand(binary),target)
    for f in resources.rglob('*'):
        if not f.is_file():continue
        with f.open('rb') as stream:magic=stream.read(4)
        if magic!=b'\xcf\xfa\xed\xfe':continue
        subprocess.run(['install_name_tool','-id','@rpath/Rhythmer-'+fmt+'/'+str(f.relative_to(dst/'Contents')),str(f)],check=True,capture_output=True)
        subprocess.run(['codesign','--force','--sign','-','--timestamp=none',str(f)],check=True,capture_output=True)
    subprocess.run(['codesign','--force','--deep','--sign','-','--timestamp=none',str(dst)],check=True,capture_output=True)
    subprocess.run(['codesign','--verify','--deep','--strict',str(dst)],check=True)
print('AU + VST3 BUILD PASS:',build/'formats',flush=True)
