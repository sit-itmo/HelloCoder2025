// GameSvcInstall.c - Install + Start the service using WinAPI (CreateService/StartService)
//
// Build:
//   cl /W4 /DUNICODE /D_UNICODE GameSvcInstall.c /link advapi32.lib
//
// Usage (run elevated):
//   GameSvcInstall.exe "C:\Path\To\GameSvc.exe"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#define SVC_NAME L"GameSvc"
#define SVC_DISPLAY L"Game Sample Service (Named Pipe)"

static void PrintLastError(const wchar_t* what)
{
    DWORD e = GetLastError();
    wchar_t* msg = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, e, 0, (LPWSTR)&msg, 0, NULL);
    wprintf(L"%s failed: %lu%s%s\n", what, e, msg ? L" - " : L"", msg ? msg : L"");
    if (msg) LocalFree(msg);
}

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2) {
        wprintf(L"Usage: %s \"C:\\Full\\Path\\GameSvc.exe\"\n", argv[0]);
        return 1;
    }

    const wchar_t* binPath = argv[1];

    SC_HANDLE hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCM) {
        PrintLastError(L"OpenSCManager");
        return 2;
    }

    // Try open existing
    SC_HANDLE hSvc = OpenServiceW(hSCM, SVC_NAME, SERVICE_CHANGE_CONFIG | SERVICE_START | SERVICE_QUERY_STATUS);
    if (!hSvc) {
        // Create new
        hSvc = CreateServiceW(
            hSCM,
            SVC_NAME,
            SVC_DISPLAY,
            SERVICE_CHANGE_CONFIG | SERVICE_START | SERVICE_QUERY_STATUS,
            SERVICE_WIN32_OWN_PROCESS,
            SERVICE_DEMAND_START,
            SERVICE_ERROR_NORMAL,
            binPath,
            NULL, NULL, NULL,
            NULL, NULL);

        if (!hSvc) {
            PrintLastError(L"CreateService");
            CloseServiceHandle(hSCM);
            return 3;
        }

        wprintf(L"Service created.\n");
    }
    else {
        wprintf(L"Service already exists (opened).\n");
    }

    // Optional: set description
    SERVICE_DESCRIPTIONW desc = { 0 };
    desc.lpDescription = (LPWSTR)L"Sample service exposing GameStart/Stop/PutKey/GetScreen via Named Pipe \\\\.\\pipe\\GameSvc.";
    ChangeServiceConfig2W(hSvc, SERVICE_CONFIG_DESCRIPTION, &desc);

    // Start it
    if (!StartServiceW(hSvc, 0, NULL)) {
        DWORD e = GetLastError();
        if (e == ERROR_SERVICE_ALREADY_RUNNING) {
            wprintf(L"Service already running.\n");
        }
        else {
            SetLastError(e);
            PrintLastError(L"StartService");
            CloseServiceHandle(hSvc);
            CloseServiceHandle(hSCM);
            return 4;
        }
    }
    else {
        wprintf(L"StartService requested.\n");
    }

    // Wait for running
    SERVICE_STATUS_PROCESS ssp = { 0 };
    DWORD bytesNeeded = 0;
    for (int i = 0; i < 50; i++) {
        if (!QueryServiceStatusEx(hSvc, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &bytesNeeded)) {
            PrintLastError(L"QueryServiceStatusEx");
            break;
        }
        if (ssp.dwCurrentState == SERVICE_RUNNING) {
            wprintf(L"Service is RUNNING.\n");
            break;
        }
        if (ssp.dwCurrentState == SERVICE_STOPPED) {
            wprintf(L"Service is STOPPED (exit code %lu).\n", ssp.dwWin32ExitCode);
            break;
        }
        Sleep(100);
    }

    CloseServiceHandle(hSvc);
    CloseServiceHandle(hSCM);
    return 0;
}
