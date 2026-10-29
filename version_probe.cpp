#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <strsafe.h>

static HMODULE GetSystemVersion() {
    static HMODULE module = LoadLibraryExW(L"version.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    return module;
}

template<typename Fn> Fn GetVersionProc(const char* name) {
    HMODULE module = GetSystemVersion();
    return module ? reinterpret_cast<Fn>(GetProcAddress(module, name)) : nullptr;
}

static DWORD WINAPI LogWorker(void*) {
    wchar_t base[MAX_PATH] = {};
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH);
    if (!n || n >= MAX_PATH) return 0;
    wchar_t path[MAX_PATH] = {};
    if (FAILED(StringCchPrintfW(path, MAX_PATH, L"%s\\SpotifyVersionProbe.log", base))) return 0;
    HANDLE f = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return 0;
    SYSTEMTIME t; GetLocalTime(&t);
    char line[256] = {};
    StringCchPrintfA(line, sizeof(line),
        "[%04u-%02u-%02u %02u:%02u:%02u] Probe DLL loaded; PID=%lu\r\n",
        t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond,
        GetCurrentProcessId());
    DWORD written = 0;
    WriteFile(f, line, lstrlenA(line), &written, nullptr);
    CloseHandle(f);
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        OutputDebugStringW(L"SpotifyVersionProbe: DLL_PROCESS_ATTACH\n");
        HANDLE thread = CreateThread(nullptr, 0, LogWorker, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}

#define PROXY(ret, name, args, callargs, fallback) \
extern "C" ret WINAPI Proxy_##name args { \
    using Fn = decltype(&Proxy_##name); \
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
