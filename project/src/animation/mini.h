// SPDX-License-Identifier: AGPL-3.0-only
// Small freestanding drawing adapter for the unchanged Sample OS artwork.
#pragma once
using size_t=__SIZE_TYPE__;
using uint32_t=unsigned int;
extern "C" {
#ifdef __wasm__
__attribute__((import_module("env"),import_name("sin"))) float art_sin(float);
__attribute__((import_module("env"),import_name("cos"))) float art_cos(float);
__attribute__((import_module("env"),import_name("exp"))) float art_exp(float);
__attribute__((import_module("env"),import_name("pow"))) float art_pow(float,float);
__attribute__((import_module("env"),import_name("tanh"))) float art_tanh(float);
#else
float art_sin(float x){return __builtin_sinf(x);}
float art_cos(float x){return __builtin_cosf(x);}
float art_exp(float x){return __builtin_expf(x);}
float art_pow(float x,float y){return __builtin_powf(x,y);}
float art_tanh(float x){return __builtin_tanhf(x);}
#endif
}
namespace std {
template<class T,size_t N>struct array {T data[N];T& operator[](size_t i){return data[i];}const T& operator[](size_t i)const{return data[i];}T* begin(){return data;}const T* begin()const{return data;}T* end(){return data+N;}const T* end()const{return data+N;}constexpr size_t size()const{return N;}void fill(const T& v){for(size_t i=0;i<N;i++)data[i]=v;}};
template<class T> T min(T a,T b){return a<b?a:b;}
template<class T> T max(T a,T b){return a>b?a:b;}
template<class T> T clamp(T a,T lo,T hi){return min(hi,max(lo,a));}
template<class T,class F>void sort(T* first,T* last,F less){for(T* p=first+1;p<last;p++){T v=*p;T* q=p;while(q>first&&less(v,q[-1])){*q=q[-1];--q;}*q=v;}}
inline float sin(float v){return art_sin(v);}inline float cos(float v){return art_cos(v);}inline float exp(float v){return art_exp(v);}inline float pow(float v,float p){return art_pow(v,p);}inline float tanh(float v){return art_tanh(v);}inline float sqrt(float v){return __builtin_sqrtf(v);}inline float abs(float v){return __builtin_fabsf(v);}
}
struct Polygon {float xy[8];float fill,stroke;};
inline Polygon polygons[384];inline unsigned polygon_count=0;
namespace juce {
struct String {
 const char* p;unsigned length;
 String(const char* s=""):p(s),length(0){while(s[length])++length;}
 String(const char* s,unsigned n):p(s),length(n){}
 bool operator==(const char* s)const{unsigned n=0;while(s[n])++n;if(n!=length)return false;for(unsigned i=0;i<n;i++)if(p[i]!=s[i])return false;return true;}
 String upToFirstOccurrenceOf(const char* marker,bool,bool)const{unsigned n=0;while(marker[n])++n;for(unsigned i=0;i+n<=length;i++){unsigned j=0;while(j<n&&p[i+j]==marker[j])++j;if(j==n)return {p,i};}return *this;}
 String trimCharactersAtEnd(const char* set)const{unsigned n=length;while(n){bool match=false;for(unsigned j=0;set[j];j++)match|=p[n-1]==set[j];if(!match)break;--n;}return {p,n};}
 struct Iterator{const char* p;unsigned operator*()const{unsigned char c=*p;if(c<128)return c;unsigned v=c&((c<224)?31:(c<240)?15:7);for(unsigned j=1;j<((c<224)?2u:(c<240)?3u:4u);j++)v=(v<<6)|(static_cast<unsigned char>(p[j])&63);return v;}Iterator& operator++(){unsigned char c=*p;p+=c<128?1:c<224?2:c<240?3:4;return *this;}bool operator!=(const Iterator& o)const{return p!=o.p;}};
 Iterator begin()const{return {p};}Iterator end()const{return {p+length};}
};
template<class T>struct Rectangle{Rectangle(T,T,T,T){}};
struct Colour {float a=1;Colour withAlpha(float v)const{return {v};}};
namespace Colours {inline Colour white{};}
struct Path {float xy[8]{};unsigned n=0;void startNewSubPath(float x,float y){n=0;lineTo(x,y);}void lineTo(float x,float y){if(n<4){xy[n*2]=x;xy[n*2+1]=y;++n;}}void closeSubPath(){}};
struct PathStrokeType {explicit PathStrokeType(float){}};
struct Graphics {
 struct ScopedSaveState {explicit ScopedSaveState(Graphics&){}};
 float alpha=1;void reduceClipRegion(Rectangle<int>){}void setColour(Colour c){alpha=c.a;}
 void fillPath(const Path& p){if(polygon_count>=384)return;auto& out=polygons[polygon_count++];for(unsigned i=0;i<8;i++)out.xy[i]=p.xy[i];out.fill=alpha;out.stroke=alpha;}
 void strokePath(const Path&,PathStrokeType){if(polygon_count)polygons[polygon_count-1].stroke=alpha;}
};
}
namespace ui {struct Paint{juce::Graphics g;};}
struct ArtSignal {
 struct UI {int track=0;}ui;
 struct Track{int last_note=60;struct Mix{int pan=0;}mix;}part;
 std::array<int,16> held_keys{};std::array<float,16> meters{};uint32_t now=0;
 const Track& track()const{return part;}
};
