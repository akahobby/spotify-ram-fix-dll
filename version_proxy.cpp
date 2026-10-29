#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Minimal probe: no background thread, memory trimming, mutex, or file I/O.
// The genuine version.dll is resolved by SYSTEM32-only search when an export is called.
static HMODULE GetSystemVersion() {
    static HMODULE module = LoadLibraryExW(L"version.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    return module;
}
template<typename Fn> Fn GetVersionProc(const char* name) {
    HMODULE module = GetSystemVersion();
    return module ? reinterpret_cast<Fn>(GetProcAddress(module, name)) : nullptr;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        // Only a debug signal during loader initialization; no disk writes or threads.
        OutputDebugStringW(L"SpotifyRamFix v0.7: DLL_PROCESS_ATTACH\n");
    }
    return TRUE;
}

#define PROXY(ret, name, args, callargs, fallback) \
extern "C" ret WINAPI Proxy_##name args { \
    using Fn = decltype(&Proxy_##name); \
    OutputDebugStringW(L"SpotifyRamFix v0.7: version API export called\n"); \
    Fn fn = GetVersionProc<Fn>(#name); \
    if (!fn) { SetLastError(ERROR_PROC_NOT_FOUND); return fallback; } \
    return fn callargs; \
}

PROXY(BOOL, GetFileVersionInfoA, (LPCSTR a, DWORD b, DWORD c, LPVOID d), (a,b,c,d), FALSE)
PROXY(BOOL, GetFileVersionInfoW, (LPCWSTR a, DWORD b, DWORD c, LPVOID d), (a,b,c,d), FALSE)
PROXY(BOOL, GetFileVersionInfoExA, (DWORD a, LPCSTR b, DWORD c, DWORD d, LPVOID e), (a,b,c,d,e), FALSE)
PROXY(BOOL, GetFileVersionInfoExW, (DWORD a, LPCWSTR b, DWORD c, DWORD d, LPVOID e), (a,b,c,d,e), FALSE)
PROXY(DWORD, GetFileVersionInfoSizeA, (LPCSTR a, LPDWORD b), (a,b), 0)
PROXY(DWORD, GetFileVersionInfoSizeW, (LPCWSTR a, LPDWORD b), (a,b), 0)
PROXY(DWORD, GetFileVersionInfoSizeExA, (DWORD a, LPCSTR b, LPDWORD c), (a,b,c), 0)
PROXY(DWORD, GetFileVersionInfoSizeExW, (DWORD a, LPCWSTR b, LPDWORD c), (a,b,c), 0)
PROXY(DWORD, VerFindFileA, (DWORD a, LPCSTR b, LPCSTR c, LPCSTR d, LPSTR e, PUINT f, LPSTR g, PUINT h), (a,b,c,d,e,f,g,h), 0)
PROXY(DWORD, VerFindFileW, (DWORD a, LPCWSTR b, LPCWSTR c, LPCWSTR d, LPWSTR e, PUINT f, LPWSTR g, PUINT h), (a,b,c,d,e,f,g,h), 0)
PROXY(DWORD, VerInstallFileA, (DWORD a, LPCSTR b, LPCSTR c, LPCSTR d, LPCSTR e, LPCSTR f, LPSTR g, PUINT h), (a,b,c,d,e,f,g,h), 0)
PROXY(DWORD, VerInstallFileW, (DWORD a, LPCWSTR b, LPCWSTR c, LPCWSTR d, LPCWSTR e, LPCWSTR f, LPWSTR g, PUINT h), (a,b,c,d,e,f,g,h), 0)
PROXY(DWORD, VerLanguageNameA, (DWORD a, LPSTR b, DWORD c), (a,b,c), 0)
PROXY(DWORD, VerLanguageNameW, (DWORD a, LPWSTR b, DWORD c), (a,b,c), 0)
PROXY(BOOL, VerQueryValueA, (LPCVOID a, LPCSTR b, LPVOID* c, PUINT d), (a,b,c,d), FALSE)
PROXY(BOOL, VerQueryValueW, (LPCVOID a, LPCWSTR b, LPVOID* c, PUINT d), (a,b,c,d), FALSE)
