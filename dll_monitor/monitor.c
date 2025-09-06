#include <windows.h>
#include <stdio.h>
#include "original.h"

typedef int (__cdecl *ADD_FUNC)(int, int);

static HMODULE real_dll = NULL;
static ADD_FUNC real_add = NULL;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        real_dll = LoadLibraryA("original.dll");
        if (real_dll) {
            real_add = (ADD_FUNC)GetProcAddress(real_dll, "add");
        }
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        if (real_dll) {
            FreeLibrary(real_dll);
        }
    }
    return TRUE;
}

__declspec(dllexport) int add(int a, int b) {
    printf("[monitor] add(%d, %d)\n", a, b);
    if (real_add) {
        int result = real_add(a, b);
        printf("[monitor] result=%d\n", result);
        return result;
    }
    return 0;
}
