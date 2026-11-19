#!/usr/bin/env python3
"""Adapt state ownership and expressive motion; retain original mesh and shading."""
from pathlib import Path
import hashlib,re,sys
root=Path(__file__).resolve().parents[1]
original=root/'src/animation/performance_art.inc'
source=original.read_text();definitions=[]
# The isolated freestanding shims already provide these types/functions.
source=re.sub(r"^#include <(?:array|algorithm|cmath)>\n", "", source, flags=re.M)
for name in ('Motion','KitHit','KitPose'):
    matches=re.findall(r'^    struct '+name+r' .*?;\n',source,re.M)
    assert len(matches)==1,name
    definitions.append(matches[0]);source=source.replace(matches[0],'')
for declaration in ('static std::array<Motion,16> motion{};','static std::array<KitPose,16> kit_poses{};'):
    assert source.count(declaration)==1
    source=source.replace(declaration,'')
old='float((s.now/50)%20000)*.05f'
assert source.count(old)==1
source=source.replace(old,'motion_clock')
source=source.replace('const float a=std::clamp(std::sqrt(level)*1.15f+(held?.32f:0.0f),0.0f,1.0f);',"""const float dt=motion_at&&s.now>=motion_at?std::clamp(float(s.now-motion_at)*.001f,0.0f,.05f):.016f;
    motion_at=s.now;
    const float target=std::clamp(std::sqrt(level)*1.15f+(held?.32f:0.0f),0.0f,1.0f);
    motion_energy+=(target-motion_energy)*(1.0f-std::exp(-dt/(target>motion_energy?.035f:.38f)));
    const float a=motion_energy;
    motion_clock+=dt*(.38f+.62f*a);""")
source=source.replace('const float sustain=held?std::min(age/3.0f,1.0f):0.0f;',"""const float sustain_target=held?std::min(age/2.2f,1.0f):0.0f;
    sustain_energy+=(sustain_target-sustain_energy)*(1.0f-std::exp(-dt/.35f));
    const float sustain=sustain_energy;""")
source=source.replace('const float t=a>.005f?motion_clock:0.0f;', 'const float t=motion_clock;')
source=source.replace('impact*.12f+breath*.08f+sustain*.08f','impact*.18f+breath*.10f+sustain*.11f')
source=source.replace('kit_yaw=.14f*std::tanh','kit_yaw=.22f*std::tanh')
output=Path(sys.argv[1]);output.parent.mkdir(parents=True,exist_ok=True)
output.write_text('// Original geometry SHA256: '+hashlib.sha256(original.read_bytes()).hexdigest()+'\n'
                  +'uint32_t motion_at=0;float motion_energy=0,motion_clock=0,sustain_energy=0;\n'+''.join(definitions)+'\nstd::array<Motion,16> motion{};\nstd::array<KitPose,16> kit_poses{};\n'+source)
