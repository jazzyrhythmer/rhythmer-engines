// SPDX-License-Identifier: AGPL-3.0-only
// Original input-driven geometric sculptures. Screen thread only, <=384 faces;
// existing meters/notes supply motion. No audio-thread work or DSP change.
#pragma once
#include <array>
#include <algorithm>
#include <cmath>
inline int performance_kind(const juce::String& name) {
    static const char* names[]{"Sophie","Aphex","Elixir","FM-106","FMini-1","CS-1","OberVave","K-WASUB","Hera","Braids","Plaits","Hortator","MONO2K","Prophet","S1Vave","Dexed","Guru-600","Acid Engine","Rhythmer Kit","Break Engine","Frodhes","Tenebrae"};
    for(unsigned i=0;i<sizeof(names)/sizeof(*names);++i)if(name==names[i])return int(i);
    return 22;
}
template<class Signal>
inline void performance_art(ui::Paint& p,const Signal& s,const juce::String& name,const juce::String& preset,float x,float y,float w,float h,int selected=-1,float opacity=1.0f,bool fit=false) {
    juce::Graphics::ScopedSaveState clip(p.g);p.g.reduceClipRegion(juce::Rectangle<int>(int(x),int(y),int(w),int(h)));
    constexpr float pi=3.14159265359f;const int kind=performance_kind(name);
    bool held=false;for(auto key:s.held_keys)held|=key>=0;
    const float level=std::clamp(s.meters[s.ui.track],0.0f,1.0f);
    const float a=std::clamp(std::sqrt(level)*1.15f+(held?.32f:0.0f),0.0f,1.0f);

    const float pitch=float(s.track().last_note%128)/127.0f,pan=s.track().mix.pan/50.0f;
    const auto motion_name=kind==18 ? preset.upToFirstOccurrenceOf(" / Pad ",false,true).trimCharactersAtEnd(" *") : preset;
    unsigned seed=2166136261u;for(auto c:motion_name)seed=(seed^unsigned(c))*16777619u;
    const float m=float(seed%997)/997.0f;
    // Bounded screen-only gesture memory: sustain evolves from the actual key
    // press, release follows the existing meter. No audio callback or timer.
    struct Motion {uint32_t at=0,down=0;int kind=-1,note=-1;unsigned preset=0;bool held=false;float level=0;};
    static std::array<Motion,16> motion{};
    auto& q=motion[size_t(std::clamp(int(s.ui.track),0,15))];
    const int note=int(s.track().last_note);
    const bool reset=q.kind!=kind||q.preset!=seed||s.now<q.at||s.now-q.at>2000;
    if(reset)q=Motion{s.now,s.now,kind,note,seed,false,0};
    const bool strike=(held&&!q.held)||note!=q.note||level>q.level+.18f;
    if(strike)q.down=s.now;
    const float age=float(s.now-q.down)*.001f;
    const float sustain=held?std::min(age/3.0f,1.0f):0.0f;
    const float impact=a*std::exp(-age*5.0f);
    q.at=s.now;q.note=note;q.held=held;q.level=level;
    const float t=a>.005f?float((s.now/50)%20000)*.05f:0.0f;
    const float register_speed=.62f+pitch*.95f;
    const float phrase=t*register_speed+float(note%12)*pi/6.0f;
    const float breath=a*(.5f+.5f*std::sin(phrase*.83f));
    const float roll=a*(.16f*std::sin(phrase*.57f)+sustain*.18f*std::sin(phrase*1.13f));
    const float expansion=1.0f+impact*.12f+breath*.08f+sustain*.08f;
    const float drive=t*register_speed;

    struct KitHit {uint32_t at=0;float strength=0;int pad=0;};
    struct KitPose {std::array<KitHit,6> hits{};std::array<int,16> previous_held{};uint32_t at=0,epoch=0;unsigned next=0;float tail=0,pitch=0,yaw=0,roll=0,yaw_velocity=0,previous_level=0;};
    static std::array<KitPose,16> kit_poses{};
    auto& kit=kit_poses[size_t(std::clamp(int(s.ui.track),0,15))];
    float kit_x=0,kit_y=0,kit_z=0,kit_yaw=0,kit_pitch=0,kit_roll=0,kit_twist=0;
    if(kind==18){
        if(reset||!kit.at){kit=KitPose{};kit.previous_held.fill(-1);kit.epoch=s.now;}
        float dt=kit.at?std::clamp(float(s.now-kit.at)*.001f,0.0f,.10f):.033f;
        kit.at=s.now;kit.tail=std::max(a,kit.tail*std::exp(-dt*2.5f));

        unsigned added=0;
        const float level_change=level-kit.previous_level;kit.previous_level=level;
        auto kit_hit=[&](int pad){
            const float strength=std::pow(std::clamp(level*1.35f,0.0f,1.0f),.85f);
            kit.hits[kit.next%6]={s.now,a,pad};++kit.next;
            if(strength<.015f)return;
            float direction=(float(pad%4)-1.5f)/1.5f;
            // Near an extreme, the new strike sends the frame back into travel.
            if(kit.yaw*direction>.34f)direction=-direction;
            const float impulse=(.10f+4.60f*std::pow(strength,1.35f))*direction;
            kit.yaw_velocity=std::clamp(kit.yaw_velocity+impulse,-6.0f,6.0f);
        };
        for(int key:s.held_keys){if(key<0)continue;
            bool was_down=false;for(int old:kit.previous_held)was_down|=old==key;
            if(!was_down&&a>.025f){kit_hit((key+128-36)%16);++added;}
        }
        if(!added&&strike&&a>.025f){kit_hit(selected<0?0:selected%16);++added;}
        kit.previous_held.fill(-1);unsigned held_index=0;
        for(int key:s.held_keys)if(key>=0&&held_index<16)kit.previous_held[held_index++]=key;
        float lift=0,depth=0,sway=0,rx=0,ry=0,rz=0,tw=0;
        for(const auto& hit:kit.hits){if(!hit.at)continue;
            float age=float(s.now-hit.at)*.001f;if(age>2.8f)continue;
            float envelope=hit.strength*std::exp(-age*3.7f),angle=hit.pad*2*pi/16;
            lift+=envelope*std::sin(age*12.5f);depth+=envelope*std::sin(age*8.5f+.25f);
            sway+=envelope*std::sin(angle+.4f)*std::sin(age*9.0f);
            rx+=envelope*std::cos(angle+.25f)*std::sin(age*10.0f);
            ry+=envelope*std::sin(angle+.5f)*std::sin(age*8.0f);
            rz+=envelope*std::sin(angle*2+.3f)*std::sin(age*11.0f);
            tw+=envelope*std::cos(angle*2+.7f)*std::sin(age*7.5f);
        }


        // No free-running oscillator: only strikes and real envelope changes add energy.
        if(!added&&std::abs(level_change)>.006f){
            float side=(float((selected<0?0:selected%16)%4)-1.5f)/1.5f;
            kit.yaw_velocity+=level_change*side*.55f;
        }
        const float damping=6.3f-std::min(1.0f,kit.tail)*.7f;
        const float step=dt*.25f;
        for(int i=0;i<4;++i){
            kit.yaw_velocity+=(-46.0f*kit.yaw-damping*kit.yaw_velocity)*step;
            kit.yaw+=kit.yaw_velocity*step;
            if(kit.yaw>.65f){kit.yaw=.65f;kit.yaw_velocity=std::min(0.0f,kit.yaw_velocity)*.35f;}
            if(kit.yaw<-.65f){kit.yaw=-.65f;kit.yaw_velocity=std::max(0.0f,kit.yaw_velocity)*.35f;}
        }
        if(std::abs(kit.yaw)<.00015f&&std::abs(kit.yaw_velocity)<.0005f){kit.yaw=0;kit.yaw_velocity=0;}
        kit_x=0;kit_y=0;kit_z=0;
        kit_pitch=0;kit_roll=0;kit_twist=0;kit_yaw=.14f*std::tanh(kit.yaw/.30f);

    }

    struct V {float x,y,z;};struct Face {std::array<V,4> v;float z,light;};
    std::array<Face,384> faces{};unsigned count=0;
    const float yaw=.35f+std::sin(phrase*.64f)*a*(.82f+sustain*.3f)+pan*.12f;
    const float tilt=.26f+(pitch-.5f)*.22f+std::sin(phrase*.43f)*a*.32f;
    auto rotate=[&](V v){

        if(kind==18){
            const float radius=std::min(1.0f,std::sqrt(v.x*v.x+v.y*v.y));
            const float twist=kit_twist*(.25f+.75f*radius*radius+.20f*v.z);
            float xx=v.x*std::cos(twist)-v.y*std::sin(twist),yy=v.x*std::sin(twist)+v.y*std::cos(twist);
            float zz=v.z+kit_twist*xx*yy*.32f;
            float tx=xx*std::cos(kit_yaw)+zz*std::sin(kit_yaw),tz=-xx*std::sin(kit_yaw)+zz*std::cos(kit_yaw);
            const float tilt=.694f+kit_pitch;
            float ty=yy*std::cos(tilt)-tz*std::sin(tilt),depth=yy*std::sin(tilt)+tz*std::cos(tilt);
            return V{tx*std::cos(kit_roll)-ty*std::sin(kit_roll)+kit_x,
                     tx*std::sin(kit_roll)+ty*std::cos(kit_roll)+kit_y,depth+kit_z};
        }

        // The original mesh remains; twist, orbit and envelope-driven depth
        // make each existing sculpture articulate rather than change identity.
        float twist=a*.16f*std::sin(v.y*1.8f+phrase+sustain*pi);
        float vx=v.x*std::cos(twist)+v.z*std::sin(twist),vz=-v.x*std::sin(twist)+v.z*std::cos(twist);
        float xx=vx*std::cos(yaw)+vz*std::sin(yaw),zz=-vx*std::sin(yaw)+vz*std::cos(yaw);
        float yy=v.y*std::cos(tilt)-zz*std::sin(tilt),depth=v.y*std::sin(tilt)+zz*std::cos(tilt);
        return V{(xx*std::cos(roll)-yy*std::sin(roll))*expansion,(xx*std::sin(roll)+yy*std::cos(roll))*expansion,depth};
    };
    auto add=[&](V v0,V v1,V v2,V v3,float light=.7f){if(count>=faces.size())return;Face f{{rotate(v0),rotate(v1),rotate(v2),rotate(v3)},0,light};for(const auto& v:f.v)f.z+=v.z*.25f;faces[count++]=f;};
    auto box=[&](V c,V d,float light=.7f){std::array<V,8> v;for(int i=0;i<8;++i)v[size_t(i)]={c.x+(i&1?d.x:-d.x),c.y+(i&2?d.y:-d.y),c.z+(i&4?d.z:-d.z)};
        for(auto q:std::array<std::array<int,4>,6>{{{{0,1,3,2}},{{4,6,7,5}},{{0,4,5,1}},{{2,3,7,6}},{{0,2,6,4}},{{1,5,7,3}}}})add(v[size_t(q[0])],v[size_t(q[1])],v[size_t(q[2])],v[size_t(q[3])],light);};
    auto lathe=[&](V c,float radius,float height,int sides,int rings,int shape){
        auto point=[&](int u,int v){float f=float(v)/rings,q=2*pi*u/sides,rr=radius;
            if(shape==0)rr*=.22f+.78f*f*f;
            if(shape==1)rr*=.55f+.45f*std::sin(pi*f);
            if(shape==2)rr*=.72f+.28f*std::cos(f*pi*4+drive*a);
            return V{c.x+rr*std::cos(q),c.y+(f-.5f)*height,c.z+rr*std::sin(q)};};
        for(int j=0;j<rings;++j)for(int i=0;i<sides;++i)add(point(i,j),point(i+1,j),point(i+1,j+1),point(i,j+1),.55f+.2f*std::sin(i*2*pi/sides));};
    auto torus=[&](V c,float radius,float tube,int sides,int rings,float twist){auto point=[&](int i,int j){float q=i*2*pi/sides,r=j*2*pi/rings+q*twist,rr=radius+tube*std::cos(r);return V{c.x+rr*std::cos(q),c.y+rr*std::sin(q),c.z+tube*std::sin(r)};};
        for(int i=0;i<sides;++i)for(int j=0;j<rings;++j)add(point(i,j),point(i+1,j),point(i+1,j+1),point(i,j+1),.62f);};
    auto ribbon=[&](int bands,int steps,float frequency,float amplitude,float phase){for(int j=0;j<bands;++j){auto point=[&](int i,int edge){float u=float(i)/steps,xx=(u-.5f)*3.2f,q=u*frequency+drive*.7f+phase+j*.6f;return V{xx,std::sin(q)*amplitude+(j-(bands-1)*.5f)*.13f+edge*.055f,std::cos(q)*.3f+j*.04f};};for(int i=0;i<steps;++i)add(point(i,0),point(i+1,0),point(i+1,1),point(i,1),.35f+j*.08f);}};
    switch(kind){
    case 0: // Sophie: a struck metal bell, opening with impact.
        lathe({0,0,0},.9f+a*.32f,1.65f,28,10,0);box({0,.85f,0},{.12f,.08f,.12f},1);
        break;
    case 1: // Aphex: fractured, opposing monoliths, with controlled instability.
        for(int i=0;i<11;++i){float u=(i-5)*.27f,q=i*1.7f+drive*1.8f;box({u,std::sin(q)*a*.27f,std::cos(q)*.2f},{.085f,.48f+.28f*std::sin(i+m)+a*.22f,.23f},i%3?.5f:1);}
        break;
    case 2: // Elixir: six sculptural resonators, not meters.
        for(int i=0;i<6;++i)lathe({(i-2.5f)*.52f,0,std::sin(float(i))*.15f},.23f+a*.07f,.9f+.5f*std::sin(i*.8f+m)+a*.3f,12,4,1);
        break;
    case 3: // FM-106: six DCO voices spread into a broad chorus veil.
        ribbon(6,40,2*pi,.48f+a*.2f,.4f);
        break;
    case 4: // FMini-1: three oscillator towers, saw-edged and detuned.
        for(int i=0;i<3;++i)lathe({(i-1)*.91f,0,.12f*i},.34f+a*.09f,1.4f+a*.25f,10+i*2,7,2);
        break;
    case 5: // CS-1: two intertwined ribbons, one for each sound layer.
        ribbon(2,64,3*pi,.65f+a*.12f,pi*.5f);
        break;
    case 6: // OberVave: a six-voice fan opens around an analog core.
        for(int j=0;j<6;++j){float q=(j-2.5f)*.45f;for(int i=0;i<18;++i){float f=float(i)/18,g=float(i+1)/18;auto v=[&](float z,float edge){return V{std::sin(q)*(1+z*.6f),(.5f-z)*1.7f+edge,std::cos(q)*(z-.5f)+std::sin(z*5+drive)*a*.1f};};add(v(f,0),v(g,0),v(g,.09f),v(f,.09f),.45f+j*.07f);}}
        break;
    case 7: // K-WASUB: counterwound bass-pressure rings.
        torus({-.5f,0,0},.58f,.17f+a*.13f,24,7,1);torus({.55f,0,.1f},.58f,.13f+a*.12f,24,7,-1);
        break;
    case 8: // Hera: hard pulse gates with receding chorus repeats.
        for(int i=0;i<7;++i){float r=1.4f-i*.16f,z=-i*.17f,hh=.74f-i*.055f;box({0,hh,z},{r,.028f,.025f});box({0,-hh,z},{r,.028f,.025f});box({-r,0,z},{.028f,hh,.025f});box({r,0,z},{.028f,hh,.025f});}
        break;
    case 9: // Braids: a three-strand woven macro oscillator.
        for(int j=0;j<3;++j)for(int i=0;i<30;++i){float u=float(i)/30,v=float(i+1)/30;auto point=[&](float f,float edge){float q=f*3*pi+j*2*pi/3+drive*.5f;return V{(f-.5f)*3.15f,std::sin(q)*(.46f+a*.15f)+edge,std::cos(q)*.45f};};add(point(u,-.065f),point(v,-.065f),point(v,.065f),point(u,.065f),.45f+j*.2f);}
        break;
    case 10: // Plaits: a constellation of faceted sound models.
        for(int i=0;i<7;++i){float q=i*2*pi/7+drive*.12f,rr=i?1.05f:0,r=.23f+a*.12f;V c{i?std::cos(q)*rr:0,i?std::sin(q)*.5f:0,std::sin(q)*.35f};std::array<V,6> v{{{c.x-r,c.y,c.z},{c.x,c.y,c.z-r},{c.x+r,c.y,c.z},{c.x,c.y,c.z+r},{c.x,c.y+r*1.8f,c.z},{c.x,c.y-r*1.8f,c.z}}};for(int n=0;n<4;++n){add(v[size_t(n)],v[size_t((n+1)%4)],v[4],v[4],.85f);add(v[size_t(n)],v[5],v[size_t((n+1)%4)],v[size_t((n+1)%4)],.4f);}}
        break;
    case 11: // Hortator: a machined drum cam, with rhythm-shaped teeth.
        for(int i=0;i<48;++i){auto point=[&](int n,float z){float q=n*2*pi/48+drive*.25f,r=(n%3==0?1.0f:.86f)+a*.12f*std::sin(n+drive*3);return V{std::cos(q)*r*1.3f,std::sin(q)*r*.78f,z};};add(point(i,-.2f),point(i+1,-.2f),point(i+1,.2f),point(i,.2f),.7f);add({0,0,.2f},point(i,.2f),point(i+1,.2f),{0,0,.2f},i%3?.35f:.75f);}
        break;
    case 12: // MONO2K: an extruded saw sculpture, sharp and monophonic.
        for(int i=0;i<9;++i){float xx=(i-4)*.34f,hh=.5f+a*.3f*std::sin(i+drive*2);add({xx-.15f,-.65f,-.2f},{xx+.15f,-.65f,-.2f},{xx+.15f,hh,.2f},{xx-.15f,-.65f,.2f},.5f+i*.045f);box({xx,-.67f,0},{.15f,.03f,.2f},.4f);}
        break;
    case 13: // Prophet: five stacked analog voice pillars.
        for(int i=0;i<5;++i){float hh=.48f+std::sin(i*.8f+m)*.22f+a*.15f;box({(i-2)*.6f,0,0},{.19f,hh,.25f},.5f+i*.09f);box({(i-2)*.6f,hh+.12f,0},{.22f,.04f,.28f},.9f);}
        break;
    case 14: // S1Vave: a broad sculpted wave surface.
        ribbon(10,30,2*pi,.38f+a*.28f,m*pi);
        break;
    case 15: // Dexed: six operator pipes, with visible modulation bridges.
        for(int i=0;i<6;++i){float hh=.8f+.3f*std::sin(i*1.7f+m)+a*.2f;lathe({(i-2.5f)*.52f,0,0},.16f,hh,12,4,2);if(i<5)box({(i-2)*.52f,.25f*(i%2?1:-1),-.1f},{.26f,.025f,.025f},.9f);}
        break;
    case 16: // Guru-600: an analog iris, breathing with the envelope.
        torus({0,0,0},.56f+a*.25f,.29f,32,10,3);
        break;
    case 17: // Acid: a resonant, twisting filter tunnel.
        for(int j=0;j<10;++j)for(int i=0;i<24;++i){auto v=[&](int z,int n){float f=float(z)/10,q=n*2*pi/24+f*(.8f+a*.8f)+drive*.25f,r=1.3f-f*.95f;return V{std::cos(q)*r,std::sin(q)*r*.7f,-f*1.7f};};add(v(j,i),v(j,i+1),v(j+1,i+1),v(j+1,i),j%2?.36f:.8f);}
        break;
    case 18: { // Kit: struck drum skin, rebound, angular pressure waves.
        const int pad=selected<0?0:selected%16,voice=pad%4;
        const float hit_angle=pad*2*pi/16;
        const float damping=voice==0?4.5f:voice==1?8.0f:voice==3?14.0f:6.0f;
        const float pulse=a*std::exp(-age*damping);
        auto skin=[&](int radial,int segment){
            const float r=float(radial)/7.0f,q=segment*2*pi/24;
            const float xx=r*std::cos(q),yy=r*std::sin(q);
            float deformation=0;
            for(const auto& hit:kit.hits){if(!hit.at)continue;
                float age=float(s.now-hit.at)*.001f;if(age>2.4f)continue;
                int voice=hit.pad%4;float angle=hit.pad*2*pi/16;
                float dx=xx-std::cos(angle)*.24f,dy=yy-std::sin(angle)*.20f,rr=std::sqrt(dx*dx+dy*dy);
                float frequency=voice==0?9.0f:voice==1?18.0f:voice==3?27.0f:12.0f;
                float damping=voice==0?4.5f:voice==1?8.0f:voice==3?14.0f:6.0f;
                deformation+=-.09f*hit.strength*std::exp(-age*24.0f)*std::exp(-rr*rr*10.0f)
                    +.055f*hit.strength*std::exp(-age*damping)*std::cos(rr*frequency-age*42.0f)*std::exp(-rr*.75f);
            }
            return V{xx,yy,.14f*std::tanh(deformation/.14f)*std::sqrt(std::max(0.0f,1.0f-r))};
        };
        // Clamped rim and a shallow shell make the surface read as percussion.
        for(int i=0;i<24;++i){
            const float q=i*2*pi/24,qq=(i+1)*2*pi/24;
            auto ring=[&](float r,float angle,float z){return V{r*std::cos(angle),r*std::sin(angle),z};};
            add(ring(1.0f,q,0),ring(1.08f,q,0),ring(1.08f,qq,0),ring(1.0f,qq,0),.76f);
            add(ring(1.08f,q,0),ring(1.08f,q,-.16f),ring(1.08f,qq,-.16f),ring(1.08f,qq,0),.28f);
        }
        for(int j=0;j<7;++j)for(int i=0;i<24;++i)
            add(skin(j,i),skin(j+1,i),skin(j+1,i+1),skin(j,i+1),.30f+.26f*pulse+.16f*float((i+pad)%3==0));
        // Expanding polygon rings: pressure leaving the struck surface.
        for(unsigned n=0;n<3&&n<kit.next;++n){
            const auto& hit=kit.hits[(kit.next-1-n)%6];float age=float(s.now-hit.at)*.001f;
            if(age>=.55f||hit.strength<.025f)continue;
            const float pulse=hit.strength*std::exp(-age*6.0f);
            for(int j=0;j<2;++j){
                const float radius=1.12f+age*1.30f+j*.19f;if(radius>1.90f)continue;
                const float thickness=.014f+.018f*pulse;
                for(int i=0;i<16;++i){
                    auto v=[&](int n,float r){float q=n*2*pi/16;return V{r*std::cos(q),r*std::sin(q),.015f};};
                    add(v(i,radius),v(i+1,radius),v(i+1,radius+thickness),v(i,radius+thickness),.14f+pulse*.65f-j*.12f-n*.08f);
                }
            }
        }
        // Short angular attack fragments; no constant orbit or idle rotation.
        if(impact>.10f&&age<.16f)for(int i=0;i<8;++i){
            float q=hit_angle+i*2*pi/8,r=1.16f+age*1.5f,tip=r+.09f+impact*.18f;
            V a0{r*std::cos(q-.035f),r*std::sin(q-.035f),.05f};
            V b0{tip*std::cos(q),tip*std::sin(q),.05f+impact*.12f};
            V c0{r*std::cos(q+.035f),r*std::sin(q+.035f),.05f};
            add(a0,b0,c0,c0,.9f);
        }
        break;
    }
    case 19: // Break: a segmented tape loop, separating and returning.
        for(int i=0;i<16;++i){float q=i*2*pi/16+drive*.18f,r=1.0f+a*.18f*std::sin(i*.7f+drive*1.8f);box({std::cos(q)*r*1.28f,std::sin(q)*r*.63f,std::sin(q*2)*a*.3f},{.12f,.12f,.12f},i==selected%16?1:.55f);}
        break;
    case 20: // Frodhes: struck tines above a solid resonant soundboard.
        box({0,-.61f,0},{1.46f,.055f,.28f},.35f);
        for(int i=0;i<12;++i){float u=float(i)/11.0f,xx=(u-.5f)*2.65f;
            float height=.31f+.43f*(1-u),bend=a*.12f*std::sin(phrase*2.1f+i*.61f)*std::exp(-age*.8f);
            box({xx,-.56f+height,bend},{.052f,height,.065f},.4f+.35f*u);
            box({xx,-.48f,.19f},{.075f,.08f,.075f},.78f);
        }
        break;
    case 21: // Tenebrae: layered, opening folds; a heavy geometric chamber.
        for(int i=0;i<9;++i){float z=(float(i)-4)*.18f,q=float(i)*.31f;
            float fold=.25f+.22f*std::sin(q)+a*.16f*std::sin(phrase*.7f+q);
            float spread=1.12f+a*.10f*std::sin(phrase+q);
            V l{-spread,-.46f,z},m0{0,-fold,z-.16f},r0{spread,-.46f,z};
            V lt{-spread,.46f,z},mt{0,fold,z-.16f},rt{spread,.46f,z};
            add(l,m0,mt,lt,.3f+float(i)*.055f);add(m0,r0,rt,mt,.78f-float(i)*.045f);
        }
        break;
    default:lathe({0,0,0},.9f,1.5f,24,8,1);
        break;
    }
    // Boot miniatures fit the complete projected silhouette, with a three-
    // pixel inset for antialiasing. Instrument pages retain their original framing.
    auto project=[&](V v){float perspective=3.6f/(3.6f-v.z*.35f);return V{v.x*w*(kind==18?.217f:.29f)*perspective,-v.y*h*(kind==18?.235f:.48f)*perspective,0};};
    float fit_scale=1,centre_x=0,centre_y=0;
    if((fit||kind==20||kind==21)&&count&&kind!=18){float left=1e6f,right=-1e6f,top=1e6f,bottom=-1e6f;
        for(unsigned i=0;i<count;++i)for(const auto& v:faces[i].v){auto q=project(v);left=std::min(left,q.x);right=std::max(right,q.x);top=std::min(top,q.y);bottom=std::max(bottom,q.y);}
        fit_scale=std::min((w-6)/std::max(1.0f,right-left),(h-6)/std::max(1.0f,bottom-top));centre_x=(left+right)*.5f;centre_y=(top+bottom)*.5f;
    }
    std::sort(faces.begin(),faces.begin()+count,[](const Face& a,const Face& b){return a.z<b.z;});
    for(unsigned i=0;i<count;++i){const auto& f=faces[i];juce::Path shape;float light=std::clamp(f.light*.55f+.15f+f.z*.12f,.12f,.9f);
        for(unsigned n=0;n<4;++n){auto v=project(f.v[n]);float xx=x+w*.5f+(v.x-centre_x)*fit_scale,yy=y+h*.5f+(v.y-centre_y)*fit_scale;if(n)shape.lineTo(xx,yy);else shape.startNewSubPath(xx,yy);}shape.closeSubPath();
        p.g.setColour(juce::Colours::white.withAlpha(light*opacity));p.g.fillPath(shape);p.g.setColour(juce::Colours::white.withAlpha(std::min(1.0f,light+.27f)*opacity));p.g.strokePath(shape,juce::PathStrokeType(.6f));
    }
}
