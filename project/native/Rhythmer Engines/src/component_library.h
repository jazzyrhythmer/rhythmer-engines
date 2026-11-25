// SPDX-License-Identifier: AGPL-3.0-or-later
// Private bundled components only; never searches arbitrary plug-in directories.
#pragma once
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
namespace re {
inline std::wstring widePath(const std::string& s){int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),nullptr,0);if(!n)return {};std::wstring w(size_t(n),L'\0');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),w.data(),n);return w;}
inline std::string utf8Path(const std::wstring& w){int n=WideCharToMultiByte(CP_UTF8,0,w.data(),int(w.size()),nullptr,0,nullptr,nullptr);std::string s(size_t(n),'\0');if(n)WideCharToMultiByte(CP_UTF8,0,w.data(),int(w.size()),s.data(),n,nullptr,nullptr);return s;}
inline void* openLibrary(const std::string& s){auto w=widePath(s);return reinterpret_cast<void*>(LoadLibraryExW(w.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS));}
inline void* librarySymbol(void* h,const char* name){return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(h),name));}
inline void closeLibrary(void* h){if(h)FreeLibrary(reinterpret_cast<HMODULE>(h));}
inline std::string libraryError(){return "Windows loader error "+std::to_string(GetLastError());}
inline std::string containingLibrary(const void* address){HMODULE h=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(address),&h))return {};wchar_t path[32768];DWORD n=GetModuleFileNameW(h,path,32768);return n&&n<32768?utf8Path(std::wstring(path,n)):std::string();}
inline constexpr const char* componentFile="engine.dll";
}
#else
#include <dlfcn.h>
namespace re {
inline void* openLibrary(const std::string& s){return dlopen(s.c_str(),RTLD_NOW|RTLD_LOCAL);}
inline void* librarySymbol(void* h,const char* name){return dlsym(h,name);}
inline void closeLibrary(void* h){if(h)dlclose(h);}
inline std::string libraryError(){auto* e=dlerror();return e?e:"Unknown library error";}
inline std::string containingLibrary(const void* address){Dl_info i{};return dladdr(address,&i)&&i.dli_fname?std::string(i.dli_fname):std::string();}
inline constexpr const char* componentFile="engine.so";
}
#endif
