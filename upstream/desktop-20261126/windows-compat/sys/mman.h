#pragma once
#include <windows.h>
#include <io.h>
#include <stdint.h>
#include <sys/types.h>
#define PROT_READ 1
#define MAP_SHARED 1
#define MAP_FAILED ((void*)-1)
static inline void*mmap(void*address,size_t count,int protection,int flags,int fd,off_t offset){
 (void)address;if(protection!=PROT_READ||flags!=MAP_SHARED||offset!=0)return MAP_FAILED;
 HANDLE file=(HANDLE)_get_osfhandle(fd);if(file==INVALID_HANDLE_VALUE)return MAP_FAILED;
 HANDLE mapping=CreateFileMappingW(file,NULL,PAGE_READONLY,0,0,NULL);if(!mapping)return MAP_FAILED;
 void*view=MapViewOfFile(mapping,FILE_MAP_READ,0,0,count);CloseHandle(mapping);return view?view:MAP_FAILED;
}
static inline int munmap(void*view,size_t count){(void)count;return UnmapViewOfFile(view)?0:-1;}
