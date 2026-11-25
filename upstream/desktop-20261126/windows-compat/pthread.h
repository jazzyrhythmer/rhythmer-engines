#pragma once
#include <windows.h>
typedef CRITICAL_SECTION pthread_mutex_t;typedef int pthread_mutexattr_t;
#define PTHREAD_MUTEX_RECURSIVE 0
#define PTHREAD_PRIO_INHERIT 0
static inline int pthread_mutexattr_init(pthread_mutexattr_t*p){*p=0;return 0;}
static inline int pthread_mutexattr_settype(pthread_mutexattr_t*p,int v){(void)p;(void)v;return 0;}
static inline int pthread_mutexattr_setprotocol(pthread_mutexattr_t*p,int v){(void)p;(void)v;return 0;}
static inline int pthread_mutexattr_destroy(pthread_mutexattr_t*p){(void)p;return 0;}
static inline int pthread_mutex_init(pthread_mutex_t*p,const pthread_mutexattr_t*a){(void)a;InitializeCriticalSection(p);return 0;}
static inline int pthread_mutex_destroy(pthread_mutex_t*p){DeleteCriticalSection(p);return 0;}
static inline int pthread_mutex_lock(pthread_mutex_t*p){EnterCriticalSection(p);return 0;}
static inline int pthread_mutex_unlock(pthread_mutex_t*p){LeaveCriticalSection(p);return 0;}

#include <stdlib.h>
#include <sched.h>
typedef HANDLE pthread_t;
struct rhy_thread_start { void*(*fn)(void*);void*arg; };
static DWORD WINAPI rhy_thread_entry(void*p){struct rhy_thread_start*j=p;void*(*fn)(void*)=j->fn;void*arg=j->arg;free(j);fn(arg);return 0;}
static inline int pthread_create(pthread_t*t,const void*attr,void*(*fn)(void*),void*arg){(void)attr;struct rhy_thread_start*j=malloc(sizeof*j);if(!j)return 12;j->fn=fn;j->arg=arg;*t=CreateThread(NULL,0,rhy_thread_entry,j,0,NULL);if(!*t){free(j);return 11;}return 0;}
static inline int pthread_join(pthread_t t,void**result){if(WaitForSingleObject(t,INFINITE)!=WAIT_OBJECT_0)return 22;if(result)*result=NULL;CloseHandle(t);return 0;}
static inline pthread_t pthread_self(void){return GetCurrentThread();}
static inline int pthread_setschedparam(pthread_t t,int policy,const struct sched_param*p){(void)policy;return SetThreadPriority(t,p&&p->sched_priority>0?THREAD_PRIORITY_ABOVE_NORMAL:THREAD_PRIORITY_NORMAL)?0:22;}

#include <errno.h>
#include <time.h>
typedef INIT_ONCE pthread_once_t;
#define PTHREAD_ONCE_INIT INIT_ONCE_STATIC_INIT
static BOOL CALLBACK rhy_once(PINIT_ONCE once,PVOID fn,PVOID*context){(void)once;(void)context;((void(*)(void))fn)();return TRUE;}
static inline int pthread_once(pthread_once_t*once,void(*fn)(void)){return InitOnceExecuteOnce(once,rhy_once,(PVOID)fn,NULL)?0:EINVAL;}
static inline int pthread_mutex_trylock(pthread_mutex_t*m){return TryEnterCriticalSection(m)?0:EBUSY;}
typedef CONDITION_VARIABLE pthread_cond_t;
static inline int pthread_cond_init(pthread_cond_t*c,const void*attr){(void)attr;InitializeConditionVariable(c);return 0;}
static inline int pthread_cond_destroy(pthread_cond_t*c){(void)c;return 0;}
static inline int pthread_cond_signal(pthread_cond_t*c){WakeConditionVariable(c);return 0;}
static inline int pthread_cond_timedwait(pthread_cond_t*c,pthread_mutex_t*m,const struct timespec*until){FILETIME ft;ULARGE_INTEGER now;GetSystemTimeAsFileTime(&ft);now.LowPart=ft.dwLowDateTime;now.HighPart=ft.dwHighDateTime;unsigned long long end=(unsigned long long)until->tv_sec*10000000ull+until->tv_nsec/100+116444736000000000ull;DWORD ms=end>now.QuadPart?(DWORD)((end-now.QuadPart+9999)/10000):0;if(SleepConditionVariableCS(c,m,ms))return 0;return GetLastError()==ERROR_TIMEOUT?ETIMEDOUT:EINVAL;}
typedef struct {int priority;} pthread_attr_t;
#ifndef PTHREAD_EXPLICIT_SCHED
#define PTHREAD_EXPLICIT_SCHED 1
#endif
static inline int pthread_attr_init(pthread_attr_t*a){a->priority=0;return 0;}
static inline int pthread_attr_destroy(pthread_attr_t*a){(void)a;return 0;}
static inline int pthread_attr_setinheritsched(pthread_attr_t*a,int value){(void)a;(void)value;return 0;}
static inline int pthread_attr_setschedpolicy(pthread_attr_t*a,int policy){(void)a;return policy==SCHED_OTHER?0:EINVAL;}
static inline int pthread_attr_setschedparam(pthread_attr_t*a,const struct sched_param*p){a->priority=p->sched_priority;return p->sched_priority==0?0:EINVAL;}
static inline int pthread_setname_np(pthread_t t,const char*name){typedef HRESULT(WINAPI*SetDescription)(HANDLE,PCWSTR);SetDescription f=(SetDescription)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"SetThreadDescription");if(!f)return 0;wchar_t wide[64];if(!MultiByteToWideChar(CP_UTF8,0,name,-1,wide,64))return EINVAL;return SUCCEEDED(f(t,wide))?0:EINVAL;}
