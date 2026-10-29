#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <strsafe.h>
#include <cwchar>
#include <string>
#include <vector>
#include <cstdio>
#pragma comment(lib, "Psapi.lib")

// Every exported Version API is forwarded to the genuine Windows system DLL.
static HMODULE GetSystemVersion() {
    static HMODULE module = LoadLibraryExW(L"version.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    return module;
}
template<typename Fn> Fn GetVersionProc(const char* name) {
    HMODULE module = GetSystemVersion();
    return module ? reinterpret_cast<Fn>(GetProcAddress(module, name)) : nullptr;
}

namespace {
HMODULE g_module = nullptr;
struct Settings { bool enabled = true; DWORD intervalMs = 30000; bool logCsv = true; };

std::wstring moduleDir() {
    wchar_t path[32768]{};
    DWORD n = GetModuleFileNameW(g_module, path, 32768);
    if (!n || n >= 32768) return L"";
    std::wstring result(path, n);
    auto i = result.find_last_of(L"\\/");
    return i == std::wstring::npos ? L"" : result.substr(0, i + 1);
}
std::wstring logDir() {
    // Spotify sandboxed utility processes may be low-integrity. LocalLow is
    // the usual writable directory for them, not Local\\Low.
    wchar_t base[32768]{};
    DWORD n = GetEnvironmentVariableW(L"USERPROFILE", base, 32768);
    if (!n || n >= 32768) return L"";
    return std::wstring(base) + L"\\AppData\\LocalLow\\SpotifyRamFix";
}
std::string timestamp() {
    SYSTEMTIME t{}; GetLocalTime(&t);
    char buf[64]{};
    StringCchPrintfA(buf, 64, "%04u-%02u-%02u %02u:%02u:%02u",
        t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond);
    return buf;
}
std::wstring logFile(const wchar_t* filename) {
    auto dir = logDir();
    if (dir.empty()) return L"";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir + L"\\" + filename;
}
bool appendText(const std::wstring& path, const std::string& message) {
    if (path.empty()) return false;
    HANDLE f = CreateFileW(path.c_str(), FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    BOOL ok = WriteFile(f, message.data(), static_cast<DWORD>(message.size()), &written, nullptr);
    CloseHandle(f);
    return ok && written == message.size();
}
void diag(const char* text, DWORD error = 0) {
    std::string line = timestamp() + " pid=" + std::to_string(GetCurrentProcessId()) +
                       " " + text + " error=" + std::to_string(error) + "\r\n";
    if (!appendText(logFile(L"diagnostic.log"), line)) {
        std::wstring debug = L"SpotifyRamFix logging failed: ";
        debug += std::wstring(line.begin(), line.end());
        OutputDebugStringW(debug.c_str());
    }
}
Settings loadSettings() {
    Settings s;
    std::wstring config = moduleDir() + L"settings.ini";
    s.enabled = GetPrivateProfileIntW(L"Settings", L"EnableTrim", 1, config.c_str()) != 0;
    int interval = GetPrivateProfileIntW(L"Settings", L"TrimIntervalMs", 30000, config.c_str());
    s.intervalMs = static_cast<DWORD>(interval < 5000 ? 5000 : interval > 3600000 ? 3600000 : interval);
    s.logCsv = GetPrivateProfileIntW(L"Settings", L"LogCsv", 1, config.c_str()) != 0;
    return s;
}
SIZE_T residentBytes() {
    PROCESS_MEMORY_COUNTERS_EX info{};
    info.cb = sizeof(info);
    return GetProcessMemoryInfo(GetCurrentProcess(),
        reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&info), sizeof(info)) ? info.WorkingSetSize : 0;
}
void appendCsv(SIZE_T before, SIZE_T after, bool ok, DWORD error) {
    std::string row = timestamp() + "," + std::to_string(GetCurrentProcessId()) +
        "," + std::to_string(before) + "," + std::to_string(after) +
        "," + std::to_string(ok ? 1 : 0) + "," + std::to_string(error) + "\r\n";
    const auto path = logFile(L"memory-log.csv");
    if (!appendText(path, row)) diag("csv_write_failed", GetLastError());
}
DWORD WINAPI TrimWorker(void*) {
    diag("worker_started");
    // Each instance manages only its own Spotify process, so low-integrity
    // children never need to open higher-integrity siblings.
    for (;;) {
        const Settings options = loadSettings();
        const SIZE_T before = residentBytes();
        BOOL ok = TRUE;
        DWORD error = 0;
        if (options.enabled) {
            ok = EmptyWorkingSet(GetCurrentProcess());
            if (!ok) error = GetLastError();
        }
        const SIZE_T after = residentBytes();
        if (options.logCsv) appendCsv(before, after, !!ok, error);
        if (!ok) diag("trim_failed", error);
        Sleep(options.intervalMs);
    }
}
} // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = instance;
        DisableThreadLibraryCalls(instance);
        // Worker handles all I/O only once the loader has released its lock.
        // No waiting or file operations occur inside DllMain.
        HANDLE thread = CreateThread(nullptr, 0, TrimWorker, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
        else OutputDebugStringW(L"SpotifyRamFix: worker CreateThread failed.\n");
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
