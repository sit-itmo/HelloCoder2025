#define HL_TRACE_PRINT_ENABLE
#define HL_TRACE_PRINT(...) {printf(__VA_ARGS__); fflush(stdout);}
#define BINRESOURCE_PRINT printf

typedef unsigned short u16;

#pragma warning(push)
#pragma warning(disable: 4200 4005)

#pragma comment(lib, "Userenv.lib")
#pragma comment(lib, "Wtsapi32.lib")

#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>

#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>

#include <WtsApi32.h>
#include <userenv.h>
#include <winsafer.h>


#include <TlHelp32.h>
#include <psapi.h>

typedef DWORD(WINAPI* WTSGetActiveConsoleSessionIdProc)(void);

#ifndef _USE_OLD_IOSTREANS
using namespace std;
#endif

#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable: 4005 4103)

#pragma warning(pop)

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#define OS_PRINT printf

#define BUFSIZE 4096 
#define PATHMAX 1024 
#define NAMEMAX 128 

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

HANDLE g_hInputFile = NULL;
HANDLE g_hProcess = NULL;

HANDLE g_hInputThread = NULL;
HANDLE g_hOutputThread = NULL;
HANDLE g_hUniversalThread = NULL;
DWORD g_dwInputThreadId = 0;
DWORD g_dwOutputThreadId = 0;
DWORD g_dwUniversalThreadId = 0;
BOOL g_ThreadRunning = FALSE;

BOOL g_bDoVerbose = FALSE;
BOOL g_bDoInputEcho = FALSE;
BOOL g_bDoQuiet = FALSE;
BOOL g_bDoImpersonate = FALSE;
BOOL g_bDoElevate = FALSE;
BOOL g_bDoLimit = FALSE;
BOOL g_bDoDisableFileSystemRedirection = FALSE;
BOOL g_bDoRedirectConsole = FALSE;
BOOL g_bDoRecreateConsole = FALSE;
BOOL g_bDoOverlapped = FALSE;
BOOL g_bDoTerminal = FALSE;

DWORD g_dwBaudRate = 115200;

WCHAR g_UserName[NAMEMAX] = { 0 };
WCHAR g_UserPassword[NAMEMAX] = { 0 };
WCHAR g_UserDomain[NAMEMAX] = { 0 };
WCHAR g_WorkDir[PATHMAX] = { 0 };
WCHAR g_Desktop[PATHMAX] = { 0 };
WCHAR g_WinStation[NAMEMAX] = { 0 };
WCHAR g_WinDesktop[NAMEMAX] = { 0 };
WCHAR g_InputSerial[NAMEMAX] = { 0 };
WCHAR g_InputFile[PATHMAX] = { 0 };
WCHAR g_OutputSerial[NAMEMAX] = { 0 };
WCHAR g_OutputFile[PATHMAX] = { 0 };
WCHAR g_LogFile[PATHMAX] = { 0 };
WCHAR g_AppPath[PATHMAX] = { 0 };
WCHAR g_AppName[PATHMAX] = { 0 };
WCHAR g_AppCmdLine[PATHMAX] = { 0 };

WCHAR g_MyCommandLine[PATHMAX] = { 0 };
WCHAR g_MyApp[PATHMAX] = { 0 };

void WrtUsage(char* p_name)
{
    printf("\nUSAGE: %s [-viEefqRroth] [-u user -p password [-d domain]] [-D winsta0\\default] [-So COM1] [-Si COM1] [-Sb 115200] [-l mylog.log] [-L log.txt] [-I file.txt] --start <app> [<app_args>]"
        "\n     -i            - impersonate from local service to active session automatically"
        "\n     -E            - elevate proecess"
        "\n     -e            - limit process rights"
        "\n     -r            - recreate console"
        "\n     -v            - verbose output"
        "\n     -o            - do overlapped io handling"
        "\n     -t            - use terminal patch for input"
        "\n     -h            - do echo of input"
        "\n     -q            - quiet output"
        "\n     -f            - disable file redirection"
        "\n     -l file.log   - print all tool output to log file"
        "\n     -u <user>     - specify user name to impersinate"
        "\n     -p <password> - provide password to login specific user"
        "\n     -d <domain>   - provide domain to login specific user (default is local .)"
        "\n     -D <desktop>  - provide desktop name to start application on (default: winsta0\\default)"
        "\n     -W <work_dir> - specify working directory to choose"
        "\n     -R            - redirect input/output/err to my console"
        "\n     -Sb <baud>    - specify baud rate for serial port (115200 - default)"
        "\n     -So COM1      - print all application output to specified com port"
        "\n     -Si COM1      - redirect serial input of specified com port to application input"
        "\n     -L file.log   - print all application output to specified log file"
        "\n     -I file.txt   - redirect content of text file to application input"
        "\n     --start       - rest stuff is for starting application and its command line"
        "\n"
        , p_name);
}

FILE* g_pLogFile = NULL;
void WrtOutputImpl(BOOL quiet, const char* format, va_list ap)
{
    DWORD dw = GetLastError();
    LPVOID lpMsgBuf = NULL;
    BOOL error_or_warning = FALSE;

    error_or_warning = strstr(format, "ERROR:") != 0 || strstr(format, "WARNING:") != 0;
    if (g_bDoVerbose || g_bDoVerbose)
    {
        if (g_pLogFile != NULL)
        {
            vfprintf(g_pLogFile, format, ap); fflush(g_pLogFile);
        }
        if (quiet == false)
        {
            vprintf(format, ap); fflush(stdout);
        }
    }
    if (error_or_warning && g_bDoVerbose)
    {
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

        if (g_pLogFile != NULL)
        {
            fwprintf(g_pLogFile, L"ERROR MESSAGE: %s", lpMsgBuf); fflush(g_pLogFile);
        }
        if (quiet == false)
        {
            wprintf(L"ERROR MESSAGE: %s", lpMsgBuf); fflush(g_pLogFile);
            vprintf(format, ap); fflush(stdout);
        }
        LocalFree(lpMsgBuf);
    }
}

void WRT_LOG_impl(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    WrtOutputImpl(g_bDoQuiet, format, ap);
    va_end(ap);
}

#define WRT_LOG(a, ...)  WRT_LOG_impl("\n[%04d] WRT: " a, __LINE__, ##__VA_ARGS__);

PVOID g_OldWow64RedirVal = NULL;
void WrtDisableFileRedirection()
{
    BOOL b = Wow64DisableWow64FsRedirection(&g_OldWow64RedirVal);
    if (b)
    {
        WRT_LOG("Disabled WOW64 file system redirection");
    }
    else
    {
        WRT_LOG("Failed to disable WOW64 file system redirection");
    }
}

BOOL WrtCloseDisableFileRedirection()
{
    if (g_OldWow64RedirVal != NULL)
    {
        BOOL b = Wow64RevertWow64FsRedirection(g_OldWow64RedirVal);
        if (b)
        {
            WRT_LOG("WOW64 file system redirection reverted");
        }
        else
        {
            WRT_LOG("Failed to reverted WOW64 file system redirection");
        }
    }
    return 1;
}

BOOL WrtLimitRights(HANDLE* p_hUser)
{
    DWORD gle = 0;

    if (p_hUser)
    {
        WRT_LOG("WARNING: No handle specified");
        return 0;
    }
    if (*p_hUser != INVALID_HANDLE_VALUE)
    {
        HANDLE hNew = NULL;
        SAFER_LEVEL_HANDLE safer = NULL;
        WRT_LOG("SaferCreateLevel...");
        if (FALSE == SaferCreateLevel(SAFER_SCOPEID_USER, SAFER_LEVELID_NORMALUSER, SAFER_LEVEL_OPEN, &safer, NULL))
        {
            WRT_LOG("ERROR: SaferCreateLevel failed {%d}.", GetLastError());
            return 0;
        }

        if (NULL != safer)
        {
            WRT_LOG("SaferComputeTokenFromLevel...");
            if (FALSE == SaferComputeTokenFromLevel(safer, *p_hUser, &hNew, 0, NULL))
            {
                WRT_LOG("ERROR: SaferComputeTokenFromLevel failed {%d}.", GetLastError());
                if (!SaferCloseLevel(safer))
                {
                    WRT_LOG("ERROR: SaferCloseLevel failed {%d}.", GetLastError());
                }
                return 0;
            }
            if (!SaferCloseLevel(safer))
            {
                WRT_LOG("ERROR: SaferCloseLevel failed {%d}.", GetLastError());
            }
        }

        if (hNew != INVALID_HANDLE_VALUE)
        {
            CloseHandle(*p_hUser); // VERIFY
            *p_hUser = hNew;
            hNew = NULL;
            WRT_LOG("DuplicateTokenEx...");
            if (!DuplicateTokenEx(*p_hUser, MAXIMUM_ALLOWED, NULL, DEFAULT_IMPERSONATION_LEVEL, TokenPrimary, &hNew))
            {
                WRT_LOG("ERROR: DuplicateTokenEx failed {%d}!", GetLastError());
                return 0;
            }
            else
            {
                CloseHandle(*p_hUser);
                *p_hUser = hNew;
                hNew = NULL;
            }
            return 1;
        }
    }
    WRT_LOG("Don't have a good user -- can't limit rights");
    return 0;
}

BOOL WrtElevateUserToken(HANDLE* p_hEnvUser)
{
    TOKEN_ELEVATION_TYPE tet;
    DWORD needed = 0;
    DWORD gle = 0;

    if (p_hEnvUser)
    {
        WRT_LOG("WARNING: No handle specified");
        return 0;
    }
    WRT_LOG("GetTokenInformation...");
    if (GetTokenInformation(*p_hEnvUser, TokenElevationType, (LPVOID)&tet, sizeof(tet), &needed))
    {
        if (tet == TokenElevationTypeLimited)
        {
            //get the associated token, which is the full-admin token
            TOKEN_LINKED_TOKEN tlt = { 0 };
            WRT_LOG("GetTokenInformation...");
            if (GetTokenInformation(*p_hEnvUser, TokenLinkedToken, (LPVOID)&tlt, sizeof(tlt), &needed))
            {
                HANDLE h_dup = NULL;
                WRT_LOG("DuplicateTokenEx...");
                if (!DuplicateTokenEx(tlt.LinkedToken, MAXIMUM_ALLOWED, NULL, DEFAULT_IMPERSONATION_LEVEL, TokenPrimary, &h_dup))
                {
                    WRT_LOG("ERROR: DuplicateTokenEx failed {%d}!", GetLastError());
                    return 0;
                }
                else
                {
                    CloseHandle(tlt.LinkedToken);
                    *p_hEnvUser = h_dup;
                    h_dup = NULL;
                }
                WRT_LOG("Got elevated token!");
                return 1;
            }
            else
            {
                WRT_LOG("ERROR: GetTokenInformation failed {%d}!", GetLastError());
                return false;
            }
        }
        else
        {
            WRT_LOG("Token is already token!");
            return 1;
        }
    }
    else
    {
        //can't tell if it's elevated or not -- continue anyway
        gle = GetLastError();
        switch (gle)
        {
        case ERROR_INVALID_PARAMETER:
            WRT_LOG("WARNIG: unsupported on 32-bit XP!");
            break;
        case ERROR_INVALID_FUNCTION:
            WRT_LOG("WARNIG: unsupported on 64-bit XP!");
            break;
        default:
            WRT_LOG("WARNIG: Can't query token to run elevated - continuing anyway!");
            break;
        }
        return 1;
    }
}

DWORD g_PipeNameIndex = 0;
BOOL WrtCreatePipe(LPHANDLE lpReadPipe, LPHANDLE lpWritePipe, LPSECURITY_ATTRIBUTES lpPipeAttributes, DWORD nSize, BOOL overlapped)
{
    HANDLE ReadPipeHandle = NULL;
    HANDLE WritePipeHandle = NULL;
    DWORD dwError = 0;
    TCHAR PipeName[PATHMAX] = { 0 };

    memset(PipeName, 0, sizeof(PipeName));
    if (nSize == 0)
    {
        nSize = 4096;
    }
    wsprintf(PipeName, L"\\\\.\\Pipe\\WrtPipeApp.%08x.%08x.%04x", GetCurrentProcessId(), GetTickCount(), g_PipeNameIndex);
    g_PipeNameIndex++;
    ReadPipeHandle = CreateNamedPipe(PipeName, PIPE_ACCESS_INBOUND | (overlapped ? FILE_FLAG_OVERLAPPED : 0),
        PIPE_TYPE_BYTE | PIPE_WAIT, 1, nSize, nSize, 120 * 1000, lpPipeAttributes);
    if (ReadPipeHandle == NULL || ReadPipeHandle == INVALID_HANDLE_VALUE)
    {
        WRT_LOG("ERROR: Faile dto create named pipe %S -> {%d}!", PipeName, GetLastError());
        return FALSE;
    }
    WritePipeHandle = CreateFile(PipeName, GENERIC_WRITE, 0, lpPipeAttributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (ReadPipeHandle == NULL || ReadPipeHandle == INVALID_HANDLE_VALUE)
    {
        WRT_LOG("ERROR: Faile dto create named pipe %S -> {%d}!", PipeName, GetLastError());
        CloseHandle(ReadPipeHandle);
        return FALSE;
    }
    *lpReadPipe = ReadPipeHandle;
    *lpWritePipe = WritePipeHandle;
    return TRUE;
}


BOOL WrtCreateAndOpenPipes()
{
    SECURITY_ATTRIBUTES saAttr = { 0 };

    WRT_LOG("Create pipes for redirection...");

    // Set the bInheritHandle flag so pipe handles are inherited. 

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    WRT_LOG("CreatePipe for the child process's STDOUT...");
    if (!WrtCreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0, g_bDoOverlapped))
    {
        WRT_LOG("ERROR: CreatePipe failed {%d}!", GetLastError());
        return 0;
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    WRT_LOG("SetHandleInformation for STDOUT is not inherited...");
    if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
    {
        WRT_LOG("ERROR: SetHandleInformation failed {%d}!", GetLastError());
        return 0;
    }

    WRT_LOG("CreatePipe for the child process's STDIN...");
    if (!WrtCreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0, FALSE))
    {
        WRT_LOG("ERROR: CreatePipe failed {%d}!", GetLastError());
        return 0;
    }

    WRT_LOG("SetHandleInformation for STDIN is not inherited...");
    if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
    {
        WRT_LOG("ERROR: SetHandleInformation failed {%d}!", GetLastError());
        return 0;
    }

    return 1;
}

VOID WrtClosePipes()
{
    if (g_hChildStd_OUT_Rd)
    {
        g_hChildStd_OUT_Rd = NULL;
        CloseHandle(g_hChildStd_OUT_Rd);
    }
    if (g_hChildStd_OUT_Wr)
    {
        g_hChildStd_OUT_Wr = NULL;
        CloseHandle(g_hChildStd_OUT_Wr);
    }
    if (g_hChildStd_IN_Rd)
    {
        g_hChildStd_IN_Rd = NULL;
        CloseHandle(g_hChildStd_IN_Rd);
    }
    if (g_hChildStd_IN_Wr)
    {
        g_hChildStd_IN_Wr = NULL;
        CloseHandle(g_hChildStd_IN_Wr);
    }
}

HANDLE g_hSerialHandle1 = NULL;
HANDLE g_hSerialHandle2 = NULL;
HANDLE g_hOutputSerial = NULL;
HANDLE g_hOutputFile = NULL;

void WrtPerformWriteOperationsToPipe(void)
{
    DWORD dwRead = 0, dwWritten = 0;
    CHAR chBuf[BUFSIZE + 1] = { 0 };
    BOOL bSuccess = FALSE;

    if (g_hInputFile == NULL || g_hInputFile == INVALID_HANDLE_VALUE)
    {
        WRT_LOG("Using standard input!");
        g_hInputFile = GetStdHandle(STD_INPUT_HANDLE);
    }
    if (g_hChildStd_IN_Wr == NULL || g_hChildStd_IN_Wr == INVALID_HANDLE_VALUE)
    {
        WRT_LOG("ERROR: No pipe provided!");
        return;
    }
    while (g_ThreadRunning)
    {
        //COMSTAT com_stat = { 0 };
        //DWORD dw_err = 0;
        //ClearCommError(g_hInputFile, &dw_err, &com_stat);
        bSuccess = ReadFile(g_hInputFile, chBuf, BUFSIZE, &dwRead, NULL);
        //PurgeComm(g_hInputFile, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
        if (!bSuccess || dwRead == 0) break;

        if (g_bDoTerminal)
        {
            if (chBuf[0] == '\r')
            {
                chBuf[0] = '\n';
                printf("\r\n"); fflush(stdout);
                if (g_pLogFile != NULL)
                {
                    fprintf(g_pLogFile, "\r\n"); fflush(g_pLogFile);
                }
                if (g_hOutputSerial)
                {
                    bSuccess = WriteFile(g_hOutputSerial, "\r\n", 2, &dwWritten, NULL);
                    if (!bSuccess) { break; }
                }
                if (g_hOutputFile)
                {
                    bSuccess = WriteFile(g_hOutputFile, "\r\n", 2, &dwWritten, NULL);
                    if (!bSuccess) break;
                }
            }
        }
        bSuccess = WriteFile(g_hChildStd_IN_Wr, chBuf, dwRead, &dwWritten, NULL);
        if (!bSuccess) break;

        chBuf[dwRead] = 0;
        if (g_bDoInputEcho)
        {
            WRT_LOG_impl("%s", chBuf);
        }
        else if (g_bDoVerbose)
        {
            WRT_LOG("APP INPUT: %s", chBuf);
        }
    }

    //CloseHandle(g_hChildStd_IN_Wr);
    //g_hChildStd_IN_Wr = NULL;
}

void WrtPerformReadOperationsFromPipe(void)
{
    DWORD dwRead = 0, dwWritten = 0;
    CHAR chBuf[BUFSIZE + 1];
    BOOL bSuccess = FALSE;

    while (g_ThreadRunning)
    {
        bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
        if (!bSuccess || dwRead == 0) break;

        chBuf[dwRead] = 0;
        printf("%s", chBuf); fflush(stdout);
        if (g_pLogFile != NULL)
        {
            fprintf(g_pLogFile, "%s", chBuf); fflush(g_pLogFile);
        }
        if (g_hOutputSerial)
        {
            //COMSTAT com_stat = { 0 };
            //DWORD dw_err = 0;
            //ClearCommError(g_hOutputSerial, &dw_err, &com_stat);
            bSuccess = WriteFile(g_hOutputSerial, chBuf, dwRead, &dwWritten, NULL);
            //PurgeComm(g_hOutputSerial, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
            if (!bSuccess) break;

        }
        if (g_hOutputFile)
        {
            bSuccess = WriteFile(g_hOutputFile, chBuf, dwRead, &dwWritten, NULL);
            if (!bSuccess) break;
        }
    }
    return;
}

void WrtPerformReadAndWriteOperations(void)
{
    DWORD dwRead_in = 0, dwWritten_in = 0;
    CHAR chBuf_in[BUFSIZE + 1] = { 0 };
    BOOL bSuccess_in = FALSE;
    OVERLAPPED input_rd_os = { 0 };
    OVERLAPPED app_rd_os = { 0 };
    OVERLAPPED output_wr_os = { 0 };
    HANDLE h_wait[2] = { 0, 0 };
    CHAR chBuf_command[BUFSIZE + 1] = { 0 };
    DWORD chBuf_command_index = 0;
    DWORD dwRead_app = 0, dwWritten_app = 0;
    CHAR chBuf_app[BUFSIZE + 1];
    BOOL bSuccess_app = FALSE;

    memset(chBuf_command, 0, sizeof(chBuf_command));
    memset(&input_rd_os, 0, sizeof(input_rd_os));
    memset(&output_wr_os, 0, sizeof(input_rd_os));
    memset(&app_rd_os, 0, sizeof(app_rd_os));

    output_wr_os.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (output_wr_os.hEvent == NULL || output_wr_os.hEvent == INVALID_HANDLE_VALUE)
    {
        WRT_LOG("ERROR: Failed to create output event {%d}!", GetLastError());
        return;
    }
    input_rd_os.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (input_rd_os.hEvent == NULL || input_rd_os.hEvent == INVALID_HANDLE_VALUE)
    {
        WRT_LOG("ERROR: Failed to create input event {%d}!", GetLastError());
        return;
    }
    app_rd_os.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (app_rd_os.hEvent == NULL || app_rd_os.hEvent == INVALID_HANDLE_VALUE)
    {
        WRT_LOG("ERROR: Failed to create app event {%d}!", GetLastError());
        return;
    }

    if (g_hInputFile == NULL || g_hInputFile == INVALID_HANDLE_VALUE)
    {
        WRT_LOG("Using standard input!");
        g_hInputFile = GetStdHandle(STD_INPUT_HANDLE);
    }
    if (g_hChildStd_IN_Wr == NULL || g_hChildStd_IN_Wr == INVALID_HANDLE_VALUE)
    {
        WRT_LOG("ERROR: No pipe provided!");
    }

    while (g_ThreadRunning)
    {
        //if (g_hSerialHandle1 != NULL)
        //{
        //    COMSTAT com_stat = { 0 };
        //    DWORD dw_err = 0;
        //    ClearCommError(g_hSerialHandle1, &dw_err, &com_stat);
        //}
        //if (g_hSerialHandle2 != NULL && g_hSerialHandle2 != g_hSerialHandle1)
        //{
        //    COMSTAT com_stat = { 0 };
        //    DWORD dw_err = 0;
        //    ClearCommError(g_hSerialHandle2, &dw_err, &com_stat);
        //}

    WAIT_AGAIN:
        if (h_wait[0] == NULL)
        {
            bSuccess_in = ReadFile(g_hInputFile, chBuf_in, BUFSIZE, &dwRead_in, &input_rd_os);
            if (!bSuccess_in && GetLastError() != ERROR_IO_PENDING)
            {
                break;
            }
            h_wait[0] = input_rd_os.hEvent;
        }

        if (h_wait[1] == NULL)
        {
            bSuccess_app = ReadFile(g_hChildStd_OUT_Rd, chBuf_app, BUFSIZE, &dwRead_app, &app_rd_os);
            if (!bSuccess_app && GetLastError() != ERROR_IO_PENDING)
            {
                break;
            }
            h_wait[1] = app_rd_os.hEvent;
        }

        DWORD wait_res = WaitForMultipleObjects(2, h_wait, FALSE, 10000);
        if (wait_res == WAIT_FAILED)
        {
            WRT_LOG("ERROR: Failed to wait for multiple objects {%d}!", GetLastError());
            break;
        }
        else if (wait_res == WAIT_TIMEOUT)
        {
            if (g_ThreadRunning)
            {
                goto WAIT_AGAIN;
            }
            break;
        }
        else if (wait_res >= WAIT_OBJECT_0 && wait_res < WAIT_OBJECT_0 + 2)
        {
            int wait_index = wait_res - WAIT_OBJECT_0;
            if (wait_index == 0)
            {
                if (!GetOverlappedResult(g_hInputFile, &input_rd_os, &dwRead_in, TRUE) || dwRead_in == 0)
                {
                    if (dwRead_in == 0 && GetLastError() == ERROR_TIMEOUT)
                    {
                        CloseHandle(input_rd_os.hEvent);
                        memset(&input_rd_os, 0, sizeof(input_rd_os));
                        h_wait[0] = NULL;
                        input_rd_os.hEvent = NULL;
                        input_rd_os.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                        if (input_rd_os.hEvent == NULL || input_rd_os.hEvent == INVALID_HANDLE_VALUE)
                        {
                            WRT_LOG("ERROR: Failed to create app event {%d}!", GetLastError());
                            break;
                        }
                        if (g_ThreadRunning)
                        {
                            goto WAIT_AGAIN;
                        }
                    }
                    else
                    {
                        WRT_LOG("ERROR: Failed to get overlapped result from input {%d}!", GetLastError());
                        break;

                    }
                }
                if (g_bDoTerminal)
                {
                    if (chBuf_in[0] == '\r')
                    {
                        chBuf_in[0] = '\n';
                        printf("\r\n", chBuf_app); fflush(stdout);
                        if (g_pLogFile != NULL)
                        {
                            fprintf(g_pLogFile, "\r\n", chBuf_app); fflush(g_pLogFile);
                        }
                        if (g_hOutputSerial)
                        {
                            bSuccess_app = WriteFile(g_hOutputSerial, "\r\n", 2, &dwWritten_app, &output_wr_os);
                            if (!bSuccess_app)
                            {
                                if (GetLastError() != ERROR_IO_PENDING)
                                {
                                    break;
                                }
                                else
                                {
                                    WaitForSingleObject(&output_wr_os.hEvent, INFINITE);
                                }
                            }
                        }
                        if (g_hOutputFile)
                        {
                            bSuccess_app = WriteFile(g_hOutputFile, "\r\n", 2, &dwWritten_app, NULL);
                            if (!bSuccess_app) break;
                        }
                    }
                }
                bSuccess_in = WriteFile(g_hChildStd_IN_Wr, chBuf_in, dwRead_in, &dwWritten_in, NULL);
                if (!bSuccess_in) break;
                FlushFileBuffers(g_hChildStd_IN_Wr);

                chBuf_in[dwRead_in] = 0;
                if (g_bDoInputEcho)
                {
                    WRT_LOG_impl("%s", chBuf_in);
                }
                else if (g_bDoVerbose)
                {
                    WRT_LOG("APP INPUT: %s", chBuf_in);
                }

                CloseHandle(input_rd_os.hEvent);
                memset(&input_rd_os, 0, sizeof(input_rd_os));
                h_wait[0] = NULL;
                input_rd_os.hEvent = NULL;
                input_rd_os.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                if (input_rd_os.hEvent == NULL || input_rd_os.hEvent == INVALID_HANDLE_VALUE)
                {
                    WRT_LOG("ERROR: Failed to create app event {%d}!", GetLastError());
                    break;
                }
            }
            else
            {
                if (!GetOverlappedResult(g_hChildStd_OUT_Rd, &app_rd_os, &dwRead_app, TRUE) || dwRead_app == 0)
                {
                    if (dwRead_in == 0 && GetLastError() == ERROR_TIMEOUT)
                    {
                        CloseHandle(app_rd_os.hEvent);
                        memset(&app_rd_os, 0, sizeof(app_rd_os));
                        h_wait[1] = NULL;
                        app_rd_os.hEvent = NULL;
                        app_rd_os.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                        if (app_rd_os.hEvent == NULL || app_rd_os.hEvent == INVALID_HANDLE_VALUE)
                        {
                            WRT_LOG("ERROR: Failed to create app event {%d}!", GetLastError());
                            break;
                        }
                        if (g_ThreadRunning)
                        {
                            goto WAIT_AGAIN;
                        }
                    }
                    else
                    {
                        WRT_LOG("ERROR: Failed to get overlapped result from app {%d}!", GetLastError());
                        break;
                    }
                }
                chBuf_app[dwRead_app] = 0;
                printf("%s", chBuf_app); fflush(stdout);
                if (g_pLogFile != NULL)
                {
                    fprintf(g_pLogFile, "%s", chBuf_app); fflush(g_pLogFile);
                }
                if (g_hOutputSerial)
                {
                    bSuccess_app = WriteFile(g_hOutputSerial, chBuf_app, dwRead_app, &dwWritten_app, &output_wr_os);
                    if (!bSuccess_app)
                    {
                        if (GetLastError() != ERROR_IO_PENDING)
                        {
                            break;
                        }
                        else
                        {
                            WaitForSingleObject(&output_wr_os.hEvent, INFINITE);
                        }
                    }
                }
                if (g_hOutputFile)
                {
                    bSuccess_app = WriteFile(g_hOutputFile, chBuf_app, dwRead_app, &dwWritten_app, NULL);
                    if (!bSuccess_app) break;
                }

                CloseHandle(app_rd_os.hEvent);
                memset(&app_rd_os, 0, sizeof(app_rd_os));
                h_wait[1] = NULL;
                app_rd_os.hEvent = NULL;
                app_rd_os.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                if (app_rd_os.hEvent == NULL || app_rd_os.hEvent == INVALID_HANDLE_VALUE)
                {
                    WRT_LOG("ERROR: Failed to create app event {%d}!", GetLastError());
                    break;
                }
            }
        }
        else
        {
            WRT_LOG("ERROR: Failed to wait for multiple objects %d, {%d}!", wait_res, GetLastError());
            break;
        }


        //if (g_hSerialHandle1 != NULL)
        //{
        //    PurgeComm(g_hSerialHandle1, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
        //}
        //if (g_hSerialHandle2 != NULL && g_hSerialHandle2 != g_hSerialHandle1)
        //{
        //    PurgeComm(g_hSerialHandle2, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
        //}
    }

    //CloseHandle(g_hChildStd_IN_Wr);
    //g_hChildStd_IN_Wr = NULL;
}

HANDLE WrtSerialOpenHandle(TCHAR* p_portName, int baud, BOOL read, BOOL setup)
{
    HANDLE h_port = CreateFile(p_portName,  // Specify port device: default "COM1"
        GENERIC_READ | GENERIC_WRITE,       // Specify mode that open device.
        0, // the devide isn't shared.
        NULL,                               // the object gets a default security.
        OPEN_EXISTING,                      // Specify which action to take on file. 
        FILE_ATTRIBUTE_NORMAL | (g_bDoOverlapped ? FILE_FLAG_OVERLAPPED : 0),
        NULL);

    if (h_port == NULL || h_port == INVALID_HANDLE_VALUE)
    {
        WRT_LOG("ERROR: COM Failed to open %S with code {%d}!", p_portName, GetLastError());
        return NULL;
    }
    if (setup)
    {
        DCB config_ = { 0 };
        if (GetCommState(h_port, &config_) == 0)
        {
            WRT_LOG("ERROR: COM Get configuration port has problem {%d}!", GetLastError());
            return NULL;
        }
        config_.ByteSize = 8;            // Byte of the Data.
        config_.StopBits = ONESTOPBIT;   // Use one bit for stopbit.
        config_.Parity = NOPARITY;       // No parity bit
        config_.BaudRate = CBR_115200;     // Buadrate 9600 bit/sec

#define BAUD_RATE_SET(a) \
    case a: \
        WRT_LOG("Set baud rate %d!", a); \
        config_.BaudRate = CBR_ ## a; break;


        switch (baud)
        {
            BAUD_RATE_SET(110)
                BAUD_RATE_SET(300)
                BAUD_RATE_SET(600)
                BAUD_RATE_SET(1200)
                BAUD_RATE_SET(2400)
                BAUD_RATE_SET(4800)
                BAUD_RATE_SET(9600)
                BAUD_RATE_SET(14400)
                BAUD_RATE_SET(19200)
                BAUD_RATE_SET(38400)
                BAUD_RATE_SET(56000)
                BAUD_RATE_SET(57600)
                BAUD_RATE_SET(115200)
                BAUD_RATE_SET(128000)
                BAUD_RATE_SET(256000)
        default:
            WRT_LOG("Set baud rate %d!", 115200);
            break;
        }

        // Set current configuration of serial communication port.
        if (SetCommState(h_port, &config_) == 0)
        {
            WRT_LOG("ERROR: COM Set configuration port has problem {%d}", GetLastError());
            return NULL;
        }
        COMMTIMEOUTS comTimeOut;
        // Specify time-out between charactor for receiving.
        comTimeOut.ReadIntervalTimeout = 3;
        // Specify value that is multiplied 
        // by the requested number of bytes to be read. 
        comTimeOut.ReadTotalTimeoutMultiplier = 3;
        // Specify value is added to the product of the 
        // ReadTotalTimeoutMultiplier member
        comTimeOut.ReadTotalTimeoutConstant = 2;
        // Specify value that is multiplied 
        // by the requested number of bytes to be sent. 
        comTimeOut.WriteTotalTimeoutMultiplier = 3;
        // Specify value is added to the product of the 
        // WriteTotalTimeoutMultiplier member
        comTimeOut.WriteTotalTimeoutConstant = 2;
        // set the time-out parameter into device control.
        SetCommTimeouts(h_port, &comTimeOut);
    }
    return h_port;
}

BOOL WrtOpenRedirectionHandles()
{
    if (lstrlen(g_InputSerial) != 0 || lstrlen(g_OutputSerial) != 0)
    {
        if (lstrlen(g_InputSerial) != 0 && lstrlen(g_OutputSerial) != 0 && lstrcmp(g_InputSerial, g_OutputSerial) == 0)
        {
            // One serial for input
            g_hSerialHandle1 = WrtSerialOpenHandle(g_InputSerial, g_dwBaudRate, 1, 1);
            if (g_hSerialHandle1 == NULL || g_hSerialHandle1 == INVALID_HANDLE_VALUE)
            {
                WRT_LOG("ERROR: Failed to open serial port %S with code {%d}", g_InputSerial, GetLastError());
                return 0;
            }
            g_hInputFile = g_hSerialHandle1;
            g_hOutputSerial = g_hSerialHandle1;
        }
        else
        {
            if (lstrlen(g_InputSerial) != 0)
            {
                g_hSerialHandle1 = WrtSerialOpenHandle(g_InputSerial, g_dwBaudRate, 1, 1);
                if (g_hSerialHandle1 == NULL || g_hSerialHandle1 == INVALID_HANDLE_VALUE)
                {
                    WRT_LOG("ERROR: Failed to open serial port %S with code {%d}", g_InputSerial, GetLastError());
                    return 0;
                }
                g_hInputFile = g_hSerialHandle1;
            }
            if (lstrlen(g_OutputSerial) != 0)
            {
                g_hSerialHandle2 = WrtSerialOpenHandle(g_OutputSerial, g_dwBaudRate, 0, 1);
                if (g_hSerialHandle2 == NULL || g_hSerialHandle2 == INVALID_HANDLE_VALUE)
                {
                    WRT_LOG("ERROR: Failed to open serial port %S with code {%d}", g_OutputSerial, GetLastError());
                    return 0;
                }
                g_hOutputSerial = g_hSerialHandle2;
            }
        }
    }

    if (lstrlen(g_InputFile) != 0)
    {
        WRT_LOG("Try open file %S as input for app!", g_InputFile);
        if (g_hInputFile != NULL)
        {
            WRT_LOG("WARNING: Input is alread specified for Serial!");
        }
        else
        {
            g_hInputFile = CreateFile(g_InputFile, GENERIC_READ,
                0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | (g_bDoOverlapped ? FILE_FLAG_OVERLAPPED : 0), NULL);
            if (g_hInputFile == INVALID_HANDLE_VALUE)
            {
                WRT_LOG("ERROR: CreateFile failed {%d}!", GetLastError());
                return 0;
            }
        }
    }

    if (lstrlen(g_OutputFile) != 0)
    {
        WRT_LOG("Try open file %S as output for app!", g_InputFile);
        g_hOutputFile = CreateFile(g_OutputFile, GENERIC_WRITE,
            0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
        if (g_hInputFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            g_hOutputFile = CreateFile(g_OutputFile, GENERIC_WRITE,
                0, NULL, CREATE_NEW, FILE_ATTRIBUTE_READONLY, NULL);
        }
        if (g_hInputFile == INVALID_HANDLE_VALUE)
        {
            g_hInputFile = NULL;
            WRT_LOG("ERROR: CreateFile failed {%d}!", GetLastError());
            return 0;
        }
    }
    return TRUE;
}

VOID WrtCloseRedirectionHandles()
{
    if (g_hInputFile != NULL && (g_hInputFile == g_hSerialHandle1 || g_hInputFile == g_hSerialHandle2))
    {
        g_hInputFile = NULL;
    }
    if (g_hSerialHandle1)
    {
        CloseHandle(g_hSerialHandle1);
        g_hSerialHandle1 = NULL;
    }
    if (g_hSerialHandle2)
    {
        CloseHandle(g_hSerialHandle2);
        g_hSerialHandle1 = NULL;
    }
    g_hOutputSerial = NULL;
    if (g_hInputFile)
    {
        CloseHandle(g_hInputFile);
        g_hInputFile = NULL;
    }
    if (g_hOutputFile)
    {
        CloseHandle(g_hOutputFile);
        g_hOutputFile = NULL;
    }
}

void WrtConsoleAdjust(u16 minLength = 8000)
{
    CONSOLE_SCREEN_BUFFER_INFO coninfo = { 0 };
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
    if (coninfo.dwSize.Y < minLength)
    {
        coninfo.dwSize.Y = minLength;
    }
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);
}

BOOL WrtConsoleRedirect()
{
    BOOL result = TRUE;
    FILE* fp = NULL;
    errno_t err = 0;

    WRT_LOG("GetStdHandle INPUT....");
    if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE)
    {
        err = freopen_s(&fp, "CONIN$", "r", stdin);
        if (err != 0)
        {
            WRT_LOG("ERROR: freopen_s failed code=%d and {%d}!", err, GetLastError());
            result = 0;
        }
        else
        {
            setvbuf(stdin, NULL, _IONBF, 0);
        }
    }
    WRT_LOG("GetStdHandle OUTPUT....");
    if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
    {
        err = freopen_s(&fp, "CONOUT$", "w", stdout);
        if (err != 0)
        {
            WRT_LOG("ERROR: freopen_s failed code=%d and {%d}!", err, GetLastError());
            result = 0;
        }
        else
        {
            setvbuf(stdout, NULL, _IONBF, 0);
        }
    }
    WRT_LOG("GetStdHandle ERROR....");
    if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
    {
        err = freopen_s(&fp, "CONOUT$", "w", stderr);
        if (err != 0)
        {
            WRT_LOG("ERROR: freopen_s failed code=%d and {%d}!", err, GetLastError());
            result = false;
        }
        else
        {
            setvbuf(stderr, NULL, _IONBF, 0);
        }
    }
    WRT_LOG("Sync C++");
    ios::sync_with_stdio(true);

    std::wcout.clear();
    std::wcerr.clear();
    std::wcin.clear();
    std::cout.clear();
    std::cerr.clear();
    std::cin.clear();
    return result;
}

void WrtConsoleRecreate()
{
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    WrtConsoleAdjust(8000);
    WrtConsoleRedirect();
}

int WrtEnablePrivilege(LPCWSTR privilegeStr, HANDLE h_current_user = NULL)
{
    HANDLE h_current_process = GetCurrentProcess();
    BOOL   h_to_close = FALSE;
    TOKEN_PRIVILEGES  token_privs = { 0 };
    LUID              priv_luid = { 0 };

    if (h_current_user == NULL)
    {
        WRT_LOG("GetCurrentProcess...");
        if (h_current_process == NULL)
        {
            WRT_LOG("ERROR: GetCurrentProcess failed {%d}!", GetLastError());
            return 0;
        }

        // Open process token
        WRT_LOG("OpenProcessToken...");
        if (!OpenProcessToken(h_current_process, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &h_current_user))
        {
            WRT_LOG("ERROR: OpenProcessToken failed {%d}!", GetLastError());
            return false;
        }
        h_to_close = true;
    }
    WRT_LOG("LookupPrivilegeValue %S...", privilegeStr);
    if (!LookupPrivilegeValue(NULL, privilegeStr, &priv_luid))
    {
        WRT_LOG("ERROR: LookupPrivilegeValue failed {%d}!", GetLastError());
        if (h_to_close)
        {
            CloseHandle(h_current_user);
        }
        return 0;
    }

    ZeroMemory(&token_privs, sizeof(token_privs));
    token_privs.PrivilegeCount = 1;
    token_privs.Privileges[0].Luid = priv_luid;
    token_privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    WRT_LOG("AdjustTokenPrivileges %S...", privilegeStr);
    if (!AdjustTokenPrivileges(h_current_user, FALSE, &token_privs, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
    {
        WRT_LOG("ERROR: LookupPrivilegeValue failed {%d}!", GetLastError());
        if (h_to_close)
        {
            CloseHandle(h_current_user);
        }
        return 0;
    }
    if (h_to_close)
    {
        CloseHandle(h_current_user);
    }
    return 1;
}

DWORD WrtGetInteractiveSessionID()
{
    DWORD session_id = 0;
    PWTS_SESSION_INFO p_session_info = NULL;
    DWORD count = 0;
    DWORD i = 0;

    WRT_LOG("WTSEnumerateSessions...");
    if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &p_session_info, &count))
    {
        WRT_LOG("WTSEnumerateSessions -> found %d count!", count);
        for (i = 0; i < count; i++)
        {
            if (p_session_info[i].State == WTSActive)	//Here is
            {
                session_id = p_session_info[i].SessionId;
                WRT_LOG("WTSEnumerateSessions -> found %d id!", session_id);
            }
        }
        WTSFreeMemory(p_session_info);
        p_session_info = NULL;
    }

    if (0 == session_id)
    {
        WTSGetActiveConsoleSessionIdProc pWTSGetActiveConsoleSessionId = NULL;
        HMODULE hMod = LoadLibrary(L"Kernel32.dll"); //GLOK
        if (NULL != hMod)
        {
            pWTSGetActiveConsoleSessionId = (WTSGetActiveConsoleSessionIdProc)GetProcAddress(hMod, "WTSGetActiveConsoleSessionId");
            WRT_LOG("WTSGetActiveConsoleSessionId = 0x%llx!", (unsigned long long)pWTSGetActiveConsoleSessionId);
        }
        else
        {
            WRT_LOG("ERROR: can't LoadLibrary {%d}...", GetLastError());
            return 0;
        }
        WRT_LOG("call pWTSGetActiveConsoleSessionId...");
        session_id = pWTSGetActiveConsoleSessionId(); //we fall back on this if needed since it apparently doesn't always work
        WRT_LOG("session_id <%d>...", session_id);
    }
    return session_id;
}

BOOL WrtApplyPrivileges(HANDLE h_token)
{
    WRT_LOG("WrtEnablePrivilege SE_INCREASE_QUOTA_NAME...");
    if (!WrtEnablePrivilege(SE_INCREASE_QUOTA_NAME, h_token))
    {
        WRT_LOG("ERROR: WrtEnablePrivilege failed {%d}!", GetLastError());
        return 0;
    }
    WRT_LOG("WrtEnablePrivilege SE_TCB_NAME...");
    if (!WrtEnablePrivilege(SE_TCB_NAME, h_token))
    {
        WRT_LOG("ERROR: WrtEnablePrivilege failed {%d}!", GetLastError());
        return 0;
    }
    WRT_LOG("WrtEnablePrivilege SE_ASSIGNPRIMARYTOKEN_NAME...");
    if (!WrtEnablePrivilege(SE_ASSIGNPRIMARYTOKEN_NAME, h_token))
    {
        WRT_LOG("ERROR: WrtEnablePrivilege failed {%d}!", GetLastError());
        return 0;
    }
    WRT_LOG("WrtEnablePrivilege SE_IMPERSONATE_NAME...");
    if (!WrtEnablePrivilege(SE_IMPERSONATE_NAME, h_token))
    {
        WRT_LOG("ERROR: WrtEnablePrivilege failed {%d}!", GetLastError());
        return 0;
    }
    return 1;
}

BOOL WrtGetLogonSID(HANDLE hToken, PSID* ppsid)
{
    BOOL bSuccess = FALSE;
    DWORD dwIndex = 0;
    DWORD dwLength = 0;
    PTOKEN_GROUPS ptg = NULL;

    WRT_LOG("Getting the Logon SID...");
    if (ppsid == NULL)
    {
        WRT_LOG("ERROR: bad args!");
        goto Cleanup;
    }

    WRT_LOG("GetTokenInformation...");
    if (!GetTokenInformation(hToken,
        TokenGroups,    // get information about the token's groups
        (LPVOID)ptg,    // pointer to TOKEN_GROUPS buffer
        0,              // size of buffer
        &dwLength       // receives required buffer size
    ))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            WRT_LOG("ERROR: GetTokenInformation failed {%d}!", GetLastError());
            goto Cleanup;
        }
        ptg = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);
        if (ptg == NULL)
        {
            WRT_LOG("ERROR: HeapAlloc failed {%d}!", GetLastError());
            goto Cleanup;
        }
    }

    // Get the token group information from the access token.
    WRT_LOG("GetTokenInformation...");
    if (!GetTokenInformation(
        hToken,         // handle to the access token
        TokenGroups,    // get information about the token's groups
        (LPVOID)ptg,   // pointer to TOKEN_GROUPS buffer
        dwLength,       // size of buffer
        &dwLength       // receives required buffer size
    ))
    {
        WRT_LOG("ERROR: GetTokenInformation failed {%d}!", GetLastError());
        goto Cleanup;
    }

    // Loop through the groups to find the logon SID.
    for (dwIndex = 0; dwIndex < ptg->GroupCount; dwIndex++)
    {
        if ((ptg->Groups[dwIndex].Attributes & SE_GROUP_LOGON_ID) == SE_GROUP_LOGON_ID)
        {
            // Found the logon SID; make a copy of it.
            WRT_LOG("GetLengthSid...");
            dwLength = GetLengthSid(ptg->Groups[dwIndex].Sid);
            *ppsid = (PSID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);
            if (*ppsid == NULL)
            {
                WRT_LOG("ERROR: HeapAlloc failed {%d}!", GetLastError());
                goto Cleanup;
            }

            WRT_LOG("CopySid...");
            if (!CopySid(dwLength, *ppsid, ptg->Groups[dwIndex].Sid))
            {
                WRT_LOG("ERROR: CopySid failed {%d}!", GetLastError());
                HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
                goto Cleanup;
            }
            break;
        }
    }
    bSuccess = TRUE;

Cleanup:
    // Free the buffer for the token groups.
    if (ptg != NULL)
    {
        HeapFree(GetProcessHeap(), 0, (LPVOID)ptg);
    }
    return bSuccess;
}

#define DESKTOP_ALL (DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | \
DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD | \
DESKTOP_JOURNALPLAYBACK | DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS | \
DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ALL (WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | \
    WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | \
    WINSTA_WRITEATTRIBUTES | WINSTA_ACCESSGLOBALATOMS | \
    WINSTA_EXITWINDOWS | WINSTA_ENUMERATE | WINSTA_READSCREEN | \
    STANDARD_RIGHTS_REQUIRED)

#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | \
    GENERIC_EXECUTE | GENERIC_ALL)

BOOL WrtAddAceToWindowStation(HWINSTA hwinsta, PSID psid)
{
    ACCESS_ALLOWED_ACE* pace = NULL;
    ACL_SIZE_INFORMATION aclSizeInfo = { 0 };
    BOOL                 bDaclExist = FALSE;
    BOOL                 bDaclPresent = FALSE;
    BOOL                 bSuccess = FALSE;
    DWORD            dwNewAclSize = 0;
    DWORD            dwSidSize = 0;
    DWORD            dwSdSizeNeeded = 0;
    PACL                 pacl = NULL;
    PACL                 pNewAcl = NULL;
    PSECURITY_DESCRIPTOR psd = NULL;
    PSECURITY_DESCRIPTOR psdNew = NULL;
    PVOID                pTempAce = NULL;
    SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
    unsigned int         i = 0;

    WRT_LOG("Adding ACE to WindowStation...");
    __try
    {
        // Obtain the DACL for the window station.
        WRT_LOG("GetUserObjectSecurity...");
        if (!GetUserObjectSecurity(hwinsta, &si, psd, dwSidSize, &dwSdSizeNeeded))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                psd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded);
                if (psd == NULL)
                {
                    WRT_LOG("ERROR: HeapAlloc failed {%d}!", GetLastError());
                    __leave;
                }

                psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded);
                if (psdNew == NULL)
                {
                    WRT_LOG("ERROR: HeapAlloc failed {%d}!", GetLastError());
                    __leave;
                }

                dwSidSize = dwSdSizeNeeded;
                WRT_LOG("GetUserObjectSecurity...");
                if (!GetUserObjectSecurity(hwinsta, &si, psd, dwSidSize, &dwSdSizeNeeded))
                {
                    WRT_LOG("ERROR: GetUserObjectSecurity failed {%d}!", GetLastError());
                    __leave;
                }
            }
            else
            {
                WRT_LOG("ERROR: GetUserObjectSecurity failed {%d}!", GetLastError());
                __leave;
            }
        }
        // Create a new DACL.
        WRT_LOG("InitializeSecurityDescriptor...");
        if (!InitializeSecurityDescriptor(psdNew, SECURITY_DESCRIPTOR_REVISION))
        {
            WRT_LOG("ERROR: InitializeSecurityDescriptor failed {%d}!", GetLastError());
            __leave;
        }

        // Get the DACL from the security descriptor.
        WRT_LOG("GetSecurityDescriptorDacl...");
        if (!GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl, &bDaclExist))
        {
            WRT_LOG("ERROR: GetSecurityDescriptorDacl failed {%d}!", GetLastError());
            __leave;
        }

        // Initialize the ACL
        SecureZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
        aclSizeInfo.AclBytesInUse = sizeof(ACL);
        // Call only if the DACL is not NULL
        if (pacl != NULL)
        {
            // get the file ACL size info
            WRT_LOG("GetAclInformation...");
            if (!GetAclInformation(pacl, (LPVOID)&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
            {
                WRT_LOG("ERROR: GetAclInformation failed {%d}!", GetLastError());
                __leave;
            }
        }

        // Compute the size of the new ACL
        dwNewAclSize = aclSizeInfo.AclBytesInUse + (2 * sizeof(ACCESS_ALLOWED_ACE)) + (2 * GetLengthSid(psid)) - (2 * sizeof(DWORD));
        // Allocate memory for the new ACL
        pNewAcl = (PACL)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwNewAclSize);
        if (pNewAcl == NULL)
        {
            WRT_LOG("ERROR: HeapAlloc failed {%d}!", GetLastError());
            __leave;
        }

        // Initialize the new DACL
        WRT_LOG("InitializeAcl...");
        if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
        {
            WRT_LOG("ERROR: InitializeAcl failed {%d}!", GetLastError());
            __leave;
        }

        // If DACL is present, copy it to a new DACL
        if (bDaclPresent)
        {
            // Copy the ACEs to the new ACL.
            if (aclSizeInfo.AceCount)
            {
                WRT_LOG("Copy %d...", aclSizeInfo.AceCount);
                for (i = 0; i < aclSizeInfo.AceCount; i++)
                {
                    // Get an ACE.
                    if (!GetAce(pacl, i, &pTempAce))
                    {
                        WRT_LOG("ERROR: GetAce failed {%d}!", GetLastError());
                        __leave;
                    }

                    // Add the ACE to the new ACL.
                    if (!AddAce(pNewAcl, ACL_REVISION, MAXDWORD, pTempAce, ((PACE_HEADER)pTempAce)->AceSize))
                    {
                        WRT_LOG("ERROR: AddAce failed {%d}!", GetLastError());
                        __leave;
                    }
                }
            }
        }

        // Add the first ACE to the window station
        pace = (ACCESS_ALLOWED_ACE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) - sizeof(DWORD));
        if (pace == NULL)
        {
            WRT_LOG("ERROR: HeapAlloc failed {%d}!", GetLastError());
            __leave;
        }

        pace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
        pace->Header.AceFlags = CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
        pace->Header.AceSize = (WORD)(sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) - sizeof(DWORD));
        pace->Mask = GENERIC_ACCESS;
        WRT_LOG("CopySid...");
        if (!CopySid(GetLengthSid(psid), &pace->SidStart, psid))
        {
            WRT_LOG("ERROR: CopySid failed {%d}!", GetLastError());
            __leave;
        }

        WRT_LOG("AddAce...");
        if (!AddAce(pNewAcl, ACL_REVISION, MAXDWORD, (LPVOID)pace, pace->Header.AceSize))
        {
            WRT_LOG("ERROR: AddAce failed {%d}!", GetLastError());
            __leave;
        }
        // Add the second ACE to the window station
        pace->Header.AceFlags = NO_PROPAGATE_INHERIT_ACE;
        pace->Mask = WINSTA_ALL;
        WRT_LOG("AddAce...");
        if (!AddAce(pNewAcl, ACL_REVISION, MAXDWORD, (LPVOID)pace, pace->Header.AceSize))
        {
            WRT_LOG("ERROR: AddAce failed {%d}!", GetLastError());
            __leave;
        }
        // Set a new DACL for the security descriptor
        WRT_LOG("SetSecurityDescriptorDacl...");
        if (!SetSecurityDescriptorDacl(psdNew, TRUE, pNewAcl, FALSE))
        {
            WRT_LOG("ERROR: SetSecurityDescriptorDacl failed {%d}!", GetLastError());
            __leave;
        }
        // Set the new security descriptor for the window station
        WRT_LOG("SetUserObjectSecurity...");
        if (!SetUserObjectSecurity(hwinsta, &si, psdNew))
        {
            WRT_LOG("ERROR: SetUserObjectSecurity failed {%d}!", GetLastError());
            __leave;
        }
        // Indicate success
        bSuccess = TRUE;
    }
    __finally
    {
        // Free the allocated buffers
        if (pace != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)pace);
        }
        if (pNewAcl != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);
        }
        if (psd != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)psd);
        }
        if (psdNew != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
        }
    }
    return bSuccess;
}

BOOL WrtAddAceToDesktop(HDESK hdesk, PSID psid)
{
    ACL_SIZE_INFORMATION aclSizeInfo = { 0 };
    BOOL                 bDaclExist = FALSE;
    BOOL                 bDaclPresent = FALSE;
    BOOL                 bSuccess = FALSE;
    DWORD            dwNewAclSize = 0;
    DWORD            dwSidSize = 0;
    DWORD            dwSdSizeNeeded = 0;
    PACL                 pacl = NULL;
    PACL                 pNewAcl = NULL;
    PSECURITY_DESCRIPTOR psd = NULL;
    PSECURITY_DESCRIPTOR psdNew = NULL;
    PVOID                pTempAce;
    SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
    unsigned int         i = 0;


    WRT_LOG("Adding ACE to Desktop...");
    __try
    {
        // Obtain the security descriptor for the desktop object
        WRT_LOG("GetUserObjectSecurity...");
        if (!GetUserObjectSecurity(hdesk, &si, psd, dwSidSize, &dwSdSizeNeeded))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                psd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded);
                if (psd == NULL)
                {
                    WRT_LOG("ERROR: HeapAlloc failed {%d}!", GetLastError());
                    __leave;
                }

                psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded);
                if (psdNew == NULL)
                {
                    WRT_LOG("ERROR: HeapAlloc failed {%d}!", GetLastError());
                    __leave;
                }

                dwSidSize = dwSdSizeNeeded;
                WRT_LOG("GetUserObjectSecurity...");
                if (!GetUserObjectSecurity(hdesk, &si, psd, dwSidSize, &dwSdSizeNeeded))
                {
                    WRT_LOG("ERROR: GetUserObjectSecurity failed {%d}!", GetLastError());
                    __leave;
                }
            }
            else
            {
                WRT_LOG("ERROR: AddAce failed {%d}!", GetLastError());
                __leave;
            }
        }

        // Create a new security descriptor
        WRT_LOG("InitializeSecurityDescriptor...");
        if (!InitializeSecurityDescriptor(psdNew, SECURITY_DESCRIPTOR_REVISION))
        {
            WRT_LOG("ERROR: InitializeSecurityDescriptor failed {%d}!", GetLastError());
            __leave;
        }
        // Obtain the DACL from the security descriptor
        WRT_LOG("GetSecurityDescriptorDacl...");
        if (!GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl, &bDaclExist))
        {
            WRT_LOG("ERROR: GetSecurityDescriptorDacl failed {%d}!", GetLastError());
            __leave;
        }

        // Initialize
        ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
        aclSizeInfo.AclBytesInUse = sizeof(ACL);

        // Call only if NULL DACL
        if (pacl != NULL)
        {
            // Determine the size of the ACL information
            WRT_LOG("GetAclInformation...");
            if (!GetAclInformation(pacl, (LPVOID)&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
            {
                WRT_LOG("ERROR: GetAclInformation failed {%d}!", GetLastError());
                __leave;
            }
        }

        // Compute the size of the new ACL
        dwNewAclSize = aclSizeInfo.AclBytesInUse + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) - sizeof(DWORD);
        // Allocate buffer for the new ACL
        pNewAcl = (PACL)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwNewAclSize);
        if (pNewAcl == NULL)
        {
            WRT_LOG("ERROR: HeapAlloc failed {%d}!", GetLastError());
            __leave;
        }
        // Initialize the new ACL
        WRT_LOG("InitializeAcl...");
        if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
        {
            WRT_LOG("ERROR: InitializeAcl failed {%d}!", GetLastError());
            __leave;
        }
        // If DACL is present, copy it to a new DACL
        if (bDaclPresent)
        {
            // Copy the ACEs to the new ACL.
            if (aclSizeInfo.AceCount)
            {
                for (i = 0; i < aclSizeInfo.AceCount; i++)
                {
                    // Get an ACE
                    if (!GetAce(pacl, i, &pTempAce))
                    {
                        WRT_LOG("ERROR: GetAce failed {%d}!", GetLastError());
                        __leave;
                    }
                    // Add the ACE to the new ACL.
                    if (!AddAce(pNewAcl, ACL_REVISION, MAXDWORD, pTempAce, ((PACE_HEADER)pTempAce)->AceSize))
                    {
                        WRT_LOG("ERROR: AddAce failed {%d}!", GetLastError());
                        __leave;
                    }
                }
            }
        }

        // Add ACE to the DACL
        WRT_LOG("AddAccessAllowedAce...");
        if (!AddAccessAllowedAce(pNewAcl, ACL_REVISION, DESKTOP_ALL, psid))
        {
            WRT_LOG("ERROR: AddAccessAllowedAce failed {%d}!", GetLastError());
            __leave;
        }
        // Set new DACL to the new security descriptor
        WRT_LOG("SetSecurityDescriptorDacl...");
        if (!SetSecurityDescriptorDacl(psdNew, TRUE, pNewAcl, FALSE))
        {
            WRT_LOG("ERROR: SetSecurityDescriptorDacl failed {%d}!", GetLastError());
            __leave;
        }
        // Set the new security descriptor for the desktop object
        WRT_LOG("SetUserObjectSecurity...");
        if (!SetUserObjectSecurity(hdesk, &si, psdNew))
        {
            WRT_LOG("ERROR: SetUserObjectSecurity failed {%d}!", GetLastError());
            __leave;
        }
        // Indicate success
        bSuccess = TRUE;
    }
    __finally
    {
        // Free buffers
        if (pNewAcl != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);
        }
        if (psd != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)psd);
        }
        if (psdNew != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
        }
    }
    return bSuccess;
}

BOOL WrtRemoveAceFromWindowStation(HWINSTA hwinsta, PSID psid)
{
    ACL_SIZE_INFORMATION aclSizeInfo = { 0 };
    BOOL                 bDaclExist = FALSE;
    BOOL                 bDaclPresent = FALSE;
    BOOL                 bSuccess = FALSE;
    DWORD            dwNewAclSize = 0;
    DWORD            dwSidSize = 0;
    DWORD            dwSdSizeNeeded = 0;
    PACL                 pacl = NULL;
    PACL                 pNewAcl = NULL;
    PSECURITY_DESCRIPTOR psd = NULL;
    PSECURITY_DESCRIPTOR psdNew = NULL;
    ACCESS_ALLOWED_ACE* pTempAce = NULL;
    SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
    unsigned int         i = 0;

    WRT_LOG("Removing ACE from Window Station...");
    __try
    {
        // Obtain the DACL for the window station
        WRT_LOG("GetUserObjectSecurity...");
        if (!GetUserObjectSecurity(hwinsta, &si, psd, dwSidSize, &dwSdSizeNeeded))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                psd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded);
                if (psd == NULL)
                {
                    WRT_LOG("ERROR: HeapAlloc failed {%d}!", GetLastError());
                    __leave;
                }
                psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded);
                if (psdNew == NULL)
                {
                    WRT_LOG("ERROR: HeapAlloc failed {%d}!", GetLastError());
                    __leave;
                }

                dwSidSize = dwSdSizeNeeded;
                WRT_LOG("GetUserObjectSecurity...");
                if (!GetUserObjectSecurity(hwinsta, &si, psd, dwSidSize, &dwSdSizeNeeded))
                {
                    WRT_LOG("ERROR: GetUserObjectSecurity failed {%d}!", GetLastError());
                    __leave;
                }
            }
            else
            {
                WRT_LOG("ERROR: GetUserObjectSecurity failed {%d}!", GetLastError());
                __leave;
            }
        }

        // Create a new DACL
        WRT_LOG("InitializeSecurityDescriptor...");
        if (!InitializeSecurityDescriptor(psdNew, SECURITY_DESCRIPTOR_REVISION))
        {
            WRT_LOG("ERROR: InitializeSecurityDescriptor failed {%d}!", GetLastError());
            __leave;
        }
        // Get the DACL from the security descriptor
        WRT_LOG("GetSecurityDescriptorDacl...");
        if (!GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl, &bDaclExist))
        {
            WRT_LOG("ERROR: GetSecurityDescriptorDacl failed {%d}!", GetLastError());
            __leave;
        }

        // Initialize the ACL
        ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
        aclSizeInfo.AclBytesInUse = sizeof(ACL);
        // Call only if the DACL is not NULL
        if (pacl != NULL)
        {
            // get the file ACL size info
            WRT_LOG("GetAclInformation...");
            if (!GetAclInformation(pacl, (LPVOID)&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
            {
                WRT_LOG("ERROR: GetAclInformation failed {%d}!", GetLastError());
                __leave;
            }
        }

        // Compute the size of the new ACL
        dwNewAclSize = aclSizeInfo.AclBytesInUse + (2 * sizeof(ACCESS_ALLOWED_ACE)) + (2 * GetLengthSid(psid)) - (2 * sizeof(DWORD));
        // Allocate memory for the new ACL
        pNewAcl = (PACL)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwNewAclSize);
        if (pNewAcl == NULL)
        {
            WRT_LOG("ERROR: HeapAlloc failed {%d}!", GetLastError());
            __leave;
        }

        // Initialize the new DACL
        WRT_LOG("InitializeAcl...");
        if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
        {
            WRT_LOG("ERROR: InitializeAcl failed {%d}!", GetLastError());
            __leave;
        }
        // If DACL is present, copy it to a new DACL
        if (bDaclPresent)
        {
            // Copy the ACEs to the new ACL.
            if (aclSizeInfo.AceCount)
            {
                for (i = 0; i < aclSizeInfo.AceCount; i++)
                {
                    // Get an ACE.
                    if (!GetAce(pacl, i, reinterpret_cast<void**>(&pTempAce)))
                    {
                        WRT_LOG("ERROR: GetAce failed {%d}!", GetLastError());
                        __leave;
                    }

                    if (!EqualSid(psid, &pTempAce->SidStart))
                    {
                        // Add the ACE to the new ACL.
                        if (!AddAce(pNewAcl, ACL_REVISION, MAXDWORD, pTempAce, ((PACE_HEADER)pTempAce)->AceSize))
                        {
                            WRT_LOG("ERROR: AddAce failed {%d}!", GetLastError());
                            __leave;
                        }
                    }
                }
            }
        }

        if (pacl != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)pacl);
            pacl = NULL;
        }
        // Set a new DACL for the security descriptor
        WRT_LOG("SetSecurityDescriptorDacl...");
        if (!SetSecurityDescriptorDacl(psdNew, TRUE, pNewAcl, FALSE))
        {
            WRT_LOG("ERROR: SetSecurityDescriptorDacl failed {%d}!", GetLastError());
            __leave;
        }
        // Set the new security descriptor for the window station
        WRT_LOG("SetUserObjectSecurity...");
        if (!SetUserObjectSecurity(hwinsta, &si, psdNew))
        {
            WRT_LOG("ERROR: SetUserObjectSecurity failed {%d}!", GetLastError());
            __leave;
        }
        // Indicate success
        bSuccess = TRUE;
    }
    __finally
    {
        // Free the allocated buffers
        if (pacl != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)pacl);
        }
        if (pNewAcl != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);
        }
        if (psd != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)psd);
        }
        if (psdNew != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
        }
    }
    return bSuccess;
}

BOOL WrtRemoveAceFromDesktop(HDESK hdesk, PSID psid)
{
    ACL_SIZE_INFORMATION aclSizeInfo = { 0 };
    BOOL                 bDaclExist = FALSE;
    BOOL                 bDaclPresent = FALSE;
    BOOL                 bSuccess = FALSE;
    DWORD            dwNewAclSize = 0;
    DWORD            dwSidSize = 0;
    DWORD            dwSdSizeNeeded = 0;
    PACL                 pacl = NULL;
    PACL                 pNewAcl = NULL;
    PSECURITY_DESCRIPTOR psd = NULL;
    PSECURITY_DESCRIPTOR psdNew = NULL;
    ACCESS_ALLOWED_ACE* pTempAce = NULL;
    SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
    unsigned int         i = 0;

    WRT_LOG("Removing ACE from Desktop...");
    __try
    {
        // Obtain the security descriptor for the desktop object
        WRT_LOG("GetUserObjectSecurity...");
        if (!GetUserObjectSecurity(hdesk, &si, psd, dwSidSize, &dwSdSizeNeeded))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                psd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded);
                if (psd == NULL)
                {
                    WRT_LOG("ERROR: HeapAlloc failed {%d}!", GetLastError());
                    __leave;
                }
                psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded);
                if (psdNew == NULL)
                {
                    WRT_LOG("ERROR: HeapAlloc failed {%d}!", GetLastError());
                    __leave;
                }

                dwSidSize = dwSdSizeNeeded;
                WRT_LOG("GetUserObjectSecurity...");
                if (!GetUserObjectSecurity(hdesk, &si, psd, dwSidSize, &dwSdSizeNeeded))
                {
                    WRT_LOG("ERROR: GetUserObjectSecurity failed {%d}!", GetLastError());
                    __leave;
                }
            }
            else
            {
                WRT_LOG("ERROR: GetUserObjectSecurity failed {%d}!", GetLastError());
                __leave;
            }
        }

        // Create a new security descriptor
        WRT_LOG("InitializeSecurityDescriptor...");
        if (!InitializeSecurityDescriptor(psdNew, SECURITY_DESCRIPTOR_REVISION))
        {
            WRT_LOG("ERROR: InitializeSecurityDescriptor failed {%d}!", GetLastError());
            __leave;
        }
        // Obtain the DACL from the security descriptor
        WRT_LOG("GetSecurityDescriptorDacl...");
        if (!GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl, &bDaclExist))
        {
            WRT_LOG("ERROR: GetSecurityDescriptorDacl failed {%d}!", GetLastError());
            __leave;
        }

        // Initialize
        ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
        aclSizeInfo.AclBytesInUse = sizeof(ACL);
        // Call only if NULL DACL
        if (pacl != NULL)
        {
            // Determine the size of the ACL information
            WRT_LOG("GetAclInformation...");
            if (!GetAclInformation(pacl, (LPVOID)&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
            {
                WRT_LOG("ERROR: AddAce GetAclInformation {%d}!", GetLastError());
                __leave;
            }
        }

        // Compute the size of the new ACL
        dwNewAclSize = aclSizeInfo.AclBytesInUse + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) - sizeof(DWORD);
        // Allocate buffer for the new ACL
        pNewAcl = (PACL)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwNewAclSize);
        if (pNewAcl == NULL)
        {
            WRT_LOG("ERROR: HeapAlloc failed {%d}!", GetLastError());
            __leave;
        }

        // Initialize the new ACL
        WRT_LOG("InitializeAcl...");
        if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
        {
            WRT_LOG("ERROR: InitializeAcl failed {%d}!", GetLastError());
            __leave;
        }
        // If DACL is present, copy it to a new DACL
        if (bDaclPresent)
        {
            // Copy the ACEs to the new ACL.
            if (aclSizeInfo.AceCount)
            {
                for (i = 0; i < aclSizeInfo.AceCount; i++)
                {
                    // Get an ACE.
                    if (!GetAce(pacl, i, reinterpret_cast<void**>(&pTempAce)))
                    {
                        WRT_LOG("ERROR: GetAce failed {%d}!", GetLastError());
                        __leave;
                    }
                    if (!EqualSid(psid, &pTempAce->SidStart))
                    {
                        // Add the ACE to the new ACL.
                        if (!AddAce(pNewAcl, ACL_REVISION, MAXDWORD, pTempAce, ((PACE_HEADER)pTempAce)->AceSize))
                        {
                            WRT_LOG("ERROR: AddAce failed {%d}!", GetLastError());
                            __leave;
                        }
                    }
                }
            }
        }

        // Set new DACL to the new security descriptor
        WRT_LOG("SetSecurityDescriptorDacl...");
        if (!SetSecurityDescriptorDacl(psdNew, TRUE, pNewAcl, FALSE))
        {
            WRT_LOG("ERROR: SetSecurityDescriptorDacl failed {%d}!", GetLastError());
            __leave;
        }
        // Set the new security descriptor for the desktop object
        WRT_LOG("SetUserObjectSecurity...");
        if (!SetUserObjectSecurity(hdesk, &si, psdNew))
        {
            WRT_LOG("ERROR: SetUserObjectSecurity failed {%d}!", GetLastError());
            __leave;
        }
        // Indicate success
        bSuccess = TRUE;
    }
    __finally
    {
        // Free buffers
        if (pacl != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)pacl);
        }
        if (pNewAcl != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);
        }
        if (psd != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)psd);
        }
        if (psdNew != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
        }
    }
    return bSuccess;
}

DWORD g_CurrentSessionId = 0xFFFFFFFF;
DWORD g_ExplorerSessionId = 0xFFFFFFFF;
DWORD g_QuerySessionId = 0xFFFFFFFF;
DWORD g_TargetSessionId = 0xFFFFFFFF;
DWORD g_ExplorerPID = 0;
void WrtGetSessionParameters()
{
    HANDLE h_proc = NULL;
    HANDLE h_snapshot = NULL;
    PROCESSENTRY32 proc_entry = { 0 };
    BOOL res = FALSE;

    DWORD current_pid = GetCurrentProcessId();
    WRT_LOG("Process ID Current = [%d]...", current_pid);
    WRT_LOG("ProcessIdToSessionId...");
    if (!ProcessIdToSessionId(current_pid, &g_CurrentSessionId))
    {
        WRT_LOG("WARNING: ProcessIdToSessionId failed {%d}!", GetLastError());
    }
    else
    {
        WRT_LOG("SessionID Current = <%d>...", g_CurrentSessionId);
    }

    if (g_bDoImpersonate)
    {
        g_QuerySessionId = WrtGetInteractiveSessionID();
        WRT_LOG("SessionID Found   = <%d>...", g_QuerySessionId);

        WRT_LOG("CreateToolhelp32Snapshot...");
        h_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
        if (h_snapshot == INVALID_HANDLE_VALUE || h_snapshot == NULL)
        {
            WRT_LOG("ERROR: CreateToolhelp32Snapshot failed {%d}!", GetLastError());
        }
        else
        {
            memset(&proc_entry, 0, sizeof(proc_entry));
            proc_entry.dwSize = sizeof(proc_entry);

            WRT_LOG("Try find my session...");
            res = Process32First(h_snapshot, &proc_entry);
            while (res && h_proc == NULL)
            {
                if (wcscmp(proc_entry.szExeFile, L"explorer.exe") == 0)
                {
                    WRT_LOG("Got explorer...");

                    g_ExplorerPID = (DWORD)proc_entry.th32ProcessID;
                    WRT_LOG("ProcessIdToSessionId...");
                    if (!ProcessIdToSessionId((DWORD)proc_entry.th32ProcessID, &g_ExplorerSessionId))
                    {
                        WRT_LOG("WARNING: ProcessIdToSessionId failed {%d}!", GetLastError());
                    }
                    else
                    {
                        WRT_LOG("Got explorer [%d] at <%d>...", g_ExplorerPID, g_ExplorerSessionId);
                        if (g_ExplorerSessionId == g_CurrentSessionId)
                        {
                            WRT_LOG("Current process already running ok...");
                            g_TargetSessionId = g_CurrentSessionId;
                            break;
                        }
                    }
                }
                res = Process32Next(h_snapshot, &proc_entry);
            }
            CloseHandle(h_snapshot);
            h_snapshot = NULL;
        }
        if (g_ExplorerSessionId != 0xFFFFFFFF && g_TargetSessionId != 0xFFFFFFFF)
        {
            WRT_LOG("Taking explorer session ID...");
            g_TargetSessionId = g_ExplorerSessionId;
        }
        else if (g_QuerySessionId != 0xFFFFFFFF && g_TargetSessionId != 0xFFFFFFFF)
        {
            WRT_LOG("Taking query session ID...");
            g_TargetSessionId = g_QuerySessionId;
        }
    }
    else
    {
        g_TargetSessionId = g_CurrentSessionId;
    }
}

void WrtApplicationStart(LPWSTR procApp, LPWSTR procCmd, LPWSTR procDir)
{
    HANDLE h_explorer = NULL;
    HANDLE h_query_token = NULL;
    HANDLE h_explorer_token = NULL;
    SECURITY_ATTRIBUTES token_secure_attrs = { 0 };
    HANDLE h_dup_token = NULL;
    LPVOID p_environment = NULL;
    DWORD dw_creation_flags = 0;
    SECURITY_ATTRIBUTES proc_attrs = { 0 };
    SECURITY_ATTRIBUTES thread_attrs = { 0 };
    PROCESS_INFORMATION proc_info = { 0 };
    STARTUPINFO startup_info = { 0 };
    HANDLE h_logged_token = NULL;
    HDESK  h_desk = NULL;
    HWINSTA h_winsta = NULL;
    HWINSTA h_winsta_save = NULL;
    PSID p_sid = NULL;
    HANDLE h_target_token = NULL;
    BOOL b_need_impersonate = FALSE;

    LPWSTR proc_user = g_UserName;
    LPWSTR proc_domain = g_UserDomain;
    LPWSTR proc_password = g_UserPassword;

    // Choose

    if (procApp) { WRT_LOG("Create APP: %S", procApp); }
    if (procCmd) { WRT_LOG("Create CMD: %S", procCmd); }
    if (procDir) { WRT_LOG("Create DIR: %S", procDir); }

    WrtApplyPrivileges(NULL);

    WRT_LOG("Prepare structures...");

    dw_creation_flags |= CREATE_NEW_CONSOLE | NORMAL_PRIORITY_CLASS;
    //dw_creation_flags |= CREATE_SUSPENDED;

    memset(&proc_attrs, 0, sizeof(proc_attrs));
    memset(&thread_attrs, 0, sizeof(thread_attrs));
    memset(&proc_info, 0, sizeof(proc_info));
    memset(&startup_info, 0, sizeof(startup_info));

    startup_info.cb = sizeof(startup_info);

    startup_info.wShowWindow = SW_SHOW;
    startup_info.dwFlags |= STARTF_USESHOWWINDOW;

    if (g_hChildStd_OUT_Wr != NULL || g_hChildStd_IN_Rd != NULL)
    {
        startup_info.hStdError = g_hChildStd_OUT_Wr;
        startup_info.hStdOutput = g_hChildStd_OUT_Wr;
        startup_info.hStdInput = g_hChildStd_IN_Rd;
        startup_info.dwFlags |= STARTF_USESTDHANDLES;
    }

    WRT_LOG("LogonUser...");
    if (proc_user != NULL && lstrlen(proc_user) != 0)
    {
        h_logged_token = NULL;
        if (!LogonUser(proc_user, proc_domain, proc_password, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &h_logged_token))
        {
            WRT_LOG("ERROR: LogonUser failed {%d}!", GetLastError());
            WRT_LOG("ERROR: LogonUser user: %S!", proc_user);
            if (proc_domain != NULL)
            {
                WRT_LOG("ERROR: LogonUser domain: %S!", proc_domain);
            }
            if (proc_password != NULL)
            {
                WRT_LOG("ERROR: LogonUser domain: %S!", proc_domain);
            }
            goto app_exit;
        }
        else
        {
            h_target_token = h_logged_token;
            b_need_impersonate = TRUE;
        }
    }
    else if (g_TargetSessionId != g_CurrentSessionId)
    {
        WRT_LOG("Need to switch session...");
        if (g_ExplorerPID != 0)
        {
            WRT_LOG("Try use PID of explorer...");
            WRT_LOG("OpenProcess...");
            h_explorer = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE, 0, g_ExplorerPID);
            if (h_explorer == NULL || h_explorer == INVALID_HANDLE_VALUE)
            {
                WRT_LOG("ERROR: OpenProcess failed {%d}!", GetLastError());
                goto app_exit;
            }
            WRT_LOG("OpenProcessToken...");
            if (!OpenProcessToken(h_explorer, MAXIMUM_ALLOWED, &h_explorer_token))
            {
                WRT_LOG("ERROR: OpenProcessToken failed {%d}!", GetLastError());
                goto app_exit;
            }

            h_target_token = h_explorer_token;
        }
        else
        {
            WRT_LOG("WTSQueryUserToken...");
            if (FALSE == WTSQueryUserToken(g_TargetSessionId, &h_query_token))
            {
                WRT_LOG("ERROR: WTSQueryUserToken failed {%d}!", GetLastError());
                goto app_exit;
            }
            else
            {
                WRT_LOG("WTSQueryUserToken -> 0x%x!", h_query_token);
                h_target_token = h_query_token;
            }
        }
        memset(&token_secure_attrs, 0, sizeof(token_secure_attrs));
        token_secure_attrs.nLength = sizeof(token_secure_attrs);
        WRT_LOG("DuplicateTokenEx...");
        if (!DuplicateTokenEx(h_target_token, MAXIMUM_ALLOWED, &token_secure_attrs, DEFAULT_IMPERSONATION_LEVEL, TokenPrimary, &h_dup_token))
        {
            WRT_LOG("ERROR: DuplicateTokenEx failed {%d}!", GetLastError());
            goto app_exit;
        }
        //do_apply_privileges(NULL);
        WRT_LOG("SetTokenInformation...");
        if (!SetTokenInformation(h_dup_token, TokenSessionId, &g_TargetSessionId, sizeof(g_TargetSessionId)))
        {
            WRT_LOG("WARNING: SetTokenInformation failed {%d}!", GetLastError());
        }
        {
            HANDLE tmp = h_target_token;
            h_target_token = h_dup_token;
            h_dup_token = tmp;
        }
        b_need_impersonate = TRUE;
    }
    else if (g_bDoElevate || g_bDoLimit)
    {
        WRT_LOG("OpenProcessToken...");
        if (!OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &h_dup_token))
        {
            WRT_LOG("ERROR: OpenProcessToken failed {%d}!", GetLastError());
            goto app_exit;
        }
        if (g_bDoLimit)
        {
            WRT_LOG("WrtLimitRights...");
            if (WrtLimitRights(&h_dup_token))
            {
                WRT_LOG("ERROR: WrtLimitRights failed {%d}!", GetLastError());
                goto app_exit;
            }
        }
        if (g_bDoElevate)
        {
            WRT_LOG("WrtElevateUserToken...");
            if (WrtElevateUserToken(&h_dup_token))
            {
                WRT_LOG("ERROR: WrtElevateUserToken failed {%d}!", GetLastError());
                goto app_exit;
            }
        }
        h_target_token = h_dup_token;
        b_need_impersonate = TRUE;
    }


    if (b_need_impersonate)
    {
        WRT_LOG("CreateEnvironmentBlock...");
        if (!CreateEnvironmentBlock(&p_environment, h_target_token, TRUE))
        {
            WRT_LOG("ERROR: CreateEnvironmentBlock failed {%d}!", GetLastError());
            goto app_exit;
        }
        dw_creation_flags |= CREATE_UNICODE_ENVIRONMENT;

        if (lstrlen(g_Desktop) == 0)
        {
            wsprintf(g_Desktop, L"WinSta0\\Default");
            wsprintf(g_WinDesktop, L"Default");
            wsprintf(g_WinStation, L"WinSta0");
        }
    }

    if (lstrlen(g_Desktop) > 0 && b_need_impersonate)
    {
        WRT_LOG("GetProcessWindowStation...");
        h_winsta_save = GetProcessWindowStation();
        if (h_winsta_save == NULL)
        {
            WRT_LOG("ERROR: GetProcessWindowStation failed {%d}!", GetLastError());
            goto app_exit;
        }
        WRT_LOG("OpenWindowStation...");
        h_winsta = OpenWindowStation(g_WinStation, FALSE, READ_CONTROL | WRITE_DAC);
        if (h_winsta == NULL)
        {
            WRT_LOG("ERROR: OpenWindowStation failed {%d}!", GetLastError());
            goto app_exit;
        }
        WRT_LOG("SetProcessWindowStation...");
        if (!SetProcessWindowStation(h_winsta))
        {
            WRT_LOG("ERROR: SetProcessWindowStation failed {%d}!", GetLastError());
            goto app_exit;
        }
        WRT_LOG("OpenDesktop...");
        h_desk = OpenDesktop(g_WinDesktop, 0, FALSE, READ_CONTROL | WRITE_DAC | DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS);
        DWORD err_gle = GetLastError();
        WRT_LOG("SetProcessWindowStation...");
        if (!SetProcessWindowStation(h_winsta_save))
        {
            WRT_LOG("ERROR: SetProcessWindowStation failed {%d}!", GetLastError());
            goto app_exit;
        }
        if (h_desk == NULL)
        {
            WRT_LOG("ERROR: OpenDesktop failed {%d}!", err_gle);
            goto app_exit;
        }

        WRT_LOG("WrtGetLogonSID...");
        if (!WrtGetLogonSID(h_target_token, &p_sid))
        {
            WRT_LOG("ERROR: WrtGetLogonSID failed {%d}!", GetLastError());
            goto app_exit;
        }
        WRT_LOG("WrtAddAceToWindowStation...");
        if (!WrtAddAceToWindowStation(h_winsta, p_sid))
        {
            WRT_LOG("ERROR: WrtAddAceToWindowStation failed {%d}!", GetLastError());
            goto app_exit;
        }
        WRT_LOG("WrtAddAceToDesktop...");
        if (!WrtAddAceToDesktop(h_desk, p_sid))
        {
            WRT_LOG("ERROR: WrtAddAceToDesktop failed {%d}!", GetLastError());
            goto app_exit;
        }
    }
    if (lstrlen(g_Desktop) > 0)
    {
        startup_info.lpDesktop = g_Desktop;
    }

    if (h_target_token != NULL)
    {
        WRT_LOG("ImpersonateLoggedOnUser...");
        if (!ImpersonateLoggedOnUser(h_target_token))
        {
            WRT_LOG("ERROR: ImpersonateLoggedOnUser failed {%d}!", GetLastError());
            goto app_exit;
        }
    }

    if (b_need_impersonate)
    {
        WRT_LOG("CreateProcessAsUser...");
        if (!CreateProcessAsUser(h_target_token, procApp, procCmd, &proc_attrs, &thread_attrs, TRUE, dw_creation_flags, p_environment, procDir, &startup_info, &proc_info))
        {
            DWORD err = GetLastError();
            if (err == 740)
            {
                WRT_LOG("ERROR: Please check the installation as some elevated permissions is required to execute the binaries {%d}!", err);
            }
            WRT_LOG("ERROR: CreateProcessAsUser failed {%d}!", GetLastError());
            goto app_exit;
        }
        WRT_LOG("PROCESS CREATION & IMPERSONATION SUCCESS [%d]!", proc_info.dwProcessId);

        //WrtRemoveAceFromWindowStation(h_winsta, p_sid);
        //WrtRemoveAceFromDesktop(h_desk, p_sid);

        if (RevertToSelf() != 0)
        {
            WRT_LOG("WARNING: RevertToSelf failed {%d}!", GetLastError());
            //goto app_exit;
        }
    }
    else
    {
        WRT_LOG("CreateProcess...");
        if (!CreateProcess(procApp, procCmd, NULL, NULL, TRUE, 0, NULL, procDir, &startup_info, &proc_info))
        {
            WRT_LOG("ERROR: CreateProcess failed {%d}!", GetLastError());
            goto app_exit;
        }
        WRT_LOG("PROCESS CREATION SUCCESS [%d]!", proc_info.dwProcessId);
    }

    g_hProcess = proc_info.hProcess;

    // Close handles to the stdin and stdout pipes no longer needed by the child process.
    // If they are not explicitly closed, there is no way to recognize that the child process has ended.
    CloseHandle(proc_info.hThread);
    if (g_hChildStd_OUT_Wr != NULL)
    {
        CloseHandle(g_hChildStd_OUT_Wr);
        g_hChildStd_OUT_Wr = NULL;
    }
    if (g_hChildStd_IN_Rd != NULL)
    {
        CloseHandle(g_hChildStd_IN_Rd);
        g_hChildStd_IN_Rd = NULL;
    }

app_exit:
    if (h_logged_token)
    {
        CloseHandle(h_logged_token);
        h_logged_token = NULL;
    }
    if (h_explorer_token)
    {
        CloseHandle(h_explorer_token);
        h_explorer_token = NULL;
    }
    if (h_explorer)
    {
        CloseHandle(h_explorer);
        h_explorer = NULL;
    }
    if (h_dup_token)
    {
        CloseHandle(h_dup_token);
        h_dup_token = NULL;
    }
    if (h_query_token)
    {
        CloseHandle(h_query_token);
        h_query_token = NULL;
    }
    if (p_environment)
    {
        DestroyEnvironmentBlock(p_environment);
        p_environment = NULL;
    }
    if (h_winsta)
    {
        CloseHandle(h_winsta);
        h_winsta = NULL;
    }
    if (h_winsta_save)
    {
        SetProcessWindowStation(h_winsta_save);
        CloseHandle(h_winsta_save);
        h_winsta_save = NULL;
    }
    if (h_desk)
    {
        CloseHandle(h_desk);
        h_desk = NULL;
    }
    if (p_sid)
    {
        HeapFree(GetProcessHeap(), 0, p_sid);
        p_sid = NULL;
    }
    return;
}

DWORD WINAPI WrtIoThreadUniversal(LPVOID lpParam)
{
    WRT_LOG(">>>>>>> START Input/Output thread...");
    WRT_LOG("=========================================== CHILD STDOUT:\n\n");
    WrtPerformReadAndWriteOperations();
    WRT_LOG("=========================================== CHILD STDOUT END!\n\n");
    return 0;
}

DWORD WINAPI WrtIoThreadInput(LPVOID lpParam)
{
    WRT_LOG(">>>>>>> START Input thread...");
    WrtPerformWriteOperationsToPipe();
    return 0;
}

DWORD WINAPI WrtIoThreadOutput(LPVOID lpParam)
{
    WRT_LOG(">>>>>>> START Output thread...");
    WRT_LOG("=========================================== CHILD STDOUT:\n\n");
    WrtPerformReadOperationsFromPipe();
    WRT_LOG("=========================================== CHILD STDOUT END!\n\n");
    return 0;
}


int main(int argc, char* argv[])
{
    // parse args
    int i = 0;
    WCHAR arg_buf[PATHMAX] = { 0 };
    WCHAR arg_buf2[PATHMAX] = { 0 };
    BOOL app_started = FALSE;
    BOOL args_started = FALSE;
    BOOL arg1_need_restart = FALSE;
    BOOL arg2_need_restart = FALSE;
    BOOL b_need_session_switch = FALSE;
    BOOL b_need_impersonate = FALSE;
    BOOL b_need_redirect = FALSE;

    // init
    memset(&g_UserName, 0, sizeof(g_UserName));
    memset(&g_UserPassword, 0, sizeof(g_UserPassword));
    memset(&g_UserDomain, 0, sizeof(g_UserDomain));
    memset(&g_WorkDir, 0, sizeof(g_WorkDir));
    memset(&g_Desktop, 0, sizeof(g_Desktop));
    memset(&g_WinStation, 0, sizeof(g_WinStation));
    memset(&g_WinDesktop, 0, sizeof(g_WinDesktop));
    memset(&g_InputSerial, 0, sizeof(g_InputSerial));
    memset(&g_InputFile, 0, sizeof(g_InputFile));
    memset(&g_OutputSerial, 0, sizeof(g_OutputSerial));
    memset(&g_OutputFile, 0, sizeof(g_OutputFile));
    memset(&g_LogFile, 0, sizeof(g_LogFile));
    memset(&g_AppPath, 0, sizeof(g_AppPath));
    memset(&g_AppName, 0, sizeof(g_AppName));
    memset(&g_AppCmdLine, 0, sizeof(g_AppCmdLine));
    memset(&g_MyCommandLine, 0, sizeof(g_MyCommandLine));
    memset(&g_MyApp, 0, sizeof(g_MyApp));

    // Set defaults
    wsprintf(g_UserDomain, L".");


    if (argc >= 1)
    {
        wsprintf(g_MyApp, L"%S", argv[0]);
        GetFullPathName(g_MyApp, sizeof(g_MyCommandLine) / sizeof(TCHAR), g_MyCommandLine, NULL);
        wsprintf(g_MyApp, L"%s", g_MyCommandLine);
    }

    for (i = 1; i < argc; i++)
    {
        arg1_need_restart = TRUE;
        arg2_need_restart = FALSE;
        if (strchr(argv[i], L' ') != NULL)
        {
            wsprintf(arg_buf, L" \"%S\"", argv[i]);
        }
        else
        {
            wsprintf(arg_buf, L" %S", argv[i]);
        }
        if ((i + 1) < argc)
        {
            if (strchr(argv[i + 1], L' ') != NULL)
            {
                wsprintf(arg_buf2, L" \"%S\"", argv[i + 1]);
            }
            else
            {
                wsprintf(arg_buf2, L" %S", argv[i + 1]);
            }
        }

        if (app_started)
        {
            wsprintf(g_AppName, L"%S", argv[i]);
            wsprintf(g_AppCmdLine, L"%S", argv[i]);
            //GetFullPathName(g_AppName, sizeof(g_AppCmdLine)/sizeof(TCHAR), g_AppCmdLine, NULL);
            //wsprintf(g_AppName, L"%s", g_AppCmdLine);
            app_started = FALSE;
            args_started = TRUE;
        }
        else if (args_started)
        {
            lstrcat(g_AppCmdLine, arg_buf);
        }
        else if (strcmp(argv[i], "-i") == 0) { g_bDoImpersonate = TRUE; arg1_need_restart = FALSE; }
        else if (strcmp(argv[i], "-E") == 0) { g_bDoElevate = TRUE; arg1_need_restart = FALSE; }
        else if (strcmp(argv[i], "-e") == 0) { g_bDoLimit = TRUE; arg1_need_restart = FALSE; }
        else if (strcmp(argv[i], "-q") == 0) { g_bDoQuiet = TRUE; }
        else if (strcmp(argv[i], "-v") == 0) { g_bDoVerbose = TRUE; }
        else if (strcmp(argv[i], "-R") == 0) { g_bDoRedirectConsole = TRUE; arg1_need_restart = FALSE; }
        else if (strcmp(argv[i], "-r") == 0) { g_bDoRecreateConsole = TRUE; }
        else if (strcmp(argv[i], "-f") == 0) { g_bDoDisableFileSystemRedirection = TRUE; }
        else if (strcmp(argv[i], "-o") == 0) { g_bDoOverlapped = TRUE; }
        else if (strcmp(argv[i], "-t") == 0) { g_bDoTerminal = TRUE; }
        else if (strcmp(argv[i], "-h") == 0) { g_bDoInputEcho = TRUE; }
        else if (strcmp(argv[i], "--start") == 0)
        {
            app_started = TRUE;
            args_started = FALSE;
        }
        else if (strcmp(argv[i], "-l") == 0)
        {
            if ((i + 1) < argc)
            {
                wsprintf(g_LogFile, L"%S", argv[i + 1]);
                i++;
                arg2_need_restart = TRUE;
            }
            else
            {
                WRT_LOG("ERROR: wrong arguments (%d)!", i);
                WrtUsage(argv[0]);
            }
        }
        else if (strcmp(argv[i], "-W") == 0)
        {
            if ((i + 1) < argc)
            {
                wsprintf(g_WorkDir, L"%S", argv[i + 1]);
                i++;
                arg2_need_restart = TRUE;
            }
            else
            {
                WRT_LOG("ERROR: wrong arguments (%d)!", i);
                WrtUsage(argv[0]);
            }
        }
        else if (strcmp(argv[i], "-u") == 0)
        {
            if ((i + 1) < argc)
            {
                wsprintf(g_UserName, L"%S", argv[i + 1]);
                i++;
                arg1_need_restart = FALSE;
            }
            else
            {
                WRT_LOG("ERROR: wrong arguments (%d)!", i);
                WrtUsage(argv[0]);
            }
        }
        else if (strcmp(argv[i], "-p") == 0)
        {
            if ((i + 1) < argc)
            {
                wsprintf(g_UserPassword, L"%S", argv[i + 1]);
                i++;
                arg1_need_restart = FALSE;
            }
            else
            {
                WRT_LOG("ERROR: wrong arguments (%d)!", i);
                WrtUsage(argv[0]);
            }
        }
        else if (strcmp(argv[i], "-d") == 0)
        {
            if ((i + 1) < argc)
            {
                wsprintf(g_UserDomain, L"%S", argv[i + 1]);
                i++;
                arg1_need_restart = FALSE;
            }
            else
            {
                WRT_LOG("ERROR: wrong arguments (%d)!", i);
                WrtUsage(argv[0]);
            }
        }
        else if (strcmp(argv[i], "-D") == 0)
        {
            if ((i + 1) < argc)
            {
                wsprintf(g_Desktop, L"%S", argv[i + 1]);
                TCHAR* p_slash_found = wcschr(g_Desktop, L'\\');
                if (p_slash_found == NULL)
                {
                    wsprintf(g_WinStation, L"WinSta0");
                    wsprintf(g_WinDesktop, L"%s", g_Desktop);
                }
                else
                {
                    wsprintf(g_WinDesktop, L"%s", p_slash_found + 1);
                    p_slash_found[0] = 0;
                    wsprintf(g_WinStation, L"%s", g_Desktop);
                }
                wsprintf(g_Desktop, L"%s\\%s", g_WinStation, g_WinDesktop);
                i++;
                arg2_need_restart = TRUE;
            }
            else
            {
                WRT_LOG("ERROR: wrong arguments (%d)!", i);
                WrtUsage(argv[0]);
            }
        }
        else if (strcmp(argv[i], "-Sb") == 0)
        {
            if ((i + 1) < argc)
            {
                g_dwBaudRate = atoi(argv[i + 1]);
                i++;
                arg2_need_restart = TRUE;
            }
            else
            {
                WRT_LOG("ERROR: wrong arguments (%d)!", i);
                WrtUsage(argv[0]);
            }
        }
        else if (strcmp(argv[i], "-So") == 0)
        {
            if ((i + 1) < argc)
            {
                wsprintf(g_OutputSerial, L"%S", argv[i + 1]);
                i++;
                arg2_need_restart = TRUE;
            }
            else
            {
                WRT_LOG("ERROR: wrong arguments (%d)!", i);
                WrtUsage(argv[0]);
            }
        }
        else if (strcmp(argv[i], "-Si") == 0)
        {
            if ((i + 1) < argc)
            {
                wsprintf(g_InputSerial, L"%S", argv[i + 1]);
                i++;
                arg2_need_restart = TRUE;
            }
            else
            {
                WRT_LOG("ERROR: wrong arguments (%d)!", i);
                WrtUsage(argv[0]);
            }
        }
        else if (strcmp(argv[i], "-L") == 0)
        {
            if ((i + 1) < argc)
            {
                wsprintf(g_OutputFile, L"%S", argv[i + 1]);
                i++;
                arg2_need_restart = TRUE;
            }
            else
            {
                WRT_LOG("ERROR: wrong arguments (%d)!", i);
                WrtUsage(argv[0]);
            }
        }
        else if (strcmp(argv[i], "-I") == 0)
        {
            if ((i + 1) < argc)
            {
                wsprintf(g_InputFile, L"%S", argv[i + 1]);
                i++;
                arg2_need_restart = TRUE;
            }
            else
            {
                WRT_LOG("ERROR: wrong arguments (%d)!", i);
                WrtUsage(argv[0]);
            }
        }
        else
        {
            WrtUsage(argv[0]);
        }

        if (arg1_need_restart)
        {
            lstrcat(g_MyCommandLine, arg_buf);
        }
        if (arg2_need_restart)
        {
            lstrcat(g_MyCommandLine, arg_buf2);
        }
    }

    if (lstrlen(g_LogFile))
    {
        g_pLogFile = _wfopen(g_LogFile, L"wt");
        if (g_pLogFile == NULL)
        {
            WRT_LOG("WARNING: Failed to open %S file for logging!", g_LogFile);
        }
        else
        {
            WRT_LOG("OPEN LOG FILE %S!", g_LogFile);
        }
    }

    if (g_bDoDisableFileSystemRedirection)
    {
        WrtDisableFileRedirection();
    }

    if (lstrlen(g_WorkDir) == 0)
    {
        GetCurrentDirectory(sizeof(g_WorkDir), g_WorkDir);
    }

    WrtGetSessionParameters();

    // Choose to restart
    b_need_session_switch = (g_TargetSessionId != g_CurrentSessionId);
    b_need_impersonate = b_need_session_switch || (lstrlen(g_UserName) > 0);
    b_need_redirect = (lstrlen(g_InputFile) > 0) || (lstrlen(g_InputSerial) > 0) || (lstrlen(g_OutputFile) > 0) || (lstrlen(g_OutputSerial) > 0);

    WRT_LOG("Restart decision: (%d %d %d %d)!", b_need_session_switch, b_need_impersonate, b_need_redirect, g_bDoRedirectConsole);
    if (g_bDoRedirectConsole && b_need_impersonate && b_need_redirect)
    {
        WRT_LOG("So need restart!");
        WrtDisableFileRedirection();
        WrtApplicationStart(g_MyApp, g_MyCommandLine, NULL);
        goto WAIT;
    }
    if (b_need_redirect || g_bDoRedirectConsole)
    {
        WrtCreateAndOpenPipes();
    }

    if (g_bDoRecreateConsole)
    {
        WrtConsoleRecreate();
    }
    WrtOpenRedirectionHandles();

    g_ThreadRunning = TRUE;
    WrtApplicationStart(g_AppName, g_AppCmdLine, g_WorkDir);

    if (g_hProcess != NULL)
    {
        if (g_bDoOverlapped)
        {
            g_hUniversalThread = CreateThread(NULL, 0, WrtIoThreadUniversal, NULL, 0, &g_dwUniversalThreadId);
            if (g_hUniversalThread == INVALID_HANDLE_VALUE || g_hUniversalThread == NULL)
            {
                WRT_LOG("ERROR: Failed to create input/output thandling thread {%d}", GetLastError());
                g_hUniversalThread = NULL;
            }
        }
        else
        {
            g_hInputThread = CreateThread(NULL, 0, WrtIoThreadInput, NULL, 0, &g_dwInputThreadId);
            if (g_hInputThread == INVALID_HANDLE_VALUE || g_hInputThread == NULL)
            {
                WRT_LOG("ERROR: Failed to create input thandling thread {%d}", GetLastError());
                g_hInputThread = NULL;
            }
            g_hOutputThread = CreateThread(NULL, 0, WrtIoThreadOutput, NULL, 0, &g_dwOutputThreadId);
            if (g_hOutputThread == INVALID_HANDLE_VALUE || g_hOutputThread == NULL)
            {
                WRT_LOG("ERROR: Failed to create output thandling thread {%d}", GetLastError());
                g_hOutputThread = NULL;
            }
        }
    }

WAIT:
    if (g_hProcess != NULL)
    {
        DWORD exit_code = 0;

        WRT_LOG("Waiting for app...");
        WaitForSingleObject(g_hProcess, INFINITE);
        WRT_LOG("App was closed...");
        if (GetExitCodeProcess(g_hProcess, &exit_code))
        {
            WRT_LOG("App EXIT CODE: %d!", exit_code);
        }
        g_ThreadRunning = FALSE;
        CloseHandle(g_hProcess);
    }
    if (g_hUniversalThread != NULL)
    {
        g_ThreadRunning = FALSE;
        WRT_LOG("Waiting for input/output thread...");
        WaitForSingleObject(g_hUniversalThread, INFINITE);
        CloseHandle(g_hUniversalThread);
        g_hUniversalThread = NULL;
    }
    if (g_hInputThread != NULL)
    {
        g_ThreadRunning = FALSE;
        WRT_LOG("No waiting for input thread...");
        WaitForSingleObject(g_hInputThread, INFINITE);
        CloseHandle(g_hInputThread);
        g_hInputThread = NULL;
    }
    if (g_hOutputThread != NULL)
    {
        g_ThreadRunning = FALSE;
        WRT_LOG("Waiting for output thread...");
        WaitForSingleObject(g_hOutputThread, INFINITE);
        CloseHandle(g_hOutputThread);
        g_hOutputThread = NULL;
    }

    WRT_LOG("Cleanup...");
CLEANUP:
    WrtCloseDisableFileRedirection();
    WrtClosePipes();
    WrtCloseRedirectionHandles();
    return 0;
}
