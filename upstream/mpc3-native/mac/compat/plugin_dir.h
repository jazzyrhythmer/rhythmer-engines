#pragma once
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
static inline int mpc_plugin_dir(char *buf,size_t n){Dl_info i;if(!dladdr((void*)&mpc_plugin_dir,&i)||!i.dli_fname)return 0;const char*s=strrchr(i.dli_fname,'/');if(!s||(size_t)(s-i.dli_fname)>=n)return 0;memcpy(buf,i.dli_fname,s-i.dli_fname);buf[s-i.dli_fname]=0;return 1;}
