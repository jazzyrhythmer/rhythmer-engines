/* SPDX-License-Identifier: MIT
 * Zig's cross-linked shared libraries register C++ finalizers but do not
 * include GCC crtbeginS's unload hook. Finalize this DSO before it unmaps;
 * process-exit handlers must never retain pointers into an unloaded core.
 */
extern void *__dso_handle;
extern void __cxa_finalize(void *);
__attribute__((destructor)) static void rhythmer_finalize_dso(void) {
    __cxa_finalize(&__dso_handle);
}
