/* SPDX-License-Identifier: MIT
 * Native Open MPC engine glue for original, independently loaded Move API-v2 DSPs.
 * Loading/state/lifecycle use the existing wrapper and Rhythmer rack staging.
 * No new host audio lock, allocation, file access or engine DSP is added here. */
#define _GNU_SOURCE
#include "move-sdk/plugin_api_v1.h"
#include "engine.h"
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef MOVE_ASYNC_READY
#define MOVE_ASYNC_READY 0
#endif
#ifndef FM1_ISOLATED
#define FM1_ISOLATED 0
#endif

typedef struct {void *library;plugin_api_v2_t *api;void *dsp;int fd;int edit_pad;wchar_t temporary[32768];} module_t;
static uint32_t tempo_bits=0x42f00000u,transport;
static uint64_t beat_bits=0xbff0000000000000ull;
static uint8_t mailbox[4096];
static void no_log(const char *s){(void)s;}
static int no_midi(const uint8_t *m,int n){(void)m;(void)n;return 0;}
static float host_bpm(void){uint32_t n=__atomic_load_n(&tempo_bits,__ATOMIC_RELAXED);float x;memcpy(&x,&n,4);return x;}
static double host_beat(void){uint64_t n=__atomic_load_n(&beat_bits,__ATOMIC_RELAXED);double x;memcpy(&x,&n,8);return x;}
static int clock_status(void){return __atomic_load_n(&transport,__ATOMIC_RELAXED)?MOVE_CLOCK_STATUS_RUNNING:MOVE_CLOCK_STATUS_STOPPED;}
static const host_api_v1_t host={.api_version=1,.sample_rate=44100,.frames_per_block=128,
    .mapped_memory=mailbox,.audio_out_offset=MOVE_AUDIO_OUT_OFFSET,.audio_in_offset=MOVE_AUDIO_IN_OFFSET,
    .log=no_log,.midi_send_internal=no_midi,.midi_send_external=no_midi,.get_clock_status=clock_status,
    .get_bpm=host_bpm,.get_beat_position=host_beat};
static void *create(const char *dir){
    if(!dir)return NULL;
    char path[2048];
    if(snprintf(path,sizeof path,"%s/dsp.dll",dir)>=(int)sizeof path)return NULL;
    wchar_t wide[32768];if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wide,32768))return NULL;
    module_t*m=calloc(1,sizeof*m);if(!m)return NULL;m->fd=-1;m->edit_pad=1;
    const wchar_t*load=wide;
    if(FM1_ISOLATED){
        WIN32_FILE_ATTRIBUTE_DATA info;
        if(!GetFileAttributesExW(wide,GetFileExInfoStandard,&info)||info.nFileSizeHigh||info.nFileSizeLow>64*1024*1024)goto fail;
        wchar_t temp[32768];DWORD count=GetTempPathW(32768,temp);
        if(!count||count>=32768||!GetTempFileNameW(temp,L"rhe",0,m->temporary))goto fail;
        if(!CopyFileW(wide,m->temporary,FALSE))goto fail;load=m->temporary;
    }
    m->library=LoadLibraryExW(load,NULL,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);if(!m->library)goto fail;
    move_plugin_init_v2_fn init=(move_plugin_init_v2_fn)GetProcAddress((HMODULE)m->library,MOVE_PLUGIN_INIT_V2_SYMBOL);
    if(init)m->api=init(&host);
    if(!m->api||m->api->api_version!=2||!m->api->create_instance||!m->api->destroy_instance||!m->api->render_block)goto fail;
    m->dsp=m->api->create_instance(dir,NULL);if(!m->dsp)goto fail;
    /* create is called by the existing rack's loading thread, never its audio callback. */
    if(MOVE_ASYNC_READY&&m->api->get_param) {
        int ready=0;
        for(unsigned n=0;n<1500;n++) {
            char text[32]={0};m->api->get_param(m->dsp,"ready",text,sizeof text);
            ready=atoi(text);if(ready)break;Sleep(10);
        }
        if(ready!=1){m->api->destroy_instance(m->dsp);goto fail;}
    }
    return m;
fail:if(m->library)FreeLibrary((HMODULE)m->library);if(m->temporary[0])DeleteFileW(m->temporary);free(m);return NULL;
}
static void destroy(void *i){module_t *m=i;m->api->destroy_instance(m->dsp);FreeLibrary((HMODULE)m->library);if(m->temporary[0])DeleteFileW(m->temporary);free(m);}
static void midi(void *i,const uint8_t *p,int n){module_t *m=i;if(m->api->on_midi)m->api->on_midi(m->dsp,p,n,MOVE_MIDI_SOURCE_EXTERNAL);}
static void set(void *i,const char *k,const char *v){
    module_t *m=i;
    if(m->api->set_param)m->api->set_param(m->dsp,k,v);
}
static int get(void *i,const char *k,char *b,int n){module_t *m=i;
    return m->api->get_param?m->api->get_param(m->dsp,k,b,n):0;}
static void render(void *i,int16_t *out,int n){module_t *m=i;m->api->render_block(m->dsp,out,n);}
static void song_pos(void *i,double ppq,double bpm,int playing,int frames){
    (void)i;(void)frames;float tempo=bpm>0?(float)bpm:120.0f;uint32_t t;memcpy(&t,&tempo,4);
    __atomic_store_n(&tempo_bits,t,__ATOMIC_RELAXED);__atomic_store_n(&transport,playing!=0,__ATOMIC_RELAXED);
    double p=playing?ppq:-1;uint64_t b;memcpy(&b,&p,8);__atomic_store_n(&beat_bits,b,__ATOMIC_RELAXED);
}
static const mpc_engine_t engine={.create=create,.destroy=destroy,.midi=midi,.set_param=set,.get_param=get,
    .render=render,.song_pos=song_pos};
const mpc_engine_t *mpc_engine(void){return &engine;}
