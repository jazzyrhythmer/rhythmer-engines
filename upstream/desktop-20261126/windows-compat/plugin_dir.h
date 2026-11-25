#pragma once
#include <windows.h>
#include <stdio.h>
#include <string.h>
static inline int mpc_plugin_dir(char*buf,size_t n){HMODULE m=NULL;wchar_t path[32768];if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCWSTR)&mpc_plugin_dir,&m))return 0;DWORD len=GetModuleFileNameW(m,path,32768);if(!len||len>=32768)return 0;wchar_t*s=wcsrchr(path,L'\\');if(!s)return 0;*s=0;return WideCharToMultiByte(CP_UTF8,0,path,-1,buf,(int)n,NULL,NULL)>0;}
