#!/usr/bin/env python3
"""Rebuild the x64 Windows instrument on macOS using Zig + MSVC-mode Clang/LLVM.
Windows SDK / MSVC headers and libraries are provided by the builder, not bundled.
"""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import argparse,json,os,shutil,subprocess,sys
p=argparse.ArgumentParser();p.add_argument('--build-root',type=Path,required=True);p.add_argument('--zig',required=True);p.add_argument('--clang',required=True);p.add_argument('--llvm-bin',type=Path,required=True);p.add_argument('--sdk-root',type=Path,required=True);p.add_argument('--host-juceaide',type=Path,required=True);p.add_argument('--cmake',default='cmake');p.add_argument('--jobs',type=int,default=2);p.add_argument('--native-only',action='store_true');p.add_argument('--stage-only',action='store_true');a=p.parse_args()
src=Path(__file__).resolve().parent;build=a.build_root.resolve();build.mkdir(parents=True,exist_ok=True);compat=src/'upstream/desktop-20261126/windows-compat';recipes=json.loads((src/'native-build-recipes.json').read_text());prefix='${SOURCE}/upstream/mpc3-native/mac/';outputs={c[c.index('-o')+1] for c in recipes['objects']+recipes['links']};skipped=set()
env={**os.environ,'TMPDIR':str(build/'tmp'),'ZIG_GLOBAL_CACHE_DIR':str(build/'zig-global'),'ZIG_LOCAL_CACHE_DIR':str(build/'zig-cache')};Path(env['TMPDIR']).mkdir(exist_ok=True)
def expand(x):
 if x in outputs:
  assert x.startswith(prefix),x
  target=str(build/'native'/x[len(prefix):]);return target[:-6]+'.dll'if target.endswith('.dylib')else target[:-3]+'.dll'if target.endswith('.so')else target
 return x.replace('${SOURCE}',str(src))
def run(c):
 subprocess.run(list(map(str,c)),env=env,cwd=src,check=True)
def compile(c):
 source=c[c.index('-c')+1];target=c[c.index('-o')+1];frodhes='/frodhes/'in source
 if frodhes and Path(source).suffix.lower()=='.s':return
 flags=[];skip=False
 for i,x in enumerate(c):
  if skip:skip=False;continue
  if i==0:continue
  if x in ['-c','-o','-target','-arch','-isysroot']:skip=True;continue
  if x.startswith(('-mmacosx','-stdlib=','-Wl,','-Dsection(x)','-flto','-mcpu=')):continue
  if x.startswith('-I')and ('/mac/compat'in x or '/desktop-20261126/mac-compat'in x):continue
  flags.append(expand(x))
 if source.endswith('/move_loader.c'):source=str(compat/'move_loader.c')
 flags=['-I'+str(compat),*flags]
 if '/tenebrae/'in source:flags+=['-include',str(compat/'tenebrae_move_info.h')]
 if frodhes:
  win=src/'upstream/source-built/frodhes/source/plugin/compat/win';flags=['-I'+str(win),'-include',str(win/'frp_win.h'),*flags]
 flags=[x for x in flags if not x.startswith('-DDISPLAY_REV_THREAD')]
 obj=Path(expand(target));obj.parent.mkdir(parents=True,exist_ok=True)
 run([a.zig,'c++'if Path(source).suffix in ['.cpp','.cc','.cxx','.mm']else'cc','-target','x86_64-windows-gnu',*flags,'-c',expand(source),'-o',obj])
for c in recipes['objects']:
 if '/frodhes/'in c[c.index('-c')+1]and Path(c[c.index('-c')+1]).suffix.lower()=='.s':skipped.add(c[c.index('-o')+1])
if not a.stage_only:
 with ThreadPoolExecutor(max_workers=a.jobs)as pool:list(pool.map(compile,recipes['objects']))
 for c in recipes['links']:
  dest=Path(expand(c[c.index('-o')+1]));dest.parent.mkdir(parents=True,exist_ok=True);symbol='move_plugin_init_v2'if dest.name=='dsp.dll'else'VSTPluginMain';defs=dest.with_suffix('.def');defs.write_text('EXPORTS\n '+symbol+'\n');objs=[expand(x)for x in c if x.endswith('.o')and x not in skipped];extra=['-lshell32','-lole32']if '/frodhes/'in str(dest)else[];run([a.zig,'c++','-target','x86_64-windows-gnu','-shared',*objs,defs,*extra,'-o',dest])
if a.native_only:raise SystemExit(0)
# Find the installed Windows Kits and MSVC directories under the builder's SDK root.
sdk=a.sdk_root.resolve();vc=sorted((sdk/'VC/Tools/MSVC').iterdir())[-1];kits=sdk/'Windows Kits/10';version=sorted((kits/'Include').iterdir())[-1].name
cm=build/'cmake';cm.mkdir(exist_ok=True);wrapper=cm/'clang-cl.py';wrapper.write_text('#!'+sys.executable+'\nimport subprocess,sys\nsys.exit(subprocess.call('+repr([str(Path(a.clang).resolve()),'--driver-mode=cl'])+'+sys.argv[1:]))\n');wrapper.chmod(0o755)
lib=cm/'llvm-lib'
if not lib.exists():lib.symlink_to(a.llvm_bin.resolve()/'llvm-ar')
incs=[vc/'include',*[kits/'Include'/version/x for x in ['ucrt','shared','um','winrt']]];flags='--no-default-config --target=x86_64-pc-windows-msvc '+' '.join('/imsvc"'+str(x)+'"'for x in incs);libs=[vc/'lib/x64',kits/'Lib'/version/'um/x64',kits/'Lib'/version/'ucrt/x64'];link='/MANIFEST:NO '+' '.join('/LIBPATH:"'+str(x)+'"'for x in libs)
def cmquote(s):return '"'+str(s).replace('\\','/').replace('"','\\"')+'"'
tool=cm/'windows-x64.cmake';lines=['set(CMAKE_SYSTEM_NAME Windows)','set(CMAKE_SYSTEM_PROCESSOR AMD64)','set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)','set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)']
for key,val in {'C_COMPILER':wrapper,'CXX_COMPILER':wrapper,'LINKER':a.llvm_bin.resolve()/'lld-link','AR':lib,'RC_COMPILER':a.llvm_bin.resolve()/'llvm-rc','MT':a.llvm_bin.resolve()/'llvm-mt','C_FLAGS_INIT':flags,'CXX_FLAGS_INIT':flags+' /EHsc','RC_FLAGS_INIT':' '.join('/I"'+str(kits/'Include'/version/x)+'"'for x in ['um','shared']),'EXE_LINKER_FLAGS_INIT':link,'SHARED_LINKER_FLAGS_INIT':link,'MODULE_LINKER_FLAGS_INIT':link}.items():lines.append('set(CMAKE_'+key+' '+cmquote(val)+')')
tool.write_text('\n'.join(lines)+'\n');desktop=build/'desktop'
if not a.stage_only:
 run([a.cmake,'-S',src/'project/desktop','-B',desktop,'-DCMAKE_TOOLCHAIN_FILE='+str(tool),'-DJUCE_PATH='+str(src/'juce'),'-DRHYTHMER_USE_INSTALLED_JUCE=ON','-DRHYTHMER_HOST_JUCEAIDE='+str(a.host_juceaide.resolve()),'-DCMAKE_BUILD_TYPE=Release']);run([a.cmake,'--build',desktop,'--target','RhythmerDesktop_VST3','-j'+str(a.jobs)])
bundle=desktop/'RhythmerDesktop_artefacts/Release/VST3/Rhythmer Engines.vst3';res=bundle/'Contents/Resources';shutil.copytree(src/'resources-template',res,dirs_exist_ok=True)
if (res/'Content/Engines/mac').exists():shutil.rmtree(res/'Content/Engines/mac')
(res/'Backend').mkdir(exist_ok=True);shutil.copy2(build/'native/instrument/rhythmer-engines.dll',res/'Backend/rhythmer-engines.dll')
for engine in (build/'native/engines').iterdir():
 if not engine.is_dir():continue
 for f in engine.rglob('*.dll'):
  if 'original-build'in f.parts:continue
  target=res/'Content/Engines/windows'/f.relative_to(build/'native/engines');target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,target)
for binary,relative in recipes.get('runtime_aliases',{}).items():
 target=res/'Content/Engines/windows'/relative.replace('.so','.dll');target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(expand(binary),target)
# Copy runtime data separately from Mac templates; no Mac binary is retained.
for f in (src/'resources-template/Content/Engines/mac').rglob('*'):
 if f.is_file()and f.suffix not in ['.so','.dylib']:
  target=res/'Content/Engines/windows'/f.relative_to(src/'resources-template/Content/Engines/mac');target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,target)
print('WINDOWS VST3 BUILD:',bundle)
