#include "minidump.h"
#include <stdlib.h>
#include <wchar.h>
#include <version.h>

#include <dbghelp.h>  
#pragma comment(lib, "dbghelp.lib")


DWORD WINAPI  MessageBoxThread(LPVOID ptr) {
    const wchar_t *txt = reinterpret_cast<const wchar_t *>(ptr);
    return MessageBoxW(NULL, txt, L"SKELDAL", MB_OK|MB_ICONEXCLAMATION|MB_SYSTEMMODAL|MB_APPLMODAL);
}

static void MessageBoxAndExit(const wchar_t *msg, const wchar_t *fname) {
    DWORD tid;
    HANDLE h = CreateThread(NULL, 0, MessageBoxThread, const_cast<wchar_t *>(msg), 0, &tid);
    WaitForSingleObject(h, INFINITE);
    if (fname != nullptr) {
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
    
        wchar_t command[MAX_PATH + 20];
        swprintf_s(command, L"explorer.exe /select,\"%s\"", fname);
        if (CreateProcessW(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
    ExitProcess(256);
}


LONG WINAPI GenerateMinidumpAndExit(EXCEPTION_POINTERS* pExceptionInfo) {
    wchar_t path[MAX_PATH];
    _wfullpath(path, L"crash-" SKELDAL_VERSION ".dmp", _countof(path));
    HANDLE hFile = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE)) {
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ThreadId           = GetCurrentThreadId();
        mdei.ExceptionPointers  = pExceptionInfo;
        mdei.ClientPointers     = FALSE;

        BOOL ret = MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            static_cast<MINIDUMP_TYPE>(MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory),
            &mdei,
            NULL,
            NULL);

        CloseHandle(hFile);
        if (ret) {
            wchar_t msg[MAX_PATH*2];
            swprintf_s(msg,L"Application crashed. The crash report has been stored to the file:\r\n\r\n%s\r\n\r\nPress OK to browse file in explorer",path);
            MessageBoxAndExit(msg,path);
            return 0;
        }
    }
    {
        DWORD error = GetLastError();
        wchar_t msg[MAX_PATH*2];
        swprintf_s(msg,L"Application crashed. Failed to write crash report to the file:\r\n\r\n%s\r\nError: %X", path, error);
        MessageBoxAndExit(msg,path);
    }



    return 0;

}