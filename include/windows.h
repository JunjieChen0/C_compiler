#ifndef _WINDOWS_H
#define _WINDOWS_H

#include "stddef.h"
#include "stdint.h"

typedef void *HANDLE;
typedef void *HWND;
typedef void *HINSTANCE;
typedef void *HMODULE;
typedef void *HDC;
typedef void *HICON;
typedef void *HCURSOR;
typedef void *HBRUSH;
typedef void *HMENU;
typedef void *HFONT;
typedef void *HBITMAP;
typedef void *HPEN;
typedef void *HRGN;

typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef int BOOL;
typedef long LONG;
typedef unsigned long ULONG;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef long HRESULT;
typedef long LRESULT;
typedef unsigned long long WPARAM;
typedef long long LPARAM;
typedef unsigned int UINT;

typedef char *LPSTR;
typedef const char *LPCSTR;
typedef wchar_t *LPWSTR;
typedef const wchar_t *LPCWSTR;
typedef void *LPVOID;
typedef const void *LPCVOID;
typedef DWORD *LPDWORD;

#define TRUE 1
#define FALSE 0
#define INVALID_HANDLE_VALUE ((HANDLE)(long long)-1)
#define INFINITE 0xFFFFFFFF

#define CALLBACK
#define WINAPI
#define APIENTRY

#define MAKEWORD(a, b) ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define MAKELONG(a, b) ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define LOWORD(l) ((WORD)(l))
#define HIWORD(l) ((WORD)((DWORD)(l) >> 16))
#define LOBYTE(w) ((BYTE)(w))
#define HIBYTE(w) ((BYTE)((WORD)(w) >> 8))

#define S_OK 0
#define S_FALSE 1
#define E_FAIL ((HRESULT)0x80004005)
#define E_OUTOFMEMORY ((HRESULT)0x8007000E)
#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
#define FAILED(hr) ((HRESULT)(hr) < 0)

typedef struct {
    long left;
    long top;
    long right;
    long bottom;
} RECT;

typedef struct {
    long x;
    long y;
} POINT;

typedef struct {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME;

void *GetStdHandle(DWORD nStdHandle);
BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, void *lpOverlapped);
BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, void *lpOverlapped);
HANDLE CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, void *lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
BOOL CloseHandle(HANDLE hObject);
DWORD GetLastError(void);
HMODULE GetModuleHandleA(LPCSTR lpModuleName);
void *GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
void *VirtualAlloc(void *lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect);
BOOL VirtualFree(void *lpAddress, size_t dwSize, DWORD dwFreeType);
void ExitProcess(UINT uExitCode);
void GetSystemTime(SYSTEMTIME *lpSystemTime);
DWORD GetTickCount(void);
void Sleep(DWORD dwMilliseconds);

#define STD_INPUT_HANDLE ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE ((DWORD)-12)

#define GENERIC_READ 0x80000000
#define GENERIC_WRITE 0x40000000
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define FILE_ATTRIBUTE_NORMAL 128
#define MEM_COMMIT 0x1000
#define MEM_RESERVE 0x2000
#define MEM_RELEASE 0x8000
#define PAGE_READWRITE 4

#endif
