#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <string>
#include <cstdio>

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

namespace {
HMODULE g_self = nullptr;

void Debug(const wchar_t* event, SIZE_T before = 0, SIZE_T after = 0,
           DWORD error = 0, BOOL ok = FALSE, SIZE_T privateBytes = 0) {
    wchar_t msg[512]{};
    swprintf_s(msg, L"SpotifyRamFix v0.8: pid=%lu %s ws_before=%llu ws_after=%llu private=%llu success=%d error=%lu\n",
        GetCurrentProcessId(), event,
        static_cast<unsigned long long>(before),
        static_cast<unsigned long long>(after),
        static_cast<unsigned long long>(privateBytes), ok ? 1 : 0, error);
    OutputDebugStringW(msg);
}

bool SampleMemory(SIZE_T& working, SIZE_T& privateBytes) {
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    pmc.cb = sizeof(pmc);
    if (!GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc)))
        return false;
    working = pmc.WorkingSetSize;
    privateBytes = pmc.PrivateUsage;
    return true;
}

DWORD WINAPI Worker(LPVOID) {
    Debug(L"worker_started");
    wchar_t modulePath[MAX_PATH]{};
    DWORD count = GetModuleFileNameW(g_self, modulePath, MAX_PATH);
    if (!count || count >= MAX_PATH) {
        Debug(L"module_path_failed", 0, 0, GetLastError());
        return 0;
    }
    std::wstring config(modulePath);
    auto slash = config.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        Debug(L"module_directory_failed");
        return 0;
    }
    config.resize(slash + 1);
    config += L"settings.ini";
    const bool enabled = GetPrivateProfileIntW(L"Settings", L"EnableTrim", 1, config.c_str()) != 0;
    int rawInterval = GetPrivateProfileIntW(L"Settings", L"TrimIntervalMs", 30000, config.c_str());
    DWORD interval = static_cast<DWORD>(rawInterval < 5000 ? 5000 : (rawInterval > 3600000 ? 3600000 : rawInterval));
    if (!enabled) {
        Debug(L"trim_disabled");
        return 0;
    }
    Debug(L"trim_enabled");
    // Give Spotify's subprocess a chance to finish startup before the first trim.
    Sleep(5000);
    for (;;) {
        SIZE_T before = 0, after = 0, privateBefore = 0, privateAfter = 0;
        if (!SampleMemory(before, privateBefore)) {
            Debug(L"sample_before_failed", 0, 0, GetLastError());
        } else {
            SetLastError(ERROR_SUCCESS);
            BOOL ok = EmptyWorkingSet(GetCurrentProcess());
            DWORD err = ok ? ERROR_SUCCESS : GetLastError();
            if (!SampleMemory(after, privateAfter)) {
                Debug(L"sample_after_failed", before, 0, GetLastError(), ok, privateBefore);
            } else {
                Debug(L"trim", before, after, err, ok, privateAfter);
            }
        }
        Sleep(interval);
    }
}
} // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = instance;
        DisableThreadLibraryCalls(instance);
        OutputDebugStringW(L"SpotifyRamFix v0.8: DLL_PROCESS_ATTACH\n");
        // Minimal setup here. Worker does all config, debugging and trimming
        // after the loader lock is released; we never wait inside DllMain.
        HANDLE thread = CreateThread(nullptr, 0, Worker, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
        else OutputDebugStringW(L"SpotifyRamFix v0.8: CreateThread failed\n");
    }
    return TRUE;
}

#define PROXY(ret, name, args, callargs, fallback) \
extern "C" ret WINAPI Proxy_##name args { \
    using Fn = decltype(&Proxy_##name); \
    OutputDebugStringW(L"SpotifyRamFix v0.8: version API export called\n"); \
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
