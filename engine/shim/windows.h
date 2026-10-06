// Minimal Windows API shim so the DfxDsp sources compile on macOS.
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <unistd.h>
#include <sys/time.h>

#ifdef __cplusplus
#include <cstddef>
#endif

typedef int32_t   BOOL;
typedef int32_t   LONG;
typedef uint32_t  ULONG;
typedef uint32_t  DWORD;
typedef uint16_t  WORD;
typedef uint8_t   BYTE;
typedef int       INT;
typedef unsigned  UINT;
typedef void*     HANDLE;
typedef void*     HWND;
typedef void*     HINSTANCE;
typedef void*     HMODULE;
typedef void*     HKEY;
typedef void*     LPVOID;
typedef const void* LPCVOID;
typedef wchar_t   WCHAR;
typedef wchar_t   TCHAR;
typedef char*     LPSTR;
typedef const char* LPCSTR;
typedef wchar_t*  LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef wchar_t*  LPTSTR;
typedef const wchar_t* LPCTSTR;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef DWORD*    LPDWORD;
typedef size_t    SIZE_T;
typedef intptr_t  LONG_PTR;
typedef uintptr_t UINT_PTR;
typedef LONG      HRESULT;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef MAX_PATH
#define MAX_PATH 1024
#endif
#ifndef INFINITE
#define INFINITE 0xFFFFFFFF
#endif
#define WINAPI
#define CALLBACK
#define APIENTRY
#define __declspec(x)
#define __cdecl
#define _T(x) L##x
#define TEXT(x) L##x
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define MB_OK 0
#define S_OK 0
#define SUCCEEDED(hr) ((hr) >= 0)
#define FAILED(hr) ((hr) < 0)

static inline DWORD GetTickCount(void) {
    struct timeval tv; gettimeofday(&tv, 0);
    return (DWORD)(tv.tv_sec * 1000u + tv.tv_usec / 1000u);
}
static inline void Sleep(DWORD ms) { usleep(ms * 1000u); }
static inline int MessageBox(HWND, LPCWSTR, LPCWSTR, UINT) { return 0; }
static inline int MessageBoxW(HWND, LPCWSTR, LPCWSTR, UINT) { return 0; }
static inline void OutputDebugStringW(LPCWSTR) {}
static inline void OutputDebugStringA(LPCSTR) {}

// MSVC string/CRT names
#define _wcsicmp wcscasecmp
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define _strdup strdup
#define _wcsdup wcsdup
#define _snprintf snprintf
#define _unlink unlink
#define _getcwd getcwd
#define _chdir chdir
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define _isnan isnan
#define _finite isfinite
#define _copysign copysign
#ifndef min
#ifdef __cplusplus
#include <algorithm>
using std::min; using std::max;
#endif
#endif
static inline int MessageBoxA(HWND, LPCSTR, LPCSTR, UINT) { return 0; }

// ---- more types ----
typedef uintptr_t WPARAM;
typedef intptr_t  LPARAM;
typedef intptr_t  LRESULT;
typedef struct { DWORD dwLowDateTime, dwHighDateTime; } FILETIME;
typedef struct { WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds; } SYSTEMTIME;
typedef void* LPSECURITY_ATTRIBUTES;
typedef void* HGLOBAL; typedef void* HMMIO; typedef void* HWAVEOUT; typedef void* HWAVEIN;
typedef char* HPSTR;
typedef struct { WORD wFormatTag, nChannels; DWORD nSamplesPerSec, nAvgBytesPerSec; WORD nBlockAlign, wBitsPerSample, cbSize; } WAVEFORMATEX;
typedef struct { char* lpData; DWORD dwBufferLength, dwBytesRecorded, dwUser, dwFlags, dwLoops; void* lpNext; DWORD reserved; } WAVEHDR;
typedef WAVEHDR* LPWAVEHDR;
typedef struct { DWORD ckid, cksize, fccType, dwDataOffset, dwFlags; } MMCKINFO;
#define LOWORD(l) ((WORD)((l) & 0xffff))
#define HIWORD(l) ((WORD)(((l) >> 16) & 0xffff))
#define LMEM_MOVEABLE 2
static inline int FreeLibrary(HMODULE) { return 0; }
#define _wtoi(s) ((int)wcstol((s), 0, 10))
#define _wtol(s) wcstol((s), 0, 10)
#define _wtof(s) wcstod((s), 0)
static inline int _wfopen_s(FILE** f, const wchar_t* path, const wchar_t* mode) {
    char p[1024], m[16]; wcstombs(p, path, sizeof p); wcstombs(m, mode, sizeof m);
    *f = fopen(p, m); return *f ? 0 : 1;
}
static inline FILE* _wfopen(const wchar_t* path, const wchar_t* mode) { FILE* f; _wfopen_s(&f, path, mode); return f; }

#ifdef __cplusplus
// MSVC-style swprintf (no size arg, %s == wide string)
#include <string>
#include <stdarg.h>
inline std::wstring msvc_fix_fmt(const wchar_t* f) {
    std::wstring o;
    for (; *f; ++f) {
        if (*f == L'%') {
            o += *f++; if (!*f) break;
            if (*f == L'%') { o += *f; continue; }
            while (*f && wcschr(L"-+ #0123456789.*", *f)) o += *f++;
            if (*f == L's') o += L"ls";
            else if (*f == L'S') o += L"s";
            else if (*f == L'h' && f[1] == L's') { o += L's'; ++f; }
            else if (*f == L'c') o += L"lc";
            else o += *f;
        } else o += *f;
    }
    return o;
}
inline int msvc_vswprintf(wchar_t* b, size_t n, const wchar_t* f, va_list ap) {
    std::wstring ff = msvc_fix_fmt(f); int r = vswprintf(b, n, ff.c_str(), ap); if (r < 0 && n) b[n-1] = 0; return r;
}
template <size_t N> int msvc_swprintf(wchar_t (&b)[N], const wchar_t* f, ...) {
    va_list ap; va_start(ap, f); int r = msvc_vswprintf(b, N, f, ap); va_end(ap); return r;
}
inline int msvc_swprintf(wchar_t* b, const wchar_t* f, ...) { va_list ap; va_start(ap, f); int r = msvc_vswprintf(b, 1024, f, ap); va_end(ap); return r; }
inline int msvc_swprintf(wchar_t* b, size_t n, const wchar_t* f, ...) {
    va_list ap; va_start(ap, f); int r = msvc_vswprintf(b, n, f, ap); va_end(ap); return r;
}
#define swprintf msvc_swprintf
#endif
#define __int64 long long

// ---- misc ----
typedef union { struct { DWORD LowPart; LONG HighPart; }; long long QuadPart; } LARGE_INTEGER;
typedef union { struct { DWORD LowPart; DWORD HighPart; }; unsigned long long QuadPart; } ULARGE_INTEGER;
typedef ULARGE_INTEGER* PULARGE_INTEGER;
typedef FILETIME* LPFILETIME;
typedef SYSTEMTIME* LPSYSTEMTIME;
static inline BOOL QueryPerformanceCounter(LARGE_INTEGER* li) { struct timeval tv; gettimeofday(&tv, 0); li->QuadPart = (long long)tv.tv_sec * 1000000LL + tv.tv_usec; return 1; }
static inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* li) { li->QuadPart = 1000000LL; return 1; }
#define GENERIC_READ 0x80000000
#define GENERIC_WRITE 0x40000000
#define wcstok_s(s, d, ctx) wcstok((s), (d), (ctx))
