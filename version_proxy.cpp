#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <strsafe.h>
#include <cwchar>
#include <string>
#include <vector>
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
struct Settings {
    bool enabled = true;
    DWORD intervalMs = 30000;
    bool logCsv = true;
};

std::wstring appPath(const wchar_t* filename) {
    wchar_t path[MAX_PATH]{};
    DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (!n || n >= MAX_PATH) return filename;
    std::wstring result(path);
    const auto slash = result.find_last_of(L"\\/");
    return (slash == std::wstring::npos ? std::wstring{} : result.substr(0, slash + 1)) + filename;
}
std::wstring logPath() {
    wchar_t base[MAX_PATH]{};
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH);
    if (!n || n >= MAX_PATH) return L"";
    return std::wstring(base) + L"\\SpotifyRamFix-memory-log.csv";
}
Settings loadSettings(const std::wstring& config) {
    Settings s;
    s.enabled = GetPrivateProfileIntW(L"Settings", L"EnableTrim", 1, config.c_str()) != 0;
    int i = GetPrivateProfileIntW(L"Settings", L"TrimIntervalMs", 30000, config.c_str());
    s.intervalMs = static_cast<DWORD>(i < 5000 ? 5000 : (i > 3600000 ? 3600000 : i));
    s.logCsv = GetPrivateProfileIntW(L"Settings", L"LogCsv", 1, config.c_str()) != 0;
    return s;
}
std::vector<DWORD> spotifyPids() {
    std::vector<DWORD> pids;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return pids;
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"Spotify.exe") == 0)
                pids.push_back(pe.th32ProcessID);
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pids;
}
SIZE_T workingSet(HANDLE process) {
    PROCESS_MEMORY_COUNTERS pmc{};
    pmc.cb = sizeof(pmc);
    return GetProcessMemoryInfo(process, &pmc, sizeof(pmc)) ? pmc.WorkingSetSize : 0;
}
void writeRows(const std::string& rows) {
    const auto path = logPath();
    if (path.empty()) return;
    HANDLE f = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return;
    LARGE_INTEGER size{};
    if (GetFileSizeEx(f, &size) && size.QuadPart == 0) {
        static constexpr char header[] = "timestamp,pid,before_bytes,after_bytes,trim_succeeded,win32_error,worker_pid\r\n";
        DWORD written = 0;
        WriteFile(f, header, sizeof(header) - 1, &written, nullptr);
    }
    DWORD written = 0;
    WriteFile(f, rows.data(), static_cast<DWORD>(rows.size()), &written, nullptr);
    CloseHandle(f);
}
std::string now() {
    SYSTEMTIME t{};
    GetLocalTime(&t);
    char buf[64]{};
    StringCchPrintfA(buf, 64, "%04u-%02u-%02u %02u:%02u:%02u",
        t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond);
    return buf;
}

DWORD WINAPI TrimWorker(void*) {
    // Only one loaded instance may enumerate/trim the Spotify process tree per session.
    HANDLE mutex = CreateMutexW(nullptr, FALSE, L"Local\\SpotifyRamFix_v04_TrimWorker");
    if (!mutex) return 0;
    const DWORD acquired = WaitForSingleObject(mutex, 0);
    if (acquired != WAIT_OBJECT_0 && acquired != WAIT_ABANDONED) {
        CloseHandle(mutex);
        return 0;
    }
    const auto config = appPath(L"settings.ini");
    while (true) {
        const Settings opts = loadSettings(config);
        const auto pids = spotifyPids();
        std::string rows;
        if (opts.logCsv) {
            for (DWORD pid : pids) {
                HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_SET_QUOTA, FALSE, pid);
                SIZE_T before = 0, after = 0;
                BOOL ok = FALSE;
                DWORD err = 0;
                if (!process) {
                    err = GetLastError();
                } else {
                    before = workingSet(process);
                    if (opts.enabled) {
                        ok = EmptyWorkingSet(process);
                        if (!ok) err = GetLastError();
                    } else {
                        ok = TRUE; // monitor-only
                    }
                    after = workingSet(process);
                    CloseHandle(process);
                }
                rows += now() + "," + std::to_string(pid) + "," +
                        std::to_string(before) + "," + std::to_string(after) + "," +
                        std::to_string(ok ? 1 : 0) + "," + std::to_string(err) + "," +
                        std::to_string(GetCurrentProcessId()) + "\r\n";
            }
            if (pids.empty()) rows = now() + ",0,0,0,0,0," + std::to_string(GetCurrentProcessId()) + "\r\n";
            writeRows(rows);
        } else if (opts.enabled) {
            for (DWORD pid : pids) {
                HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_SET_QUOTA, FALSE, pid);
                if (process) { EmptyWorkingSet(process); CloseHandle(process); }
            }
        }
        Sleep(opts.intervalMs);
    }
    // The host process terminates the thread; keep the mutex owned during its lifetime.
}
} // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        HANDLE worker = CreateThread(nullptr, 0, TrimWorker, nullptr, 0, nullptr);
        if (worker) CloseHandle(worker);
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
