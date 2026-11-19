/* SPDX-License-Identifier: GPL-3.0-only
 * Narrow native boundary for original FM-1 synthesis units. No device/CPU,
 * storage, firmware UI, updater or firmware sequencer is executed here.
 * The existing Rhythmer sequencer/host drives this API through the common loader.
 * Each isolated mapping owns the original C globals and voice/FX memory.
 */
#include "move-sdk/plugin_api_v1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int active;
static const host_api_v1_t *native_host;
static void *native_create(const char *dir,const char *json) {
    (void)dir;(void)json;if(active)return NULL;active=1;native_init();
    if(native_count()<1||native_count()>NATIVE_MAX_PARAMS){active=0;return NULL;}return &active;
}
static void native_destroy(void *p){(void)p;active=0;}
static void native_midi(void *p,const uint8_t *m,int n,int source){
    (void)p;(void)source;
    if(n>=3)native_note(m);
    else if(n==2&&(m[0]&0xf0)==0xd0){uint8_t message[3]={m[0],m[1],0};native_note(message);}
}
static void native_set(void *p,const char *k,const char *v) {
    (void)p;
    if(!strcmp(k,"state")) {
#ifdef NATIVE_CUSTOM_STATE
        native_state_set(v);return;
#else
        if(strncmp(v,"FM1S1:",6))return;const char *s=v+6;int values[NATIVE_MAX_PARAMS];
        int count=0;
        for(int i=0;i<native_count()&&*s;++i) {
            char *end;long n=strtol(s,&end,10);
            if(end==s||n<native_min(i)||n>native_max(i)||(*end!=','&&*end))return;
            values[i]=(int)n;s=*end?end+1:end;++count;
        }
#ifdef NATIVE_MIN_STATE_PARAMS
        if(*s||count<NATIVE_MIN_STATE_PARAMS)return;
#else
        if(*s||count!=native_count())return;
#endif
        for(int i=0;i<count;++i)native_control(i,values[i]);
#ifdef NATIVE_SYNC
        native_sync();
#endif
        return;
#endif
    }
    if(k[0]!='p')return;char *end;long i=strtol(k+1,&end,10);
    if(*end||i<0||i>=native_count())return;int n=atoi(v);
    if(n<native_min(i))n=native_min(i);if(n>native_max(i))n=native_max(i);
    native_control((int)i,n);
#ifdef NATIVE_SYNC
    native_sync();
#endif
}
static int native_get(void *p,const char *key,char *out,int capacity) {
    (void)p;if(capacity<=0)return 0;out[0]=0;
    if(!strcmp(key,"state")) {
#ifdef NATIVE_CUSTOM_STATE
        return native_state_get(out,capacity);
#else
        int used=snprintf(out,(size_t)capacity,"FM1S1:");if(used<0||used>=capacity)return 0;
        for(int i=0;i<native_count();++i) {
            int n=snprintf(out+used,(size_t)(capacity-used),"%d%s",native_value(i),i+1<native_count()?",":"");
            if(n<0||n>=capacity-used)return 0;used+=n;
        }
        return used;
#endif
    }
    if(!strcmp(key,"chain_params")) {
        int used=snprintf(out,(size_t)capacity,"[");if(used<0||used>=capacity)return 0;
        for(int i=0;i<native_count();++i) {
            char name[192];unsigned at=0;
            for(const unsigned char *s=(const unsigned char*)native_name(i);*s&&at<sizeof(name)-2;++s) {
                if(*s=='"'||*s=='\\')name[at++]='\\';if(*s>=32)name[at++]=(char)*s;
            }name[at]=0;
            int n=snprintf(out+used,(size_t)(capacity-used),"%s{\"key\":\"p%d\",\"name\":\"%s\",\"dynamic_name\":true,\"type\":\"int\",\"display\":\"int\",\"min\":%d,\"max\":%d,\"default\":%d}",
                i?",":"",i,name,native_min(i),native_max(i),native_value(i));
            if(n<0||n>=capacity-used)return 0;used+=n;
        }
        if(used+2>capacity)return 0;out[used++]=']';out[used]=0;return used;
    }
    if(key[0]=='p') {
        char *end;long i=strtol(key+1,&end,10);
        if(i>=0&&i<native_count()) {
#ifdef NATIVE_DISPLAY
            if(!strcmp(end,"_display"))return native_display((int)i,out,capacity);
#endif
            if(!strcmp(end,"_name"))return snprintf(out,(size_t)capacity,"%s",native_name((int)i));
            if(!*end)return snprintf(out,(size_t)capacity,"%d",native_value((int)i));
        }
    }
    return 0;
}
static void native_render(void *p,int16_t *out,int frames){(void)p;
#ifdef NATIVE_TEMPO
    if(native_host&&native_host->get_bpm){float bpm=native_host->get_bpm();if(bpm>=20&&bpm<=999)native_tempo(bpm);}
#endif
    native_audio(out,frames);}
static plugin_api_v2_t facade={.api_version=2,.create_instance=native_create,.destroy_instance=native_destroy,
    .on_midi=native_midi,.set_param=native_set,.get_param=native_get,.render_block=native_render};
__attribute__((visibility("default"))) plugin_api_v2_t *move_plugin_init_v2(const host_api_v1_t *host){native_host=host;return &facade;}
